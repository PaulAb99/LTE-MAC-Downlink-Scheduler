#include "Generator.h"
#include "packet_m.h"

Define_Module(Generator);

Generator::~Generator()
{
    cancelAndDelete(sendTimer);
}

void Generator::initialize()
{
    userId = par("userId");
    sendTimer = new cMessage("gen");
    scheduleAt(simTime() + par("interarrivalTime"), sendTimer);
}

void Generator::handleMessage(cMessage *msg)
{
    DataPacket *pkt = new DataPacket("data");
    pkt->setUserId(userId);
    pkt->setWeight(1);
    pkt->setCreatedAt(simTime());
    pkt->setByteLength(100);
    send(pkt, "out");
    scheduleAt(simTime() + par("interarrivalTime"), msg);
}
