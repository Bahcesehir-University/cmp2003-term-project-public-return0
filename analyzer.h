#ifndef ANALYZER_H
#define ANALYZER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <array>

struct ZoneCount {
    std::string zone;
    long long count;
};

struct SlotCount {
    std::string zone;
    int hour;
    long long count;
};

class TripAnalyzer {
private:
    std::unordered_map<std::string, long long> zoneCounts;
    std::unordered_map<std::string, std::array<long long, 24>> slotCounts;

    static bool is_digit(char c);
    static int parseHour(const char* ts, const char* te);
    static void processLine(
        char* ls, char* le,
        std::unordered_map<std::string, long long>& zc,
        std::unordered_map<std::string, std::array<long long, 24>>& sc
    );

public:
    void ingestStdin();
    std::vector<ZoneCount> topZones(int k = 10) const;
    std::vector<SlotCount> topBusySlots(int k = 10) const;
};

#endif
