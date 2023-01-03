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

#include "ExceptionLogger.h"
#include "ExceptionLoggerInterface.h"


class ExceptionLoggerImpl {
public:
    static ExceptionLoggerImpl& instance() {
        static ExceptionLoggerImpl singelton;
        return singelton;
    }
    
    void logMessage(const std::string & errorDomain, int32_t code, const std::string & message);

    void logMessage(const std::string & errorDomain, int32_t code, const std::unordered_map<std::string, std::string> & customValues);

    void setLoggerDelegate(const std::shared_ptr<ExceptionLoggerInterface> & delegate);
    
private:
    std::shared_ptr<ExceptionLoggerInterface> delegate;
};
