#include <iostream>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <string>
#include <atomic>
#include <vector>
#include <chrono>
#include "Logger.h"

// Define ENABLE_PERF_LOGGING to enable performance logging, comment out to disable it
//#define ENABLE_PERF_LOGGING

#ifdef ENABLE_PERF_LOGGING
#define PERF_LOG_START(key) PerformanceLogger::getInstance().startSection(key)
#define PERF_LOG_END(key) PerformanceLogger::getInstance().endSection(key)
#else
#define PERF_LOG_START(key)
#define PERF_LOG_END(key)
#endif

#ifdef ENABLE_PERF_LOGGING


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

    // Enhanced print_stats to show total and average durations
    void print_stats() {
        std::lock_guard<std::mutex> lock(global_map_mutex);
        LogDebug <<= "----PerformanceLogger-----";
        for (const auto& pair : global_map) {
            auto total = double(pair.second.first) / 1000.0;
            auto count = pair.second.second;
            double average = count > 0 ? total / count : 0.0;
            LogDebug << pair.first << ": Total = " << total << " ms, Average = " << average <<= " ms";
        }
        LogDebug <<= "--------------------------";
    }

private:
    struct ThreadLocalData {
        std::unordered_map<std::string, long long> local_map;  // Sum of durations for each key
        std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> start_times;
        std::unordered_map<std::string, long long> counts;     // Number of logs for each key
    };

    static thread_local ThreadLocalData local_data;
    std::unordered_map<std::string, std::pair<long long, long long>> global_map;  // Pair of total duration and count
    std::mutex global_map_mutex;
    std::chrono::steady_clock::time_point last_print_time = std::chrono::steady_clock::now();


    PerformanceLogger() {}

    void log(const std::string& key, long long duration) {
        local_data.local_map[key] += duration;
        local_data.counts[key]++;

        // Periodically flush to reduce lock contention
        if (++local_data.counts[key] % 1000 == 0) {
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
        local_data.local_map.clear();
        local_data.counts.clear();
    }

public:
    PerformanceLogger(const PerformanceLogger&) = delete;
    PerformanceLogger& operator=(const PerformanceLogger&) = delete;
};

thread_local PerformanceLogger::ThreadLocalData PerformanceLogger::local_data;
#endif
