// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from icosahedron.djinni

#pragma once

#include "LayerInterface.h"
#include <memory>

class IcosahedronLayerCallbackInterface;

class IcosahedronLayerInterface {
public:
    virtual ~IcosahedronLayerInterface() = default;

    static /*not-null*/ std::shared_ptr<IcosahedronLayerInterface> create(const /*not-null*/ std::shared_ptr<IcosahedronLayerCallbackInterface> & callbackHandler);

    virtual /*not-null*/ std::shared_ptr<::LayerInterface> asLayerInterface() = 0;
};