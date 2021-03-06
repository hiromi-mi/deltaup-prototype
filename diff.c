#include "common.h"
#include <assert.h>
#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <divsufsort.h>
#include <divsufsort64.h>

// http://karel.tsuda.ac.jp/lec/EditDistance/
#define i_whole (i + wind_cnt * (WINDOW_SIZE - 1))

#define INTEGRATE_MAX (255 - 1)

int diff(const char *orig, const size_t orignum, const char *new,
         const size_t newnum);

// from bsdiff
// License: LICENSE.bsdiff
size_t matchlen(const char *orig, size_t origlen, const char *new,
                size_t newlen) {
   size_t i = 0;
   for (; (i < origlen) && (i < newlen); i++) {
      if (orig[i] != new[i])
         break;
   }
   return i;
}

// from bsdiff
// License: LICENSE.bsdiff
size_t search(const int64_t *SA, const char *orig, size_t origlen,
              const char *new, size_t newlen, int64_t start, int64_t end,
              size_t *pos) {
   if (end - start < 2) {
      size_t s = matchlen(orig + SA[start], origlen - SA[start], new, newlen);
      size_t e = matchlen(orig + SA[end], origlen - SA[end], new, newlen);
      if (s > e) {
         *pos = SA[start];
         return s;
      } else {
         *pos = SA[end];
         return e;
      }
   }
   size_t x = start + (end - start) / 2;
   if (memcmp(orig + SA[x], new, min(origlen - SA[x], newlen)) < 0) {
      return search(SA, orig, origlen, new, newlen, x, end, pos);
   } else {
      return search(SA, orig, origlen, new, newlen, start, x, pos);
   }
}

int diff_elf(const char *orig, const size_t orignum, const char *new,
             const size_t newnum) {
   Elf64_Ehdr orig_eh, new_eh;
   memcpy(&orig_eh, orig, sizeof(orig_eh));
   memcpy(&new_eh, new, sizeof(new_eh));
   Elf64_Shdr orig_sh, new_sh;

   /*
   for (int i = 0; i < min(orig_eh->e_shnum, new_eh->e_shnum); i++) {
      memcpy(&orig_sh, &orig[i * orig_eh->e_shentsize + orig_eh->e_shoff],
             sizeof(orig_sh));
      memcpy(&new_sh, &new[i * new_eh->e_shentsize + new_eh->e_shoff],
             sizeof(new_sh));

      // strncmp(orig[orig_sh->sh_name]
   }
   */

   return diff(orig, orignum, new, newnum);
}

// algorithm from bsdiff
// License: LICENSE.bsdiff
int diff(const char *orig, const size_t orignum, const char *new,
         const size_t newnum) {
   int64_t *SA = malloc(sizeof(saidx_t) * (1LL << 23));
   assert(divsufsort64((unsigned char *)orig, SA, orignum) == 0);
   // assert(sufcheck64((unsigned char *)orig, SA, orignum, 0) == 0);

   size_t pos;
   uint64_t len = 0, scan = 0, lastscan = 0, lastpos = 0, lastoffset = 0;
   uint64_t scsc;
   int64_t lenf = 0;
   int64_t lenb = 0;

   uint64_t prev_lastpos = 0;
   while (scan < newnum) {
      uint64_t oldscore = 0;
      // scan sucessor
      scan += len;
      for (scsc = scan; scan < newnum; scan++) {
         // ここが new+scan, newnum-scan になってた
         len = search(SA, orig, orignum, new + scan, newnum - scan, 0, orignum,
                      &pos);

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

      if ((len != oldscore) || (scan == newnum)) {
         int64_t s = 0;
         int64_t Sf = 0;
         lenf = 0;
         // 前回の一致点から前方探索
         // max. (一致した文字数*2 - 全体の文字数) を計算
         for (size_t i = 0; (lastscan + i < scan) && (lastpos + i < orignum) &&
                            (lastscan + i < newnum);) {
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
         lenb = 0;
         if (scan < newnum) {
            int64_t s = 0;
            int64_t Sb = 0;
            for (uint64_t i = 1; (scan >= lastscan + i) && (pos >= i); i++) {
               if (orig[pos - i] == new[scan - i]) {
                  s++;
               }
               if (s * 2 - i > Sb * 2 - lenb) {
                  Sb = s;
                  lenb = i;
               }
            }
         }

         if (lastscan + lenf > scan - lenb) {
            size_t overlap = (lastscan + lenf) - (scan - lenb);
            int64_t s = 0;
            int64_t lens = 0, Ss = 0;
            for (size_t i = 0; i < overlap; i++) {
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
         int64_t lastpos_delta = lastpos - (int64_t)prev_lastpos;
         fwrite(&lastpos_delta, sizeof(lastpos_delta), 1, stdout);

         fwrite(&lenf, sizeof(lenf), 1, stdout);

         int64_t insert_len = 0;
         insert_len = (scan - lenb) - (lastscan + lenf);
         // insert_len が newnum を超過しているときに newnumに合わせる
         if (lastscan + lenf + insert_len > newnum) {
            insert_len = min(newnum - (lastscan + lenf), 0);
         }
         fwrite(&insert_len, sizeof(insert_len), 1, stdout);

         // ADD
         for (int64_t j = 0; j < lenf; j++) {
            char tmpchar = new[lastscan + j] - orig[lastpos + j];
            fwrite(&tmpchar, 1, 1, stdout);
         }

         // INSERT
         if (insert_len >= 0) {
            fwrite(&new[lastscan + lenf], 1, insert_len, stdout);
         }

         lastscan = scan - lenb;
         prev_lastpos = lastpos;
         lastpos = pos - lenb;
         lastoffset = pos - scan;
      }
   }

   free(SA);
   return 0;
}

int main(int argc, const char **argv) {
   if (argc <= 2) {
      fprintf(stderr, "Usage: oldfile newfile\n");
      exit(-1);
   }

   size_t orignum, newnum;
   char *orig = read_file(argv[1], &orignum);
   char *new = read_file(argv[2], &newnum);

   if (orignum >= 4 && orig[0] == 0x7f && orig[1] == 'E' && orig[2] == 'L' &&
       orig[3] == 'F') {
      diff_elf(orig, orignum, new, newnum);
   } else {
      diff(orig, orignum, new, newnum);
   }
   free(orig);
   free(new);
   return 0;
}
