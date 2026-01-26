#include "FontManifestParser.h"
#include <string>

#include "json.h"

FontData FontManifestParser::parse(const char *buf, size_t bufSize) {
    const auto json = nlohmann::json::parse(buf, buf + bufSize);

    const auto &info = json.at("info");
    auto name = info.at("face").get<std::string>();
    auto size = info.at("size").get<double>();

    auto distanceRange = json.at("distanceField").at("distanceRange").get<double>();

    const auto &common = json.at("common");
    auto lineHeight = common.at("lineHeight").get<double>();
    auto base = common.at("base").get<double>();
    auto imageSize = common.at("scaleW").get<double>();

    const FontWrapper fontWrapper{name, lineHeight / size, base / size, Vec2D(imageSize, imageSize), size, distanceRange};

    const auto &chars = json.at("chars");
    std::vector<FontGlyph> glyphs{};
    if (chars.is_array()) {
        for (const auto &glyphEntry : chars) {

            auto width = glyphEntry.at("width").get<double>();
            auto height = glyphEntry.at("height").get<double>();
            auto s0 = glyphEntry.at("x").get<double>();
            auto s1 = s0 + width;
            auto t0 = glyphEntry.at("y").get<double>();
            auto t1 = t0 + height;

            s0 = s0 / imageSize;
            s1 = s1 / imageSize;
            t0 = t0 / imageSize;
            t1 = t1 / imageSize;

            Vec2D bearing{glyphEntry.at("xoffset").get<double>() / size, -glyphEntry.at("yoffset").get<double>() / size};
            Vec2D advance{glyphEntry.at("xadvance").get<double>() / size, 0.0};
            Vec2D bbox{width / size, height / size};

            glyphs.emplace_back(glyphEntry.at("char").get<std::string>(), advance, bbox, bearing,
                                Quad2dD(Vec2D(s0, t1), Vec2D(s1, t1), Vec2D(s1, t0), Vec2D(s0, t0)));
        }
    }
    return FontData(fontWrapper, glyphs);
}
