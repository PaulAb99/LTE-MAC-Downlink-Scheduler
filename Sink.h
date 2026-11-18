#ifndef SINK_H
#define SINK_H

#include <omnetpp.h>
#include <vector>
#include <string>

using namespace omnetpp;

// Data destination: deletes arriving packets and records delay statistics
// (mean, min, max, packet count) per user and for the whole system.
class Sink : public cSimpleModule
{
  protected:
    simsignal_t delaySignal;
    cStdDev systemDelay;
    std::vector<cStdDev> userDelay;
    std::vector<std::string> priority;

    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    void ensureUser(int id);
    std::string classify(int weight) const;
};

#endif
