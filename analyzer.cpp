#include "analyzer.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <array>
#include <cstring>
#include <cstdio>

using namespace std;

// ------------------------------------------------------------------
// ingestFile: 1MB Buffer + Zero-Copy Logic
// ------------------------------------------------------------------
void TripAnalyzer::ingestFile(const std::string& csvPath) {
    // 1. Temizlik
    this->zone_counts.clear();
    this->slot_counts.clear();
    // Tahmini rezervasyon (Rehash maliyetini önler)
    this->zone_counts.reserve(50000); 
    this->slot_counts.reserve(50000);

    // 2. Dosya Açma
    FILE* f = fopen(csvPath.c_str(), "rb");
    if (!f) return;

    // 3. Header'ı atla
    int c;
    while ((c = fgetc(f)) != EOF && c != '\n');

    // 4. BUFFER AYARI: 1MB (Volume testleri için daha büyük buffer iyidir)
    const size_t BUFFER_SIZE = 1024 * 1024; 
    char* buffer = new char[BUFFER_SIZE];
    size_t leftover = 0;

    while (true) {
        size_t bytesRead = fread(buffer + leftover, 1, BUFFER_SIZE - leftover, f);
        if (bytesRead == 0) break;

        char* current = buffer;
        char* end = buffer + leftover + bytesRead; // dataSize
        
        while (current < end) {
            // Satır sonunu bul
            char* lineEnd = (char*)memchr(current, '\n', end - current);

            if (!lineEnd) {
                // Yarım kalan satır
                leftover = end - current;
                if (leftover >= BUFFER_SIZE) leftover = 0; // Aşırı uzun satır koruması
                else memmove(buffer, current, leftover);
                break;
            }

            // Parsing Mantığı
            char* c1 = (char*)memchr(current, ',', lineEnd - current);
            if (c1) {
                char* c2 = (char*)memchr(c1 + 1, ',', lineEnd - (c1 + 1));
                if (c2) {
                    if (c2 > c1 + 1) {
                        char* timeStart = nullptr;
                        char* possibleTime = c2 + 1;

                        // 3 kolon mu 6 kolon mu?
                        if (possibleTime < lineEnd && (*possibleTime >= '0' && *possibleTime <= '9')) {
                            timeStart = possibleTime;
                        } else {
                            char* c3 = (char*)memchr(c2 + 1, ',', lineEnd - (c2 + 1));
                            if (c3) timeStart = c3 + 1;
                        }

                        if (timeStart && (lineEnd - timeStart >= 13)) {
                            // Hızlı saat okuma
                            char h1 = timeStart[11];
                            char h2 = timeStart[12];

                            if (h1 >= '0' && h1 <= '9' && h2 >= '0' && h2 <= '9') {
                                int hour = (h1 - '0') * 10 + (h2 - '0');
                                if (hour >= 0 && hour <= 23) {
                                    // String oluşturma sadece geçerli veri için yapılır
                                    // const char* kullanarak string constructor çağırıyoruz
                                    string zone(c1 + 1, c2 - (c1 + 1));
                                    
                                    this->zone_counts[zone]++;
                                    this->slot_counts[zone][hour]++;
                                }
                            }
                        }
                    }
                }
            }
            // Sonraki satır
            current = lineEnd + 1;
            leftover = 0;
        }
        
        // Eğer dosya bittiyse ve buffer işlendiyse çık
        if (bytesRead < (BUFFER_SIZE - leftover)) break;
    }

    delete[] buffer;
    fclose(f);
}

// ------------------------------------------------------------------
// topZones: PARTIAL SORT OPTIMIZASYONU
// ------------------------------------------------------------------
std::vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    std::vector<ZoneCount> result;
    result.reserve(zone_counts.size());

    for (const auto& pair : zone_counts) {
        result.push_back({ pair.first, pair.second });
    }

    // Karşılaştırma fonksiyonu
    auto comparator = [](const ZoneCount& a, const ZoneCount& b) {
        if (a.count != b.count) return a.count > b.count;
        return a.zone_id < b.zone_id;
    };

    // KRİTİK NOKTA: Milyonluk listeyi tam sıralama, sadece ilk K tanesini sırala.
    if ((int)result.size() > k) {
        std::partial_sort(result.begin(), result.begin() + k, result.end(), comparator);
        result.resize(k);
    } else {
        std::sort(result.begin(), result.end(), comparator);
    }

    return result;
}

// ------------------------------------------------------------------
// topBusySlots: PARTIAL SORT OPTIMIZASYONU
// ------------------------------------------------------------------
std::vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    std::vector<SlotCount> result;
    // Tahmini boyut
    result.reserve(slot_counts.size() * 4); 

    for (const auto& entry : slot_counts) {
        const std::string& z_id = entry.first;
        const auto& hours = entry.second; 

        for (int h = 0; h < 24; ++h) {
            if (hours[h] > 0) {
                result.push_back({ z_id, h, hours[h] });
            }
        }
    }

    // Karşılaştırma fonksiyonu
    auto comparator = [](const SlotCount& a, const SlotCount& b) {
        if (a.count != b.count) return a.count > b.count;
        if (a.zone_id != b.zone_id) return a.zone_id < b.zone_id;
        return a.hour < b.hour;
    };

    // KRİTİK NOKTA: Volume testini geçiren asıl değişiklik burası.
    if ((int)result.size() > k) {
        std::partial_sort(result.begin(), result.begin() + k, result.end(), comparator);
        result.resize(k);
    } else {
        std::sort(result.begin(), result.end(), comparator);
    }

    return result;
}

