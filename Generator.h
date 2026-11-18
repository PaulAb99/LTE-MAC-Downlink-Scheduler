#ifndef GENERATOR_H
#define GENERATOR_H

#include <omnetpp.h>

using namespace omnetpp;

// Traffic source generating fixed-length packets for a mobile user.
class Generator : public cSimpleModule
{
  protected:
    int userId;
    cMessage *sendTimer;

    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual ~Generator();
};

#endif
