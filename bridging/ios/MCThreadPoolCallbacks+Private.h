// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from task_scheduler.djinni
#ifdef __cplusplus

#include "ThreadPoolCallbacks.h"
#include <memory>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@protocol MCThreadPoolCallbacks;

namespace djinni_generated {

class ThreadPoolCallbacks
{
public:
    using CppType = std::shared_ptr<::ThreadPoolCallbacks>;
    using CppOptType = std::shared_ptr<::ThreadPoolCallbacks>;
    using ObjcType = id<MCThreadPoolCallbacks>;

    using Boxed = ThreadPoolCallbacks;

    static CppType toCpp(ObjcType objc);
    static ObjcType fromCppOpt(const CppOptType& cpp);
    static ObjcType fromCpp(const CppType& cpp) { return fromCppOpt(cpp); }

private:
    class ObjcProxy;
};

} // namespace djinni_generated

#endif
