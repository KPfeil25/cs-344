#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#define PREFIX "movies_"

//char* for all becuase this is how we read them from the file
struct movie
{
   char *title;
   char *year;
   char *languages;
   char* rating_value;
   struct movie *next;
};

int count_movies(struct movie *list) {
   int i = 0;
   while (list != NULL) {
      i++;
      list = list->next;
   }
   return i - 1; //this is to acccount for the first line where the columns are named
}

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
   struct movie *dummy = list->next; // pass over the columns headers
   int i, j = 0, add_to = 0;

   // same steps as the last function, but this time the array of years is being returned
   while (dummy != NULL) {
      add_to = 1;
      //printf("The year being checked is %d\n", atoi(dummy->year));
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

// function from https://stackoverflow.com/questions/5309471/getting-file-extension-in-c#:~:text=3%20Answers&text=You%20can%20use%20the%20strrchr,the%20string%20as%20the%20extension.
char* file_extension(char* file_name) {
    char *dot = strrchr(file_name, '.');
    if (!dot || dot == file_name) {
        return "";
    }
    return dot + 1;
}

void process_csv(char* file_name) {
    struct movie* list = processFile(file_name); // need the list of movies for access to the names to print
    struct movie* dummy = list;
    int num_years = num_unique_years(list); // for the for loop
    int* year_arr = unique_years(list, num_years); // create the array of different years in the list of movies
    int i, year;
    char number_str[5], year_str[4]; // for converting the year into a string
    int rand_num = rand() % 100000; // get random number
    sprintf(number_str, "%d", rand_num); // convert the random number to a string
    char dir_name[20] = "pfeilke.movies.";
    char dir_dummy[20]; // for resetting the name further down
    strcat(dir_name, number_str); // concatenate the directory name and random number
    strcpy(dir_dummy, dir_name);
    int did_it_work = mkdir(dir_name, 0750); // make the directory with permission rwxr-x--- 
    char* file_path;

    if (!did_it_work) { // case where the directory was created
        printf("Directory created with name %s\n", dir_name);
    }
    
    for (i = 0; i < num_years; i++) {
        strcpy(year_str, ""); // making sure that the year_str variable is empty
        strcpy(dir_name, dir_dummy); // likewise for the dir name
        year = year_arr[i]; // grab the year from the array
        sprintf(year_str, "%d", year); // convert to string
        strcat(year_str, ".txt"); // add the .txt file extension
        strcat(dir_name, "/"); // add the / for the file path
        file_path = strcat(dir_name, year_str); // add the strings together
        //printf("The file path is %s\n", file_path);
        int did_it_open = open(file_path, O_CREAT | O_WRONLY, 0640); // create the file with permissions rw-r-----
        dummy = list->next; // skip the column header and begin to run the linked list
        while (dummy != NULL) {
            if (atoi(dummy->year) == year) { // if the year equals the year being checked
                write(did_it_open, strcat(dummy->title, "\n"), (strlen(dummy->title) + 1)); // write to the file
            }
            dummy = dummy->next; // progress along the linked list
        }
    }
}

void largest_csv() {
    DIR *currDir = opendir("."); // open the current directory
    struct dirent *aDir; // create a directory pointer
    struct stat dirStat; // use this struct to get the size of the files
    int max_file_size = 0;
    char file_name[30];

    while((aDir = readdir(currDir)) != NULL){ 
        stat(aDir->d_name, &dirStat); // get the stats
        
        if(strncmp(PREFIX, aDir->d_name, strlen(PREFIX)) == 0 && (strcmp(file_extension(aDir->d_name), "csv") == 0)){ // case where the file has the beginning of movies and the extension of csv
            if (dirStat.st_size >= max_file_size) { // check if the file is larger than the current largest
                max_file_size = dirStat.st_size; // reset the largest size
                strcpy(file_name, aDir->d_name); // copy it to the file path variable
            }
        }
    }
    printf("Now processing the chosen file named %s\n", file_name); // print the message
    process_csv(file_name); // process the file
}

void smallest_csv() {
    // all the same function as just above but for the smallest file
    DIR *currDir = opendir(".");
    struct dirent *aDir;
    struct stat dirStat;
    int min_file_size, i = 0;
    char file_name[30];


    while((aDir = readdir(currDir)) != NULL){
        stat(aDir->d_name, &dirStat);

        if (i == 0) {
            min_file_size = dirStat.st_size;
        }

        if(strncmp(PREFIX, aDir->d_name, strlen(PREFIX)) == 0 && (strcmp(file_extension(aDir->d_name), "csv") == 0)){
            if (dirStat.st_size <= min_file_size) {
                min_file_size = dirStat.st_size;
                strcpy(file_name, aDir->d_name);
            }
        }
        i++;
    }
    printf("Now processing the chosen file named %s\n", file_name);
    process_csv(file_name);
}

void named_csv() {
    // again, all the same as the functions above but this is for the user inputted name
    DIR *currDir = opendir(".");
    struct dirent *aDir;
    struct stat dirStat;
    char file_name[50];
    char user_input[50];
    int does_it_exist = 0;

    printf("Enter the complete name of the file you want processed:\n");
    scanf("%s", &user_input);

    while((aDir = readdir(currDir)) != NULL){ 
        //printf("The name being checked is %s\n", aDir->d_name);
        if (strcmp(user_input, aDir->d_name) == 0) {
            does_it_exist = 1; // setting this to 1 so that the error message is not printed out
            strcpy(file_name, aDir->d_name);
            printf("Now processing the chosen file named %s\n", file_name);
            process_csv(file_name);
        }
    }
    if (!does_it_exist) { // case where the file was not found
        printf("This file was not able to be found\n");
    }
}

int choice() {
    // game loop for the outer choice
    int choice;
    int go_again = 1;
    do {
        printf("1. Select file to process\n");
        printf("2. Exit the program\n");
        scanf("%d", &choice);
        if (choice == 1 || choice == 2) {
            return choice;
        }
    } while (go_again == 1);
}

int next_choice() {
    // game loop for the inner choice
    int choice;
    int go_again = 1;
    do {
        printf("Which file you want to process?\n");
        printf("Enter 1 to pick the largest file\n");
        printf("Enter 2 to pick the smallest file\n");
        printf("Enter 3 to specify the name of a file\n");
        printf("Enter a choice from 1 to 3:\n");
        scanf("%d", &choice);
        if (choice == 1 || choice == 2 || choice == 3) {
            return choice;
        }
    } while (go_again == 1);
}

int game_loop() {
    // game loop for everything
    int first, second;
    do {
        first = choice();
        if (first == 1) {
            second = next_choice();
            if (second == 1) {
                largest_csv();
            }
            else if (second == 2) {
                smallest_csv();
            }
            else if (second == 3) {
                named_csv();
            }
        }
    } while (first != 2);

}

int main() {
    time_t t; // declare for seeding the random
    srand((unsigned) time(&t)); // seed the random numbers
    game_loop(); // the whole prog :)
}