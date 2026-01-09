#include "analyzer.h"
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

struct SlotKey {
    std::string zone;
    int hour;
    bool operator==(const SlotKey& other) const {
        return hour == other.hour && zone == other.zone;
    }
};

struct SlotKeyHash {
    std::size_t operator()(const SlotKey& k) const {
        return std::hash<std::string>()(k.zone) ^ (std::hash<int>()(k.hour) << 1);
    }
};

namespace {
    std::unordered_map<std::string, long long> zoneCounts;
    std::unordered_map<SlotKey, long long, SlotKeyHash> slotCounts;
}

static inline bool extractHour(std::string_view timeStr, int& hourOut) {
    if (timeStr.size() < 13) return false;
    try {
        hourOut = std::stoi(std::string(timeStr.substr(11, 2)));
        return (hourOut >= 0 && hourOut <= 23);
    }
    catch (...) {
        return false;
    }
}

void TripAnalyzer::ingestFile(const std::string& csvPath) {
    zoneCounts.clear();
    slotCounts.clear();
    zoneCounts.reserve(100000);
    slotCounts.reserve(100000);

    std::ifstream file(csvPath);
    if (!file.is_open()) return;

    std::string line;
    std::getline(file, line); // skip header

    while (std::getline(file, line)) {
        // Split into tokens
        std::vector<std::string_view> tokens;
        tokens.reserve(6);
        size_t start = 0, end;
        while ((end = line.find(',', start)) != std::string::npos) {
            tokens.emplace_back(line.c_str() + start, end - start);
            start = end + 1;
        }
        tokens.emplace_back(line.c_str() + start);

        if (tokens.size() != 6) continue;

        bool anyEmpty = false;
        for (auto& t : tokens) {
            if (t.empty()) { anyEmpty = true; break; }
        }
        if (anyEmpty) continue;

        std::string_view zone = tokens[1];
        std::string_view time = tokens[3];

        int hour;
        if (!extractHour(time, hour)) continue;

        zoneCounts[std::string(zone)]++;
        slotCounts[{std::string(zone), hour}]++;
    }
}

std::vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    std::vector<ZoneCount> result;
    result.reserve(zoneCounts.size());
    for (const auto& kv : zoneCounts) {
        result.push_back({ kv.first, kv.second });
    }

    auto cmp = [](const ZoneCount& a, const ZoneCount& b) {
        if (a.count != b.count) return a.count > b.count;
        return a.zone < b.zone;
    };

    if ((int)result.size() > k) {
        std::nth_element(result.begin(), result.begin() + k, result.end(), cmp);
        result.resize(k);
    }
    std::sort(result.begin(), result.end(), cmp);
    return result;
}

std::vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    std::vector<SlotCount> result;
    result.reserve(slotCounts.size());
    for (const auto& kv : slotCounts) {
        result.push_back({ kv.first.zone, kv.first.hour, kv.second });
    }

    auto cmp = [](const SlotCount& a, const SlotCount& b) {
        if (a.count != b.count) return a.count > b.count;
        if (a.zone != b.zone) return a.zone < b.zone;
        return a.hour < b.hour;
    };

    if ((int)result.size() > k) {
        std::nth_element(result.begin(), result.begin() + k, result.end(), cmp);
        result.resize(k);
    }
    std::sort(result.begin(), result.end(), cmp);
    return result;
}
