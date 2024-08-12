//typedef unsigned int uint;
//typedef unsigned char u8;
//typedef unsigned short u16;
//typedef unsigned int u32;
//typedef unsigned long u64;
//typedef signed char s8;
//typedef signed short s16;
//typedef signed int s32;
//typedef signed long long s64;

typedef unsigned int uint;
typedef std::uint8_t u8;
typedef std::uint16_t u16;
typedef std::uint32_t u32;
typedef std::uint64_t u64;
typedef std::int8_t s8;
typedef std::int16_t s16;
typedef std::int32_t s32;
typedef std::int64_t s64;

#define MODE_USER 0b10000
#define MODE_SYSTEM 0b11111
#define MODE_FIQ 0b10001
#define MODE_IRQ 0b10010
#define MODE_SVC 0b10011
#define MODE_ABT 0b10111
#define MODE_UND 0b11011

#define lenOfArray(arr) (sizeof(arr) / sizeof(arr[0]))

#define print(x) std::cout << (x) << "\n"
#define printBits(x, n) std::cout << std::bitset<n>(x) << "\n"
#define printAndCrash(x) print(x); assert(0);