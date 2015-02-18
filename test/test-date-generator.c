#include <stdio.h>     // printf, ...

#include <date-generator.h>

int main()
{
   int i;
   struct dateGenerator_t * dateGen;

   dateGen = dateGenerator_create();

   for (i = 0; i < 20; i++) {
     printf("%f, ", dateGenerator_nextDate(dateGen));
   }
}
