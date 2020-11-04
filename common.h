#include <stddef.h>
/*
// こちらよりも下のように INSERT を積極的に利用したほうが小さくなる
#define SUBSTITUTE 0
#define DELETE 1
#define INSERT 2
#define MATCH 3
*/
#define MATCH 0
#define SUBSTITUTE 1
#define INSERT 2
#define DELETE 3
#define ADD 4
#define SEEK 5
#define SKIP 9
typedef char Action;

#define max(x, y) ((x) < (y) ? (y) : (x))
#define min(x, y) ((y) < (x) ? (y) : (x))

#define INF 1000000

int argmin(int *costs, size_t n);
char *read_file(const char *filename, size_t *len);
