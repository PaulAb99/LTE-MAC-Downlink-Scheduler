#include "Sink.h"
#include "packet_m.h"
#include "MetricsCsv.h"

#include <cstdio>

Define_Module(Sink);

void Sink::initialize()
{
    delaySignal = registerSignal("packetDelay");
    systemDelay.setName("systemDelay");
}

void Sink::ensureUser(int id)
{
    if (id < 0)
        id = 0;
    if (static_cast<int>(userDelay.size()) <= id) {
        userDelay.resize(id + 1);
        priority.resize(id + 1);
    }
}

std::string Sink::classify(int weight) const
{
    if (weight >= 6)
        return "HP";
    if (weight >= 2)
        return "MP";
    return "LP";
}

void Sink::handleMessage(cMessage *msg)
{
    DataPacket *pkt = check_and_cast<DataPacket *>(msg);
    simtime_t delay = simTime() - pkt->getCreatedAt();
    if (simTime() >= getSimulation()->getWarmupPeriod()) {
        int id = pkt->getUserId();
        ensureUser(id);
        userDelay[id].collect(delay.dbl());
        systemDelay.collect(delay.dbl());
        if (priority[id].empty())
            priority[id] = classify(pkt->getWeight());
        emit(delaySignal, delay);
    }
    delete pkt;
}

void Sink::finish()
{
    cModule *parent = getParentModule();
    std::string algorithm = parent->hasPar("algorithm") ? parent->par("algorithm").stringValue() : "wrr";
    double load = parent->hasPar("offeredLoad") ? parent->par("offeredLoad").doubleValue() : -1;

    if (systemDelay.getCount() > 0) {
        recordScalar("systemMeanDelay", systemDelay.getMean());
        recordScalar("systemMinDelay", systemDelay.getMin());
        recordScalar("systemMaxDelay", systemDelay.getMax());
        recordScalar("systemPackets", static_cast<long>(systemDelay.getCount()));
        writeMetric(algorithm, load, -1, "ALL",
                    systemDelay.getMean(), systemDelay.getMin(), systemDelay.getMax(),
                    static_cast<long>(systemDelay.getCount()));
    }

    for (int i = 0; i < static_cast<int>(userDelay.size()); ++i) {
        if (userDelay[i].getCount() <= 0)
            continue;
        std::string prio = priority[i].empty() ? "LP" : priority[i];
        char name[64];
        std::sprintf(name, "user%d-meanDelay", i);
        recordScalar(name, userDelay[i].getMean());
        std::sprintf(name, "user%d-minDelay", i);
        recordScalar(name, userDelay[i].getMin());
        std::sprintf(name, "user%d-maxDelay", i);
        recordScalar(name, userDelay[i].getMax());
        std::sprintf(name, "user%d-packets", i);
        recordScalar(name, static_cast<long>(userDelay[i].getCount()));
        writeMetric(algorithm, load, i, prio,
                    userDelay[i].getMean(), userDelay[i].getMin(), userDelay[i].getMax(),
                    static_cast<long>(userDelay[i].getCount()));
    }
}
