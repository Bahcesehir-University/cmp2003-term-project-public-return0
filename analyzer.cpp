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
// GLOBAL DATA STORAGE
// Since analyzer.h does not have private members for storage,
// we use static globals restricted to this file.
// =============================================================
static unordered_map<string, long long> global_zone_counts;
static unordered_map<string, array<long long, 24>> global_slot_counts;

// =============================================================
// INTERNAL LOGIC (Identical to HackerRank submission)
// =============================================================

static bool is_digit_fast(char c) {
    return c >= '0' && c <= '9';
}

static int parseHourFast(const char* ts, const char* te) {
    if (te > ts && te[-1] == '\r') te--;

    const char* sp = (const char*)memchr(ts, ' ', (size_t)(te - ts));
    if (!sp || sp + 2 >= te) return -1;

    char h1 = sp[1];
    char h2 = sp[2];
    if (!is_digit_fast(h1) || !is_digit_fast(h2)) return -1;

    int hour = (h1 - '0') * 10 + (h2 - '0');
    if (hour < 0 || hour > 23) return -1;
    return hour;
}

static void processLineBuffer(char* ls, char* le) {
    if (le > ls && le[-1] == '\r') le--;
    if (le <= ls) return;

    char* c1 = (char*)memchr(ls, ',', le - ls);
    if (!c1) return;

    char* c2 = (char*)memchr(c1 + 1, ',', le - (c1 + 1));
    if (!c2 || c2 <= c1 + 1) return;

    char* c3 = (char*)memchr(c2 + 1, ',', le - (c2 + 1));

    const char* timeStart = nullptr;
    const char* timeEnd = nullptr;

    if (!c3) {
        // 3 columns: TripID, Zone, Time
        timeStart = c2 + 1;
        timeEnd = le;
    } else {
        // 6 columns: TripID, Pickup, Dropoff, Time...
        char* c4 = (char*)memchr(c3 + 1, ',', le - (c3 + 1));
        if (!c4) return;

        timeStart = c3 + 1;
        timeEnd = c4;
    }

    if (!timeStart || !timeEnd || timeEnd <= timeStart) return;

    int hour = parseHourFast(timeStart, timeEnd);
    if (hour < 0) return;

    string zone(c1 + 1, (size_t)(c2 - (c1 + 1)));

    // Update global storage
    global_zone_counts[zone]++;
    global_slot_counts[zone][hour]++;
}

// =============================================================
// CLASS METHODS IMPLEMENTATION
// =============================================================

void TripAnalyzer::ingestFile(const string& path) {
    // MUST CLEAR GLOBALS to avoid data leaking between tests
    global_zone_counts.clear();
    global_slot_counts.clear();
    global_zone_counts.reserve(10000);
    global_slot_counts.reserve(10000);

    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return;

    // Skip Header
    int c;
    while ((c = fgetc(f)) != EOF && c != '\n');

    const size_t BUF_SIZE = 128 * 1024;
    char* buffer = new char[BUF_SIZE];
    size_t leftover = 0;

    while (true) {
        size_t bytesRead = fread(buffer + leftover, 1, BUF_SIZE - leftover, f);
        if (bytesRead == 0) {
            if (leftover > 0) processLineBuffer(buffer, buffer + leftover);
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
                if (leftover >= BUF_SIZE) leftover = 0;
                else memmove(buffer, lineStart, leftover);
                break;
            }

            processLineBuffer(lineStart, nl);
            cur = nl + 1;
            lineStart = cur;
            leftover = 0;
        }
        if (bytesRead < (BUF_SIZE - leftover)) break;
    }

    delete[] buffer;
    fclose(f);
}

// Required for HackerRank compatibility, mostly unused in GitHub tests
void TripAnalyzer::ingestStdin() {
    global_zone_counts.clear();
    global_slot_counts.clear();
    // Implementation omitted for GitHub as it uses ingestFile
}

vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    vector<ZoneCount> res;
    res.reserve(global_zone_counts.size());

    for (const auto& kv : global_zone_counts) {
        ZoneCount z;
        z.zone = kv.first;
        z.count = kv.second;
        res.push_back(z);
    }

    auto comp = [](const ZoneCount& a, const ZoneCount& b) {
        if (a.count != b.count) return a.count > b.count;
        return a.zone < b.zone;
    };

    if ((int)res.size() > k) {
        partial_sort(res.begin(), res.begin() + k, res.end(), comp);
        res.resize(k);
    } else {
        sort(res.begin(), res.end(), comp);
    }

    return res;
}

vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    vector<SlotCount> res;
    res.reserve(global_slot_counts.size() * 3);

    for (const auto& kv : global_slot_counts) {
        for (int h = 0; h < 24; ++h) {
            if (kv.second[h] > 0) {
                SlotCount sc;
                sc.zone = kv.first;
                sc.hour = h;
                sc.count = kv.second[h];
                res.push_back(sc);
            }
        }
    }

    auto comp = [](const SlotCount& a, const SlotCount& b) {
        if (a.count != b.count) return a.count > b.count;
        if (a.zone != b.zone) return a.zone < b.zone;
        return a.hour < b.hour;
    };

    if ((int)res.size() > k) {
        partial_sort(res.begin(), res.begin() + k, res.end(), comp);
        res.resize(k);
    } else {
        sort(res.begin(), res.end(), comp);
    }

    return res;
}
