// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from network_activity_manager.djinni
#ifdef __cplusplus

#include "NetworkActivityListenerInterface.h"
#include <memory>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@protocol MCNetworkActivityListenerInterface;

namespace djinni_generated {

class NetworkActivityListenerInterface
{
public:
    using CppType = std::shared_ptr<::NetworkActivityListenerInterface>;
    using CppOptType = std::shared_ptr<::NetworkActivityListenerInterface>;
    using ObjcType = id<MCNetworkActivityListenerInterface>;

    using Boxed = NetworkActivityListenerInterface;

    static CppType toCpp(ObjcType objc);
    static ObjcType fromCppOpt(const CppOptType& cpp);
    static ObjcType fromCpp(const CppType& cpp) { return fromCppOpt(cpp); }

private:
    class ObjcProxy;
};

}  // namespace djinni_generated

#endif