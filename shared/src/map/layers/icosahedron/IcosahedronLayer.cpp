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
#include "RenderPass.h"
#include "RenderObject.h"
#include "BlendMode.h"
#include <protozero/pbf_reader.hpp>
#include <cmath>

IcosahedronLayer::IcosahedronLayer(const /*not-null*/ std::shared_ptr<IcosahedronLayerCallbackInterface> & callbackHandler): callbackHandler(callbackHandler) {

}

void IcosahedronLayer::onAdded(const std::shared_ptr<MapInterface> & mapInterface, int32_t layerIndex) {
    this->mapInterface = mapInterface;

    std::shared_ptr<Mailbox> selfMailbox = mailbox;
    if (!mailbox) {
        selfMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
    }

    
    shader = mapInterface->getShaderFactory()->createIcosahedronColorShader();
    shader->setColor(1.0, 0.0, 0.0, 1.0);
    shader->asShaderProgramInterface()->setBlendMode(BlendMode::MULTIPLY);
    object = mapInterface->getGraphicsObjectFactory()->createIcosahedronObject(shader->asShaderProgramInterface());

    auto renderObject =  std::make_shared<RenderObject>(object->asGraphicsObject());
    std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(RenderPassConfig(0, false), std::vector<std::shared_ptr<::RenderObjectInterface>>{ renderObject });

    renderPasses = { renderPass };

    callbackHandler->getData().then([=](auto dataFuture) {
        const auto data = dataFuture.get();
        protozero::pbf_reader pbfData((char *)data.buf(), data.len());

        std::vector<float> verticesBuffer;
        std::vector<uint32_t> indicesBuffer;

        int32_t currentIndex = 0;

        while (pbfData.next()) {
            switch (pbfData.tag()) {
                case 1: {
                    protozero::pbf_reader cell = pbfData.get_message();
                    int count = 0;
                    while (cell.next()) {
                        count += 1;
                        protozero::pbf_reader vertex = cell.get_message();
                        while (vertex.next()) {
                            switch (vertex.tag()) {
                                case 1: {
                                    auto lat = vertex.get_float();
                                    verticesBuffer.push_back(lat * 180 / M_PI);
                                    break;
                                }
                                case 2: {
                                    auto lon = vertex.get_float();
                                    verticesBuffer.push_back(lon * 180 / M_PI);
                                    break;
                                }
                                case 3: {
                                    auto value = vertex.get_float();
                                    verticesBuffer.push_back(value);
                                    break;
                                }
                                default:
                                    assert(false);
                            }
                        }
                    }
                    assert(count == 3);
                    indicesBuffer.push_back(currentIndex);
                    indicesBuffer.push_back(currentIndex + 1);
                    indicesBuffer.push_back(currentIndex + 2);
                    currentIndex += count;
                }
            }
        }

        LogDebug << "verticesBuffer.size: " <<= verticesBuffer.size();
        LogDebug << "indicesBuffer.size: " <<= indicesBuffer.size();

        auto i = SharedBytes((int64_t)indicesBuffer.data(), (int32_t)indicesBuffer.size(), (int32_t)sizeof(uint32_t));
        auto v = SharedBytes((int64_t)verticesBuffer.data(), (int32_t)verticesBuffer.size(), (int32_t)sizeof(float));
        object->setVertices(v, i);


        auto selfActor = WeakActor<IcosahedronLayer>(selfMailbox, shared_from_this());
        selfActor.message(MailboxExecutionEnvironment::graphics, &IcosahedronLayer::setupObject);
    });
}

void IcosahedronLayer::setupObject() {
    object->asGraphicsObject()->setup(mapInterface->getRenderingContext());
}

std::vector<std::shared_ptr< ::RenderPassInterface>> IcosahedronLayer::buildRenderPasses() {
    return renderPasses;
}

/*not-null*/ std::shared_ptr<::LayerInterface> IcosahedronLayer::asLayerInterface() {
    return shared_from_this();
}

