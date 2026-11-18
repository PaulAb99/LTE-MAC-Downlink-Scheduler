simple Generator
{
    parameters:
        @display("i=block/source");
        int userId = default(0);
        volatile double interarrivalTime @unit(s) = default(exponential(1ms));
    gates:
        output out;
}
