// PRB allocation algorithms for one LTE MAC downlink scheduling cycle (TTI = 1 ms).
// Pure C++ functions (no OMNeT++ headers) so they can be unit-tested standalone.
//
// Algorithms (see LTE_scheduling.html and Scheduling_algorithms.pdf):
//   fixed - Fixed allocation: each user gets B/K PRBs (capped by queue length); unused PRBs are not redistributed.
//   rr    - Round Robin (work-conserving): p[i] = waitCycles[i] with cyclic cursor tie-break, 1 PRB per pass.
//   wrr   - Weighted Round Robin (work-conserving): p[i] = waitCycles[i] * W[i], up to W[i] PRBs per pass.
//   lq    - Longest Queue auction (work-conserving): each PRB goes to argmax_i (q_rem[i] * W[i]).
//   os    - Opportunistic Scheduling auction (work-conserving): p[i] = r[i], up to maxPerUser PRBs per pass.
//   pf    - Proportional Fair auction (work-conserving): p[i] = waitCycles[i] * W[i] * r[i], up to maxPerUser PRBs per pass.

#ifndef ALLOC_H
#define ALLOC_H

#include <vector>
#include <algorithm>

inline void clearGrant(std::size_t n, std::vector<int> &grant)
{
    grant.assign(n, 0);
}

inline void allocateFixed(const std::vector<int> &queue,
                          int numPrbs,
                          std::vector<int> &grant)
{
    const int n = static_cast<int>(queue.size());
    clearGrant(n, grant);
    if (n <= 0 || numPrbs <= 0)
        return;
    const int base = numPrbs / n;
    const int rem = numPrbs % n;
    for (int i = 0; i < n; ++i) {
        int share = base + (i < rem ? 1 : 0);
        if (share > queue[i])
            share = queue[i];
        if (share < 0)
            share = 0;
        grant[i] = share;
    }
}

// Ranks active users by descending score[i] (with cyclic tie-break from cursor),
// then repeatedly allocates up to capPerPass[i] PRBs per user per pass until all
// PRBs are allocated or all eligible queues are empty. Returns the last served user index.
inline int allocateRankedPasses(const std::vector<int> &queue,
                                const std::vector<double> &score,
                                const std::vector<int> &capPerPass,
                                int numPrbs,
                                int cursor,
                                bool requirePositiveScore,
                                std::vector<int> &grant)
{
    const int n = static_cast<int>(queue.size());
    clearGrant(n, grant);
    if (n <= 0 || numPrbs <= 0)
        return 0;
    if (cursor < 0 || cursor >= n)
        cursor = 0;

    struct Candidate {
        int index;
        double score;
        int tie;
    };
    std::vector<Candidate> order;
    for (int i = 0; i < n; ++i) {
        if (queue[i] <= 0 || capPerPass[i] <= 0)
            continue;
        if (requirePositiveScore && score[i] <= 0.0)
            continue;
        Candidate c;
        c.index = i;
        c.score = score[i];
        c.tie = (i - cursor + n) % n;
        order.push_back(c);
    }

    int last = (cursor + n - 1) % n;
    if (order.empty())
        return last;

    std::sort(order.begin(), order.end(), [](const Candidate &a, const Candidate &b) {
        if (a.score != b.score)
            return a.score > b.score;
        return a.tie < b.tie;
    });

    int left = numPrbs;
    bool progress = true;
    while (left > 0 && progress) {
        progress = false;
        for (std::size_t k = 0; k < order.size() && left > 0; ++k) {
            int i = order[k].index;
            int rem = queue[i] - grant[i];
            if (rem <= 0)
                continue;
            int give = std::min(left, std::min(capPerPass[i], rem));
            grant[i] += give;
            left -= give;
            last = i;
            progress = true;
        }
    }
    return last;
}

// Work-conserving Round Robin: p[i] = waitCycles[i] with cyclic cursor tie-break, 1 PRB per pass.
// Returns the cursor for the next scheduling cycle.
inline int allocateRR(const std::vector<int> &queue,
                      const std::vector<double> &waitCycles,
                      int numPrbs,
                      std::vector<int> &grant,
                      int cursor)
{
    const int n = static_cast<int>(queue.size());
    if (n <= 0 || numPrbs <= 0) {
        clearGrant(n > 0 ? n : 0, grant);
        return 0;
    }
    int last = allocateRankedPasses(queue, waitCycles, std::vector<int>(n, 1),
                                    numPrbs, cursor, false, grant);
    return (last + 1) % n;
}

// Work-conserving Weighted Round Robin: p[i] = waitCycles[i] * W[i], up to W[i] PRBs per pass.
// Returns the cursor for the next scheduling cycle.
inline int allocateWRR(const std::vector<int> &queue,
                       const std::vector<int> &weight,
                       const std::vector<double> &waitCycles,
                       int numPrbs,
                       std::vector<int> &grant,
                       int cursor)
{
    const int n = static_cast<int>(queue.size());
    if (n <= 0 || numPrbs <= 0) {
        clearGrant(n > 0 ? n : 0, grant);
        return 0;
    }
    if (cursor < 0 || cursor >= n)
        cursor = 0;

    std::vector<double> score(n, 0.0);
    for (int i = 0; i < n; ++i) {
        if (weight[i] > 0)
            score[i] = waitCycles[i] * static_cast<double>(weight[i]);
    }
    allocateRankedPasses(queue, score, weight, numPrbs, cursor, false, grant);
    return (cursor + 1) % n;
}

// Longest Queue weighted auction: each PRB goes to user i maximizing leftQ[i] * W[i].
inline void allocateLQ(const std::vector<int> &queue,
                       const std::vector<int> &weight,
                       int numPrbs,
                       std::vector<int> &grant)
{
    const int n = static_cast<int>(queue.size());
    clearGrant(n, grant);
    if (n <= 0 || numPrbs <= 0)
        return;
    std::vector<int> leftQ = queue;
    int left = numPrbs;
    while (left > 0) {
        int best = -1;
        long bestScore = -1;
        for (int i = 0; i < n; ++i) {
            if (leftQ[i] <= 0 || weight[i] <= 0)
                continue;
            long score = static_cast<long>(leftQ[i]) * weight[i];
            if (best < 0 || score > bestScore || (score == bestScore && i < best)) {
                best = i;
                bestScore = score;
            }
        }
        if (best < 0)
            break;
        grant[best]++;
        leftQ[best]--;
        left--;
    }
}

// Work-conserving score auction: each pass awards up to maxPerUser PRBs to users in
// descending order of p[i], repeating until all PRBs are allocated or all queues are empty.
inline void allocateByScoreAuction(const std::vector<int> &queue,
                                   const std::vector<double> &score,
                                   int numPrbs,
                                   int maxPerUser,
                                   std::vector<int> &grant)
{
    const int n = static_cast<int>(queue.size());
    if (n <= 0 || numPrbs <= 0) {
        clearGrant(n > 0 ? n : 0, grant);
        return;
    }
    if (maxPerUser <= 0 || maxPerUser > numPrbs)
        maxPerUser = numPrbs;
    allocateRankedPasses(queue, score, std::vector<int>(n, maxPerUser),
                         numPrbs, 0, true, grant);
}

// Opportunistic Scheduling (OS): p[i] = quality[i] (radio link quality r[i]).
inline void allocateOS(const std::vector<int> &queue,
                       const std::vector<double> &quality,
                       int numPrbs,
                       int maxPerUser,
                       std::vector<int> &grant)
{
    allocateByScoreAuction(queue, quality, numPrbs, maxPerUser, grant);
}

// Proportional Fair (PF): p[i] = waitCycles[i] * W[i] * quality[i].
inline void allocatePF(const std::vector<int> &queue,
                       const std::vector<int> &weight,
                       const std::vector<double> &quality,
                       const std::vector<double> &waitCycles,
                       int numPrbs,
                       int maxPerUser,
                       std::vector<int> &grant)
{
    const int n = static_cast<int>(queue.size());
    std::vector<double> score(n, 0.0);
    for (int i = 0; i < n; ++i) {
        double w = weight[i] > 0 ? static_cast<double>(weight[i]) : 1.0;
        double wt = waitCycles[i] > 0.0 ? waitCycles[i] : 1.0;
        double rq = quality[i] > 0.0 ? quality[i] : 0.0;
        score[i] = wt * w * rq;
    }
    allocateByScoreAuction(queue, score, numPrbs, maxPerUser, grant);
}

#endif
