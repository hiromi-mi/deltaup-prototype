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
   char new_num;
   size_t i = 0;
   long long len;
   long long insertlen;
   size_t i_delta = 0;
   size_t prev_i = 0;

   while (!feof(fp)) {
      // SEEK
      if (fread(&i_delta, sizeof(i_delta), 1, fp) == 0) {
         break;
      }
      prev_i += i_delta;
      i = prev_i;

      // ADD
      assert(fread(&len, sizeof(len), 1, fp) > 0);
      // INSERT
      if (fread(&insertlen, sizeof(insertlen), 1, fp) == 0) {
         break;
      }
      assert(len >= 0);
      for (long long j = 0; j < len; j++) {
         assert(fread(&new_num, 1, 1, fp) > 0);
         putchar(new_num + orig[i]);
         i++;
      }

      for (long long j = 0; j < insertlen; j++) {
         assert(fread(&new_num, 1, 1, fp) > 0);
         putchar(new_num);
      }
   }
   fclose(fp);
   return 0;
}
