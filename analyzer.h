#ifndef ANALYZER_H
#define ANALYZER_H

#include <string>
#include <vector>

using namespace std;

struct ZoneCount {
    string zone;
    long long count;
};

struct SlotCount {
    string zone;
    int hour;
    long long count;
};

class TripAnalyzer {
public:
    
    void ingestFile(const string& path);

  
    void ingestStdin();

    vector<ZoneCount> topZones(int k = 10) const;
    vector<SlotCount> topBusySlots(int k = 10) const;
};

#endif

