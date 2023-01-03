/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */


#pragma once

#include "ExceptionLoggerInterface.h"
#include "ExceptionLoggerDelegateInterface.h"


class ExceptionLogger {
public:
    static ExceptionLogger& instance() {
        static ExceptionLogger singelton;
        return singelton;
    }
    
    void logMessage(const std::string & errorDomain, int32_t code, const std::string & message);

    void logMessage(const std::string & errorDomain, int32_t code, const std::unordered_map<std::string, std::string> & customValues);

    void setLoggerDelegate(const std::shared_ptr<ExceptionLoggerDelegateInterface> & delegate);
    
private:
    std::shared_ptr<ExceptionLoggerDelegateInterface> delegate;
};
