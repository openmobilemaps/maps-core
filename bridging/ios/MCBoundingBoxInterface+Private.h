// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from coordinate_system.djinni
#ifdef __cplusplus

#include "BoundingBoxInterface.h"
#include <memory>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@class MCBoundingBoxInterface;

namespace djinni_generated {

class BoundingBoxInterface
{
public:
    using CppType = std::shared_ptr<::BoundingBoxInterface>;
    using CppOptType = std::shared_ptr<::BoundingBoxInterface>;
    using ObjcType = MCBoundingBoxInterface*;

    using Boxed = BoundingBoxInterface;

    static CppType toCpp(ObjcType objc);
    static ObjcType fromCppOpt(const CppOptType& cpp);
    static ObjcType fromCpp(const CppType& cpp) { return fromCppOpt(cpp); }

private:
    class ObjcProxy;
};

} // namespace djinni_generated

#endif