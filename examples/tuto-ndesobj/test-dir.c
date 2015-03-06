#include <motsim.h>
#include <directory.h>


int main()
{
   struct directory_t * directory;

   motSim_create();
   printf("go ...\n");
   directory = directory_create();

   return 0;
}
