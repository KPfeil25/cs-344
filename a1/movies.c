#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//char* for all becuase this is how we read them from the file
struct movie
{
   char *title;
   char *year;
   char *languages;
   char* rating_value;
   struct movie *next;
};


// function repurposed from the provided code
struct movie *createMovie(char *currLine)
{
    struct movie *currMovie = malloc(sizeof(struct movie));

    // For use with strtok_r
    char *saveptr;

    // The first token is the title
    char *token = strtok_r(currLine, ",", &saveptr);
    currMovie->title = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currMovie->title, token);

    // The next token is the year
    token = strtok_r(NULL, ",", &saveptr);
    currMovie->year = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currMovie->year, token);

    // The next token is the languages
    token = strtok_r(NULL, ",", &saveptr);
    currMovie->languages = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currMovie->languages, token);

    // The last token is the rating value
    token = strtok_r(NULL, "\n", &saveptr);
    currMovie->rating_value = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currMovie->rating_value, token);

    // Set the next node to NULL in the newly created student entry
    currMovie->next = NULL;
    return currMovie;
}

//again, repurposed from the provided code
struct movie *processFile(char *filePath)
{
    // Open the specified file for reading only
    FILE *movieFile = fopen(filePath, "r");

    char *currLine = NULL;
    size_t len = 0;
    ssize_t nread;
    char *token;

    // The head of the linked list
    struct movie *head = NULL;
    // The tail of the linked list
    struct movie *tail = NULL;

    // Read the file line by line
    while ((nread = getline(&currLine, &len, movieFile)) != -1)
    {
        // Get a new student node corresponding to the current line
        struct movie *newNode = createMovie(currLine);

        // Is this the first node in the linked list?
        if (head == NULL)
        {
            // This is the first node in the linked link
            // Set the head and the tail to this node
            head = newNode;
            tail = newNode;
        }
        else
        {
            // This is not the first node.
            // Add this node to the list and advance the tail
            tail->next = newNode;
            tail = newNode;
        }
    }
    free(currLine);
    fclose(movieFile);
    return head;
}

/*
* Print data for the given movie
*/
void printMovie(struct movie* aMovie){
  printf("%s, %s %s, %s\n", aMovie->title,
               aMovie->year,
               aMovie->languages,
               aMovie->rating_value);
}
/*
* Print the linked list of movies
*/
void printAllMovies(struct movie *list)
{
    while (list != NULL)
    {
        printMovie(list);
        list = list->next;
    }
}


// count the total movies
int count_movies(struct movie *list) {
   int i = 0;
   while (list != NULL) {
      i++;
      list = list->next;
   }
   return i - 1; //this is to acccount for the first line where the columns are named
}

//print all of the options, exists to reduce clutter in later functions
void print_options() {
   printf("1. Show movies released in the specified year\n");
   printf("2. Show highest rates movie for each year\n");
   printf("3. Show the title and year of release of all movies in a specific language\n");
   printf("4. Exit the program\n");
}

void print_movies_in_year(struct movie *list) {
   int year_choice, i = 0;
   printf("Enter the year you would like to see all the movies for: ");
   scanf("%d", &year_choice); //take in year from user
   while (list != NULL) { //run along linked list
      if (atoi(list->year) == year_choice) { //check if inputted year = current year
         printf("%s was realeased in %d\n", list->title, year_choice);
         i++; //counter to check if no movies were released
      }
      list = list->next;
   }
   if (i == 0) { // no movies were released in the inputted year
      printf("There were no movies on this list that were released in %d\n", year_choice);
   }
}

//count the number of unique years to make an array to put them in
int num_unique_years(struct movie *list) {
   int num_movies = count_movies(list);
   int* year_arr = (int*)malloc(num_movies * sizeof(int)); // allocate memory for all movies in the case that all are unique years
   struct movie *dummy = list; // dummy to run along the list
   int i, j = 0, add_to = 0;

   while (dummy != NULL) {
      add_to = 1;
      for (i = 0; i < num_movies; i++) {
         if (atoi(dummy->year) == year_arr[i]) { // case where the year already is in the list
            add_to = 0; //set this to zero so the year is not added
         }
      }
      if (add_to == 1) { // case where add_to has not been changed, so add the movie to the list
         year_arr[j] = atoi(dummy->year);
         j++; // increase counter of the number of movies by 1
      }
      dummy = dummy->next;
   }
   free(year_arr);
   return j;

}

int* unique_years(struct movie *list, int num_unique_years) {
   int* year_arr = (int*)malloc(num_unique_years * sizeof(int)); // exact size thanks to the function that counted the movies
   struct movie *dummy = list;
   int i, j = 0, add_to = 0;

   // same steps as the last function, but this time the array of years is being returned
   while (dummy != NULL) {
      add_to = 1;
      for (i = 0; i < num_unique_years; i++) {
         if (atoi(dummy->year) == year_arr[i]) {
            add_to = 0;
         }
      }
      if (add_to == 1) {
         year_arr[j] = atoi(dummy->year);
         j++;
      }
      dummy = dummy->next;
   }
   return year_arr;
}

void show_highest_rated(struct movie *list, int* year_arr, int num_years) {
   int i = 0, j = 0, year = 0;
   double max_rating;
   char* max_rating_name;
   struct movie* dummy = list;
   char* stupid_variable; //to use with strtod()

   for (i = 0; i < num_years; i++) {
      year = year_arr[i];
      dummy = list->next; //so the column headers are always skipped
      max_rating = 0; // reset the rating
      while (dummy != NULL) {
         if (atoi(dummy->year) == year) { // if the year is the current year
            if (strtod(dummy->rating_value, &stupid_variable) > max_rating) { //check if it is greater than the max
               max_rating = strtod(dummy->rating_value, &stupid_variable); // if it is, set the max rating to its new rating
               max_rating_name = dummy->title; // also take the title
            }
         }
         dummy = dummy->next;
      }
      printf("%d %.2f %s\n", year, max_rating, max_rating_name);
   }
}

void print_languages(struct movie* list) {
   struct movie* dummy = list;
   char* token, *theRest, *languages;
   char input_lang[20]; //told on the assignment description that we can assume all languages are under 20 characters
   int i = 0;

   printf("Enter the language: ");
   scanf("%s", input_lang);

   while (dummy != NULL) {
      languages = dummy->languages; // for use with strtok_r
      theRest = languages; // ^
      while ((token = strtok_r(theRest, "[];", &theRest))) { // create tokens that are delimited at [,], and ;
         if (strcmp(input_lang, token) == 0) { // check if the token is equal to what the user inputted
            printf("%d %s\n", atoi(dummy->year), dummy->title);
            i++; // increase the counter so we can know if no movies in that language exist
         }
      }
      dummy = dummy->next;
   }
   if (i == 0) {
         printf("No movies in this langauge have been found in the list.\n");
   }
}

// just a function that checks the users choice
int get_option() {
   int choice, okay_input;

   print_options();
   do {
      printf("\nEnter your choice from 1-4: ");
      scanf("%d", &choice);
      if (choice < 0 || choice > 4) {
         printf("Please enter a valid choice.");
         okay_input = 0;
      }
      else {
         return choice;
      }
   } while (okay_input == 0);
}

// simple game loop that runs everything
void game_loop(struct movie* list) {
   int choice;
   int num_of_years = num_unique_years(list);
   int* years_arr = unique_years(list, num_of_years);
   while (choice != 4) {
      choice = get_option();
      if (choice == 1) {
         print_movies_in_year(list);
      }
      else if (choice == 2) {
         show_highest_rated(list, years_arr, num_of_years);
      }
      else if (choice == 3) {
         print_languages(list);
      }
      printf("\n");
   }
}

void free_mem(struct movie* list) {
   while (list != NULL) {
      free(list->title);
      free(list->year);
      free(list->languages);
      free(list->rating_value);
      free(list->next);
      list = list->next;
   }
}

int main(int argc, char* argv[]) {
   
   if (argc < 2 ) {
      printf("The file that is processed needs to be provided as an argument\n");
      return EXIT_FAILURE;
   }
   
   struct movie *list = processFile(argv[1]);
   int num_movies = count_movies(list);
   printf("Processed file %s and parsed data for %d movies\n\n", argv[1], num_movies);
   game_loop(list);
   free_mem(list);
   
   
   return EXIT_SUCCESS;

   return 0;
}

