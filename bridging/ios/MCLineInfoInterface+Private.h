// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from line.djinni
#ifdef __cplusplus

#include "LineInfoInterface.h"
#include <memory>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@class MCLineInfoInterface;

namespace djinni_generated {

class LineInfoInterface
{
public:
    using CppType = std::shared_ptr<::LineInfoInterface>;
    using CppOptType = std::shared_ptr<::LineInfoInterface>;
    using ObjcType = MCLineInfoInterface*;

    using Boxed = LineInfoInterface;

    static CppType toCpp(ObjcType objc);
    static ObjcType fromCppOpt(const CppOptType& cpp);
    static ObjcType fromCpp(const CppType& cpp) { return fromCppOpt(cpp); }

private:
    class ObjcProxy;
};

} // namespace djinni_generated

#endif
