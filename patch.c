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

   fp = fopen(argv[2], "rb");
   if (fp == NULL) {
      exit(-1);
   }
   size_t cnt_num = 0;
   size_t prev_cnt_num = 0;
   unsigned char cnt_num_delta;
   char new_num;
   size_t cnt;
   Action action;
   // size_t len;
   unsigned char len;
   size_t i = 0;
   unsigned char tmp;
   for (cnt = 0; !feof(fp); cnt++) {
      // if (fscanf(fp, "%lx,%d", &cnt_num, &action) < 0) {
      prev_cnt_num = cnt_num;
      if (fread(&cnt_num_delta, sizeof(cnt_num_delta), 1, fp) == 0) {
         break;
      }
      cnt_num = cnt_num_delta + prev_cnt_num;
      assert(fread(&action, sizeof(Action), 1, fp) > 0);
      if (ferror(fp) != 0) {
         fputs("Error!\n", stderr);
         break;
      }
      // TODO 想定外の値に対してエラー
      while (i < cnt_num && i < orignum) {
         putchar(orig[i]);
         i++;
      }
      switch ((Action)action) {
         case SUBSTITUTE:
         case ADD:
            // while (fscanf(fp, ",%hhx", &new_num) > 0) {
            assert(fread(&tmp, 1, 1, fp) > 0);
            for (unsigned char j = 0; j < tmp; j++) {
               assert(fread(&new_num, 1, 1, fp) > 0);
               if ((Action)action == SUBSTITUTE) {
                  putchar(new_num);
               } else if ((Action)action == ADD) {
                  putchar(new_num + orig[i]);
               }
               i++;
            }
            // fscanf(fp, "\n");
            break;
         case INSERT:
            assert(fread(&tmp, 1, 1, fp) > 0);
            for (unsigned char j = 0; j < tmp; j++) {
               assert(fread(&new_num, 1, 1, fp) > 0);
               putchar(new_num);
            }
            /*
            while (fscanf(fp, ",%hhx", &new_num) > 0) {
               putchar(new_num);
            }
            fscanf(fp, "\n");
            */
            break;
         case DELETE:
            assert(fread(&len, 1, 1, fp) > 0);
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
   // fprintf(stderr, "cnt: %ld\n", cnt);
   fclose(fp);
   return 0;
}
