// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from core.djinni
#ifdef __cplusplus

#include "MapCamera3dInterface.h"
#include <memory>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@class MCMapCamera3dInterface;

namespace djinni_generated {

class MapCamera3dInterface
{
public:
    using CppType = std::shared_ptr<::MapCamera3dInterface>;
    using CppOptType = std::shared_ptr<::MapCamera3dInterface>;
    using ObjcType = MCMapCamera3dInterface*;

    using Boxed = MapCamera3dInterface;

    static CppType toCpp(ObjcType objc);
    static ObjcType fromCppOpt(const CppOptType& cpp);
    static ObjcType fromCpp(const CppType& cpp) { return fromCppOpt(cpp); }

private:
    class ObjcProxy;
};

} // namespace djinni_generated

#endif