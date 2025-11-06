#pragma once

#ifndef __clang__
#define _Nonnull
#define _Nullable
#endif

#if defined(__GNUC__) || defined(__clang__)
#define sve4_likely(x) __builtin_expect(!!(x), 1)
#define sve4_unlikely(x) __builtin_expect(!!(x), 0)
#define sve4_has_include(x) __has_include(x)
#define sve4_gnu_attribute(...) __attribute__(__VA_ARGS__)
#define sve4_noreturn_dummy(...)
#else
#define sve4_likely(x) (x)
#define sve4_unlikely(x) (x)
#define sve4_has_include(x) 0
#define sve4_gnu_attribute(...)
#define sve4_noreturn_dummy(...) return __VA_ARGS
#endif
