simple UserBuffer
{
    parameters:
        @display("i=block/queue");
        int userId = default(0);
        int weight = default(1);
    gates:
        input in;        // from generator
        input grantIn;   // from scheduler: number of radio blocks granted
        output out;      // to sink
}
