simple Scheduler
{
    parameters:
        @display("i=block/cogwheel");
        int numUsers = default(10);
        int numPrbs = default(30);
        double cycle @unit(s) = default(1ms);
        string algorithm = default("wrr");   // fixed, rr, wrr, lq, os, pf
        int maxPrbsPerUser = default(10);    // per-pass RB limit in auction algorithms (os, pf)
    gates:
        output grant[];
}
