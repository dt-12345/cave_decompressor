#pragma once
#include "lib.hpp"

#include "nn/util/util_snprintf.hpp"
#include "nn.hpp"

extern char sAppVersion[0x10];
extern u32  sAppVersionIndex;
constexpr inline const u32 cOffsetCount  = 6;

#define FUNCTION_OFFSET(function, offset_array) reinterpret_cast<function>(exl::util::modules::GetTargetStart() + ::GetAppVersionOffset(offset_array))
#define SINGLETON_OFFSET(class, offset_array) \
public: \
    static class *GetInstance() { return *reinterpret_cast<class**>(exl::util::modules::GetTargetStart() + ::GetAppVersionOffset(offset_array)); }

#define GLOBAL_OFFSET(get_func_name, type, offset_array) \
    static type *get_func_name() { return reinterpret_cast<type*>(exl::util::modules::GetTargetStart() + ::GetAppVersionOffset(offset_array)); }


u32 InitializeAppVersion();

ALWAYS_INLINE u32 GetAppVersionIndex() { return sAppVersionIndex; }

ALWAYS_INLINE size_t GetAppVersionOffset(const size_t *offset_array) { return (offset_array[::GetAppVersionIndex()] - 0x71'0000'0000); }
