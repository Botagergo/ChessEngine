#pragma once

#ifdef _DEBUG
#define ASSERT(x) assert(x)
#else
#define ASSERT(x) {}
#endif

#define MAX_MOVES 100
#define MAX_DEPTH 50

#define DEFAULT_HASH_TABLE_SIZE 32