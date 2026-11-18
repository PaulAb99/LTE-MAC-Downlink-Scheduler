// Standalone unit tests for Alloc.h:
// g++ -std=c++11 alloc_test.cpp -o alloc_test && ./alloc_test
#include "Alloc.h"
#include <cstdio>
#include <cstdlib>
#include <vector>

static int fails = 0;

static void expect(const char *name, const std::vector<int> &got, const std::vector<int> &exp)
{
    if (got == exp) {
        std::printf("PASS %s\n", name);
        return;
    }
    std::printf("FAIL %s\n  got:", name);
    for (std::size_t i = 0; i < got.size(); ++i)
        std::printf(" %d", got[i]);
    std::printf("\n  exp:");
    for (std::size_t i = 0; i < exp.size(); ++i)
        std::printf(" %d", exp[i]);
    std::printf("\n");
    fails++;
}

static int sum(const std::vector<int> &v)
{
    int s = 0;
    for (std::size_t i = 0; i < v.size(); ++i)
        s += v[i];
    return s;
}

int main()
{
    std::vector<int> q(10, 100), g;
    allocateFixed(q, 30, g);
    expect("fixed equal", g, std::vector<int>(10, 3));

    std::vector<int> qshort(10, 1);
    allocateFixed(qshort, 30, g);
    expect("fixed capped by queue", g, std::vector<int>(10, 1));

    int cursor = 0;
    std::vector<double> wt10(10, 1.0);
    cursor = allocateRR(q, wt10, 30, g, cursor);
    expect("rr equal", g, std::vector<int>(10, 3));

    std::vector<int> q3(3, 100);
    cursor = 0;
    cursor = allocateRR(q3, std::vector<double>(3, 1.0), 5, g, cursor);
    expect("rr 5 over 3", g, std::vector<int>({2, 2, 1}));
    if (cursor != 2) {
        std::printf("FAIL rr cursor got %d exp 2\n", cursor);
        fails++;
    } else {
        std::printf("PASS rr cursor\n");
    }

    // User that waited longest wins RR auction first.
    allocateRR(q3, std::vector<double>({1.0, 4.0, 2.0}), 5, g, 0);
    expect("rr waitCycles priority", g, std::vector<int>({1, 2, 2}));

    // WRR: p[i] = waitCycles[i] * W[i], up to W[i] per pass, work-conserving across passes.
    std::vector<int> w(10, 1);
    w[0] = 8; w[1] = 6;
    w[2] = w[3] = w[4] = w[5] = w[6] = 2;
    w[7] = w[8] = w[9] = 1;
    cursor = 0;
    allocateWRR(q, w, wt10, 30, g, cursor);
    expect("wrr weights", g, std::vector<int>({11, 6, 2, 2, 2, 2, 2, 1, 1, 1}));
    if (sum(g) != 30) {
        std::printf("FAIL wrr sum %d\n", sum(g));
        fails++;
    }

    // WRR is work-conserving when only low-weight users have traffic.
    std::vector<int> qLpOnly(10, 0);
    qLpOnly[7] = 100; qLpOnly[8] = 100; qLpOnly[9] = 100;
    allocateWRR(qLpOnly, w, wt10, 30, g, 0);
    expect("wrr redistributes unused PRBs", g, std::vector<int>({0, 0, 0, 0, 0, 0, 0, 10, 10, 10}));

    std::vector<int> w3(3, 1);
    std::vector<int> q3b(3, 100);
    q3b[0] = 0;
    allocateWRR(q3b, w3, std::vector<double>(3, 1.0), 30, g, 0);
    expect("wrr empty queue redistributes", g, std::vector<int>({0, 15, 15}));

    std::vector<int> qcap(2);
    qcap[0] = 2; qcap[1] = 100;
    allocateWRR(qcap, std::vector<int>(2, 1), std::vector<double>(2, 1.0), 10, g, 0);
    expect("wrr queue cap then leftover", g, std::vector<int>({2, 8}));

    allocateLQ(std::vector<int>({10, 10}), std::vector<int>({8, 1}), 5, g);
    expect("lq prefers high weight", g, std::vector<int>({5, 0}));

    // OS: 4 users, each winning user capped at 10 PRBs per pass, 30 PRBs total -> top 3 qualities get 10 each
    allocateOS(std::vector<int>(4, 100), std::vector<double>({0.9, 0.4, 0.8, 0.7}), 30, 10, g);
    expect("os top 3 get 10 PRBs", g, std::vector<int>({10, 0, 10, 10}));

    // OS work-conserving across multiple passes when top users have short queues.
    allocateOS(std::vector<int>({5, 100, 5, 5}), std::vector<double>({0.9, 0.4, 0.8, 0.7}), 30, 10, g);
    expect("os multi-pass work-conserving", g, std::vector<int>({5, 15, 5, 5}));

    // PF: user 1 waited longer and has higher weight, overriding lower channel quality
    allocatePF(std::vector<int>(2, 100),
               std::vector<int>({1, 4}),
               std::vector<double>({0.8, 0.5}),
               std::vector<double>({1.0, 3.0}),
               10, 6, g);
    expect("pf combines wait, weight, quality", g, std::vector<int>({4, 6}));

    if (fails) {
        std::printf("%d failed\n", fails);
        return 1;
    }
    std::printf("all passed\n");
    return 0;
}
