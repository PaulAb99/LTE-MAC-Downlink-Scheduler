#ifndef METRICSCSV_H
#define METRICSCSV_H

#include <fstream>
#include <string>
#include <iomanip>

// Appends a summary row to metrics.csv in the simulation working directory.
inline void writeMetric(const std::string &algorithm,
                        double load,
                        int user,
                        const std::string &priority,
                        double meanDelay,
                        double minDelay,
                        double maxDelay,
                        long packets)
{
    std::ifstream in("metrics.csv", std::ios::binary | std::ios::ate);
    bool needHeader = !in.good() || in.tellg() <= 0;
    in.close();

    std::ofstream out("metrics.csv", std::ios::app);
    if (needHeader)
        out << "algorithm,load,user,priority,mean_delay,min_delay,max_delay,packets\n";

    out << std::setprecision(10)
        << algorithm << ","
        << load << "," << user << "," << priority << ","
        << meanDelay << "," << minDelay << "," << maxDelay << ","
        << packets << "\n";
}

#endif
