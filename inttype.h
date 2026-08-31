#ifndef INTTYPE_H
#define INTTYPE_H

typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char i8;
typedef signed short int i16;
typedef signed int i32;
typedef signed long long i64;

typedef float f32;
typedef double f64;
typedef u8 boolean;

#if UINTPTR_MAX == UINT64_MAX
#elif UINTPTR_MAX == UINT32_MAX
#elif UINTPTR_MAX == UINT16_MAX
#elif UINTPTR_MAX == UINT32_MAX
#else
    #error "Unsupported pointer size"
#endif


#if   defined( ARQBITS64 )
    typedef u64 askf_addr_t;
#elif defined( ARQBITS32 )
    typedef u32 askf_addr_t;
#elif defined( ARQBITS16 )
    typedef u16 askf_addr_t;
#elif defined( ARQBITS8  )
    typedef u8 askf_addr_t;
#else 
    typedef u64 askf_addr_t; // default to 64bits
#endif

typedef u8 ascii;

#define NULL (void *)0

#define KSTR(str) ((u8 *)str)

#define FILL(ptr, c, len_bytes)                                                \
  do {                                                                         \
    for (u64 x = 0; x < len_bytes; x++)                                        \
      *(((u8 *)ptr) + x) = c;                                                  \
  } while (0);

#define COPY(old_ptr, new_ptr, len_bytes)                                      \
  do {                                                                         \
    for (u64 x = 0; x < len_bytes; x++)                                        \
      *(((u8 *)new_ptr) + x) = *(((u8 *)old_ptr) + x);                         \
  } while (0);

#define ALIGN_UP(addr, a) (((addr) + (a - 1)) & ~(a - 1))
#define ALIGN_DOWN(addr, a) ((addr) & ~(a - 1))

#define TRUE 1
#define FALSE 0

#define UNUSED(x) (void)x

#define KB(n) ((u64)(n * 1024ULL))
#define MB(n) ((u64)(n * (1024ULL * 1024ULL)))
#define GB(n) ((u64)(n * (1024ULL * 1024ULL * 1024ULL)))

#define POISON 0xDEDEDEDEDEDEDEDEULL

#endif
