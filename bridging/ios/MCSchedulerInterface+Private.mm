// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from task_scheduler.djinni

#import "MCSchedulerInterface+Private.h"
#import "MCSchedulerInterface.h"
#import "DJICppWrapperCache+Private.h"
#import "DJIError.h"
#import "DJIMarshal+Private.h"
#import "DJIObjcWrapperCache+Private.h"
#import "MCSchedulerGraphicsTaskCallbacks+Private.h"
#import "MCTaskInterface+Private.h"
#import "MCThreadPoolCallbacks+Private.h"
#include <exception>
#include <stdexcept>
#include <utility>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@interface MCSchedulerInterfaceCppProxy : NSObject<MCSchedulerInterface>

- (id)initWithCpp:(const std::shared_ptr<::SchedulerInterface>&)cppRef;

@end

@implementation MCSchedulerInterfaceCppProxy {
    ::djinni::CppProxyCache::Handle<std::shared_ptr<::SchedulerInterface>> _cppRefHandle;
}

- (id)initWithCpp:(const std::shared_ptr<::SchedulerInterface>&)cppRef
{
    if (self = [super init]) {
        _cppRefHandle.assign(cppRef);
    }
    return self;
}

- (void)addTask:(nullable id<MCTaskInterface>)task {
    try {
        _cppRefHandle.get()->addTask(::djinni_generated::TaskInterface::toCpp(task));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)addTasks:(nonnull NSArray<id<MCTaskInterface>> *)tasks {
    try {
        _cppRefHandle.get()->addTasks(::djinni::List<::djinni_generated::TaskInterface>::toCpp(tasks));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)removeTask:(nonnull NSString *)id {
    try {
        _cppRefHandle.get()->removeTask(::djinni::String::toCpp(id));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)clear {
    try {
        _cppRefHandle.get()->clear();
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)pause {
    try {
        _cppRefHandle.get()->pause();
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)resume {
    try {
        _cppRefHandle.get()->resume();
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)destroy {
    try {
        _cppRefHandle.get()->destroy();
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (BOOL)hasSeparateGraphicsInvocation {
    try {
        auto objcpp_result_ = _cppRefHandle.get()->hasSeparateGraphicsInvocation();
        return ::djinni::Bool::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (BOOL)runGraphicsTasks {
    try {
        auto objcpp_result_ = _cppRefHandle.get()->runGraphicsTasks();
        return ::djinni::Bool::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)setSchedulerGraphicsTaskCallbacks:(nullable MCSchedulerGraphicsTaskCallbacks *)callbacks {
    try {
        _cppRefHandle.get()->setSchedulerGraphicsTaskCallbacks(::djinni_generated::SchedulerGraphicsTaskCallbacks::toCpp(callbacks));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (nullable id<MCThreadPoolCallbacks>)getThreadPoolCallbacks {
    try {
        auto objcpp_result_ = _cppRefHandle.get()->getThreadPoolCallbacks();
        return ::djinni_generated::ThreadPoolCallbacks::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

namespace djinni_generated {

class SchedulerInterface::ObjcProxy final
: public ::SchedulerInterface
, private ::djinni::ObjcProxyBase<ObjcType>
{
    friend class ::djinni_generated::SchedulerInterface;
public:
    using ObjcProxyBase::ObjcProxyBase;
    void addTask(const /*not-null*/ std::shared_ptr<::TaskInterface> & c_task) override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() addTask:(::djinni_generated::TaskInterface::fromCpp(c_task))];
        }
    }
    void addTasks(const std::vector</*not-null*/ std::shared_ptr<::TaskInterface>> & c_tasks) override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() addTasks:(::djinni::List<::djinni_generated::TaskInterface>::fromCpp(c_tasks))];
        }
    }
    void removeTask(const std::string & c_id) override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() removeTask:(::djinni::String::fromCpp(c_id))];
        }
    }
    void clear() override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() clear];
        }
    }
    void pause() override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() pause];
        }
    }
    void resume() override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() resume];
        }
    }
    void destroy() override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() destroy];
        }
    }
    bool hasSeparateGraphicsInvocation() override
    {
        @autoreleasepool {
            auto objcpp_result_ = [djinni_private_get_proxied_objc_object() hasSeparateGraphicsInvocation];
            return ::djinni::Bool::toCpp(objcpp_result_);
        }
    }
    bool runGraphicsTasks() override
    {
        @autoreleasepool {
            auto objcpp_result_ = [djinni_private_get_proxied_objc_object() runGraphicsTasks];
            return ::djinni::Bool::toCpp(objcpp_result_);
        }
    }
    void setSchedulerGraphicsTaskCallbacks(const /*not-null*/ std::shared_ptr<::SchedulerGraphicsTaskCallbacks> & c_callbacks) override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() setSchedulerGraphicsTaskCallbacks:(::djinni_generated::SchedulerGraphicsTaskCallbacks::fromCpp(c_callbacks))];
        }
    }
    /*not-null*/ std::shared_ptr<::ThreadPoolCallbacks> getThreadPoolCallbacks() override
    {
        @autoreleasepool {
            auto objcpp_result_ = [djinni_private_get_proxied_objc_object() getThreadPoolCallbacks];
            return ::djinni_generated::ThreadPoolCallbacks::toCpp(objcpp_result_);
        }
    }
};

} // namespace djinni_generated

namespace djinni_generated {

auto SchedulerInterface::toCpp(ObjcType objc) -> CppType
{
    if (!objc) {
        return nullptr;
    }
    if ([(id)objc isKindOfClass:[MCSchedulerInterfaceCppProxy class]]) {
        return ((MCSchedulerInterfaceCppProxy*)objc)->_cppRefHandle.get();
    }
    return ::djinni::get_objc_proxy<ObjcProxy>(objc);
}

auto SchedulerInterface::fromCppOpt(const CppOptType& cpp) -> ObjcType
{
    if (!cpp) {
        return nil;
    }
    if (auto cppPtr = dynamic_cast<ObjcProxy*>(cpp.get())) {
        return cppPtr->djinni_private_get_proxied_objc_object();
    }
    return ::djinni::get_cpp_proxy<MCSchedulerInterfaceCppProxy>(cpp);
}

} // namespace djinni_generated

@end
