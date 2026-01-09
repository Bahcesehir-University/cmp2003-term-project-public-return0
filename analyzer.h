#ifndef TRIP_ANALYZER_H
#define TRIP_ANALYZER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <array>

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
private:
    // Verileri saklayan haritalar
    unordered_map<string, long long> zoneCounts;
    unordered_map<string, array<long long, 24>> slotCounts;

public:
    // GitHub için gerekli (Dosyadan okuma)
    void ingestFile(const string& csvPath);

    // HackerRank uyumluluğu için (Stdin okuma)
    void ingestStdin();

    vector<ZoneCount> topZones(int k = 10) const;
    vector<SlotCount> topBusySlots(int k = 10) const;
};

#endif
