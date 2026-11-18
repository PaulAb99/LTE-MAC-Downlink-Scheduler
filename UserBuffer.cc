#include "UserBuffer.h"
#include "packet_m.h"

Define_Module(UserBuffer);

UserBuffer::~UserBuffer()
{
    while (!queue.empty()) {
        delete queue.front();
        queue.pop_front();
    }
}

void UserBuffer::initialize()
{
    userId = par("userId");
    weight = par("weight");
    if (weight < 1)
        weight = 1;
}

void UserBuffer::handleMessage(cMessage *msg)
{
    if (msg->arrivedOn("grantIn")) {
        GrantMsg *grant = check_and_cast<GrantMsg *>(msg);
        int n = grant->getNumBlocks();
        delete grant;
        int sent = 0;
        while (sent < n && !queue.empty()) {
            DataPacket *pkt = queue.front();
            queue.pop_front();
            pkt->setWeight(weight);
            send(pkt, "out");
            ++sent;
        }
        return;
    }
    queue.push_back(check_and_cast<DataPacket *>(msg));
}

void UserBuffer::finish()
{
    recordScalar("residualQueue", static_cast<long>(queue.size()));
}
