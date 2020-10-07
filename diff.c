#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// http://karel.tsuda.ac.jp/lec/EditDistance/

typedef enum {
   NONE = 0,
   DELETE = 1 << 0,
   INSERT = 1 << 1,
   SUBSTITUTE = 1 << 2,
   MATCH = 1 << 3
} Action;

int min(int* costs) {
   int minindex = 0;
   int minval = 1000010;
   for (int index = 0;index < 4; index++) {
      if (minval >= costs[index]) {
         minval = costs[index];
         minindex = index;
      }
   }
   return minindex;
}
int main(int argc, const char** argv) {
   if (argc <= 2) {
      fprintf(stderr, "Usage: oldfile newfile\n");
      exit(-1);
   }
   const int ORIG_MAX = 4098*2;
   const int NEW_MAX = 4098*2;
   char orig[ORIG_MAX];
   char new[NEW_MAX];
   memset(orig,0,  ORIG_MAX);
   memset(new, 0, NEW_MAX);

   FILE *fp;
   fp = fopen(argv[1], "r");
   if (fp == NULL) {
      exit(-1);
   }
   const int orignum = fread(orig, sizeof(char), ORIG_MAX-1, fp) / sizeof(char) + 1;
   fclose(fp);

   fp = fopen(argv[2], "r");
   if (fp == NULL) {
      exit(-1);
   }
   const int newnum = fread(new, sizeof(char), NEW_MAX-1, fp) / sizeof(char) + 1;
   fclose(fp);

   int* table[orignum];
   Action* act[orignum];
   for (int i=0;i<orignum;i++) {
      // ここを char にして act 側に侵食していた...
      table[i] = malloc(newnum * sizeof(int));
      act[i]   = malloc(newnum * sizeof(Action));
   }
   table[0][0] = 0;
   act[0][0] = NONE;
   for (int i=1;i<orignum;i++) {
      table[i][0] = i;
      act[i][0] = DELETE;
   }
   for (int j=1;j<newnum;j++) {
      table[0][j] = j;
      act[0][j] = INSERT;
   }
   for (int i=1;i<orignum;i++) {
      for (int j=1;j<newnum;j++) {
         int costs[4] = {1000000, 1000000, 1000000, 1000000};
         if (orig[i] == new[i]) {
            costs[3] = table[i-1][j-1]; // MATCH
         }
         costs[2] = table[i-1][j-1]+1; // SUBSTITUTE
         costs[1] = table[i][j-1]+1; // INSERT
         costs[0] = table[i-1][j]+1; // DELETE

         int index = min(costs);
         // |= ではいけない
         act[i][j] = 1 << index;
         table[i][j] = costs[index];
      }
   }
   fprintf(stderr, "score: %d\n", table[orignum-1][newnum-1]);
   int i = orignum-1, j = newnum-1, score = table[orignum-1][newnum-1];
   while (!(i <= 0 && j <= 0)) {
      switch(act[i][j]) {
         case NONE:
            fprintf(stderr, "Error\n");
            exit(-1);
            break;
         case MATCH:
            //puts("Match");
            //printf("%d,0,0\n", MATCH);
            i--;
            j--;
            break;
         case SUBSTITUTE:
            //printf("Substitute %d to %d\n", orig[i], new[j]);
            i--;
            j--;
            printf("%d,%d,%c\n", i, SUBSTITUTE, new[j]);
            score--;
            break;
         case INSERT:
            //printf("Insert %d\n", new[j]);
            j--;
            printf("%d,%d,%c\n", i, INSERT, new[j]);
            score--;
            break;
         case DELETE:
            i--;
            printf("%d,%d,%c\n", i, DELETE, orig[i]);
            //printf("Delete %d\n", orig[i]);
            score--;
            break;
         default:
            fprintf(stderr, "Unknown: %d at %d %d\n", act[i][j], i, j);
            exit(-1);
      }
      //cnt++;
   }
   if (score != 0) {
      fprintf(stderr, "Score should be zero: %d\n", score);
      exit(-1);
   }
   return 0;
}
