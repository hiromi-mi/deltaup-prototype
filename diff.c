#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "common.h"

// http://karel.tsuda.ac.jp/lec/EditDistance/

int argmin(int* costs, size_t n) {
   assert(n > 0);
   int minindex = 0;
   int minval = costs[0];
   for (size_t index=1;index<n;index++) {
      if (minval > costs[index]) {
         minindex = index;
         minval = costs[index];
      }
   }
   return minindex;
}

int main(int argc, const char** argv) {
   if (argc <= 2) {
      fprintf(stderr, "Usage: oldfile newfile\n");
      exit(-1);
   }
   const int ORIG_MAX = 1 << 22;
   const int NEW_MAX = 1 << 22;
   char* orig = calloc(ORIG_MAX, 1);
   char* new = calloc(NEW_MAX, 1);

   FILE *fp;
   fp = fopen(argv[1], "r");
   if (fp == NULL) {
      exit(-1);
   }
   // 「0文字目」が存在しない
   const size_t orignum = fread(orig, 1, ORIG_MAX-1, fp) + 1;
   char* origptr = orig;
   fclose(fp);

   fp = fopen(argv[2], "r");
   if (fp == NULL) {
      exit(-1);
   }
   // 「0文字目」が存在しない, \0 をスルー
   const size_t newnum = fread(new, 1, NEW_MAX-1, fp) + 1;
   char* newptr = new;
   fclose(fp);

   const size_t WINDOW_SIZE = 1 << 10;
   int* table[WINDOW_SIZE];
   Action* act[WINDOW_SIZE];
   for (size_t i=0;i<WINDOW_SIZE;i++) {
      // ここを char にして act 側に侵食していた...
      table[i] = calloc(WINDOW_SIZE * sizeof(int), 1);
      act[i]   = calloc(WINDOW_SIZE * sizeof(Action), 1);
   }
   for (int wind_cnt=1;(wind_cnt-1)*WINDOW_SIZE< max(orignum, newnum);wind_cnt++) {
      // 最初の位置での計算
      table[0][0] = 0;
      act[0][0] = MATCH;
      for (size_t i=1;i<WINDOW_SIZE;i++) {
         table[i][0] = i + table[0][0];
         act[i][0] = DELETE;
      }
      for (size_t j=1;j<WINDOW_SIZE;j++) {
         table[0][j] = j + table[0][0];
         act[0][j] = INSERT;
      }
      for (size_t i=1;i<min(WINDOW_SIZE, orignum - (wind_cnt-1)*WINDOW_SIZE);i++) {
         for (size_t j=1;j<min(WINDOW_SIZE, newnum - (wind_cnt-1)*WINDOW_SIZE);j++) {
            int costs[4] = {1000000, 1000000, 1000000, 1000000};
            costs[0] = table[i-1][j]+1; // DELETE
            costs[1] = table[i][j-1]+1; // INSERT
            costs[2] = table[i-1][j-1]+1; // SUBSTITUTE
            if (origptr[i-1] == newptr[j-1]) {
               costs[3] = table[i-1][j-1]; // MATCH
            }

            int index = argmin(costs, 4);
            // |= ではいけない
            act[i][j] = 1 << index;
            table[i][j] = costs[index];
         }
      }
      long i = min(orignum - (wind_cnt-1)*WINDOW_SIZE, WINDOW_SIZE)-1;
      long j = min(newnum - (wind_cnt-1)*WINDOW_SIZE, WINDOW_SIZE)-1;
      int score = table[i][j];
      fprintf(stderr, "score: %d (%ld, %ld)\n", score, i, j);
      while (i >= 0 && j >= 0) {
         switch(act[i][j]) {
            case MATCH:
               i--;
               j--;
               break;
            case SUBSTITUTE:
               i--;
               j--;
               printf("%ld,%d,%c\n", i, SUBSTITUTE, new[j]);
               score--;
               break;
            case INSERT:
               j--;
               printf("%ld,%d,%c\n", i, INSERT, new[j]);
               score--;
               break;
            case DELETE:
               i--;
               printf("%ld,%d,%c\n", i, DELETE, orig[i]);
               score--;
               break;
            default:
               fprintf(stderr, "Unknown: %d at %ld %ld\n", act[i][j], i, j);
               exit(-1);
         }
      }
      assert(score == 0);
      origptr += WINDOW_SIZE;
      newptr += WINDOW_SIZE;
   }

   free(orig);
   free(new);
   return 0;
}
