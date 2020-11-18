#include "common.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <divsufsort.h>
#include <divsufsort64.h>

// http://karel.tsuda.ac.jp/lec/EditDistance/
#define i_whole (i + wind_cnt * (WINDOW_SIZE - 1))

#define INTEGRATE_MAX (255 - 1)

// from bsdiff
// License: LICENSE.bsdiff
long long matchlen(const char *orig, size_t origlen, const char *new,
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
long long search(const long *SA, const char *orig, size_t origlen,
                 const char *new, size_t newlen, long long start, long long end,
                 long long *pos) {
   if (end - start < 2) {
      long long s =
          matchlen(orig + SA[start], origlen - SA[start], new, newlen);
      long long e = matchlen(orig + SA[end], origlen - SA[end], new, newlen);
      if (s > e) {
         *pos = SA[start];
         return s;
      } else {
         *pos = SA[end];
         return e;
      }
   }
   long long x = start + (end - start) / 2;
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

   long *SA = malloc(sizeof(saidx_t) * (1LL << 23));
   assert(divsufsort64((unsigned char *)orig, SA, orignum) == 0);
   // assert(sufcheck64((unsigned char *)orig, SA, orignum, 0) == 0);

   long long pos;
   unsigned long long len = 0, scan = 0, lastscan = 0, lastpos = 0,
                      lastoffset = 0;
   unsigned long long scsc;
   long long s = 0;
   long long Sf = 0;
   long long lenf = 0;
   long long lenb = 0, Sb = 0;

   unsigned long long prev_lastpos = 0;
   while (scan < newnum) {
      unsigned long long oldscore = 0;
      // scan sucessor
      for (scsc = scan += len; scan < newnum; scan++) {
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
         lenf = 0;
         s = 0;
         Sf = 0;
         // 前回の一致点から前方探索
         // max. (一致した文字数*2 - 全体の文字数) を計算
         for (long long i = 0; (lastscan + i < scan) &&
                               (lastpos + i < orignum) &&
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
            s = 0;
            Sb = 0;
            for (long long i = 1; (scan >= lastscan + i) && (pos >= i); i++) {
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
            long long overlap = (lastscan + lenf) - (scan - lenb);
            long long s = 0;
            long long lens = 0, Ss = 0;
            for (long long i = 0; i < overlap; i++) {
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
         long long tmp2 = lastpos - (long long)prev_lastpos;
         fwrite(&tmp2, sizeof(tmp2), 1, stdout);

         long long tmp = 0;
         fwrite(&lenf, sizeof(lenf), 1, stdout);

         tmp = (scan - lenb) - (lastscan + lenf);
         // fprintf(stderr, "%d, %d, %d, %d, %d\n", scan, newnum, tmp, lenf,
         // lastscan);
         // newnum を超過しているときに newnumに合わせる
         if (lastscan + lenf + tmp > newnum) {
            tmp = min(newnum - (lastscan + lenf), 0);
         }
         fwrite(&tmp, sizeof(tmp), 1, stdout);

         for (long long j = 0; j < lenf; j++) {
            char tmpchar = new[lastscan + j] - orig[lastpos + j];
            fwrite(&tmpchar, 1, 1, stdout);
         }
         if (tmp >= 0) {
            fwrite(&new[lastscan + lenf], 1, tmp, stdout);
         }

         lastscan = scan - lenb;
         prev_lastpos = lastpos;
         lastpos = pos - lenb;
         lastoffset = pos - scan;
      }
   }

   free(orig);
   free(new);
   free(SA);
   return 0;
}
