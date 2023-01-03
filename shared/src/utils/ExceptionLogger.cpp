/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */


#include "ExceptionLogger.h"

void ExceptionLoggerInterface::setLoggerDelegate(const std::shared_ptr<ExceptionLoggerDelegateInterface> & delegate) {
    ExceptionLogger::instance().setLoggerDelegate(delegate);
}

void ExceptionLogger::logMessage(const std::string & errorDomain, int32_t code, const std::string & message) {
    this->logMessage(errorDomain, code, {{"message", message}});
}

void ExceptionLogger::logMessage(const std::string & errorDomain, int32_t code, const std::unordered_map<std::string, std::string> & customValues) {
    auto delegate = this->delegate;
    if (!delegate) {
        return;
    }
    delegate->logMessage(errorDomain, code, customValues);
}

void ExceptionLogger::setLoggerDelegate(const std::shared_ptr<ExceptionLoggerDelegateInterface> & delegate) {
    this->delegate = delegate;
}
