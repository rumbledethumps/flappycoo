#ifndef TEST_H
#define TEST_H

#ifdef __mos__
// No platform branch in utest.h matches the 6502. The AIX branch needs only
// clock_gettime(). clock_gettime() and isatty() are missing from the llvm-mos
// C library, so both are stubs here.
#define _AIX 1
#define CLOCK_REALTIME 0
#define clock_gettime(id, ts) ((void)(id), (ts)->tv_sec = 0, (ts)->tv_nsec = 0)
#define isatty(fd) 0
#include "utest.h"
#undef _AIX
#undef CLOCK_REALTIME
#undef clock_gettime
#undef isatty
// Failure messages are printed to stdout only. The extra copy for --output
// doubles the code of every EXPECT and ASSERT, and with it the test ROM does
// not fit in RAM.
#undef UTEST_PRINTF
#define UTEST_PRINTF(...) printf(__VA_ARGS__)
#else
#include "utest.h"
#endif

#endif
