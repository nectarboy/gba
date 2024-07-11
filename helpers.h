typedef unsigned int uint;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long s64;

#define MODE_USER 0b10000
#define MODE_SYSTEM 0b11111
#define MODE_FIQ 0b10001
#define MODE_IRQ 0b10010
#define MODE_SVC 0b10011
#define MODE_ABT 0b10111
#define MODE_UND 0b11011

#define lenOfArray(arr) (sizeof(arr) / sizeof(arr[0]))