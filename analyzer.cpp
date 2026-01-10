#include "analyzer.h"
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

