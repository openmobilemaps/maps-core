/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "IcosahedronLayer.h"
#include "Logger.h"
#include <protozero/pbf_reader.hpp>

IcosahedronLayer::IcosahedronLayer(const /*not-null*/ std::shared_ptr<IcosahedronLayerCallbackInterface> & callbackHandler): callbackHandler(callbackHandler) {

}

void IcosahedronLayer::onAdded(const std::shared_ptr<MapInterface> & mapInterface, int32_t layerIndex) {
    callbackHandler->getData().then([](auto dataFuture) {
        const auto data = dataFuture.get();
        protozero::pbf_reader pbfData((char *)data.buf(), data.len());

        LogDebug <<= pbfData.length();

        std::vector<float> floats;

        while (pbfData.next()) {
            switch (pbfData.tag()) {
                case 1: {
                    protozero::pbf_reader cell = pbfData.get_message();
                    while (cell.next()) {
                        protozero::pbf_reader v1 = cell.get_message();
                        while (v1.next()) {
                            switch (v1.tag()) {
                                case 1: {
                                    auto lat = v1.get_float();
                                    floats.push_back(lat);
                                    break;
                                }
                                case 2: {
                                    auto lon = v1.get_float();
                                    floats.push_back(lon);
                                    break;
                                }
                                case 3: {
                                    auto value = v1.get_float();
                                    floats.push_back(value);
                                    break;
                                }
                                default:
                                    assert(false);
                            }
                        }
                    }
                }
            }
        }

        LogDebug <<= floats.size();
    });
}

/*not-null*/ std::shared_ptr<::LayerInterface> IcosahedronLayer::asLayerInterface() {
    return shared_from_this();
}
