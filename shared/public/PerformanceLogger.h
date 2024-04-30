#include <iostream>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <string>
#include <atomic>
#include <vector>
#include <chrono>
#include "Logger.h"

#pragma once

// Define ENABLE_PERF_LOGGING to enable performance logging, comment out to disable it
//#define ENABLE_PERF_LOGGING

#ifdef ENABLE_PERF_LOGGING

#define PERF_LOG_START(key) PerformanceLogger::getInstance().startSection(key)
#define PERF_LOG_END(key) PerformanceLogger::getInstance().endSection(key)

class PerformanceLogger {
public:
    static PerformanceLogger& getInstance() {
        static PerformanceLogger instance;
        return instance;
    }

    void startSection(const std::string& key) {
        auto& start_time = local_data.start_times[key];
        start_time = std::chrono::high_resolution_clock::now();
    }

    void endSection(const std::string& key) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto& start_time = local_data.start_times[key];
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        log(key, duration);
    }

    void print_stats() {
        std::lock_guard<std::mutex> lock(global_map_mutex);
        LogDebug <<= "--------------------------";
        LogDebug <<= "Key,Total Time,Unit,Average Time,Unit"; // CSV header
        for (const auto& pair : global_map) {
            const auto total = double(pair.second.first) / 1000.0; // Convert nanoseconds to milliseconds
            const auto count = pair.second.second;
            const std::string time_unit = "ms";
            const double average = count > 0 ? total / count : 0.0;
            LogDebug << pair.first << "," << total << "," << time_unit << "," << average << "," <<= time_unit;
        }
        LogDebug <<= "--------------------------";
    }



private:
    struct ThreadLocalData {
        std::unordered_map<std::string, long long> local_map;
        std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> start_times;
        std::unordered_map<std::string, long long> counts;
        int estimated_global_size = 0;  // Estimate of the global map size
    };

    static thread_local ThreadLocalData local_data;
    std::unordered_map<std::string, std::pair<long long, long long>> global_map;
    std::mutex global_map_mutex;
    std::chrono::steady_clock::time_point last_print_time = std::chrono::steady_clock::now();

    PerformanceLogger() {}

    void log(const std::string& key, long long duration) {
        local_data.local_map[key] += duration;
        local_data.counts[key]++;

        int flush_threshold = std::max(1, int(local_data.estimated_global_size * 0.1));
        if (++local_data.counts[key] % flush_threshold == 0) {
            flush_local_data();
            bool print = false;
            {
                std::lock_guard<std::mutex> lock(global_map_mutex);
                auto now = std::chrono::steady_clock::now();
                print = std::chrono::duration_cast<std::chrono::seconds>(now - last_print_time).count() > 10;
                if (print) {
                    last_print_time = now;
                }
            }
            if (print) {
                print_stats();
            }
        }
    }

    void flush_local_data() {
        std::lock_guard<std::mutex> lock(global_map_mutex);
        for (const auto& entry : local_data.local_map) {
            global_map[entry.first].first += entry.second;
            global_map[entry.first].second += local_data.counts[entry.first];
        }
        // Update local estimated size after flushing
        local_data.estimated_global_size = global_map.size();
        local_data.local_map.clear();
        local_data.counts.clear();
    }

public:
    PerformanceLogger(const PerformanceLogger&) = delete;
    PerformanceLogger& operator=(const PerformanceLogger&) = delete;
};

#else
    #define PERF_LOG_START(key)
    #define PERF_LOG_END(key)
#endif
