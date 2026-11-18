#ifndef USERBUFFER_H
#define USERBUFFER_H

#include <omnetpp.h>
#include <deque>

class DataPacket;

using namespace omnetpp;

// FIFO queue for a user. The scheduler reads its length and sends a GrantMsg
// with the number of radio blocks (PRBs) to transmit in the current cycle.
class UserBuffer : public cSimpleModule
{
  protected:
    int userId;
    int weight;
    std::deque<DataPacket *> queue;

    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

  public:
    virtual ~UserBuffer();
    int getQueueLength() const { return static_cast<int>(queue.size()); }
    int getWeight() const { return weight; }
};

#endif
