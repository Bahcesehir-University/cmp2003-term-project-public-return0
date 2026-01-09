#include "analyzer.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>

using namespace std;



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

// Bu fonksiyon artik map referanslarini parametre olarak aliyor
static void processLine(char* ls, char* le,
                        unordered_map<string, long long>& zc,
                        unordered_map<string, array<long long, 24>>& sc) {
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
        // 3 kolonlu
        timeStart = c2 + 1;
        timeEnd = le;
    } else {
        // 6 kolonlu
        char* c4 = (char*)memchr(c3 + 1, ',', le - (c3 + 1));
        if (!c4) return;
        
        timeStart = c3 + 1;
        timeEnd = c4;
    }

    if (!timeStart || !timeEnd || timeEnd <= timeStart) return;

    int hour = parseHour(timeStart, timeEnd);
    if (hour < 0) return;

    string zone(c1 + 1, (size_t)(c2 - (c1 + 1)));

    zc[zone]++;
    sc[zone][hour]++;
}


void TripAnalyzer::ingestFile(const string& csvPath) {
    // 1. Temizlik
    this->zoneCounts.clear();
    this->slotCounts.clear();
    
    // 2. Performans (Rehash önleme)
    this->zoneCounts.reserve(50000);
    this->slotCounts.reserve(50000);

    FILE* f = fopen(csvPath.c_str(), "rb");
    if (!f) return;

    // 3. Buffer (1MB - Volume Test icin kritik)
    const size_t BUF = 1024 * 1024;
    char* buffer = new char[BUF];
    size_t leftover = 0;

    // Header atla
    int c;
    while ((c = fgetc(f)) != EOF && c != '\n');

    while (true) {
        size_t bytesRead = fread(buffer + leftover, 1, BUF - leftover, f);
        if (bytesRead == 0) {
            if (leftover > 0)
                processLine(buffer, buffer + leftover, this->zoneCounts, this->slotCounts);
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
                if (leftover >= BUF) leftover = 0;
                else memmove(buffer, lineStart, leftover);
                break;
            }

            // this->zoneCounts ve this->slotCounts referans olarak gonderiliyor
            processLine(lineStart, nl, this->zoneCounts, this->slotCounts);
            
            cur = nl + 1;
            lineStart = cur;
            leftover = 0;
        }
        if (bytesRead < (BUF - leftover)) break;
    }
    delete[] buffer;
    fclose(f);
}

void TripAnalyzer::ingestStdin() {
    // GitHub testleri kullanmaz
}

vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    vector<ZoneCount> res;
    res.reserve(zoneCounts.size());
    for (const auto& kv : zoneCounts)
        res.push_back({kv.first, kv.second});

    auto comp = [](const ZoneCount& a, const ZoneCount& b) {
        if (a.count != b.count) return a.count > b.count;
        return a.zone < b.zone;
    };

    // PARTIAL SORT: Hiz testi icin
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
    res.reserve(slotCounts.size() * 3);

    for (const auto& kv : slotCounts) {
        for (int h = 0; h < 24; ++h) {
            if (kv.second[h] > 0)
                res.push_back({kv.first, h, kv.second[h]});
        }
    }

    auto comp = [](const SlotCount& a, const SlotCount& b) {
        if (a.count != b.count) return a.count > b.count;
        if (a.zone != b.zone) return a.zone < b.zone;
        return a.hour < b.hour;
    };

    // PARTIAL SORT: Hiz testi icin
    if ((int)res.size() > k) {
        partial_sort(res.begin(), res.begin() + k, res.end(), comp);
        res.resize(k);
    } else {
        sort(res.begin(), res.end(), comp);
    }

    return res;
}

