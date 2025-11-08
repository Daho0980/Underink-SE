#ifndef PLATFORM_H
#define PLATFORM_H

#if defined(__MACH__) || defined(__APPLE__)
#define IS_APPLE 1
#else
#define IS_APPLE 0
#endif

#if defined(__linux__)
#define IS_LINUX 1
#else
#define IS_LINUX 0
#endif

#if defined(_WIN32) || defined(_WIN64)
#define IS_WIN 1
#else
#define IS_WIN 0
#endif

#if ( (IS_APPLE+IS_LINUX+IS_WIN) != 1 )
#define WTF_IS_THIS_OS 1
#else
#define WTF_IS_THIS_OS 0
#endif


#if defined(__aarch64__) || defined(_M_ARM64)
#define IS_ARM64 1
#else
#define IS_ARM64 0
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define IS_X86_64 1
#else
#define IS_X86_64 0
#endif

#if ( (IS_ARM64+IS_X86_64) != 1 )
#define WTF_IS_THIS_CPU 1
#else
#define WTF_IS_THIS_CPU 0
#endif

#endif