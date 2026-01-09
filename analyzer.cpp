#include "analyzer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <array>
#include <cstring>
#include <cstdio>

using namespace std;

// =============================================================
// GLOBAL DATA STORAGE (Since header has no private members)
// =============================================================
// Map 1: Zone -> Total Count
static unordered_map<string, long long> global_zone_counts;
// Map 2: Zone -> Array of counts per hour (0-23)
static unordered_map<string, array<long long, 24>> global_slot_counts;

// =============================================================
// HELPER FUNCTIONS (Internal Logic)
// =============================================================

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int parseHour(const char* ts, const char* te) {
    if (te > ts && te[-1] == '\r') te--;

    const char* sp = (const char*)memchr(ts, ' ', (size_t)(te - ts));
    if (!sp || sp + 2 >= te) return -1;

    char h1 = sp[1];
    char h2 = sp[2];
    if (!is_digit(h1) || !is_digit(h2)) return -1;

    int hour = (h1 - '0') * 10 + (h2 - '0');
    if (hour < 0 || hour > 23) return -1;
    return hour;
}

// Logic to process a single line from buffer
static void processLine(char* ls, char* le) {
    if (le > ls && le[-1] == '\r') le--;
    if (le <= ls) return;

    // Find 1st comma
    char* c1 = (char*)memchr(ls, ',', le - ls);
    if (!c1) return;

    // Find 2nd comma
    char* c2 = (char*)memchr(c1 + 1, ',', le - (c1 + 1));
    if (!c2 || c2 <= c1 + 1) return;

    // Find 3rd comma (to distinguish 3-col vs 6-col)
    char* c3 = (char*)memchr(c2 + 1, ',', le - (c2 + 1));

    const char* timeStart = nullptr;
    const char* timeEnd = nullptr;

    if (!c3) {
        // 3 columns format: TripID, Zone, Time
        timeStart = c2 + 1;
        timeEnd = le;
    } else {
        // 6 columns format: TripID, Pickup, Dropoff, Time...
        char* c4 = (char*)memchr(c3 + 1, ',', le - (c3 + 1));
        
        // Safety check for valid columns
        if (!c4) return;

        timeStart = c3 + 1;
        timeEnd = c4;
    }

    if (!timeStart || !timeEnd || timeEnd <= timeStart) return;

    int hour = parseHour(timeStart, timeEnd);
    if (hour < 0) return;

    // Construct string key only for valid rows
    string zone(c1 + 1, (size_t)(c2 - (c1 + 1)));

    // Update global maps
    global_zone_counts[zone]++;
    global_slot_counts[zone][hour]++;
}

// =============================================================
// CLASS IMPLEMENTATION
// =============================================================

// -------------------------------------------------------------
// ingestFile (Buffered File Read for GitHub)
// -------------------------------------------------------------
void TripAnalyzer::ingestFile(const string& path) {
    // IMPORTANT: Clear globals to prevent test contamination
    global_zone_counts.clear();
    global_slot_counts.clear();
    global_zone_counts.reserve(10000);
    global_slot_counts.reserve(10000);

    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return;

    // Skip Header
    int c;
    while ((c = fgetc(f)) != EOF && c != '\n');

    const size_t BUF = 1 << 17; // 128KB Buffer
    char* buffer = new char[BUF];
    size_t leftover = 0;

    while (true) {
        size_t bytesRead = fread(buffer + leftover, 1, BUF - leftover, f);
        if (bytesRead == 0) {
            // Process remaining part if file ended without newline
            if (leftover > 0) processLine(buffer, buffer + leftover);
            break;
        }

        size_t dataSize = leftover + bytesRead;
        char* cur = buffer;
        char* end = buffer + dataSize;
        char* lineStart = cur;

        while (cur < end) {
            char* nl = (char*)memchr(cur, '\n', end - cur);
            if (!nl) {
                leftover = end - lineStart;
                if (leftover >= BUF) leftover = 0; // Edge case
                else memmove(buffer, lineStart, leftover);
                break;
            }

            processLine(lineStart, nl);
            cur = nl + 1;
            lineStart = cur;
            leftover = 0;
        }
        if (bytesRead < (BUF - leftover)) break;
    }

    delete[] buffer;
    fclose(f);
}

// -------------------------------------------------------------
// ingestStdin (For HackerRank)
// -------------------------------------------------------------
void TripAnalyzer::ingestStdin() {
    // IMPORTANT: Clear globals
    global_zone_counts.clear();
    global_slot_counts.clear();
    global_zone_counts.reserve(10000);
    global_slot_counts.reserve(10000);

    // Skip Header (cin)
    string dummy;
    getline(cin, dummy);

    const size_t BUF = 1 << 16;
    char* buffer = new char[BUF];
    size_t leftover = 0;

    while (true) {
        size_t bytesRead = fread(buffer + leftover, 1, BUF - leftover, stdin);
        if (bytesRead == 0) {
            if (leftover > 0) processLine(buffer, buffer + leftover);
            break;
        }

        size_t dataSize = leftover + bytesRead;
        char* cur = buffer;
        char* end = buffer + dataSize;
        char* lineStart = cur;

        while (cur < end) {
            char* nl = (char*)memchr(cur, '\n', end - cur);
            if (!nl) {
                leftover = end - lineStart;
                memmove(buffer, lineStart, leftover);
                break;
            }
            processLine(lineStart, nl);
            cur = nl + 1;
            lineStart = cur;
            leftover = 0;
        }
    }
    delete[] buffer;
}

// -------------------------------------------------------------
// topZones
// -------------------------------------------------------------
vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    vector<ZoneCount> res;
    res.reserve(global_zone_counts.size());

    for (const auto& kv : global_zone_counts) {
        res.push_back({kv.first, kv.second});
    }

    // Sort: Count (Desc) -> Zone (Asc)
    sort(res.begin(), res.end(),
         [](const ZoneCount& a, const ZoneCount& b) {
             if (a.count != b.count) return a.count > b.count;
             return a.zone < b.zone;
         });

    if ((int)res.size() > k) res.resize(k);
    return res;
}

// -------------------------------------------------------------
// topBusySlots
// -------------------------------------------------------------
vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    vector<SlotCount> res;
    // Estimation
    res.reserve(global_slot_counts.size() * 3);

    for (const auto& kv : global_slot_counts) {
        for (int h = 0; h < 24; ++h) {
            if (kv.second[h] > 0) {
                res.push_back({kv.first, h, kv.second[h]});
            }
        }
    }

    // Sort: Count (Desc) -> Zone (Asc) -> Hour (Asc)
    sort(res.begin(), res.end(),
         [](const SlotCount& a, const SlotCount& b) {
             if (a.count != b.count) return a.count > b.count;
             if (a.zone != b.zone) return a.zone < b.zone;
             return a.hour < b.hour;
         });

    if ((int)res.size() > k) res.resize(k);
    return res;
}
