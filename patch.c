#include "common.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, const char **argv) {
   if (argc <= 2) {
      fprintf(stderr, "Usage: oldfile difffile\n");
      exit(-1);
   }
   const int ORIG_MAX = 2 << 23;
   // TODO calloc を使うと0埋めされる
   char *orig = malloc(sizeof(char) * ORIG_MAX);
   // char buf[5] = {0};
   memset(orig, 0, ORIG_MAX);
   FILE *fp;
   fp = fopen(argv[1], "r");
   if (fp == NULL) {
      exit(-1);
   }
   // ここを orignum += 1 とうすると '\0' の不要なものまで出力される
   const size_t orignum =
       fread(orig, sizeof(char), ORIG_MAX - 1, fp) / sizeof(char);
   fclose(fp);

   fp = fopen(argv[2], "r");
   if (fp == NULL) {
      exit(-1);
   }
   size_t cnt_num;
   char new_num;
   size_t cnt;
   int action;
   size_t len;
   size_t i = 0;
   for (cnt = 0; !feof(fp); cnt++) {
      if (fscanf(fp, "%lx,%d", &cnt_num, &action) < 0) {
         break;
      }
      // TODO 想定外の値に対してエラー
      while (i < cnt_num && i < orignum) {
         putchar(orig[i]);
         i++;
      }
      switch ((Action)action) {
         case SUBSTITUTE:
            while (fscanf(fp, ",%hhx", &new_num) > 0) {
               putchar(new_num);
               i++;
            }
            fscanf(fp, "\n");
            break;
         case INSERT:
            while (fscanf(fp, ",%hhx", &new_num) > 0) {
               putchar(new_num);
            }
            fscanf(fp, "\n");
            break;
         case DELETE:
            fscanf(fp, ",%ld\n", &len);
            i += len;
            break;
         default:
            fprintf(stderr, "Error: %d on Commond No. %ld\n", action, cnt);
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
