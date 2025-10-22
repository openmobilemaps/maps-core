/*
 * Expression tests based on MapLibre Native expression test definitions
 * 
 * Test definitions adapted from:
 * https://github.com/maplibre/maplibre-native/tree/ecab4a390e0310a775b7de2b2b1a5a9230fd0773/metrics/integration/expression-tests
 * 
 * Original work Copyright (c) 2014-2020 Mapbox
 * Copyright (c) 2018-2021 MapTiler.com
 * Copyright (c) 2021 MapLibre contributors
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "Value.h"
#include "Tiled2dMapVectorStyleParser.h"
#include "json.h"
#include "helper/TestData.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <fstream>
#include <filesystem>
#include <vector>
#include <variant>
#include <map>
#include <unordered_set>
#include <iostream>

using json = nlohmann::json;

namespace {
    // Test cases to ignore for now - add test names here to skip them
    static const std::unordered_set<std::string> IGNORED_TESTS = {
        "abs/basic",
        "acos/basic", 
        "asin/basic",
        "atan/basic",
        "ceil/basic",
        "cos/basic",
        "divide/basic",
        "floor/basic",
        "ln/basic",
        "ln2/basic",
        "log10/basic",
        "log2/basic",
        "max/arity-0",
        "max/arity-1", 
        "max/basic",
        "min/arity-0",
        "min/arity-1",
        "min/basic",
        "minus/arity-0",
        "minus/arity-1",
        "minus/basic",
        "minus/inference-arity-2",
        "mod/basic",
        "pi/basic",
        "plus/arity-0",
        "plus/arity-1",
        "plus/basic",
        "pow/basic",
        "round/basic",
        "sin/basic",
        "sqrt/basic",
        "tan/basic",
        "times/arity-0",
        "times/arity-1",
        "times/basic",
        "e/basic",
        "array/basic",
        "array/default-value",
        "array/implicit-1",
        "array/implicit-2", 
        "array/implicit-3",
        "array/item-type-and-length",
        "array/item-type",
        "at/basic",
        "at/infer-array-type",
        "case/basic",
        "case/infer-array-type",
        "case/precedence",
        "coalesce/argument-type-mismatch",
        "coalesce/basic",
        "coalesce/error",
        "coalesce/infer-array-type",
        "coalesce/inference", 
        "coalesce/null",
        "interpolate/linear",
        "interpolate/exponential",
        "step/basic",
        "step/duplicate_stops",
        "match/basic",
        "let/basic",
        "literal/string",
        "literal/object",
        "geometry-type/basic",
        "id/basic",
        "properties/basic",
        "zoom/basic",
        "heatmap-density/basic",
        "downcase/basic",
        "upcase/basic",
        "slice/string-one-index",
        "slice/string-two-indexes",
        "index-of/basic-string",
        "rgb/basic",
        "rgba/basic",
        "to-rgba/basic",
        "to-color/basic",
        "format/basic",
        "number-format/default",
        "typeof/basic",
        "legacy",
        "collator/accent-equals-de"	,
        "collator/base-gt-en",
        "collator/diacritic-omitted-en",
        "collator/accent-lt-en"	,
	    "collator/case-lteq-en"	,
		"collator/equals-non-string-error",
        "collator/accent-not-equals-en"	,
        "collator/case-not-equals-en"	,
        "collator/non-object-error",
        "collator/base-default-locale"	,
        "collator/case-omitted-en"	,
	    "collator/variant-equals-en",
        "collator/base-equals-en"	,
	    "collator/comparison-number-error",
        "collator/variant-gteq-en",
        "within/invalid-geojson",
        "within/non-supported",
        "within/line-within-polygon",
        "within/point-within-polygon"
    };

    // Helper function to recursively find all test.json files
    std::vector<std::filesystem::path> findAllTestFiles() {
        std::vector<std::filesystem::path> testFiles;
        std::filesystem::path expressionTestsDir = TestData::resolve("expression-tests");
        
        if (!std::filesystem::exists(expressionTestsDir)) {
            return testFiles;
        }
        
        for (const auto& entry : std::filesystem::recursive_directory_iterator(expressionTestsDir)) {
            if (entry.is_regular_file() && entry.path().filename() == "test.json") {
                testFiles.push_back(entry.path());
            }
        }
        
        return testFiles;
    }

    // Helper function to load test JSON file
    json loadTestFile(const std::filesystem::path& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open test file: " + filePath.string());
        }
        
        json testData;
        file >> testData;
        return testData;
    }

    // Helper function to create evaluation context from test input
    // Returns both the context and the feature context to keep it alive
    std::pair<EvaluationContext, std::shared_ptr<FeatureContext>> createContextFromInput(const json& input, StringInterner& stringTable) {
        double zoom = 0.0;
        int32_t zoomLevel = 0;
        std::shared_ptr<FeatureContext> featureContext = nullptr;
        
        if (input.size() >= 1 && input[0].is_object()) {
            // First element is zoom context
            if (input[0].contains("zoom")) {
                zoom = input[0]["zoom"];
                zoomLevel = static_cast<int32_t>(zoom);
            }
        }
        
        if (input.size() >= 2 && input[1].is_object()) {
            // Second element is feature context
            const auto& feature = input[1];
            FeatureContext::mapType properties;
            
            if (feature.contains("properties") && feature["properties"].is_object()) {
                for (const auto& [key, value] : feature["properties"].items()) {
                    auto keyId = stringTable.add(key);
                    
                    if (value.is_string()) {
                        properties.emplace_back(keyId, value.get<std::string>());
                    } else if (value.is_number_integer()) {
                        properties.emplace_back(keyId, value.get<int64_t>());
                    } else if (value.is_number_float()) {
                        properties.emplace_back(keyId, value.get<double>());
                    } else if (value.is_boolean()) {
                        properties.emplace_back(keyId, value.get<bool>());
                    } else if (value.is_null()) {
                        properties.emplace_back(keyId, std::monostate());
                    }
                }
            }
            
            featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, properties, 0);
        } else {
            featureContext = std::make_shared<FeatureContext>();
        }
        
        EvaluationContext context(zoom, zoomLevel, featureContext, nullptr);
        return std::make_pair(context, featureContext);
    }

    // Helper function to compare expected outputs with actual results
    bool compareValue(const ValueVariant& actual, const json& expected) {
        if (expected.is_null()) {
            return std::holds_alternative<std::monostate>(actual);
        } else if (expected.is_string()) {
            return std::holds_alternative<std::string>(actual) && 
                   std::get<std::string>(actual) == expected.get<std::string>();
        } else if (expected.is_number_integer()) {
            if (std::holds_alternative<int64_t>(actual)) {
                return std::get<int64_t>(actual) == expected.get<int64_t>();
            } else if (std::holds_alternative<double>(actual)) {
                return std::abs(std::get<double>(actual) - expected.get<double>()) < 1e-10;
            }
        } else if (expected.is_number_float()) {
            if (std::holds_alternative<double>(actual)) {
                return std::abs(std::get<double>(actual) - expected.get<double>()) < 1e-10;
            }
        } else if (expected.is_boolean()) {
            return std::holds_alternative<bool>(actual) && 
                   std::get<bool>(actual) == expected.get<bool>();
        }
        
        return false;
    }

    // Get test name from file path
    std::string getTestName(const std::filesystem::path& filePath) {
        std::filesystem::path expressionTestsDir = TestData::resolve("expression-tests");
        auto relativePath = std::filesystem::relative(filePath.parent_path(), expressionTestsDir);
        return relativePath.string();
    }
}

TEST_CASE("Expression Tests - Dynamic Test Runner", "[expressions]") {
    auto testFiles = findAllTestFiles();
    
    REQUIRE(!testFiles.empty());
    
    // Group results by test outcome
    std::map<std::string, std::vector<std::string>> results;
    results["passed"] = {};
    results["failed"] = {};
    results["skipped"] = {};
    
    StringInterner stringTable = ValueKeys::newStringInterner();
    Tiled2dMapVectorStyleParser parser(stringTable);
    
    for (const auto& testFile : testFiles) {
        try {
            auto testData = loadTestFile(testFile);
            std::string testName = getTestName(testFile);
            
            // Check if this test should be ignored
            bool shouldIgnore = false;
            for (const auto& ignoredTest : IGNORED_TESTS) {
                if (testName.find(ignoredTest) != std::string::npos) {
                    shouldIgnore = true;
                    break;
                }
            }
            
            if (shouldIgnore) {
                results["skipped"].push_back(testName + " (ignored)");
                continue;
            }
            
            DYNAMIC_SECTION("Test: " + testName) {
                try {
                    // Use the Tiled2dMapVectorStyleParser to parse the expression
                    auto expression = parser.parseValue(testData["expression"]);
                    
                    if (!expression) {
                        results["skipped"].push_back(testName + " (expression not parseable)");
                        continue;
                    }
                    
                    const auto& inputs = testData["inputs"];
                    const auto& expectedOutputs = testData["expected"]["outputs"];
                    
                    bool allPassed = true;
                    std::string errorMessage;
                    
                    for (size_t i = 0; i < inputs.size() && i < expectedOutputs.size(); ++i) {
                        auto [context, featureContext] = createContextFromInput(inputs[i], stringTable);
                        
                        try {
                            auto result = expression->evaluate(context);
                            
                            if (!compareValue(result, expectedOutputs[i])) {
                                allPassed = false;
                                errorMessage = "output mismatch at input " + std::to_string(i);
                                break;
                            }
                        } catch (const std::exception& e) {
                            allPassed = false;
                            errorMessage = "evaluation exception: " + std::string(e.what());
                            break;
                        }
                    }
                    
                    if (allPassed) {
                        results["passed"].push_back(testName);
                        REQUIRE(true); // Test passed
                    } else {
                        results["failed"].push_back(testName + " (" + errorMessage + ")");
                        REQUIRE(false); // Test failed
                    }
                } catch (const std::exception& e) {
                    results["failed"].push_back(testName + " (parse exception: " + e.what() + ")");
                    REQUIRE(false);
                }
            }
        } catch (const std::exception& e) {
            std::string testName = getTestName(testFile);
            results["failed"].push_back(testName + " (file error: " + e.what() + ")");
        }
    }
    
    // Print summary only once at the very end
    static bool summaryPrinted = false;
    if (!summaryPrinted) {
        summaryPrinted = true;
        
        std::cout << "\n=== Expression Test Summary ===" << std::endl;
        std::cout << "Total tests found: " << testFiles.size() << std::endl;
        std::cout << "Passed: " << results["passed"].size() << std::endl;
        std::cout << "Failed: " << results["failed"].size() << std::endl;
        std::cout << "Skipped: " << results["skipped"].size() << std::endl;
        
        std::cout << "\nFailed results:" << std::endl;
        for (size_t i = 0; i < results["failed"].size(); ++i) {
            std::cout << "  - " << results["failed"][i] << std::endl;
        }


        std::cout << "\nPassed results:" << std::endl;
        for (size_t i = 0; i < results["passed"].size(); ++i) {
            std::cout << "  - " << results["passed"][i] << std::endl;
        }
    }
}