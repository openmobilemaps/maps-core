// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from icon.djinni

#pragma once

#include "LayerInterface.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class IconInfoInterface;
class IconLayerCallbackInterface;

class IconLayerInterface {
public:
    virtual ~IconLayerInterface() = default;

    static /*not-null*/ std::shared_ptr<IconLayerInterface> create();

    virtual void setIcons(const std::vector</*not-null*/ std::shared_ptr<IconInfoInterface>> & icons) = 0;

    virtual std::vector</*not-null*/ std::shared_ptr<IconInfoInterface>> getIcons() = 0;

    virtual void remove(const /*not-null*/ std::shared_ptr<IconInfoInterface> & icon) = 0;

    virtual void removeList(const std::vector</*not-null*/ std::shared_ptr<IconInfoInterface>> & icons) = 0;

    virtual void removeIdentifier(const std::string & identifier) = 0;

    virtual void removeIdentifierList(const std::vector<std::string> & identifiers) = 0;

    virtual void add(const /*not-null*/ std::shared_ptr<IconInfoInterface> & icon) = 0;

    virtual void addList(const std::vector</*not-null*/ std::shared_ptr<IconInfoInterface>> & icons) = 0;

    virtual void clear() = 0;

    virtual void setCallbackHandler(const /*not-null*/ std::shared_ptr<IconLayerCallbackInterface> & handler) = 0;

    virtual /*not-null*/ std::shared_ptr<::LayerInterface> asLayerInterface() = 0;

    virtual void invalidate() = 0;

    virtual void setLayerClickable(bool isLayerClickable) = 0;

    virtual void setRenderPassIndex(int32_t index) = 0;

    /** scale an icon, use repetitions for pulsating effect (repetions == -1 -> forever) */
    virtual void animateIconScale(const std::string & identifier, float from, float to, float duration, int32_t repetitions) = 0;
};
