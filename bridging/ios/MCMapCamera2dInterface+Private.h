// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from core.djinni
#ifdef __cplusplus

#include "MapCamera2dInterface.h"
#include <memory>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@class MCMapCamera2dInterface;

namespace djinni_generated {

class MapCamera2dInterface
{
public:
    using CppType = std::shared_ptr<::MapCamera2dInterface>;
    using CppOptType = std::shared_ptr<::MapCamera2dInterface>;
    using ObjcType = MCMapCamera2dInterface*;

    using Boxed = MapCamera2dInterface;

    static CppType toCpp(ObjcType objc);
    static ObjcType fromCppOpt(const CppOptType& cpp);
    static ObjcType fromCpp(const CppType& cpp) { return fromCppOpt(cpp); }

private:
    class ObjcProxy;
};

} // namespace djinni_generated

#endif
