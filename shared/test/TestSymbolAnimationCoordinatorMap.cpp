#include "SymbolAnimationCoordinatorMap.h"
#include "helper/TestData.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <fstream>
#include <random>
#include <thread>
#include <vector>

// Function to read the log file and parse the arguments
std::vector<std::tuple<size_t, Vec2D, int, double, double, int64_t, int64_t>> readLogFile(const char *filename) {
    std::vector<std::tuple<size_t, Vec2D, int, double, double, int64_t, int64_t>> logEntries;
    std::ifstream file(TestData::resolve(filename));
    std::string line;

    std::string threadIdLabel, crossTileIdentifierLabel, coordLabel, zoomIdentifierLabel, xToleranceLabel, yToleranceLabel, animationDurationLabel, animationDelayLabel;
    size_t crossTileIdentifier;
    double coordX;
    double coordY;
    int zoomIdentifier;
    double xTolerance, yTolerance;
    int64_t animationDuration, animationDelay;

    std::vector<std::string> tokens;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        tokens.clear();
        while (std::getline(iss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() == 8) {
            // Remove labels and parse values
            crossTileIdentifier = std::stoull(tokens[1].substr(tokens[1].find(":") + 1));
            coordX = std::stod(tokens[2].substr(tokens[2].find("(") + 1, tokens[2].find(";") - tokens[2].find("(") - 1));
            coordY = std::stod(tokens[2].substr(tokens[2].find(";") + 1, tokens[2].find(")") - tokens[2].find(";") - 1));
            zoomIdentifier = std::stoi(tokens[3].substr(tokens[3].find(":") + 1));
            xTolerance = std::stod(tokens[4].substr(tokens[4].find(":") + 1));
            yTolerance = std::stod(tokens[5].substr(tokens[5].find(":") + 1));
            animationDuration = std::stoll(tokens[6].substr(tokens[6].find(":") + 1));
            animationDelay = std::stoll(tokens[7].substr(tokens[7].find(":") + 1));

            logEntries.emplace_back(crossTileIdentifier, Vec2D{coordX, coordY}, zoomIdentifier, xTolerance, yTolerance, animationDuration, animationDelay);
        }
    }

    return logEntries;
}

TEST_CASE("functionality") {
    SymbolAnimationCoordinatorMap coordinatorMap;

    SECTION("return same coordinator") {
        std::shared_ptr<SymbolAnimationCoordinator> first, second;
        {
            first = coordinatorMap.getOrAddAnimationController(0, Vec2D(0, 0), 0, 0.0, 0.0, 10, 0);
            first->getIconAlpha(0.5, 10);
            first->getIconAlpha(0.5, 20);
            REQUIRE(first->getIconAlpha(1.0, 20) == 0.5);
        }

        {
            second = coordinatorMap.getOrAddAnimationController(0, Vec2D(0, 0), 1, 0.0, 0.0, 10, 0);
            REQUIRE(second->getIconAlpha(1.0, 20) == 0.5);
        }

        REQUIRE(first == second);
    }

    SECTION("return different coordinators for different crossTileIdentifiers") {
        auto first = coordinatorMap.getOrAddAnimationController(1, Vec2D(0, 0), 0, 0.0, 0.0, 10, 0);
        auto second = coordinatorMap.getOrAddAnimationController(2, Vec2D(0, 0), 1, 0.0, 0.0, 10, 0);
        REQUIRE(first != second);
    }

    SECTION("return same coordinator for same crossTileIdentifier and zoomIdentifier") {
        auto first = coordinatorMap.getOrAddAnimationController(1, Vec2D(0, 0), 0, 0.0, 0.0, 10, 0);
        auto second = coordinatorMap.getOrAddAnimationController(1, Vec2D(0, 0), 0, 0.0, 0.0, 10, 0);
        REQUIRE(first == second);
    }

    SECTION("return same coordinators for same crossTileIdentifier but different zoomIdentifiers") {
        auto first = coordinatorMap.getOrAddAnimationController(1, Vec2D(0, 0), 0, 0.0, 0.0, 10, 0);
        auto second = coordinatorMap.getOrAddAnimationController(1, Vec2D(0, 0), 1, 0.0, 0.0, 10, 0);
        REQUIRE(first == second);
    }

    SECTION("clearAnimationCoordinators removes unused coordinators") {
        auto first = coordinatorMap.getOrAddAnimationController(1, Vec2D(0, 0), 0, 0.0, 0.0, 10, 0);
        coordinatorMap.clearAnimationCoordinators();
        auto second = coordinatorMap.getOrAddAnimationController(1, Vec2D(0, 0), 1, 0.0, 0.0, 10, 0);
        REQUIRE(first != second);
    }

    SECTION("clearAnimationCoordinators removes only unused coordinators") {
        auto first = coordinatorMap.getOrAddAnimationController(1, Vec2D(0, 0), 0, 0.0, 0.0, 10, 0);
        first->increaseUsage();
        coordinatorMap.clearAnimationCoordinators();
        auto second = coordinatorMap.getOrAddAnimationController(1, Vec2D(0, 0), 1, 0.0, 0.0, 10, 0);
        REQUIRE(first == second);
    }
};

TEST_CASE("SymbolAnimationCoordinatorMap with multiple threads") {

    BENCHMARK("Benchmark random data") {
        std::atomic<size_t> counter{0};
        int callsNum = 1000;
        size_t numThreads = std::thread::hardware_concurrency();

        SymbolAnimationCoordinatorMap coordinatorMap;

        auto threadFunc = [&coordinatorMap, &counter, &callsNum](size_t threadIndex) {
            for (size_t i = 0; i < callsNum; ++i) {
                size_t crossTileIdentifier = threadIndex * 1000 + i;
                Vec2D coord{static_cast<double>(threadIndex), static_cast<double>(i)};
                int zoomIdentifier = static_cast<int>(threadIndex % 10);
                double xTolerance = 0.1;
                double yTolerance = 0.1;
                int64_t animationDuration = 300;
                int64_t animationDelay = 0;

                coordinatorMap.getOrAddAnimationController(crossTileIdentifier, coord, zoomIdentifier, xTolerance, yTolerance,
                                                           animationDuration, animationDelay);
                ++counter;
            }
        };

        std::vector<std::thread> threads;

        for (size_t i = 0; i < numThreads; ++i) {
            threads.emplace_back(threadFunc, i);
        }

        for (auto &thread : threads) {
            thread.join();
        }

        REQUIRE(counter == numThreads * callsNum);
    };


    auto logEntries = readLogFile("symbolAnimationCoordinator/recording.txt");

    BENCHMARK("Simulate from log file") {
        std::atomic<size_t> counter{0};
        size_t numThreads = std::thread::hardware_concurrency();

        int runCount = 1;

        SymbolAnimationCoordinatorMap coordinatorMap;

        auto threadFunc = [&coordinatorMap, &counter, &runCount, &logEntries](size_t threadIndex, size_t numThreads) {
            std::mt19937 g(Catch::getSeed());
            std::uniform_int_distribution<size_t> dist(0, logEntries.size() - 1);

            for (int run = 0; run < runCount; ++run) {
                for (size_t i = threadIndex; i < logEntries.size(); i += numThreads) {
                    size_t randomIndex = dist(g);
                    auto &[crossTileIdentifier, coord, zoomIdentifier, xTolerance, yTolerance, animationDuration, animationDelay] =
                        logEntries[randomIndex];
                    coordinatorMap.getOrAddAnimationController(crossTileIdentifier, coord, zoomIdentifier, xTolerance, yTolerance,
                                                               animationDuration, animationDelay);
                    ++counter;
                }
            }
        };

        std::vector<std::thread> threads;

        for (size_t i = 0; i < numThreads; ++i) {
            threads.emplace_back(threadFunc, i, numThreads);
        }

        for (auto &thread : threads) {
            thread.join();
        }

        REQUIRE(!logEntries.empty());
        REQUIRE(counter == logEntries.size() * runCount);

        return counter == runCount;
    };
}
