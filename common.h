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
typedef char Action;

#define max(x, y) ((x) < (y) ? (y) : (x))
#define min(x, y) ((y) < (x) ? (y) : (x))

#define INF 1000000
