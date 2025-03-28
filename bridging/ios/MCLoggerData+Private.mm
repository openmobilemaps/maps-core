// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from map_helpers.djinni

#import "MCLoggerData+Private.h"
#import "DJIMarshal+Private.h"
#include <cassert>

namespace djinni_generated {

auto LoggerData::toCpp(ObjcType obj) -> CppType
{
    assert(obj);
    return {::djinni::String::toCpp(obj.id),
            ::djinni::List<::djinni::I64>::toCpp(obj.buckets),
            ::djinni::I32::toCpp(obj.bucketSizeMs),
            ::djinni::I64::toCpp(obj.start),
            ::djinni::I64::toCpp(obj.end),
            ::djinni::I64::toCpp(obj.numSamples),
            ::djinni::F64::toCpp(obj.average),
            ::djinni::F64::toCpp(obj.variance),
            ::djinni::F64::toCpp(obj.stdDeviation)};
}

auto LoggerData::fromCpp(const CppType& cpp) -> ObjcType
{
    return [[MCLoggerData alloc] initWithId:(::djinni::String::fromCpp(cpp.id))
                                    buckets:(::djinni::List<::djinni::I64>::fromCpp(cpp.buckets))
                               bucketSizeMs:(::djinni::I32::fromCpp(cpp.bucketSizeMs))
                                      start:(::djinni::I64::fromCpp(cpp.start))
                                        end:(::djinni::I64::fromCpp(cpp.end))
                                 numSamples:(::djinni::I64::fromCpp(cpp.numSamples))
                                    average:(::djinni::F64::fromCpp(cpp.average))
                                   variance:(::djinni::F64::fromCpp(cpp.variance))
                               stdDeviation:(::djinni::F64::fromCpp(cpp.stdDeviation))];
}

} // namespace djinni_generated
