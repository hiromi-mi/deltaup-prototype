#include "common.h"
#include <assert.h>
#include <errno.h>
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
   for (int wind_cnt = 0; wind_cnt * WINDOW_CHARS < max(orignum, newnum);
        wind_cnt++) {
      Action buffer_act[WINDOW_SIZE * 2 + 10];
      char buffer_char[WINDOW_SIZE * 2 + 10][WINDOW_SIZE];
      unsigned char buffer_char_len[WINDOW_SIZE * 2 + 10];
      size_t buffer_index[WINDOW_SIZE * 2 + 10];
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
      fprintf(stderr, "score: %d (%ld, %ld)\n", score, i_max, j_max);
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
                       i_whole ||
                   buffer_char_len[buf_index] > 254) {
                  buffer_act[buf_index] = SUBSTITUTE;
                  buffer_index[buf_index] = i_whole;
                  buffer_char[buf_index][0] = newptr[j];
                  buffer_char_len[buf_index] = 1;
                  buf_index++;
               } else {
                  buffer_char[buf_index - 1][buffer_char_len[buf_index - 1]++] =
                      newptr[j];
               }
               score--;
               break;
            case INSERT:
               j--;
               if (buf_index == 0 || buffer_act[buf_index - 1] != INSERT ||
                   buffer_index[buf_index - 1] != i_whole ||
                   buffer_char_len[buf_index] > 254) {
                  buffer_act[buf_index] = INSERT;
                  buffer_char[buf_index][0] = newptr[j];
                  buffer_char_len[buf_index] = 1;
                  buffer_index[buf_index] = i_whole;
                  buf_index++;
               } else {
                  buffer_char[buf_index - 1][buffer_char_len[buf_index - 1]++] =
                      newptr[j];
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

      long tmp;
      // 逆順で入力されるのでその逆順で出力
      for (long k = buf_index - 1; k >= 0; k--) {
         switch (buffer_act[k]) {
            case INSERT:
               fwrite(&buffer_index[k], sizeof(buffer_index[k]), 1, stdout);
               fwrite(&buffer_act[k], 1, 1, stdout);
               fwrite(&buffer_char_len[k], 1, 1, stdout);
               // 逆順で出力
               assert(buffer_char_len[k] > 0);
               for (long l = buffer_char_len[k] - 1; l >= 0; l--) {
                  fwrite(&buffer_char[k][l], 1, 1, stdout);
               }
               /*
               printf("%lx,%d", buffer_index[k], buffer_act[k]);
               for (long l = buffer_char_len[k] - 1; l >= 0; l--) {
                  printf(",%hhx", buffer_char[k][l]);
               }
               printf("\n");
               */
               break;
            case SUBSTITUTE:
               assert(buffer_char_len[k] > 0);
               tmp = buffer_index[k] + 1 - buffer_char_len[k];
               fwrite(&tmp, sizeof(buffer_index[k]), 1, stdout);
               fwrite(&buffer_act[k], 1, 1, stdout);
               fwrite(&buffer_char_len[k], 1, 1, stdout);
               for (long l = buffer_char_len[k] - 1; l >= 0; l--) {
                  fwrite(&buffer_char[k][l], 1, 1, stdout);
               }
               // これは使えない
               // fwrite(buffer_char[k], 1, buffer_char_len[k], stdout);
               /*j
               printf("%lx,%d", buffer_index[k] + 1 - buffer_char_len[k],
                      buffer_act[k]);
               for (long l = buffer_char_len[k] - 1; l >= 0; l--) {
                  printf(",%hhx", buffer_char[k][l]);
               }
               printf("\n");
               */
               break;
            case DELETE:
               tmp = buffer_index[k] + 1 - buffer_char_len[k];
               fwrite(&tmp, sizeof(buffer_index[k]), 1, stdout);
               fwrite(&buffer_act[k], 1, 1, stdout);
               fwrite(&buffer_char_len[k], 1, 1, stdout);
               /*
               printf("%lx,%d,%ld\n", buffer_index[k] + 1 - buffer_char_len[k],
                      buffer_act[k], buffer_char_len[k]);
                      */
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
