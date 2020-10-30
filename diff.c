#include "common.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// http://karel.tsuda.ac.jp/lec/EditDistance/
#define i_whole (i + wind_cnt * (WINDOW_SIZE - 1))

#define INTEGRATE_MAX (255 - 1)

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
   *len = fread(file, 1, MAX_LEN - 1, fp) + 1;
   // 「0文字目」が存在しないため
   fclose(fp);

   return file;
}

int main(int argc, const char **argv) {
   if (argc <= 2) {
      fprintf(stderr, "Usage: oldfile newfile\n");
      exit(-1);
   }

   size_t orignum, newnum;
   char *orig = read_file(argv[1], &orignum);
   char *new = read_file(argv[2], &newnum);
   char *origptr = orig;
   char *newptr = new;

   // +1 は aaa とあったとき, |aaa, a|aa, aa|a とかするように
   // ここが 256 を上回ると buffer_index_delta を unsigned char
   // ではない型にしないといけない
   const size_t WINDOW_SIZE = (1 << 7) + 1;
   // ここを-1 しないと WINDOW は文字数+1 でできていることに矛盾
   const size_t WINDOW_CHARS = WINDOW_SIZE - 1;
   int *table[WINDOW_SIZE];
   Action *act[WINDOW_SIZE];
   for (size_t i = 0; i < WINDOW_SIZE; i++) {
      // ここを char にして act 側に侵食していた...
      table[i] = calloc(WINDOW_SIZE * sizeof(int), 1);
      act[i] = calloc(WINDOW_SIZE * sizeof(Action), 1);
   }

   size_t prev_last_index = 0;
   for (int wind_cnt = 0; wind_cnt * WINDOW_CHARS < max(orignum, newnum);
        wind_cnt++) {
      Action buffer_act[WINDOW_SIZE * 2 + 10];
      char buffer_chars[WINDOW_SIZE * 2 + 10][WINDOW_SIZE];
      char buffer_char_add[WINDOW_SIZE * 2 + 10][WINDOW_SIZE];
      unsigned char buffer_char_len[WINDOW_SIZE * 2 + 10];
      size_t buffer_index[WINDOW_SIZE * 2 + 10];
      unsigned char buffer_index_delta[WINDOW_SIZE * 2 + 10];
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

      size_t i_max = min(WINDOW_SIZE,
                         max((long)orignum - wind_cnt * (long)WINDOW_CHARS, 1));
      size_t j_max = min(WINDOW_SIZE,
                         max((long)newnum - wind_cnt * (long)WINDOW_CHARS, 1));

      for (size_t i = 1; i < i_max; i++) {
         for (size_t j = 1; j < j_max; j++) {
            int costs[4] = {INF, INF, INF, INF};
            costs[DELETE] = table[i - 1][j] + 1;         // DELETE
            costs[INSERT] = table[i][j - 1] + 1;         // INSERT
            costs[SUBSTITUTE] = table[i - 1][j - 1] + 1; // SUBSTITUTE
            if (origptr[i - 1] == newptr[j - 1]) {
               costs[MATCH] = table[i - 1][j - 1]; // MATCH
            }

            int index = argmin(costs, 4);
            // |= ではいけない
            act[i][j] = index;
            table[i][j] = costs[index];
         }
      }
      long i = i_max - 1;
      long j = j_max - 1;
      int score = table[i][j];
      // fprintf(stderr, "score: %d (%ld, %ld)\n", score, i_max, j_max);
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
               //  TODO 必要に応じて SUBSTITUTE を ADD にする
               if (buf_index == 0 || buffer_act[buf_index - 1] != SUBSTITUTE ||
                   buffer_index[buf_index - 1] -
                           buffer_char_len[buf_index - 1] !=
                       i_whole ||
                   buffer_char_len[buf_index] > INTEGRATE_MAX) {
                  buffer_act[buf_index] = SUBSTITUTE;
                  buffer_index[buf_index] = i_whole;
                  buffer_chars[buf_index][0] = newptr[j];
                  buffer_char_add[buf_index][0] = newptr[j] - origptr[i];
                  buffer_char_len[buf_index] = 1;
                  buf_index++;
               } else {
                  buffer_char_add[buf_index - 1]
                                 [buffer_char_len[buf_index - 1]] =
                                     newptr[j] - origptr[i];
                  buffer_chars[buf_index - 1]
                              [buffer_char_len[buf_index - 1]++] = newptr[j];
               }
               score--;
               break;
            case INSERT:
               j--;
               if (buf_index == 0 || buffer_act[buf_index - 1] != INSERT ||
                   buffer_index[buf_index - 1] != i_whole ||
                   buffer_char_len[buf_index] > INTEGRATE_MAX) {
                  buffer_act[buf_index] = INSERT;
                  buffer_chars[buf_index][0] = newptr[j];
                  buffer_char_len[buf_index] = 1;
                  buffer_index[buf_index] = i_whole;
                  buf_index++;
               } else {
                  buffer_chars[buf_index - 1]
                              [buffer_char_len[buf_index - 1]++] = newptr[j];
               }
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

      char temp[WINDOW_SIZE + 1];
      for (long k = buf_index - 1; k >= 0; k--) {
         switch (buffer_act[k]) {
            case SUBSTITUTE:
            case ADD:
            case DELETE:
               buffer_index[k] = (buffer_index[k] + 1 - buffer_char_len[k]);
               break;
            default:
               break;
         }

         switch (buffer_act[k]) {
            case SUBSTITUTE:
            case ADD:
            case INSERT:
               for (int i = 0; i < buffer_char_len[k]; i++) {
                  temp[i] = buffer_chars[k][i];
               }
               for (int i = 0; i < buffer_char_len[k]; i++) {
                  buffer_chars[k][buffer_char_len[k] - i - 1] = temp[i];
               }
               break;
            default:
               break;
         }
      }

      //      for (long k = buf_index - 2; k >= 0; k--) {
      //         if (buffer_act[k] == SUBSTITUTE && buffer_act[k + 1] ==
      //         SUBSTITUTE) {
      //            int diff = buffer_index[k] -buffer_index[k + 1]  -
      //            buffer_char_len[k+1]; assert(diff >= 0); if (diff <= 20 &&
      //                buffer_char_len[k] + buffer_char_len[k + 1] + diff <
      //                255) {
      //               buffer_act[k + 1] = SUBSTITUTE;
      //               buffer_act[k] = SKIP;
      //               size_t temp = buffer_index[k] - buffer_index[k+1] +
      //               buffer_char_len[k]; size_t l; for
      //               (l=buffer_char_len[k+1]; l <
      //               buffer_index[k]-buffer_index[k+1]; l++) {
      //                  buffer_chars[k+1][l] =  orig[buffer_index[k+1]+l];
      //               }
      //               for (; l < temp; l++) {
      //                  buffer_chars[k + 1][l] =
      //                  buffer_chars[k][l+buffer_index[k+1]-buffer_index[k]];
      //               }
      //               buffer_char_len[k + 1] = temp;
      //               fputs(buffer_chars[k+1], stderr);
      //            }
      //         }
      //      }
      // しばらくぶりに使うとき
      // 番兵
      if (buf_index > 0 &&
          buffer_index[buf_index - 1] - prev_last_index > WINDOW_SIZE) {
         // SEEK で次に使いうる最初のindex に移動
         buffer_act[buf_index] = SEEK;
         buffer_index[buf_index] = buffer_index[buf_index - 1];
         buffer_index_delta[buf_index] = 0;
         buf_index++;
         prev_last_index = buffer_index[buf_index - 1];
      }
      buffer_index[buf_index] = prev_last_index;
      for (long k = buf_index - 1; k >= 0; k--) {
         if (buffer_act[k] == SKIP) {
            continue;
         }
         buffer_index_delta[k] = buffer_index[k] - buffer_index[k + 1];
         fprintf(stderr, "%ld: delta: %u index: %ld, act: %u len: %u\n", k,
                 buffer_index_delta[k], buffer_index[k], buffer_act[k],
                 buffer_char_len[k]);
      }
      if (buf_index > 0) {
         prev_last_index = buffer_index[0];
      }

      // 逆順で入力されるのでその逆順で出力
      for (long k = buf_index - 1; k >= 0; k--) {
         if (buffer_act[k] == SKIP)
            continue;

         fwrite(&buffer_act[k], sizeof(buffer_act[k]), 1, stdout);

         if (buffer_act[k] == SEEK) {
            assert(buffer_index_delta[k] == 0);
            fwrite(&buffer_index[k], sizeof(buffer_index[k]), 1, stdout);
            continue;
         }

         fwrite(&buffer_index_delta[k], sizeof(buffer_index_delta[k]), 1,
                stdout);
         fwrite(&buffer_char_len[k], sizeof(buffer_index_delta[k]), 1, stdout);

         assert(buffer_char_len[k] > 0);
         switch (buffer_act[k]) {
            case INSERT:
               fwrite(buffer_chars[k], 1, buffer_char_len[k], stdout);
               break;
            case SUBSTITUTE:
            case ADD:
               if (buffer_act[k] == SUBSTITUTE) {
                  fwrite(buffer_chars[k], 1, buffer_char_len[k], stdout);
               } else if (buffer_act[k] == ADD) {
                  fwrite(buffer_char_add[k], 1, buffer_char_len[k], stdout);
               }
               break;
            case DELETE:
               break;
            default:
               assert(1);
               break;
         }
      }

      origptr += WINDOW_CHARS;
      newptr += WINDOW_CHARS;
   }

   free(orig);
   free(new);
   return 0;
}
