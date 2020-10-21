typedef enum {
   NONE = 0,
   DELETE = 1 << 0,
   INSERT = 1 << 1,
   SUBSTITUTE = 1 << 2,
   MATCH = 1 << 3
} Action;

#define max(x,y) ((x) < (y) ? y : x)
#define min(x,y) ((y) < (x) ? y : x)

#define INF 1000000
