#pragma once

#ifndef __clang__
#define _Nonnull
#define _Nullable
#endif

#if defined(__GNUC__) || defined(__clang__)
#define sve4_likely(x) __builtin_expect(!!(x), 1)
#define sve4_unlikely(x) __builtin_expect(!!(x), 0)
#elif defined(_MSC_VER)
#define sve4_likely(x) (__assume(!!(x)), (x))
#define sve4_unlikely(x) (__assume(!(x)), (x))
#else
#define sve4_likely(x) (x)
#define sve4_unlikely(x) (x)
#endif
