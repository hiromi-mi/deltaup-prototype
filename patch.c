#include "common.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, const char **argv) {
   if (argc <= 2) {
      fprintf(stderr, "Usage: oldfile difffile\n");
      exit(-1);
   }
   // ここを orignum += 1 とうすると '\0' の不要なものまで出力される
   size_t orignum;
   char *orig = read_file(argv[1], &orignum);

   FILE *fp = fopen(argv[2], "rb");
   if (fp == NULL) {
      perror("fopen");
      exit(-1);
   }
   size_t index = 0;
   size_t prev_index = 0;
   unsigned char index_delta;
   char new_num;
   size_t cnt;
   Action action;
   size_t i = 0;
   long long len;
   long long insertlen;

   while (!feof(fp)) {
      // SEEK
      if (fread(&i, sizeof(i), 1, fp) == 0) {
         break;
      }

      // ADD
      assert(fread(&len, sizeof(len), 1, fp) > 0);
      // INSERT
      if (fread(&insertlen, sizeof(insertlen), 1, fp) == 0) {
         break;
      }
      assert(len >= 0);
      for (int j = 0; j < len; j++) {
         assert(fread(&new_num, 1, 1, fp) > 0);
         putchar(new_num + orig[i]);
         i++;
      }

      for (unsigned long j = 0; j < insertlen; j++) {
         assert(fread(&new_num, 1, 1, fp) > 0);
         putchar(new_num);
      }
   }
   /*
   for (cnt = 0; !feof(fp); cnt++) {
      prev_index = index;

      if (fread(&action, sizeof(Action), 1, fp) == 0)
         break;
      if ((Action)action == SEEK) {
         fread(&index, sizeof(index), 1, fp);
         fread(&i, 4, 1, fp);
         continue;
      }
      assert(fread(&index_delta, sizeof(index_delta), 1, fp) > 0);
      index = index_delta + prev_index;
      //      unsigned int act1 = index_delta & (1 << 7);
      //      index_delta &= ~(1 << 7);
      // fprintf(stderr, "%ld %u %ld\n", prev_index, index_delta, index);
      if (ferror(fp) != 0) {
         fputs("Error!\n", stderr);
         break;
      }

      assert(fread(&len, 4, 1, fp) > 0);

      //      unsigned int act2 = len & (1 << 7);
      //      len &= ~(1 << 7);
      //      if (act1 == 0) {
      //         if (act2 == 0) {
      //            action = SEEK;
      //         } else {
      //            action = SUBSTITUTE;
      //         }
      //      } else {
      //         if (act2 == 0) {
      //            action = INSERT;
      //         } else {
      //            action = DELETE;
      //         }
      //      }
      switch ((Action)action) {
         case SUBSTITUTE:
         case ADD:
            for (unsigned long j = 0; j < len; j++) {
               assert(fread(&new_num, 1, 1, fp) >= 0);
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
            for (unsigned long j = 0; j < len; j++) {
               assert(fread(&new_num, 1, 1, fp) >= 0);
               putchar(new_num);
            }
            break;
         case DELETE:
            i += len;
            break;
         default:
            fprintf(stderr, "Error: %d on Commond No. %ld\n", action, cnt);
            exit(-1);
      }
   }
   // fprintf(stderr, "cnt: %ld\n", cnt);
   */
   fclose(fp);
   return 0;
}
