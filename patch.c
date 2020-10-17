#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

int main(int argc, const char** argv) {
   if (argc <= 2) {
      fprintf(stderr, "Usage: oldfile difffile\n");
      exit(-1);
   }
   const int ORIG_MAX = 2 << 22;
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
   const int orignum = fread(orig, sizeof(char), ORIG_MAX-1, fp) / sizeof(char);
   fclose(fp);

   fp = fopen(argv[2], "r");
   if (fp == NULL) {
      exit(-1);
   }
   Action act[10000];
   int cnt_num[10000];
   char new_num[10000];
   int cnt;
   int action;
   for (cnt=0;!feof(fp);cnt++) {
      fscanf(fp, "%d,%d,%c\n", &cnt_num[cnt], &action, &new_num[cnt]);
      // TODO 想定外の値に対してエラー
      act[cnt] = (Action)action;
   }
   fclose(fp);
   int i=0;
   for (int cmd =cnt-1;cmd>=0;cmd--) {
      while (i < cnt_num[cmd]) {
         putchar(orig[i]);
         i++;
      }
      switch(act[cmd]) {
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
   while (i < orignum) {
      putchar(orig[i]);
      i++;
   }
   return 0;
}
