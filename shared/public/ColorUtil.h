// (c) Dean McNamee <dean@gmail.com>, 2012.
// C++ port by Mapbox, Konstantin Käfer <mail@kkaefer.com>, 2014-2017.
//
// https://github.com/deanm/css-color-parser-js
// https://github.com/kkaefer/css-color-parser-cpp
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#pragma once

#include "Color.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

class ColorUtil {
public:
    static Color c(int r, int g, int b, float a = 1.0) {
        return Color(((float)r) / 255.0,((float)g) / 255.0,((float)b) / 255.0, a);
    }
    static Color cInt(int r, int g, int b, int a = 255) {
        return Color(((float)r) / 255.0,((float)g) / 255.0,((float)b) / 255.0, ((float)a) / 255.0);
    }
private:
    static const std::map<std::string, Color> namedColors;

    template <typename T>
    static uint8_t clamp_css_byte(T i) {  // Clamp to integer 0 .. 255.
        i = std::round(i);  // Seems to be what Chrome does (vs truncation).
        return i < 0 ? 0 : i > 255 ? 255 : uint8_t(i);
    }

    template <typename T>
    static float clamp_css_float(T f) {  // Clamp to float 0.0 .. 1.0.
        return f < 0 ? 0 : f > 1 ? 1 : float(f);
    }

    static float parseFloat(const std::string& str) {
        return strtof(str.c_str(), nullptr);
    }

    static int64_t parseInt(const std::string& str, uint8_t base = 10) {
        return strtoll(str.c_str(), nullptr, base);
    }

    static uint8_t parse_css_int(const std::string& str) {  // int or percentage.
        if (str.length() && str.back() == '%') {
            return clamp_css_byte(parseFloat(str) / 100.0f * 255.0f);
        } else {
            return clamp_css_byte(parseInt(str));
        }
    }

    static float parse_css_float(const std::string& str) {  // float or percentage.
        if (str.length() && str.back() == '%') {
            return clamp_css_float(parseFloat(str) / 100.0f);
        } else {
            return clamp_css_float(parseFloat(str));
        }
    }

    static float css_hue_to_rgb(float m1, float m2, float h) {
        if (h < 0.0f) {
            h += 1.0f;
        } else if (h > 1.0f) {
            h -= 1.0f;
        }

        if (h * 6.0f < 1.0f) {
            return m1 + (m2 - m1) * h * 6.0f;
        }
        if (h * 2.0f < 1.0f) {
            return m2;
        }
        if (h * 3.0f < 2.0f) {
            return m1 + (m2 - m1) * (2.0f / 3.0f - h) * 6.0f;
        }
        return m1;
    }

    static std::vector<std::string> split(const std::string& s, char delim) {
        std::vector<std::string> elems;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            elems.push_back(item);
        }
        return elems;
    }

public:

    static std::optional<Color> fromString(const std::string &input) {
        std::string str = input;

        // Remove all whitespace, not compliant, but should just be more accepting.
        str.erase(std::remove(str.begin(), str.end(), ' '), str.end());

        // Convert to lowercase.
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);

        if (namedColors.count(str) != 0) {
            return namedColors.at(str);
        }

        // #abc and #abc123 syntax.
        if (str.length() && str.front() == '#') {
            if (str.length() == 4) {
                int64_t iv = parseInt(str.substr(1), 16);  // TODO(deanm): Stricter parsing.
                if (!(iv >= 0 && iv <= 0xfff)) {
                    return std::nullopt;
                } else {
                    return c(
                        static_cast<uint8_t>(((iv & 0xf00) >> 4) | ((iv & 0xf00) >> 8)),
                        static_cast<uint8_t>((iv & 0xf0) | ((iv & 0xf0) >> 4)),
                        static_cast<uint8_t>((iv & 0xf) | ((iv & 0xf) << 4)),
                        1
                    );
                }
            } else if (str.length() == 7) {
                int64_t iv = parseInt(str.substr(1), 16);  // TODO(deanm): Stricter parsing.
                if (!(iv >= 0 && iv <= 0xffffff)) {
                    return std::nullopt;  // Covers NaN.
                } else {
                    return c(
                        static_cast<uint8_t>((iv & 0xff0000) >> 16),
                        static_cast<uint8_t>((iv & 0xff00) >> 8),
                        static_cast<uint8_t>(iv & 0xff),
                        1
                    );
                }
            } else if (str.length() == 9) {
                int64_t iv = parseInt(str.substr(1), 16);  // TODO(deanm): Stricter parsing.
                if (!(iv >= 0 && iv <= 0xffffffff)) {
                    return std::nullopt;  // Covers NaN.
                } else {
                    return cInt(
                            static_cast<uint8_t>((iv & 0xff0000) >> 16),
                            static_cast<uint8_t>((iv & 0xff00) >> 8),
                            static_cast<uint8_t>(iv & 0xff),
                            static_cast<uint8_t>((iv & 0xff000000) >> 24)
                    );
                }
            }

            return std::nullopt;
        }

        size_t op = str.find_first_of('('), ep = str.find_first_of(')');
        if (op != std::string::npos && ep + 1 == str.length()) {
            const std::string fname = str.substr(0, op);
            const std::vector<std::string> params = split(str.substr(op + 1, ep - (op + 1)), ',');

            float alpha = 1.0f;

            if (fname == "rgba" || fname == "rgb") {
                if (fname == "rgba") {
                    if (params.size() != 4) {
                        return std::nullopt;
                    }
                    alpha = parse_css_float(params.back());
                } else {
                    if (params.size() != 3) {
                        return std::nullopt;
                    }
                }

                return c(
                        parse_css_int(params[0]),
                        parse_css_int(params[1]),
                        parse_css_int(params[2]),
                        alpha
                );

            } else if (fname == "hsla" || fname == "hsl") {
                if (fname == "hsla") {
                    if (params.size() != 4) {
                        return std::nullopt;
                    }
                    alpha = parse_css_float(params.back());
                } else {
                    if (params.size() != 3) {
                        return std::nullopt;
                    }
                }

                float h = parseFloat(params[0]) / 360.0f;
                float i;
                // Normalize the hue to [0..1[
                h = std::modf(h, &i);

                // NOTE(deanm): According to the CSS spec s/l should only be
                // percentages, but we don't bother and let float or percentage.
                float s = parse_css_float(params[1]);
                float l = parse_css_float(params[2]);

                float m2 = l <= 0.5f ? l * (s + 1.0f) : l + s - l * s;
                float m1 = l * 2.0f - m2;

                return c(
                    clamp_css_byte(css_hue_to_rgb(m1, m2, h + 1.0f / 3.0f) * 255.0f),
                    clamp_css_byte(css_hue_to_rgb(m1, m2, h) * 255.0f),
                    clamp_css_byte(css_hue_to_rgb(m1, m2, h - 1.0f / 3.0f) * 255.0f),
                    alpha
                );
            }
        }

        return std::nullopt;
    }
};
