// Packet and control messages for the LTE downlink MAC scheduling model.

packet DataPacket
{
    int userId;
    int weight;
    simtime_t createdAt;
}

// Scheduler -> UserBuffer: number of radio blocks (PRBs) granted in the current TTI.
message GrantMsg
{
    int numBlocks;
}
