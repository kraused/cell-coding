
#ifndef CELL_CODING_COMMON_COMPILER_H
#define CELL_CODING_COMMON_COMPILER_H 1

#define   likely(x) 	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

#endif

