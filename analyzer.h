#ifndef ANALYZER_H
#define ANALYZER_H

#include <string>
#include <vector>

using namespace std;

struct ZoneCount {
    std::string zone;
    string zone;
    long long count;
};

struct SlotCount {
    std::string zone;
    int hour;              // 0–23
    string zone;
    int hour;
    long long count;
};

class TripAnalyzer {
public:
    // Parse Trips.csv, skip dirty rows, never crash
    void ingestFile(const std::string& csvPath);
    // GitHub Classroom testleri ICIN
    void ingestFile(const string& path);

    // Top K zones: count desc, zone asc
    std::vector<ZoneCount> topZones(int k = 10) const;
    // HackerRank ICIN
    void ingestStdin();

    // Top K slots: count desc, zone asc, hour asc
    std::vector<SlotCount> topBusySlots(int k = 10) const;
    vector<ZoneCount> topZones(int k = 10) const;
    vector<SlotCount> topBusySlots(int k = 10) const;
};

#endif
