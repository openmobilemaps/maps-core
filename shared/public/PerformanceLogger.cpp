#include "PerformanceLogger.h"

#ifdef ENABLE_PERF_LOGGING
thread_local PerformanceLogger::ThreadLocalData PerformanceLogger::local_data;
#endif
