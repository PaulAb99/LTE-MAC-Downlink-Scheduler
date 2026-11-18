simple Sink
{
    parameters:
        @display("i=block/sink");
        @signal[packetDelay](type="simtime_t");
        @statistic[packetDelay](title="Packet delay"; unit="s"; record=mean,min,max,count,histogram);
    gates:
        input in[];
}
