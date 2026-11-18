# OMNeT++ LTE Downlink MAC Scheduling Model

Simulation model of an LTE downlink MAC-level packet scheduler (`B = 30` PRBs, scheduling cycle `TB = 1 ms`) for `K` mobile users with differentiated priorities (`W[i]`).

## Modules

- **`LteNetwork` (`LteNetwork.ned`)**: Top-level LTE downlink network with `user[numUsers]`, `scheduler`, and `sink`.
- **`User` (`User.ned`)**: Compound module per UE containing:
  - **`Generator` (`Generator.ned`, `Generator.cc`)**: Generates fixed-length data packets (`100 B`) with exponential interarrival times controlled by the offered network load.
  - **`UserBuffer` (`UserBuffer.ned`, `UserBuffer.cc`)**: FIFO queue storing generated packets and transmitting up to `m` radio blocks to `Sink` upon receiving a `GrantMsg(m)` from the scheduler.
- **`Scheduler` (`Scheduler.ned`, `Scheduler.cc`, `Alloc.h`)**: Activated every `1 ms` TTI; inspects queue lengths and allocates `B = 30` PRBs across active users until either all PRBs are allocated or all queues are empty:
  - `fixed`: Fixed allocation (`B / K` PRBs per user, capped by queue length; unused PRBs are not redistributed).
  - `rr`: Work-conserving Round Robin auction (`p[i] = t_now - t_last_served[i]`, 1 PRB per pass).
  - `wrr`: Work-conserving Weighted Round Robin auction (`p[i] = (t_now - t_last_served[i]) * W[i]`, up to `W[i]` PRBs per pass).
  - `lq`: Work-conserving Longest Queue auction (`p[i] = q[i] * W[i]`).
  - `os`: Work-conserving Opportunistic Scheduling auction (`p[i] = r[i]`, up to `maxPrbsPerUser` PRBs per pass).
  - `pf`: Work-conserving Proportional Fair auction (`p[i] = (t_now - t_last_served[i]) * W[i] * r[i]`, up to `maxPrbsPerUser` PRBs per pass).
- **`Sink` (`Sink.ned`, `Sink.cc`)**: Deletes arriving packets and records per-user and system-wide delay statistics (`mean`, `min`, `max`, packet count) to OMNeT++ scalars/histograms (`results/`) and `metrics.csv`.

## Building and Running

Inside `opp_env shell omnetpp-6.5.0`:

```bash
./makemake.sh
./run.sh
```

Standalone unit tests for the PRB allocation algorithms in `Alloc.h`:

```bash
g++ -std=c++11 alloc_test.cpp -o alloc_test && ./alloc_test
```

Individual configurations in `omnetpp.ini`:

```bash
./assignments -u Cmdenv -c LteTwoUsers
./assignments -u Cmdenv -c LteBase
./assignments -u Cmdenv -c LteLoadSweep -r 0..23
./assignments -u Qtenv -c LteBase
```
