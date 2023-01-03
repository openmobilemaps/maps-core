/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */


#include "ExceptionLoggerImpl.h"

void ExceptionLogger::setLoggerDelegate(const std::shared_ptr<ExceptionLoggerInterface> & delegate) {
    ExceptionLoggerImpl::instance().setLoggerDelegate(delegate);
}

void ExceptionLoggerImpl::logMessage(const std::string & errorDomain, int32_t code, const std::string & message) {
    this->logMessage(errorDomain, code, {{"message", message}});
}

void ExceptionLoggerImpl::logMessage(const std::string & errorDomain, int32_t code, const std::unordered_map<std::string, std::string> & customValues) {
    auto delegate = this->delegate;
    if (!delegate) {
        return;
    }
    delegate->logMessage(errorDomain, code, customValues);
}

void ExceptionLoggerImpl::setLoggerDelegate(const std::shared_ptr<ExceptionLoggerInterface> & delegate) {
    this->delegate = delegate;
}
