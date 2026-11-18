#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <omnetpp.h>
#include <vector>
#include <string>

class UserBuffer;

using namespace omnetpp;

// LTE Downlink MAC Scheduler.
// Activated every TB = 1 ms cycle, reads user queue lengths, allocates B = 30 PRBs,
// and sends a GrantMsg to each scheduled user.
class Scheduler : public cSimpleModule
{
  protected:
    int numUsers;
    int numPrbs;
    int maxPrbsPerUser;
    simtime_t cycle;
    std::string algorithm;
    int cursor;
    cMessage *cycleTimer;

    std::vector<UserBuffer *> buffers;
    std::vector<simtime_t> lastServedTime;

    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    void scheduleCycle();

  public:
    virtual ~Scheduler();
};

#endif
