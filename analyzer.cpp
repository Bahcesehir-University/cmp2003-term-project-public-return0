#include "analyzer.h"

#include <algorithm>
#include <unordered_map>
#include <array>
#include <cstring>
#include <cstdio>
#include <fstream>

using namespace std;

// ---------------- storage ----------------
static unordered_map<string, long long> zoneCounts;
static unordered_map<string, array<long long, 24>> slotCounts;

// ---------------- helpers ----------------
static inline bool is_digit(char c) {
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
    return (hour >= 0 && hour <= 23) ? hour : -1;
}

static void processLine(
    char* ls, char* le,
    unordered_map<string, long long>& zc,
    unordered_map<string, array<long long, 24>>& sc
) {
    if (le > ls && le[-1] == '\r') le--;
    if (le <= ls) return;

    char* c1 = (char*)memchr(ls, ',', le - ls);
    if (!c1) return;

    char* c2 = (char*)memchr(c1 + 1, ',', le - (c1 + 1));
    if (!c2 || c2 <= c1 + 1) return;

    char* c3 = (char*)memchr(c2 + 1, ',', le - (c2 + 1));

    const char* timeStart = nullptr;
    const char* timeEnd   = nullptr;

    if (!c3) {
        timeStart = c2 + 1;
        timeEnd   = le;
    } else {
        char* c4 = (char*)memchr(c3 + 1, ',', le - (c3 + 1));
        char* c5 = c4 ? (char*)memchr(c4 + 1, ',', le - (c4 + 1)) : nullptr;
        if (!c4 || !c5) return;

        timeStart = c3 + 1;
        timeEnd   = c4;
    }

    if (!timeStart || !timeEnd || timeEnd <= timeStart) return;

    int hour = parseHour(timeStart, timeEnd);
    if (hour < 0) return;

    string zone(c1 + 1, (size_t)(c2 - (c1 + 1)));

    zc[zone]++;
    sc[zone][hour]++;
}

// ---------------- TripAnalyzer ----------------
void TripAnalyzer::ingestFile(const string& path) {
    zoneCounts.clear();
    slotCounts.clear();

    ifstream file(path);
    if (!file.is_open()) return;

    string line;
    if (!getline(file, line)) return; // header

    while (getline(file, line)) {
        if (line.empty()) continue;
        processLine(&line[0], &line[0] + line.size(), zoneCounts, slotCounts);
    }
}

void TripAnalyzer::ingestStdin() {
    zoneCounts.clear();
    slotCounts.clear();

    const size_t BUF = 1 << 16;
    static char buffer[BUF];
    size_t leftover = 0;

    while (true) {
        size_t bytesRead = fread(buffer + leftover, 1, BUF - leftover, stdin);

        if (bytesRead == 0) {
            if (leftover > 0)
                processLine(buffer, buffer + leftover, zoneCounts, slotCounts);
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

            char* lineEnd = nl;
            cur = nl + 1;

            processLine(lineStart, lineEnd, zoneCounts, slotCounts);

            lineStart = cur;
            leftover = 0;
        }
    }
}

// ---------------- ranking ----------------
vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    vector<ZoneCount> res;
    for (const auto& kv : zoneCounts)
        res.push_back({kv.first, kv.second});

    sort(res.begin(), res.end(),
         [](const ZoneCount& a, const ZoneCount& b) {
             if (a.count != b.count) return a.count > b.count;
             return a.zone < b.zone;
         });

    if ((int)res.size() > k) res.resize(k);
    return res;
}

vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    vector<SlotCount> res;

    for (const auto& kv : slotCounts) {
        for (int h = 0; h < 24; ++h)
            if (kv.second[h] > 0)
                res.push_back({kv.first, h, kv.second[h]});
    }

    sort(res.begin(), res.end(),
         [](const SlotCount& a, const SlotCount& b) {
             if (a.count != b.count) return a.count > b.count;
             if (a.zone != b.zone) return a.zone < b.zone;
             return a.hour < b.hour;
         });

    if ((int)res.size() > k) res.resize(k);
    return res;
}
