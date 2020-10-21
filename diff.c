#include "common.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// http://karel.tsuda.ac.jp/lec/EditDistance/
#define i_whole (i + wind_cnt * (WINDOW_SIZE - 1))

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

int main(int argc, const char **argv) {
   if (argc <= 2) {
      fprintf(stderr, "Usage: oldfile newfile\n");
      exit(-1);
   }
   const int ORIG_MAX = 1 << 22;
   const int NEW_MAX = 1 << 22;
   char *orig = calloc(ORIG_MAX, 1);
   char *new = calloc(NEW_MAX, 1);

   FILE *fp;
   fp = fopen(argv[1], "r");
   if (fp == NULL) {
      exit(-1);
   }
   // 「0文字目」が存在しない
   const size_t orignum = fread(orig, 1, ORIG_MAX - 1, fp) + 1;
   char *origptr = orig;
   fclose(fp);

   fp = fopen(argv[2], "r");
   if (fp == NULL) {
      exit(-1);
   }
   // 「0文字目」が存在しない, \0 をスルー
   const size_t newnum = fread(new, 1, NEW_MAX - 1, fp) + 1;
   char *newptr = new;
   fclose(fp);

   // +1 は aaa とあったとき, |aaa, a|aa, aa|a とかするように
   const size_t WINDOW_SIZE = (1 << 10) + 1;
   // ここを-1 しないと WINDOW は文字数+1 でできていることに矛盾
   const size_t WINDOW_CHARS = WINDOW_SIZE - 1;
   int *table[WINDOW_SIZE];
   Action *act[WINDOW_SIZE];
   for (size_t i = 0; i < WINDOW_SIZE; i++) {
      // ここを char にして act 側に侵食していた...
      table[i] = calloc(WINDOW_SIZE * sizeof(int), 1);
      act[i] = calloc(WINDOW_SIZE * sizeof(Action), 1);
   }
   for (int wind_cnt = 0; wind_cnt * WINDOW_CHARS < max(orignum, newnum);
        wind_cnt++) {
      Action buffer_act[WINDOW_SIZE * 2 + 10];
      char buffer_char[WINDOW_SIZE * 2 + 10][WINDOW_SIZE];
      size_t buffer_char_len[WINDOW_SIZE * 2 + 10];
      size_t buffer_index[WINDOW_SIZE * 2 + 10];
      char buffer[WINDOW_SIZE * 2 + 10][20];
      size_t buf_index = 0;
      // 最初の位置での計算
      table[0][0] = 0;
      act[0][0] = MATCH;
      for (size_t i = 1; i < WINDOW_SIZE; i++) {
         table[i][0] = i + table[0][0];
         act[i][0] = DELETE;
      }
      for (size_t j = 1; j < WINDOW_SIZE; j++) {
         table[0][j] = j + table[0][0];
         act[0][j] = INSERT;
      }

      size_t i_max =
          min(WINDOW_SIZE, max(orignum - wind_cnt * WINDOW_CHARS, 1));
      size_t j_max = min(WINDOW_SIZE, max(newnum - wind_cnt * WINDOW_CHARS, 1));

      for (size_t i = 1; i < i_max; i++) {
         for (size_t j = 1; j < j_max; j++) {
            int costs[4] = {INF, INF, INF, INF};
            costs[0] = table[i - 1][j] + 1;     // DELETE
            costs[1] = table[i][j - 1] + 1;     // INSERT
            costs[2] = table[i - 1][j - 1] + 1; // SUBSTITUTE
            if (origptr[i - 1] == newptr[j - 1]) {
               costs[3] = table[i - 1][j - 1]; // MATCH
            }

            int index = argmin(costs, 4);
            // |= ではいけない
            act[i][j] = 1 << index;
            table[i][j] = costs[index];
         }
      }
      long i = i_max - 1;
      long j = j_max - 1;
      int score = table[i][j];
      fprintf(stderr, "score: %d (%ld, %ld)\n", score, i, j);
      while (i >= 0 && j >= 0) {
         switch (act[i][j]) {
            case MATCH:
               i--;
               j--;
               break;
            case SUBSTITUTE:
               i--;
               j--;
               //  buffer_index[buf_index-1]-buffer_char_len[buf_index-1]
               //  ここが - なのは逆順だから
               if (buf_index == 0 || buffer_act[buf_index - 1] != SUBSTITUTE ||
                   buffer_index[buf_index - 1] -
                           buffer_char_len[buf_index - 1] !=
                       i_whole) {
                  buffer_act[buf_index] = SUBSTITUTE;
                  buffer_index[buf_index] = i_whole;
                  buffer_char[buf_index][0] = newptr[j];
                  buffer_char_len[buf_index] = 1;
                  buf_index++;
               } else {
                  buffer_char[buf_index - 1][buffer_char_len[buf_index - 1]++] =
                      newptr[j];
               }
               snprintf(buffer[buf_index], 20, "%lx,%d,%hhx", i_whole,
                        SUBSTITUTE, newptr[j]);
               score--;
               break;
            case INSERT:
               j--;
               if (buf_index == 0 || buffer_act[buf_index - 1] != INSERT ||
                   buffer_index[buf_index - 1] != i_whole) {
                  buffer_act[buf_index] = INSERT;
                  buffer_char[buf_index][0] = newptr[j];
                  buffer_char_len[buf_index] = 1;
                  buffer_index[buf_index] = i_whole;
                  buf_index++;
               } else {
                  buffer_char[buf_index - 1][buffer_char_len[buf_index - 1]++] =
                      newptr[j];
               }
               snprintf(buffer[buf_index], 20, "%lx,%d,%hhx", i_whole, INSERT,
                        newptr[j]);
               score--;
               break;
            case DELETE:
               i--;
               if (buf_index == 0 || buffer_act[buf_index - 1] != DELETE ||
                   buffer_index[buf_index - 1] -
                           buffer_char_len[buf_index - 1] !=
                       i_whole) {
                  buffer_act[buf_index] = DELETE;
                  buffer_index[buf_index] = i_whole;
                  buffer_char_len[buf_index] = 1;
                  snprintf(buffer[buf_index], 20, "%lx,%d,1", i_whole, DELETE);
                  buf_index++;
               } else {
                  buffer_char_len[buf_index - 1]++;
               }
               score--;
               break;
            default:
               fprintf(stderr, "Unknown: %d at %ld %ld\n", act[i][j], i, j);
               exit(-1);
         }
      }
      assert(score == 0);

      // 逆順で入力されるのでその逆順で出力
      for (long k = buf_index - 1; k >= 0; k--) {
         switch (buffer_act[k]) {
            case INSERT:
               printf("%lx,%d", buffer_index[k], buffer_act[k]);
               for (long l = buffer_char_len[k] - 1; l >= 0; l--) {
                  printf(",%hhx", buffer_char[k][l]);
               }
               printf("\n");
               break;
            case SUBSTITUTE:
               printf("%lx,%d", buffer_index[k] + 1 - buffer_char_len[k],
                      buffer_act[k]);
               for (long l = buffer_char_len[k] - 1; l >= 0; l--) {
                  printf(",%hhx", buffer_char[k][l]);
               }
               printf("\n");
               break;
            case DELETE:
               printf("%lx,%d,%ld\n", buffer_index[k] + 1 - buffer_char_len[k],
                      buffer_act[k], buffer_char_len[k]);
               break;
            default:
               assert(1);
               break;
         }
         // puts(buffer[k]);
      }

      origptr += WINDOW_CHARS;
      newptr += WINDOW_CHARS;
   }

   free(orig);
   free(new);
   return 0;
}
