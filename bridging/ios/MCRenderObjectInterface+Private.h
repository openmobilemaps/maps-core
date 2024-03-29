// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from core.djinni
#ifdef __cplusplus

#include "RenderObjectInterface.h"
#include <memory>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@protocol MCRenderObjectInterface;

namespace djinni_generated {

class RenderObjectInterface
{
public:
    using CppType = std::shared_ptr<::RenderObjectInterface>;
    using CppOptType = std::shared_ptr<::RenderObjectInterface>;
    using ObjcType = id<MCRenderObjectInterface>;

    using Boxed = RenderObjectInterface;

    static CppType toCpp(ObjcType objc);
    static ObjcType fromCppOpt(const CppOptType& cpp);
    static ObjcType fromCpp(const CppType& cpp) { return fromCppOpt(cpp); }

private:
    class ObjcProxy;
};

} // namespace djinni_generated

#endif
