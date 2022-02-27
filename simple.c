#include<stdio.h>
int foo(int a, int b);
int bar(int a);
int baz(int a);

int main(int argc, char **argv) {
   int a = 0;
   int b = 0;
   scanf("%d", &a); 
   scanf("%d", &b);
   if (a == b ) {
      printf("SAME");
   } else {
      if (a > b) {
         printf("GREATER");
      } else {
         printf("LESS");
         if (a == 0) {
            printf("ZERO");
         } else {
            if (a < 0) {
               printf("NEGATIVE");
            } else {
               printf("POSITIVE");
            }
         }
      }
   }
   return 0;
}
