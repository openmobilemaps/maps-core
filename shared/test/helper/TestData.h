/*
 * Copyright (c) 2025 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once
#include <vector>
#include <string>

// Helper for loading test data.
// Files are loaded from the <project-root>/shared/test/data/ directory.
// This absolute path is determined at build time and is independent of the
// runtime working directory of the test executable.
class TestData {
public:
    // Returns the absolute path to file at path relative to
    // <project-root>/shared/test/data directory
    static std::string resolve(const char *relPath);

    // Read the file at path into a string.
    // relPath is relative to the <project-root>/shared/test/data/ directory.
    static std::string readFileToString(const char *relPath);
    
    // Read the file at path into a char buffer.
    // relPath is relative to the <project-root>/shared/test/data/ directory.
    static std::vector<char> readFileToBuffer(const char *relPath);
};
