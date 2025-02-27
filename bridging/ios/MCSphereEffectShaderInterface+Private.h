// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from shader.djinni
#ifdef __cplusplus

#include "SphereEffectShaderInterface.h"
#include <memory>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@protocol MCSphereEffectShaderInterface;

namespace djinni_generated {

class SphereEffectShaderInterface
{
public:
    using CppType = std::shared_ptr<::SphereEffectShaderInterface>;
    using CppOptType = std::shared_ptr<::SphereEffectShaderInterface>;
    using ObjcType = id<MCSphereEffectShaderInterface>;

    using Boxed = SphereEffectShaderInterface;

    static CppType toCpp(ObjcType objc);
    static ObjcType fromCppOpt(const CppOptType& cpp);
    static ObjcType fromCpp(const CppType& cpp) { return fromCppOpt(cpp); }

private:
    class ObjcProxy;
};

} // namespace djinni_generated

#endif
