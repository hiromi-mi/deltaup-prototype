#include "common.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <divsufsort.h>

// http://karel.tsuda.ac.jp/lec/EditDistance/
#define i_whole (i + wind_cnt * (WINDOW_SIZE - 1))

#define INTEGRATE_MAX (255 - 1)

// from bsdiff
// License: LICENSE.bsdiff
int matchlen(const char *orig, size_t origlen, const char *new, size_t newlen) {
   size_t i = 0;
   for (; i < origlen && i < newlen; i++) {
      if (orig[i] != new[i])
         break;
   }
   return i;
}

// from bsdiff
// License: LICENSE.bsdiff
long long search(const int *SA, const char *orig, size_t origlen,
                 const char *new, size_t newlen, int start, int end, int *pos) {
   if (end - start < 2) {
      int s = matchlen(orig + SA[start], origlen - SA[start], new, newlen);
      int e = matchlen(orig + SA[end], origlen - SA[end], new, newlen);
      if (s > e) {
         *pos = SA[start];
         return s;
      } else {
         *pos = SA[end];
         return e;
      }
   }
   int x = start + (end - start) / 2;
   if (memcmp(orig + SA[x], new, min(origlen - SA[x], newlen)) < 0) {
      return search(SA, orig, origlen, new, newlen, x, end, pos);
   } else {
      return search(SA, orig, origlen, new, newlen, start, x, pos);
   }
}

// algorithm from bsdiff
// License: LICENSE.bsdiff
int main(int argc, const char **argv) {
   if (argc <= 2) {
      fprintf(stderr, "Usage: oldfile newfile\n");
      exit(-1);
   }

   size_t orignum, newnum;
   char *orig = read_file(argv[1], &orignum);
   char *new = read_file(argv[2], &newnum);

   int *SA = malloc(sizeof(saidx_t) * (1LL << 23));
   divsufsort((unsigned char *)orig, SA, orignum);
   int pos;
   int len = 0, scan = 0, lastscan = 0, lastpos = 0, lastoffset = 0;
   int oldscore;
   int scsc;
   while (scan < newnum) {
      oldscore = 0;
      // scan sucessor
      scsc = scan += len;
      for (; scan < newnum; scan++) {
         len = search(SA, orig, orignum, new, newnum, 0, orignum, &pos);

         // 今回一致範囲 [scan ... scan+len]
         // と前回一致範囲の続きの一致バイト数を徐々に増やしていくl
         // 今回一致範囲から
         for (; scsc < scan + len; scsc++) {
            if ((scsc + lastoffset < orignum) &&
                (orig[scsc + lastoffset] == new[scsc])) {
               // 一致して延長できていたら
               oldscore++;
            }
         }

         // 前回一致したものを新旧完全に一致しながら延長できている
         if ((len == oldscore) && (len != 0)) {
            break;
         }

         // 8文字以上の不一致
         if (len > oldscore + 8) {
            break;
         }

         // scan を進めていく. 今回一致部分を先に進めていく
         if ((scan + lastoffset < orignum) &&
             (orig[scan + lastoffset] == new[scan])) {
            oldscore--;
         }
      }

      int s = 0, Sf = 0, lenf = 0;
      int lenb = 0, Sb = 0;
      if ((len != oldscore) || (scan == newnum)) {
         // 前回の一致点から前方探索
         // max. (一致した文字数*2 - 全体の文字数) を計算
         for (int i = 0; (lastscan + i < scan) && (lastpos + i < orignum);) {
            if (orig[lastpos + i] == new[lastscan + i]) {
               s++;
            }
            i++;
            if (s * 2 - i > Sf * 2 - lenf) {
               Sf = s;
               lenf = i;
            }
         }
         // 今回の一致点からのbackward
         // max. (一致した文字数*2 - 全体の文字数) を計算
         if (scan < newnum) {
            s = 0;
            Sb = 0;
            for (int i = 1; (scan >= lastscan + i) && (pos >= i); i++) {
               if (orig[pos - i] == new[scan - i]) {
                  s++;
               }
               if (s * 2 - i > Sb * 2 - lenb) {
                  Sb = s;
                  lenb = i;
               }
            }
         }
      }

      if (lastscan + lenf > scan - lenb) {
         int overlap = (lastscan + lenf) - (scan - lenb);
         int s = 0;
         int lens = 0, Ss = 0;
         for (int i = 0; i < overlap; i++) {
            if (new[lastscan + lenf - overlap + i] ==
                orig[lastpos + lenf - overlap + i]) {
               s++;
            }
            if (new[scan - lenb + i] == orig[pos - lenb + i]) {
               s--;
            }
            if (s > Ss) {
               Ss = s;
               lens = i + 1;
            }
         }

         lenf += lens - overlap;
         lenb -= lens;
      }
      lastscan = scan - lenb;
      lastpos = pos - lenb;
      lastoffset = pos - scan;
   }

   // 「0文字目」が存在しないため
   orignum++;
   newnum++;
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
      char buffer_chars_add[WINDOW_SIZE * 2 + 10][WINDOW_SIZE];
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
            costs[DELETE] =
                table[i - 1][j] + 1 + (act[i - 1][j] != DELETE) * 2; // DELETE
            costs[INSERT] =
                table[i][j - 1] + 1 + (act[i][j - 1] != INSERT) * 2; // INSERT
            costs[SUBSTITUTE] =
                table[i - 1][j - 1] + 3 +
                (act[i - 1][j - 1] != SUBSTITUTE) * 2; // SUBSTITUTE
            if (origptr[i - 1] == newptr[j - 1]) {
               costs[MATCH] = table[i - 1][j - 1] +
                              (act[i - 1][j - 1] != MATCH) * 2; // MATCH
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
                  buffer_chars_add[buf_index][0] = newptr[j] - origptr[i];
                  buffer_char_len[buf_index] = 1;
                  buf_index++;
               } else {
                  buffer_chars_add[buf_index - 1]
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

      // due to movable points
      // assert(score == 0);

      // reverse  buffer_index and buffer_chars
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
            case INSERT:
               for (int i = 0; i < buffer_char_len[k]; i++) {
                  temp[i] = buffer_chars[k][i];
               }
               for (int i = 0; i < buffer_char_len[k]; i++) {
                  buffer_chars[k][buffer_char_len[k] - i - 1] = temp[i];
               }
               break;
            case ADD:
               for (int i = 0; i < buffer_char_len[k]; i++) {
                  temp[i] = buffer_chars_add[k][i];
               }
               for (int i = 0; i < buffer_char_len[k]; i++) {
                  buffer_chars_add[k][buffer_char_len[k] - i - 1] = temp[i];
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

      // creating delta
      buffer_index[buf_index] = prev_last_index;
      for (long k = buf_index - 1; k >= 0; k--) {
         if (buffer_act[k] == SKIP) {
            continue;
         }
         buffer_index_delta[k] = buffer_index[k] - buffer_index[k + 1];
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
            case SUBSTITUTE:
               fwrite(buffer_chars[k], 1, buffer_char_len[k], stdout);
               break;
            case ADD:
               fwrite(buffer_chars_add[k], 1, buffer_char_len[k], stdout);
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
   free(SA);
   return 0;
}
