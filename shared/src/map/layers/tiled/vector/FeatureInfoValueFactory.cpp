/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "FeatureInfoValueFactory.h"
#include "VectorLayerFeatureInfoValue.h"

VectorLayerFeatureInfoValue FeatureInfoValueFactory::createString(const std::string & value){
    return VectorLayerFeatureInfoValue(value,
                                       std::nullopt,
                                       std::nullopt,
                                       std::nullopt,
                                       std::nullopt,
                                       std::nullopt,
                                       std::nullopt);
}

VectorLayerFeatureInfoValue FeatureInfoValueFactory::createDouble(double value){
    return VectorLayerFeatureInfoValue(std::nullopt,
                                       value,
                                       std::nullopt,
                                       std::nullopt,
                                       std::nullopt,
                                       std::nullopt,
                                       std::nullopt);
}

VectorLayerFeatureInfoValue FeatureInfoValueFactory::createInt(int64_t value){
    return VectorLayerFeatureInfoValue(std::nullopt,
                                       std::nullopt,
                                       value,
                                       std::nullopt,
                                       std::nullopt,
                                       std::nullopt,
                                       std::nullopt);
}

VectorLayerFeatureInfoValue FeatureInfoValueFactory::createBool(bool value){
    return VectorLayerFeatureInfoValue(std::nullopt,
                                       std::nullopt,
                                       std::nullopt,
                                       value,
                                       std::nullopt,
                                       std::nullopt,
                                       std::nullopt);
}

VectorLayerFeatureInfoValue FeatureInfoValueFactory::createColor(const ::Color & value){
    return VectorLayerFeatureInfoValue(std::nullopt,
                                       std::nullopt,
                                       std::nullopt,
                                       std::nullopt,
                                       value,
                                       std::nullopt,
                                       std::nullopt);
}

VectorLayerFeatureInfoValue FeatureInfoValueFactory::createListFloat(const std::vector<float> & value){
    return VectorLayerFeatureInfoValue(std::nullopt,
                                       std::nullopt,
                                       std::nullopt,
                                       std::nullopt,
                                       std::nullopt,
                                       value,
                                       std::nullopt);
}

VectorLayerFeatureInfoValue FeatureInfoValueFactory::createListString(const std::vector<std::string> & value){
    return VectorLayerFeatureInfoValue(std::nullopt,
                                       std::nullopt,
                                       std::nullopt,
                                       std::nullopt,
                                       std::nullopt,
                                       std::nullopt,
                                       value);
}
