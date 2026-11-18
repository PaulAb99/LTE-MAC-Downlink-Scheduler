// LTE downlink MAC-level scheduling network.
// user[k] = Generator + UserBuffer queue.
// scheduler allocates B = 30 PRBs every TB = 1 ms cycle.
// sink deletes packets and collects per-user and system-wide delay statistics.
network LteNetwork
{
    parameters:
        int numUsers = default(10);
        int numPrbs = default(30);
        double cycle @unit(s) = default(1ms);
        string algorithm = default("wrr");
        int maxPrbsPerUser = default(10);
        double offeredLoad = default(0.5);
        @display("bgb=920,460");
    submodules:
        user[numUsers]: User {
            @display("p=90,70,column,70");
            userId = index;
        }
        scheduler: Scheduler {
            @display("p=430,200;i=block/cogwheel");
            numUsers = parent.numUsers;
            numPrbs = parent.numPrbs;
            cycle = parent.cycle;
            algorithm = parent.algorithm;
            maxPrbsPerUser = parent.maxPrbsPerUser;
        }
        sink: Sink {
            @display("p=760,200;i=block/sink");
        }
    connections:
        for i=0..numUsers-1 {
            scheduler.grant++ --> user[i].grantIn;
            user[i].out --> sink.in++;
        }
}
