#ifndef ASKF_SIGNATURE_MACRO_HELPERS_H
#define ASKF_SIGNATURE_MACRO_HELPERS_H

#include "../inttype.h"
#include "optimizer.h"

#define _DEF_SIG_2ARGS(a, b)                       \
    (((u64)(a) << SIGNATURE_BITS) | (u64)(b))

#define _DEF_SIG_3ARGS(a, b, c)                 \
    (((u64)(a) << (SIGNATURE_BITS * 2)) |       \
     ((u64)(b) << SIGNATURE_BITS) |             \
      (u64)(c))

#define _DEF_SIG_4ARGS(a, b, c, d)              \
    (((u64)(a) << (SIGNATURE_BITS * 3)) |       \
     ((u64)(b) << (SIGNATURE_BITS * 2)) |       \
     ((u64)(c) << SIGNATURE_BITS) |             \
      (u64)(d))

#define _DEF_SIG_5ARGS(a, b, c, d, e)              \
    (((u64)(a) << (SIGNATURE_BITS * 4)) |       \
     ((u64)(b) << (SIGNATURE_BITS * 3)) |       \
     ((u64)(c) << (SIGNATURE_BITS * 2)) |       \
     ((u64)(d) << SIGNATURE_BITS) |              \
      (u64)(e))
    
#define _DEF_SIG_6ARGS(a, b, c, d, e, f)        \
    (((u64)(a) << (SIGNATURE_BITS * 5)) |       \
     ((u64)(b) << (SIGNATURE_BITS * 4)) |       \
     ((u64)(c) << (SIGNATURE_BITS * 3)) |       \
     ((u64)(d) << (SIGNATURE_BITS * 2)) |       \
     ((u64)(e) << SIGNATURE_BITS ) |             \
      (u64)(f))


#endif
