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

// =============================================================
// GLOBAL VERİ SAKLAMA (Header değişmediği için mecburi)
// =============================================================
// Zone -> Toplam Sayı
static unordered_map<string, long long> global_zone_counts;
// Zone -> [0-23 Saatleri için Sabit Dizi]
static unordered_map<string, array<long long, 24>> global_slot_counts;

// =============================================================
// YARDIMCI PARSE FONKSİYONLARI (HackerRank Mantığıyla Aynı)
// =============================================================

static bool is_digit_fast(char c) {
    return c >= '0' && c <= '9';
}

// Hızlı saat okuma (stoi kullanmaz)
static int parseHourFast(const char* ts, const char* te) {
    // Windows satır sonu (\r) temizliği
    if (te > ts && te[-1] == '\r') te--;

    // Saat bilgisini bul (Boşluktan sonraki kısım)
    const char* sp = (const char*)memchr(ts, ' ', (size_t)(te - ts));
    if (!sp || sp + 2 >= te) return -1;

    char h1 = sp[1];
    char h2 = sp[2];
    if (!is_digit_fast(h1) || !is_digit_fast(h2)) return -1;

    int hour = (h1 - '0') * 10 + (h2 - '0');
    if (hour < 0 || hour > 23) return -1;
    return hour;
}

// Tek bir satırı işleyen fonksiyon
static void processLineBuffer(char* ls, char* le) {
    if (le > ls && le[-1] == '\r') le--;
    if (le <= ls) return;

    // 1. Virgül
    char* c1 = (char*)memchr(ls, ',', le - ls);
    if (!c1) return;

    // 2. Virgül
    char* c2 = (char*)memchr(c1 + 1, ',', le - (c1 + 1));
    if (!c2 || c2 <= c1 + 1) return;

    // 3. Virgül (3 kolon mu 6 kolon mu ayrımı için)
    char* c3 = (char*)memchr(c2 + 1, ',', le - (c2 + 1));

    const char* timeStart = nullptr;
    const char* timeEnd = nullptr;

    if (!c3) {
        // 3 kolonlu: TripID, Zone, Time
        timeStart = c2 + 1;
        timeEnd = le;
    } else {
        // 6 kolonlu: TripID, Pickup, Dropoff, Time...
        char* c4 = (char*)memchr(c3 + 1, ',', le - (c3 + 1));
        if (!c4) return; // Hatalı satır

        timeStart = c3 + 1;
        timeEnd = c4;
    }

    if (!timeStart || !timeEnd || timeEnd <= timeStart) return;

    int hour = parseHourFast(timeStart, timeEnd);
    if (hour < 0) return;

    // String oluşturma (sadece geçerli veri için)
    string zone(c1 + 1, (size_t)(c2 - (c1 + 1)));

    // Global map'lere kayıt
    global_zone_counts[zone]++;
    global_slot_counts[zone][hour]++;
}

// =============================================================
// SINIF FONKSİYONLARI (analyzer.cpp)
// =============================================================

void TripAnalyzer::ingestFile(const string& path) {
    // 1. TEMİZLİK: Her testte hafızayı sıfırla (Çok Kritik!)
    global_zone_counts.clear();
    global_slot_counts.clear();
    global_zone_counts.reserve(10000);
    global_slot_counts.reserve(10000);

    // 2. DOSYA OKUMA (fopen - binary)
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return;

    // Header'ı atla
    int c;
    while ((c = fgetc(f)) != EOF && c != '\n');

    // 3. BUFFER OKUMA (128 KB)
    const size_t BUF_SIZE = 128 * 1024;
    char* buffer = new char[BUF_SIZE];
    size_t leftover = 0;

    while (true) {
        size_t bytesRead = fread(buffer + leftover, 1, BUF_SIZE - leftover, f);
        if (bytesRead == 0) {
            // Dosya bitti, kalan parça varsa işle
            if (leftover > 0) processLineBuffer(buffer, buffer + leftover);
            break;
        }

        size_t dataSize = leftover + bytesRead;
        char* cur = buffer;
        char* end = buffer + dataSize;
        char* lineStart = cur;

        while (cur < end) {
            // Satır sonunu bul
            char* nl = (char*)memchr(cur, '\n', end - cur);
            if (!nl) {
                // Satır yarım kaldı, başa taşı
                leftover = end - lineStart;
                if (leftover >= BUF_SIZE) leftover = 0;
                else memmove(buffer, lineStart, leftover);
                break;
            }

            // Satırı işle
            processLineBuffer(lineStart, nl);
            
            // Sonraki satıra geç
            cur = nl + 1;
            lineStart = cur;
            leftover = 0;
        }
        if (bytesRead < (BUF_SIZE - leftover)) break;
    }

    delete[] buffer;
    fclose(f);
}

// HackerRank için gereken boş fonksiyon (GitHub testleri kullanmaz ama hata vermemesi için kalsın)
void TripAnalyzer::ingestStdin() {}

vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    vector<ZoneCount> res;
    res.reserve(global_zone_counts.size());

    for (const auto& kv : global_zone_counts) {
        // DİKKAT: .h dosyasındaki struct isimleri: zone ve count
        res.push_back({kv.first, kv.second});
    }

    // Karşılaştırma
    auto comp = [](const ZoneCount& a, const ZoneCount& b) {
        if (a.count != b.count) return a.count > b.count;
        return a.zone < b.zone;
    };

    // Partial Sort (Hız için)
    if ((int)res.size() > k) {
        std::partial_sort(res.begin(), res.begin() + k, res.end(), comp);
        res.resize(k);
    } else {
        std::sort(res.begin(), res.end(), comp);
    }

    return res;
}

vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    vector<SlotCount> res;
    res.reserve(global_slot_counts.size() * 3);

    for (const auto& kv : global_slot_counts) {
        for (int h = 0; h < 24; ++h) {
            if (

