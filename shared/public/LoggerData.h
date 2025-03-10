// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from map_helpers.djinni

#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

struct LoggerData final {
    std::string id;
    std::vector<int64_t> buckets;
    int32_t bucketSizeMs;
    int64_t start;
    int64_t end;
    int64_t numSamples;
    double average;
    double variance;
    double stdDeviation;

    LoggerData(std::string id_,
               std::vector<int64_t> buckets_,
               int32_t bucketSizeMs_,
               int64_t start_,
               int64_t end_,
               int64_t numSamples_,
               double average_,
               double variance_,
               double stdDeviation_)
    : id(std::move(id_))
    , buckets(std::move(buckets_))
    , bucketSizeMs(std::move(bucketSizeMs_))
    , start(std::move(start_))
    , end(std::move(end_))
    , numSamples(std::move(numSamples_))
    , average(std::move(average_))
    , variance(std::move(variance_))
    , stdDeviation(std::move(stdDeviation_))
    {}
};
