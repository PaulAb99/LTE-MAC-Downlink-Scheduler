// Mobile user module: packet generator + FIFO buffer queue.
module User
{
    parameters:
        int userId = default(0);
        int weight = default(1);
        volatile double interarrivalTime @unit(s) = default(exponential(1ms));
        @display("i=block/user");
    gates:
        input grantIn;
        output out;
    submodules:
        gen: Generator {
            @display("p=50,40;i=block/source");
            userId = parent.userId;
            interarrivalTime = default(parent.interarrivalTime);
        }
        buffer: UserBuffer {
            @display("p=50,130;i=block/queue");
            userId = parent.userId;
            weight = default(parent.weight);
        }
    connections:
        gen.out --> buffer.in;
        grantIn --> buffer.grantIn;
        buffer.out --> out;
}
