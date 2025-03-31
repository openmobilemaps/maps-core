/*
 * Copyright (c) 2025 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TestData.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>

#ifdef OPENMOBILEMAPS_TESTDATA_DIR_
#define xstr(X) str(X)
#define str(X) #X
// Absolute path to data/ dir. Set from build system.
static const char *TESTDATA_DIR = xstr(OPENMOBILEMAPS_TESTDATA_DIR);
#else
// Absolute path to data/ dir. Defined relative to this file; usually works but
// __FILE__ is not guaranteed to contain an absolute path.
static const char *TESTDATA_DIR = __FILE__ "../../../data";
#endif

static std::filesystem::path getTestdataDir() {
    const char *testdataDirEnv = getenv("OPENMOBILEMAPS_TESTDATA_DIR");
    if (testdataDirEnv) {
        std::cerr << "Testdata directory set from envvar OPENMOBILEMAPS_TESTDATA_DIR=" << testdataDirEnv << std::endl;
    }
    const char *testdataDir = testdataDirEnv ? testdataDirEnv : TESTDATA_DIR;
    return std::filesystem::path(testdataDir).lexically_normal();
}

std::string TestData::resolve(const char *relPath) {
    static std::filesystem::path testdataDir = getTestdataDir();
    return testdataDir / std::filesystem::path(relPath);
}

std::string TestData::readFileToString(const char *relPath) {
    std::string filePath = resolve(relPath);
    std::ifstream file{filePath};
    if (!file) {
        throw std::runtime_error(std::string("Unable to open file ") + filePath + ": " + strerror(errno));
    }
    std::string content{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    if (file.fail()) {
        throw std::runtime_error(std::string("Error reading file ") + filePath + ": " + strerror(errno));
    }
    return content;
}

std::vector<char> TestData::readFileToBuffer(const char *relPath) {
    std::string filePath = resolve(relPath);
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error(std::string("Unable to open file ") + filePath + ": " + strerror(errno));
    }

    std::size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(fileSize);
    if (!file.read(buffer.data(), fileSize)) {
        throw std::runtime_error(std::string("Error reading file ") + filePath + ": " + strerror(errno));
    }

    return buffer;
}
