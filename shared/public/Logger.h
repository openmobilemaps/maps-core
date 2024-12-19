/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#ifndef MAPSCORE__Logger__Logger__
#define MAPSCORE__Logger__Logger__

#ifndef LogError
#define LogError utility::Logger(0)
#endif

#ifndef LogWarning
#define LogWarning utility::Logger(1)
#endif

#ifndef LogDebug
#define LogDebug utility::Logger(2)
#endif

#ifndef LogInfo
#define LogInfo utility::Logger(3)
#endif

#ifndef LogTrace
#define LogTrace utility::Logger(4)
#endif

#include <iostream>
#include <sstream>

namespace utility {
class Logger {
  public:
    std::stringstream &stream() const;

    Logger(int p);

    template <typename T> friend bool operator<<=(const Logger &logger, T thing);

    template <typename T> friend const Logger &operator<<(const Logger &logger, T thing);

  private:
    void log(int prio, const char *tag, const char *fmt, ...) const;

  private:
    mutable std::stringstream ss;
    mutable int priority = -1;
};

template <typename T> const Logger &operator<<(const Logger &logger, T thing) {
    logger.stream() << thing;
    return logger;
}

template <typename T> bool operator<<=(const Logger &logger, T thing) {
    logger.stream() << thing;

    logger.log(3, "Shared-Lib-C++:", logger.stream().str().c_str());

    logger.stream().str("");
    logger.priority = -1;

    return true;
}
} // namespace utility

#endif // MAPSCORE__Logger__Logger__
