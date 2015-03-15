#include <motsim.h>
#include <directory.h>


int main()
{
   struct directory_t * directory;

   motSim_create();
   printf("go ...\n");
   directory = directory_create();
   printf("directory created ...\n");

   directory_insertEntry(directory, "Carl", "08 07 06 05 04");
   directory_insertEntry(directory, "Daisy", "04 02 03 05 01");
   directory_insertEntry(directory, "Bob", "06 05 04 03 02");
   directory_insertEntry(directory, "Alice", "07 06 05 04 03");
   directory_insertEntry(directory, "Eve", "05 06 04 02 03");

   printf("directory populated ...\n");

   directory_print(directory);

   return 0;
}
