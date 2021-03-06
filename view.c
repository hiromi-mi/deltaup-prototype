#include "common.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, const char **argv) {
   if (argc <= 1) {
      fprintf(stderr, "Usage: difffile\n");
      exit(-1);
   }
   FILE *fp = fopen(argv[1], "rb");
   if (fp == NULL) {
      perror("fopen");
      exit(-1);
   }

   size_t cnt;
   size_t index = 0;
   size_t prev_index = 0;
   char new_num;
   unsigned int len;
   unsigned char index_delta;
   Action action;

   for (cnt = 0; !feof(fp); cnt++) {
      prev_index = index;

      if (fread(&action, sizeof(Action), 1, fp) == 0)
         break;
      if ((Action)action == SEEK) {
         fread(&index, sizeof(index), 1, fp);
         int tmp;
         fread(&tmp, 4, 1, fp);
         printf("SEEK, %ld, %d\n", index, tmp);
         continue;
      }
      assert(fread(&index_delta, sizeof(index_delta), 1, fp) > 0);
      index = index_delta + prev_index;
      assert(fread(&len, 4, 1, fp) > 0);

      switch ((Action)action) {
         case SUBSTITUTE:
            printf("SUBSTITUTE, %ld, %u: ", index, len);
            for (unsigned long j = 0; j < len; j++) {
               assert(fread(&new_num, 1, 1, fp) > 0);
               printf("%hhu ", new_num);
            }
            putchar('\n');
            break;
         case ADD:
            printf("ADD, %ld, %u: ", index, len);
            for (unsigned long j = 0; j < len; j++) {
               assert(fread(&new_num, 1, 1, fp) > 0);
               // printf("%d ", new_num + orig[i] - '0');
               printf("%hhu ", new_num);
            }
            putchar('\n');
            break;
         case INSERT:
            printf("INSERT, %ld, %u: ", index, len);
            for (unsigned long j = 0; j < len; j++) {
               assert(fread(&new_num, 1, 1, fp) > 0);
               printf("%hhu ", new_num);
            }
            putchar('\n');
            break;
         case DELETE:
            printf("DELETE, %ld, %u: \n", index, len);
            break;
         default:
            fprintf(stderr, "Error: %d on Commond No. %ld\n", action, cnt);
            exit(-1);
      }
   }

   fclose(fp);
}
