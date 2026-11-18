#include "Scheduler.h"
#include "UserBuffer.h"
#include "packet_m.h"
#include "Alloc.h"

#include <algorithm>

Define_Module(Scheduler);

Scheduler::~Scheduler()
{
    cancelAndDelete(cycleTimer);
}

void Scheduler::initialize()
{
    numUsers = par("numUsers");
    numPrbs = par("numPrbs");
    maxPrbsPerUser = par("maxPrbsPerUser");
    cycle = par("cycle");
    algorithm = par("algorithm").stringValue();
    cursor = 0;

    if (numUsers <= 0 || numPrbs <= 0)
        throw cRuntimeError("numUsers and numPrbs must be positive");

    buffers.resize(numUsers);
    lastServedTime.assign(numUsers, SIMTIME_ZERO);

    for (int i = 0; i < numUsers; ++i) {
        cModule *user = getParentModule()->getSubmodule("user", i);
        if (!user)
            throw cRuntimeError("Cannot find user[%d]", i);
        buffers[i] = check_and_cast<UserBuffer *>(user->getSubmodule("buffer"));
    }

    cycleTimer = new cMessage("schedule");
    scheduleAt(cycle, cycleTimer);
}

void Scheduler::handleMessage(cMessage *msg)
{
    scheduleCycle();
    scheduleAt(simTime() + cycle, msg);
}

void Scheduler::scheduleCycle()
{
    std::vector<int> queue(numUsers);
    std::vector<int> weight(numUsers);
    std::vector<double> waitCycles(numUsers);
    std::vector<int> grant;

    for (int i = 0; i < numUsers; ++i) {
        queue[i] = buffers[i]->getQueueLength();
        weight[i] = buffers[i]->getWeight();
        waitCycles[i] = std::max(1.0, (simTime() - lastServedTime[i]).dbl() / cycle.dbl());
    }

    if (algorithm == "fixed") {
        allocateFixed(queue, numPrbs, grant);
    } else if (algorithm == "rr") {
        cursor = allocateRR(queue, waitCycles, numPrbs, grant, cursor);
    } else if (algorithm == "wrr") {
        cursor = allocateWRR(queue, weight, waitCycles, numPrbs, grant, cursor);
    } else if (algorithm == "lq") {
        allocateLQ(queue, weight, numPrbs, grant);
    } else if (algorithm == "os" || algorithm == "pf") {
        std::vector<double> quality(numUsers);
        for (int i = 0; i < numUsers; ++i)
            quality[i] = uniform(0.1, 1.0);
        if (algorithm == "os")
            allocateOS(queue, quality, numPrbs, maxPrbsPerUser, grant);
        else
            allocatePF(queue, weight, quality, waitCycles, numPrbs, maxPrbsPerUser, grant);
    } else {
        throw cRuntimeError("Unknown algorithm: %s (expected fixed|rr|wrr|lq|os|pf)", algorithm.c_str());
    }

    for (int i = 0; i < numUsers; ++i) {
        if (grant[i] <= 0)
            continue;
        lastServedTime[i] = simTime();
        GrantMsg *g = new GrantMsg("grant");
        g->setNumBlocks(grant[i]);
        send(g, "grant", i);
    }
}
