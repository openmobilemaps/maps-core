/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TextHelper.h"

#include "GlyphDescription.h"
#include "MapCamera2dInterface.h"
#include "Quad2dD.h"
#include "TextDescription.h"
#include "BoundingBox.h"
#include "SymbolInfo.h"
#include <codecvt>
#include <iosfwd>
#include <locale>
#include <string>
#include "Vec2DHelper.h"

unsigned char *StrToUprExt(unsigned char *pString);

std::vector<std::string> TextHelper::splitWstring(const std::string &word) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wword = converter.from_bytes(word);
    std::vector<std::string> characters;
    for (auto iter : wword) {
        try {
            characters.push_back(converter.to_bytes(iter));
        } // thrown by std::wstring_convert.to_bytes() for emojis
        catch (const std::range_error & exception)
        {}
    }
    return characters;
}

TextHelper::TextHelper(const std::shared_ptr<MapInterface> &mapInterface)
    : mapInterface(mapInterface) {}

void TextHelper::setMapInterface(const std::weak_ptr< ::MapInterface> &mapInterface) {
    this->mapInterface = mapInterface;
}

std::shared_ptr<TextLayerObject> TextHelper::textLayerObject(const std::shared_ptr<TextInfoInterface> &text,
                                                             std::optional<FontData> fontData,
                                                             Vec2F offset,
                                                             double lineHeight,
                                                             double letterSpacing,
                                                             int64_t maxCharacterWidth,
                                                             double maxCharacterAngle,
                                                             SymbolAlignment rotationAlignment) {

    if (!fontData) {
        return nullptr;
    }

    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) {
        return nullptr;
    }
    
    auto const &formattedText = text->getText();
    
    std::shared_ptr<::TextShaderInterface> shader;
    std::shared_ptr<::TextInterface> factoryObject;


    if (!formattedText.empty() && !(formattedText.size() == 1 && formattedText.at(0).text.empty())) {
        const auto &objectFactory = mapInterface->getGraphicsObjectFactory();
        const auto &shaderFactory = mapInterface->getShaderFactory();
        
        shader = shaderFactory->createTextShader();
        factoryObject = objectFactory->createText(shader->asShaderProgramInterface());
    }
    
    auto textObject = std::make_shared<TextLayerObject>(factoryObject, text, shader, mapInterface, *fontData, offset, lineHeight, letterSpacing, maxCharacterWidth, maxCharacterAngle, rotationAlignment);
    return textObject;
}

std::string TextHelper::uppercase(const std::string &string) {
    std::string s = "";
    for (int i = 0; i < string.size(); ++i) {
        auto a = StrToUprExt((unsigned char *)&string[i]);

        if (a) {
            s.insert(s.end(), *a);
        }
    }

    return s;
}

unsigned char *StrToUprExt(unsigned char *pString) {
    if (pString && *pString) {
        unsigned char *p = pString;
        unsigned char *pExtChar = 0;
        while (*p) {
            if ((*p >= 0x61) && (*p <= 0x7a)) // US ASCII
                (*p) -= 0x20;
            else if (*p > 0xc0) {
                pExtChar = p;
                p++;
                switch (*pExtChar) {
                case 0xc3: // Latin 1
                    // 0x9f Three byte capital 0xe1 0xba 0x9e
                    if ((*p >= 0xa0) && (*p <= 0xbe) && (*p != 0xb7))
                        (*p) -= 0x20; // US ASCII shift
                    else if (*p == 0xbf) {
                        *pExtChar = 0xc5;
                        (*p) = 0xb8;
                    }
                    break;
                case 0xc4:                                                // Latin ext
                    if ((*p >= 0x80) && (*p <= 0xb7) && (*p % 2))         // Odd
                        (*p)--;                                           // Prev char is upr
                    else if ((*p >= 0xb9) && (*p <= 0xbe) && (!(*p % 2))) // Even
                        (*p)--;                                           // Prev char is upr
                    break;
                case 0xc5: // Latin ext
                    if (*p == 0x80) {
                        *pExtChar = 0xc4;
                        (*p) = 0xbf;
                    } else if ((*p >= 0x81) && (*p <= 0x88) && (!(*p % 2))) // Even
                        (*p)--;                                             // Prev char is upr
                    else if ((*p >= 0x8a) && (*p <= 0xb7) && (*p % 2))      // Odd
                        (*p)--;                                             // Prev char is upr
                    else if (*p == 0xb8) {
                        *pExtChar = 0xc5;
                        (*p) = 0xb8;
                    } else if ((*p >= 0xb9) && (*p <= 0xbe) && (!(*p % 2))) // Even
                        (*p)--;                                             // Prev char is upr
                    break;
                case 0xc6: // Latin ext
                    switch (*p) {
                    case 0x83:
                    case 0x85:
                    case 0x88:
                    case 0x8c:
                    case 0x92:
                    case 0x99:
                    case 0xa1:
                    case 0xa3:
                    case 0xa5:
                    case 0xa8:
                    case 0xad:
                    case 0xb0:
                    case 0xb4:
                    case 0xb6:
                    case 0xb9:
                    case 0xbd:
                        (*p)--; // Prev char is upr
                        break;
                    case 0x80:
                        *pExtChar = 0xc9;
                        (*p) = 0x83;
                        break;
                    case 0x95:
                        *pExtChar = 0xc7;
                        (*p) = 0xb6;
                        break;
                    case 0x9a:
                        *pExtChar = 0xc8;
                        (*p) = 0xbd;
                        break;
                    case 0x9e:
                        *pExtChar = 0xc8;
                        (*p) = 0xa0;
                        break;
                    default:
                        break;
                    }
                    break;
                case 0xc7: // Latin ext
                    if (*p == 0x85)
                        (*p)--; // Prev char is upr
                    else if (*p == 0x86)
                        (*p) = 0x84;
                    else if (*p == 0x88)
                        (*p)--; // Prev char is upr
                    else if (*p == 0x89)
                        (*p) = 0x87;
                    else if (*p == 0x8b)
                        (*p)--; // Prev char is upr
                    else if (*p == 0x8c)
                        (*p) = 0x8a;
                    else if ((*p >= 0x8d) && (*p <= 0x9c) && (!(*p % 2))) // Even
                        (*p)--;                                           // Prev char is upr
                    else if ((*p >= 0x9e) && (*p <= 0xaf) && (*p % 2))    // Odd
                        (*p)--;                                           // Prev char is upr
                    else if (*p == 0xb2)
                        (*p)--; // Prev char is upr
                    else if (*p == 0xb3)
                        (*p) = 0xb1;
                    else if (*p == 0xb5)
                        (*p)--;                                        // Prev char is upr
                    else if ((*p >= 0xb9) && (*p <= 0xbf) && (*p % 2)) // Odd
                        (*p)--;                                        // Prev char is upr
                    break;
                case 0xc8:                                             // Latin ext
                    if ((*p >= 0x80) && (*p <= 0x9f) && (*p % 2))      // Odd
                        (*p)--;                                        // Prev char is upr
                    else if ((*p >= 0xa2) && (*p <= 0xb3) && (*p % 2)) // Odd
                        (*p)--;                                        // Prev char is upr
                    else if (*p == 0xbc)
                        (*p)--; // Prev char is upr
                    // 0xbf Three byte capital 0xe2 0xb1 0xbe
                    break;
                case 0xc9: // Latin ext
                    switch (*p) {
                    case 0x80: // Three byte capital 0xe2 0xb1 0xbf
                    case 0x90: // Three byte capital 0xe2 0xb1 0xaf
                    case 0x91: // Three byte capital 0xe2 0xb1 0xad
                    case 0x92: // Three byte capital 0xe2 0xb1 0xb0
                    case 0x9c: // Three byte capital 0xea 0x9e 0xab
                    case 0xa1: // Three byte capital 0xea 0x9e 0xac
                    case 0xa5: // Three byte capital 0xea 0x9e 0x8d
                    case 0xa6: // Three byte capital 0xea 0x9e 0xaa
                    case 0xab: // Three byte capital 0xe2 0xb1 0xa2
                    case 0xac: // Three byte capital 0xea 0x9e 0xad
                    case 0xb1: // Three byte capital 0xe2 0xb1 0xae
                    case 0xbd: // Three byte capital 0xe2 0xb1 0xa4
                        break;
                    case 0x82:
                        (*p)--; // Prev char is upr
                        break;
                    case 0x93:
                        *pExtChar = 0xc6;
                        (*p) = 0x81;
                        break;
                    case 0x94:
                        *pExtChar = 0xc6;
                        (*p) = 0x86;
                        break;
                    case 0x97:
                        *pExtChar = 0xc6;
                        (*p) = 0x8a;
                        break;
                    case 0x98:
                        *pExtChar = 0xc6;
                        (*p) = 0x8e;
                        break;
                    case 0x99:
                        *pExtChar = 0xc6;
                        (*p) = 0x8f;
                        break;
                    case 0x9b:
                        *pExtChar = 0xc6;
                        (*p) = 0x90;
                        break;
                    case 0xa0:
                        *pExtChar = 0xc6;
                        (*p) = 0x93;
                        break;
                    case 0xa3:
                        *pExtChar = 0xc6;
                        (*p) = 0x94;
                        break;
                    case 0xa8:
                        *pExtChar = 0xc6;
                        (*p) = 0x97;
                        break;
                    case 0xa9:
                        *pExtChar = 0xc6;
                        (*p) = 0x96;
                        break;
                    case 0xaf:
                        *pExtChar = 0xc6;
                        (*p) = 0x9c;
                        break;
                    case 0xb2:
                        *pExtChar = 0xc6;
                        (*p) = 0x9d;
                        break;
                    default:
                        if ((*p >= 0x87) && (*p <= 0x8f) && (*p % 2)) // Odd
                            (*p)--;                                   // Prev char is upr
                        break;
                    }
                    break;

                case 0xca: // Latin ext
                    switch (*p) {
                    case 0x82: // Three byte capital 0xea 0x9f 0x85
                    case 0x87: // Three byte capital 0xea 0x9e 0xb1
                    case 0x9d: // Three byte capital 0xea 0x9e 0xb2
                    case 0x9e: // Three byte capital 0xea 0x9e 0xb0
                        break;
                    case 0x83:
                        *pExtChar = 0xc6;
                        (*p) = 0xa9;
                        break;
                    case 0x88:
                        *pExtChar = 0xc6;
                        (*p) = 0xae;
                        break;
                    case 0x89:
                        *pExtChar = 0xc9;
                        (*p) = 0x84;
                        break;
                    case 0x8a:
                        *pExtChar = 0xc6;
                        (*p) = 0xb1;
                        break;
                    case 0x8b:
                        *pExtChar = 0xc6;
                        (*p) = 0xb2;
                        break;
                    case 0x8c:
                        *pExtChar = 0xc9;
                        (*p) = 0x85;
                        break;
                    case 0x92:
                        *pExtChar = 0xc6;
                        (*p) = 0xb7;
                        break;
                    default:
                        break;
                    }
                    break;
                case 0xcd: // Greek & Coptic
                    switch (*p) {
                    case 0xb1:
                    case 0xb3:
                    case 0xb7:
                        (*p)--; // Prev char is upr
                        break;
                    case 0xbb:
                        *pExtChar = 0xcf;
                        (*p) = 0xbd;
                        break;
                    case 0xbc:
                        *pExtChar = 0xcf;
                        (*p) = 0xbe;
                        break;
                    case 0xbd:
                        *pExtChar = 0xcf;
                        (*p) = 0xbf;
                        break;
                    default:
                        break;
                    }
                    break;
                case 0xce: // Greek & Coptic
                    if (*p == 0xac)
                        (*p) = 0x86;
                    else if (*p == 0xad)
                        (*p) = 0x88;
                    else if (*p == 0xae)
                        (*p) = 0x89;
                    else if (*p == 0xaf)
                        (*p) = 0x8a;
                    else if ((*p >= 0xb1) && (*p <= 0xbf))
                        (*p) -= 0x20; // US ASCII shift
                    break;
                case 0xcf: // Greek & Coptic
                    if ((*p >= 0x80) && (*p <= 0x8b)) {
                        *pExtChar = 0xce;
                        (*p) += 0x20;
                    } else if (*p == 0x8c) {
                        *pExtChar = 0xce;
                        (*p) = 0x8c;
                    } else if (*p == 0x8d) {
                        *pExtChar = 0xce;
                        (*p) = 0x8e;
                    } else if (*p == 0x8e) {
                        *pExtChar = 0xce;
                        (*p) = 0x8f;
                    } else if (*p == 0x91)
                        (*p) = 0xb4;
                    else if (*p == 0x97)
                        (*p) = 0x8f;
                    else if ((*p >= 0x98) && (*p <= 0xaf) && (*p % 2)) // Odd
                        (*p)--;                                        // Prev char is upr
                    else if (*p == 0xb2)
                        (*p) = 0xb9;
                    else if (*p == 0xb3) {
                        *pExtChar = 0xcd;
                        (*p) = 0xbf;
                    } else if (*p == 0xb8)
                        (*p)--; // Prev char is upr
                    else if (*p == 0xbb)
                        (*p)--; // Prev char is upr
                    break;
                case 0xd0: // Cyrillic
                    if ((*p >= 0xb0) && (*p <= 0xbf))
                        (*p) -= 0x20; // US ASCII shift
                    break;
                case 0xd1: // Cyrillic supplement
                    if ((*p >= 0x80) && (*p <= 0x8f)) {
                        *pExtChar = 0xd0;
                        (*p) += 0x20;
                    } else if ((*p >= 0x90) && (*p <= 0x9f)) {
                        *pExtChar = 0xd0;
                        (*p) -= 0x10;
                    } else if ((*p >= 0xa0) && (*p <= 0xbf) && (*p % 2)) // Odd
                        (*p)--;                                          // Prev char is upr
                    break;
                case 0xd2: // Cyrillic supplement
                    if (*p == 0x81)
                        (*p)--;                                        // Prev char is upr
                    else if ((*p >= 0x8a) && (*p <= 0xbf) && (*p % 2)) // Odd
                        (*p)--;                                        // Prev char is upr
                    break;
                case 0xd3:                                           // Cyrillic supplement
                    if ((*p >= 0x81) && (*p <= 0x8e) && (!(*p % 2))) // Even
                        (*p)--;                                      // Prev char is upr
                    else if (*p == 0x8f)
                        (*p) = 0x80;
                    else if ((*p >= 0x90) && (*p <= 0xbf) && (*p % 2)) // Odd
                        (*p)--;                                        // Prev char is upr
                    break;
                case 0xd4:                                        // Cyrillic supplement & Armenian
                    if ((*p >= 0x80) && (*p <= 0xaf) && (*p % 2)) // Odd
                        (*p)--;                                   // Prev char is upr
                    break;
                case 0xd5: // Armenian
                    if ((*p >= 0xa1) && (*p <= 0xaf)) {
                        *pExtChar = 0xd4;
                        (*p) += 0x10;
                    } else if ((*p >= 0xb0) && (*p <= 0xbf)) {
                        (*p) -= 0x30;
                    }
                    break;
                case 0xd6: // Armenian
                    if ((*p >= 0x80) && (*p <= 0x86)) {
                        *pExtChar = 0xd5;
                        (*p) += 0x10;
                    }
                    break;
                case 0xe1: // Three byte code
                    pExtChar = p;
                    p++;
                    switch (*pExtChar) {
                    case 0x82: // Georgian mkhedruli
                        break;
                    case 0x83: // Georgian mkhedruli
                        if (((*p >= 0x90) && (*p <= 0xba)) || (*p == 0xbd) || (*p == 0xbe) || (*p == 0xbf)) {
                            *pExtChar = 0xb2;
                        }
                        break;
                    case 0x8f: // Cherokee
                        if ((*p >= 0xb8) && (*p <= 0xbd)) {
                            (*p) -= 0x08;
                        }
                        break;
                    case 0xb6: // Latin ext
                        if (*p == 0x8e) {
                            *(p - 2) = 0xea;
                            *(p - 1) = 0x9f;
                            (*p) = 0x86;
                        }
                        break;
                    case 0xb8:                                        // Latin ext
                        if ((*p >= 0x80) && (*p <= 0xbf) && (*p % 2)) // Odd
                            (*p)--;                                   // Prev char is upr
                        break;
                    case 0xb9:                                        // Latin ext
                        if ((*p >= 0x80) && (*p <= 0xbf) && (*p % 2)) // Odd
                            (*p)--;                                   // Prev char is upr
                        break;
                    case 0xba:                                             // Latin ext
                        if ((*p >= 0x80) && (*p <= 0x93) && (*p % 2))      // Odd
                            (*p)--;                                        // Prev char is upr
                        else if ((*p >= 0xa0) && (*p <= 0xbf) && (*p % 2)) // Odd
                            (*p)--;                                        // Prev char is upr
                        break;
                    case 0xbb:                                        // Latin ext
                        if ((*p >= 0x80) && (*p <= 0xbf) && (*p % 2)) // Odd
                            (*p)--;                                   // Prev char is upr
                        break;
                    case 0xbc: // Greek ext
                        if ((*p >= 0x80) && (*p <= 0x87))
                            (*p) += 0x08;
                        else if ((*p >= 0x90) && (*p <= 0x97))
                            (*p) += 0x08;
                        else if ((*p >= 0xa0) && (*p <= 0xa7))
                            (*p) += 0x08;
                        else if ((*p >= 0xb0) && (*p <= 0xb7))
                            (*p) += 0x08;
                        break;
                    case 0xbd: // Greek ext
                        if ((*p >= 0x80) && (*p <= 0x87))
                            (*p) += 0x08;
                        else if (((*p >= 0x90) && (*p <= 0x97)) && (*p % 2)) // Odd
                            (*p) += 0x08;
                        else if ((*p >= 0xa0) && (*p <= 0xa7))
                            (*p) += 0x08;
                        else if ((*p >= 0xb0) && (*p <= 0xb1)) {
                            *(p - 1) = 0xbe;
                            (*p) += 0x0a;
                        } else if ((*p >= 0xb2) && (*p <= 0xb5)) {
                            *(p - 1) = 0xbf;
                            (*p) -= 0x2a;
                        } else if ((*p >= 0xb6) && (*p <= 0xb7)) {
                            *(p - 1) = 0xbf;
                            (*p) -= 0x1e;
                        } else if ((*p >= 0xb8) && (*p <= 0xb9)) {
                            *(p - 1) = 0xbf;
                        } else if ((*p >= 0xba) && (*p <= 0xbb)) {
                            *(p - 1) = 0xbf;
                            (*p) -= 0x10;
                        } else if ((*p >= 0xbc) && (*p <= 0xbd)) {
                            *(p - 1) = 0xbf;
                            (*p) -= 0x02;
                        }
                        break;
                    case 0xbe: // Greek ext
                        if ((*p >= 0x80) && (*p <= 0x87))
                            (*p) += 0x08;
                        else if ((*p >= 0x90) && (*p <= 0x97))
                            (*p) += 0x08;
                        else if ((*p >= 0xa0) && (*p <= 0xa7))
                            (*p) += 0x08;
                        else if ((*p >= 0xb0) && (*p <= 0xb1))
                            (*p) += 0x08;
                        else if (*p == 0xb3)
                            (*p) += 0x09;
                        break;
                    case 0xbf: // Greek ext
                        if (*p == 0x83)
                            (*p) += 0x09;
                        else if ((*p >= 0x90) && (*p <= 0x91))
                            *p += 0x08;
                        else if ((*p >= 0xa0) && (*p <= 0xa1))
                            (*p) += 0x08;
                        else if (*p == 0xa5)
                            (*p) += 0x07;
                        else if (*p == 0xb3)
                            (*p) += 0x09;
                        break;
                    default:
                        break;
                    }
                    break;
                case 0xe2: // Three byte code
                    pExtChar = p;
                    p++;
                    switch (*pExtChar) {
                    case 0xb0: // Glagolitic
                        if ((*p >= 0xb0) && (*p <= 0xbf)) {
                            (*p) -= 0x30;
                        }
                        break;
                    case 0xb1: // Glagolitic
                        if ((*p >= 0x80) && (*p <= 0x9e)) {
                            *pExtChar = 0xb0;
                            (*p) += 0x10;
                        } else { // Latin ext
                            switch (*p) {
                            case 0xa1:
                            case 0xa8:
                            case 0xaa:
                            case 0xac:
                            case 0xb3:
                            case 0xb6:
                                (*p)--; // Prev char is upr
                                break;
                            case 0xa5: // Two byte capital  0xc8 0xba
                            case 0xa6: // Two byte capital  0xc8 0xbe
                                break;
                            default:
                                break;
                            }
                        }
                        break;
                    case 0xb2:                                        // Coptic
                        if ((*p >= 0x80) && (*p <= 0xbf) && (*p % 2)) // Odd
                            (*p)--;                                   // Prev char is upr
                        break;
                    case 0xb3:                                         // Coptic
                        if (((*p >= 0x80) && (*p <= 0xa3) && (*p % 2)) // Odd
                            || (*p == 0xac) || (*p == 0xae) || (*p == 0xb3))
                            (*p)--; // Prev char is upr
                        break;
                    default:
                        break;
                    }
                    break;
                case 0xea: // Three byte code
                    pExtChar = p;
                    p++;
                    switch (*pExtChar) {
                    case 0x99:                                        // Cyrillic
                        if ((*p >= 0x80) && (*p <= 0xad) && (*p % 2)) // Odd
                            (*p)--;                                   // Prev char is upr
                        break;
                    case 0x9a:                                        // Cyrillic
                        if ((*p >= 0x80) && (*p <= 0x9b) && (*p % 2)) // Odd
                            (*p)--;                                   // Prev char is upr
                        break;
                    case 0x9c:                                                                              // Latin ext
                        if ((((*p >= 0xa2) && (*p <= 0xaf)) || ((*p >= 0xb2) && (*p <= 0xbf))) && (*p % 2)) // Odd
                            (*p)--;                                                                         // Prev char is upr
                        break;
                    case 0x9d:                                         // Latin ext
                        if (((*p >= 0x80) && (*p <= 0xaf) && (*p % 2)) // Odd
                            || (*p == 0xba) || (*p == 0xbc) || (*p == 0xbf))
                            (*p)--; // Prev char is upr
                        break;
                    case 0x9e: // Latin ext
                        if (((((*p >= 0x80) && (*p <= 0x87)) || ((*p >= 0x96) && (*p <= 0xa9)) || ((*p >= 0xb4) && (*p <= 0xbf))) &&
                             (*p % 2)) // Odd
                            || (*p == 0x8c) || (*p == 0x91) || (*p == 0x93))
                            (*p)--; // Prev char is upr
                        else if (*p == 0x94) {
                            *(p - 2) = 0xea;
                            *(p - 1) = 0x9f;
                            *(p) = 0x84;
                        }
                        break;
                    case 0x9f: // Latin ext
                        if ((*p == 0x83) || (*p == 0x88) || (*p == 0x8a) || (*p == 0xb6))
                            (*p)--; // Prev char is upr
                        break;
                    case 0xad:
                        // Latin ext
                        if (*p == 0x93) {
                            *pExtChar = 0x9e;
                            (*p) = 0xb3;
                        }
                        // Cherokee
                        else if ((*p >= 0xb0) && (*p <= 0xbf)) {
                            *(p - 2) = 0xe1;
                            *pExtChar = 0x8e;
                            (*p) -= 0x10;
                        }
                        break;
                    case 0xae: // Cherokee
                        if ((*p >= 0x80) && (*p <= 0x8f)) {
                            *(p - 2) = 0xe1;
                            *pExtChar = 0x8e;
                            (*p) += 0x30;
                        } else if ((*p >= 0x90) && (*p <= 0xbf)) {
                            *(p - 2) = 0xe1;
                            *pExtChar = 0x8f;
                            (*p) -= 0x10;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                case 0xef: // Three byte code
                    pExtChar = p;
                    p++;
                    switch (*pExtChar) {
                    case 0xbd: // Latin fullwidth
                        if ((*p >= 0x81) && (*p <= 0x9a)) {
                            *pExtChar = 0xbc;
                            (*p) += 0x20;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                case 0xf0: // Four byte code
                    pExtChar = p;
                    p++;
                    switch (*pExtChar) {
                    case 0x90:
                        pExtChar = p;
                        p++;
                        switch (*pExtChar) {
                        case 0x90: // Deseret
                            if ((*p >= 0xa8) && (*p <= 0xbf)) {
                                (*p) -= 0x28;
                            }
                            break;
                        case 0x91: // Deseret
                            if ((*p >= 0x80) && (*p <= 0x8f)) {
                                *pExtChar = 0x90;
                                (*p) += 0x18;
                            }
                            break;
                        case 0x93: // Osage
                            if ((*p >= 0x98) && (*p <= 0xa7)) {
                                *pExtChar = 0x92;
                                (*p) += 0x18;
                            } else if ((*p >= 0xa8) && (*p <= 0xbb))
                                (*p) -= 0x28;
                            break;
                        case 0xb3: // Old hungarian
                            if ((*p >= 0x80) && (*p <= 0xb2))
                                *pExtChar = 0xb2;
                            break;
                        default:
                            break;
                        }
                        break;
                    case 0x91:
                        pExtChar = p;
                        p++;
                        switch (*pExtChar) {
                        case 0xa3: // Warang citi
                            if ((*p >= 0x80) && (*p <= 0x9f)) {
                                *pExtChar = 0xa2;
                                (*p) += 0x20;
                            }
                            break;
                        default:
                            break;
                        }
                        break;
                    case 0x96:
                        pExtChar = p;
                        p++;
                        switch (*pExtChar) {
                        case 0xb9: // Medefaidrin
                            if ((*p >= 0xa0) && (*p <= 0xbf))
                                (*p) -= 0x20;
                            break;
                        default:
                            break;
                        }
                        break;
                    case 0x9E:
                        pExtChar = p;
                        p++;
                        switch (*pExtChar) {
                        case 0xA4: // Adlam
                            if ((*p >= 0xa2) && (*p <= 0xbf))
                                (*p) -= 0x22;
                            break;
                        case 0xA5: // Adlam
                            if ((*p >= 0x80) && (*p <= 0x83)) {
                                *(pExtChar) = 0xa4;
                                (*p) += 0x1e;
                            }
                            break;
                        default:
                            break;
                        }
                        break;
                    }
                    break;
                default:
                    break;
                }
                pExtChar = 0;
            }
            p++;
        }
    }
    return pString;
}

Quad2dD TextHelper::rotateQuad2d(const Quad2dD &quad, const Vec2D &aroundPoint, double angleDegrees) {
    Vec2D midPoint = aroundPoint;
    auto newTopLeft = Vec2DHelper::rotate(quad.topLeft, midPoint, angleDegrees);
    auto newTopRight = Vec2DHelper::rotate(quad.topRight, midPoint, angleDegrees);
    auto newBottomLeft = Vec2DHelper::rotate(quad.bottomLeft, midPoint, angleDegrees);
    auto newBottomRight = Vec2DHelper::rotate(quad.bottomRight, midPoint, angleDegrees);
    return Quad2dD(newTopLeft, newTopRight, newBottomRight, newBottomLeft);
}
