//
//  Logger.h
//  speate_logic
//
//  Created by Marco Zimmermann on 13/02/14.
//  Copyright (c) 2014 Marco Zimmermann. All rights reserved.
//

#ifndef __ubb_shared__Logger__Logger__
#define __ubb_shared__Logger__Logger__

#ifndef LogError
#define LogError utility::Logger()(0)
#endif

#ifndef LogWarning
#define LogWarning utility::Logger()(1)
#endif

#ifndef LogDebug
#define LogDebug utility::Logger()(2)
#endif

#ifndef LogInfo
#define LogInfo utility::Logger()(3)
#endif

#ifndef LogTrace
#define LogTrace utility::Logger()(4)
#endif

//#define SHOW_COLORS 0
#define LOG_LEVEL 2

#include <sstream>
#include <iostream>

namespace utility
{
    class Logger
    {
      public:
        std::stringstream& stream() const;

        Logger& operator()(int p);

        template <typename T>
        friend bool operator<<=(const Logger& logger, T thing);

        template <typename T>
        friend const Logger& operator<<(const Logger& logger, T thing);

      private:
        void log(int prio, const char* tag, const char* fmt, ...) const;

      private:
        mutable std::stringstream ss;
        mutable int priority = -1;
    };

    template <typename T>
    const Logger& operator<<(const Logger& logger, T thing)
    {
        logger.stream() << thing;
        return logger;
    }

    template <typename T>
    bool operator<<=(const Logger& logger, T thing)
    {
        logger.stream() << thing;

        logger.log(3, "Shared-Lib-C++:", logger.stream().str().c_str());

        logger.stream().str("");
        logger.priority = -1;

        return true;
    }
}

#endif // defined(__utility__Logger__)
