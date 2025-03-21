// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from map_helpers.djinni
#ifdef __cplusplus

#include "ErrorManagerListener.h"
#include <memory>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@protocol MCErrorManagerListener;

namespace djinni_generated {

class ErrorManagerListener
{
public:
    using CppType = std::shared_ptr<::ErrorManagerListener>;
    using CppOptType = std::shared_ptr<::ErrorManagerListener>;
    using ObjcType = id<MCErrorManagerListener>;

    using Boxed = ErrorManagerListener;

    static CppType toCpp(ObjcType objc);
    static ObjcType fromCppOpt(const CppOptType& cpp);
    static ObjcType fromCpp(const CppType& cpp) { return fromCppOpt(cpp); }

private:
    class ObjcProxy;
};

} // namespace djinni_generated

#endif
