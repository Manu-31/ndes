#include <contact.h>

struct directory_t;

/**
 * @brief Déclaration des fonctions spécifiques liées au ndesObject
 */
declareObjectFunctions(directory);

/**
 * @brief Create a directory
 */
struct directory_t * directory_create();

/**
 * @brief Search a name in the directory
 * @param dir the directory to search in
 * @param name a name to search for
 * @result a contact nor NULL
 */
struct contact_t * directory_searchName(struct directory_t * dir, char * name);

/**
 * @brief Insert a name+number in a directory
 * @param dir the directory 
 * @param name the name of the new contact
 * @param num the tel number of the new contact
 *
 * The strings are copied and thus can be freed after the contact
 * insertin
 */
void directory_insertEntry(struct directory_t *dit, char * name, char * num);

/**
 * @brief print the directory content
 * @param dir the directory to print
 */
void directory_print(struct directory_t *dir);
