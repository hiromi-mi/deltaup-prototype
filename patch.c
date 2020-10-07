#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
   NONE = 0,
   DELETE = 1 << 0,
   INSERT = 1 << 1,
   SUBSTITUTE = 1 << 2,
   MATCH = 1 << 3
} Action;


int main(int argc, const char** argv) {
   if (argc <= 2) {
      fprintf(stderr, "Usage: oldfile difffile\n");
      exit(-1);
   }
   const int ORIG_MAX = 4098*2;
   char orig[ORIG_MAX];
   memset(orig,0,  ORIG_MAX);
   FILE *fp;
   fp = fopen(argv[1], "r");
   if (fp == NULL) {
      exit(-1);
   }
   const int orignum = fread(orig, sizeof(char), ORIG_MAX-1, fp) / sizeof(char) + 1;
   fclose(fp);

   fp = fopen(argv[2], "r");
   if (fp == NULL) {
      exit(-1);
   }
   Action act[10000];
   char orig_num[10000];
   char new_num[10000];
   int cnt;
   for (cnt=0;!feof(fp);cnt++) {
      fscanf(fp, "%d,%hhd,%hhd\n", &act[cnt], &orig_num[cnt], &new_num[cnt]);
   }
   fclose(fp);
   int i=0;
   for (int cmd =cnt-1;cmd>=0 && i < orignum;cmd--) {
      switch(act[cmd]) {
         case MATCH:
            putchar(orig[i]);
            i++;
            break;
         case SUBSTITUTE:
            putchar(new_num[cmd]);
            i++;
            break;
         case INSERT:
            putchar(new_num[cmd]);
            break;
         case DELETE:
            i++;
            break;
         default:
            fprintf(stderr, "Error");
            exit(-1);
      }
   }
   return 0;
}
