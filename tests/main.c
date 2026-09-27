#include "test.h"

#ifdef __mos__
#include <stdlib.h>
// On llvm-mos, argc is 0 unless this function is defined. utest options such
// as --filter are read from argv.
void *__argv_mem(size_t size) { return malloc(size); }
#endif

UTEST_MAIN()
