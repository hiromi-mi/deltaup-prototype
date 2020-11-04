#include "common.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int argmin(int *costs, size_t n) {
   assert(n > 0);
   int minindex = 0;
   int minval = costs[0];
   for (size_t index = 1; index < n; index++) {
      if (minval > costs[index]) {
         minindex = index;
         minval = costs[index];
      }
   }
   return minindex;
}

char *read_file(const char *filename, size_t *len) {
   const int MAX_LEN = 1 << 22;
   char *file = calloc(MAX_LEN, 1);
   FILE *fp;

   if (file == NULL) {
      perror("calloc");
      exit(-1);
   }

   fp = fopen(filename, "r");
   if (fp == NULL) {
      perror("fopen");
      exit(-1);
   }
   *len = fread(file, 1, MAX_LEN - 1, fp);
   fclose(fp);

   return file;
}
