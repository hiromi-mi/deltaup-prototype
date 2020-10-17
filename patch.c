#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "common.h"

int main(int argc, const char** argv) {
   if (argc <= 2) {
      fprintf(stderr, "Usage: oldfile difffile\n");
      exit(-1);
   }
   const int ORIG_MAX = 2 << 23;
   // TODO calloc を使うと0埋めされる
   char* orig = malloc(sizeof(char) * ORIG_MAX);
   // char buf[5] = {0};
   memset(orig,0,  ORIG_MAX);
   FILE *fp;
   fp = fopen(argv[1], "r");
   if (fp == NULL) {
      exit(-1);
   }
   // ここを orignum += 1 とうすると '\0' の不要なものまで出力される
   const size_t orignum = fread(orig, sizeof(char), ORIG_MAX-1, fp) / sizeof(char);
   fclose(fp);

   fp = fopen(argv[2], "r");
   if (fp == NULL) {
      exit(-1);
   }
   Action act[500000];
   size_t cnt_num[500000];
   char new_num[500000];
   size_t cnt;
   int action;
   size_t i = 0;
   for (cnt=0;!feof(fp);cnt++) {
      fscanf(fp, "%lx,%d", &cnt_num[cnt], &action);
      // TODO 想定外の値に対してエラー
      act[cnt] = (Action)action;
      while (i < cnt_num[cnt]) {
         putchar(orig[i]);
         i++;
      }
      switch(act[cnt]) {
         case SUBSTITUTE:
            fscanf(fp, ",%hhx\n", &new_num[cnt]);
            putchar(new_num[cnt]);
            i++;
            break;
         case INSERT:
            fscanf(fp, ",%hhx\n", &new_num[cnt]);
            putchar(new_num[cnt]);
            char c;
            /*
            while (fscanf(fp, " %hhx", &c) > 0) {
               putchar(c);
            }*/
            break;
         case DELETE:
            fscanf(fp, "\n");
            i++;
            break;
         default:
            fprintf(stderr, "Error");
            exit(-1);
      }
   }
   while (i < orignum) {
      putchar(orig[i]);
      i++;
   }
   fclose(fp);
   return 0;
}
