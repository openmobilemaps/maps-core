#include "Tiled2dMapVectorBackgroundTile.h"

Tiled2dMapVectorBackgroundTile::Tiled2dMapVectorBackgroundTile(const std::weak_ptr<MapInterface> &mapInterface,
                            const Tiled2dMapTileInfo &tileInfo,
                            const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                            const std::shared_ptr<BackgroundVectorLayerDescription> &description):
Tiled2dMapVectorTile(mapInterface, tileInfo, description, vectorLayer) {
    auto strongMapInterface = mapInterface.lock();
    if (!strongMapInterface) {
        return;
    }

    shader = strongMapInterface->getShaderFactory()->createColorShader();

    auto object = strongMapInterface->getGraphicsObjectFactory()->createQuad(shader->asShaderProgramInterface());
    object->setFrame(Quad2dD(Vec2D(-1, 1),
                             Vec2D(1, 1),
                             Vec2D(1, -1),
                             Vec2D(-1, -1)),
                     RectD(0, 0, 1, 1));

    auto color = description->style.getColor(EvaluationContext(std::nullopt, FeatureContext()));
    shader->setColor(color.r, color.g, color.b, color.a * alpha);
    renderObject = std::make_shared<RenderObject>(object->asGraphicsObject(), true);

#ifdef __APPLE__
    setup();
#else
    auto selfActor = WeakActor(mailbox, shared_from_this()->weak_from_this());
    selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorBackgroundTile::setup);
#endif
}

void Tiled2dMapVectorBackgroundTile::updateLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                            const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) {

}

void Tiled2dMapVectorBackgroundTile::update() {}

std::vector<std::shared_ptr<::RenderObjectInterface>> Tiled2dMapVectorBackgroundTile::getRenderObjects() {
    return { renderObject };
}

void Tiled2dMapVectorBackgroundTile::clear() {
    renderObject->getGraphicsObject()->clear();
}

void Tiled2dMapVectorBackgroundTile::setup() {
    auto mapInterface = this->mapInterface.lock();
    if (!renderObject->getGraphicsObject()->isReady() && mapInterface) {
        renderObject->getGraphicsObject()->setup(mapInterface->getRenderingContext());
    }
}

void Tiled2dMapVectorBackgroundTile::setScissorRect(const std::optional<::RectI> &scissorRect) {}

void Tiled2dMapVectorBackgroundTile::setTileData(const std::shared_ptr<MaskingObjectInterface> &tileMask,
                         const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) {}

void Tiled2dMapVectorBackgroundTile::updateTileMask(const std::shared_ptr<MaskingObjectInterface> &tileMask) {}
