// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from shader.djinni
#ifdef __cplusplus

#include "RasterShaderInterface.h"
#include <memory>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@protocol MCRasterShaderInterface;

namespace djinni_generated {

class RasterShaderInterface
{
public:
    using CppType = std::shared_ptr<::RasterShaderInterface>;
    using CppOptType = std::shared_ptr<::RasterShaderInterface>;
    using ObjcType = id<MCRasterShaderInterface>;

    using Boxed = RasterShaderInterface;

    static CppType toCpp(ObjcType objc);
    static ObjcType fromCppOpt(const CppOptType& cpp);
    static ObjcType fromCpp(const CppType& cpp) { return fromCppOpt(cpp); }

private:
    class ObjcProxy;
};

} // namespace djinni_generated

#endif
