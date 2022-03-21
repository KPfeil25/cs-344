#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

/*

THE STRUCTURE OF THE PROGRAM WAS TAKEN FROM THE EXAMPLE PROGRAM
FUNCTIONALITY THAT WAS SPECFIC TO THIS PROGRAM WAS CHANGED/ ADDED

*/

// Size of the buffers, 50 lines of up to 1000 chars
#define SIZE 50000

#define NUM_ITEMS 50

// Buffer 1, shared resource between input thread and line seperator thread
char buffer_1[SIZE];
// Number of items in the buffer
int count_1 = 0;
// Index where the input thread will put the next item
int prod_idx_1 = 0;
// Index where the line seperator thread will pick up the next item
int con_idx_1 = 0;
// Initialize the mutex for buffer 1
pthread_mutex_t mutex_1 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 1
pthread_cond_t full_1 = PTHREAD_COND_INITIALIZER;


// Buffer 2, shared resource between line seperator thread and the plus sign thread
char buffer_2[SIZE];
// Number of items in the buffer
int count_2 = 0;
// Index where the line seperator thread will put the next item
int prod_idx_2 = 0;
// Index where the plus sign thread will pick up the next item
int con_idx_2 = 0;
// Initialize the mutex for buffer 2
pthread_mutex_t mutex_2 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 2
pthread_cond_t full_2 = PTHREAD_COND_INITIALIZER;

// Buffer 2, shared resource between plus sign thread and the output thread
char buffer_3[SIZE];
// Number of items in the buffer
int count_3 = 0;
// Index where the plus sign thread will put the next item
int prod_idx_3 = 0;
// Index where the output thread will pick up the next item
int con_idx_3 = 0;
// Initialize the mutex for buffer 2
pthread_mutex_t mutex_3 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 2
pthread_cond_t full_3 = PTHREAD_COND_INITIALIZER;

int stop_check = 0; // global to check if STOP is found
int num_chars = 0;

// used to check if STOP has been entered
int check_stop(char* input) {
  int resutl;
    if (input[0] == 'S' && input[1] == 'T' && input[2] == 'O' && input[3] == 'P' && input[4] == '\n') {
        resutl = 1;
        return resutl;
    }
    resutl = 0;
    return resutl;
}

// counting the number of characters in a string
int count_stuff(char* input) {
    int i, count = 0;
    for (i = 0; i < strlen(input); i++) {
        if (input[i] != '\n') { 
            count++;
        }
    }
    return count;
}

// helped for the get_input function later
void get_user_input() {
    char* buffer = (char*)malloc(sizeof(char) * 1000);
    num_chars = 0;
    while (num_chars < 80) { // while they have entered less than 80 chars
      fgets(buffer, 1000, stdin); // take in input
      num_chars += count_stuff(buffer); // count them
      strcat(buffer_1, buffer); // add on to the buffer
      if (check_stop(buffer) == 1) { // check if they entered STOP
        stop_check = 1; // set global
        break; // user has entered STOP, break out of the loop
      }
    }
}

// put the string into buffer 1
void put_buff_1(char* a_line) {
  // Lock the mutex before putting the item in the buffer
  pthread_mutex_lock(&mutex_1);
  // Put the item in the buffer
  strcpy(buffer_1, a_line);
  // Increment the index where the next item will be put.
  prod_idx_1 = prod_idx_1 + 1;
  count_1++;
  // Signal to the consumer that the buffer is no longer empty
  pthread_cond_signal(&full_1);
  // Unlock the mutex
  pthread_mutex_unlock(&mutex_1);
  
}

// uses the helper from above to get input
void *get_input(void *args) {
  int i;
  
  for (i = 0; i < NUM_ITEMS; i++) {
    while (!stop_check) { // case where they have not entered STOP
      get_user_input();
      put_buff_1(buffer_1);
    }
    put_buff_1(buffer_1);
  }
  
  return NULL;
}

// get item from buffer 1
char* get_buff_1(){
  // Lock the mutex before checking if the buffer has data
  pthread_mutex_lock(&mutex_1);
  while (count_1 == 0) {
    pthread_cond_wait(&full_1, &mutex_1);
  }
  char* a_line = (char*)malloc(sizeof(char) * 1000);
  strcpy(a_line, buffer_1);
  // Increment the index from which the item will be picked up
  con_idx_1 = con_idx_1 + 1;
  count_1--;
  // Unlock the mutex
  pthread_mutex_unlock(&mutex_1);
  // Return the item
  return a_line;
}

// put something into buffer 2
void put_buff_2(char* a_line){
  // Lock the mutex before putting the item in the buffer
  pthread_mutex_lock(&mutex_2);
  // Put the item in the buffer
  strcpy(buffer_2, a_line);
  // Increment the index where the next item will be put.
  prod_idx_2 = prod_idx_2 + 1;
  count_2++;
  // Signal to the consumer that the buffer is no longer empty
  pthread_cond_signal(&full_2);
  // Unlock the mutex
  pthread_mutex_unlock(&mutex_2);
}

// takes in the string from buffer 1 and replaces all newlines with spaces
// returns to the second buffer
void *replace_newline(void *args) {
  int i;
  for (int i = 0; i < NUM_ITEMS; i++) {
    char* string = (char*)malloc(sizeof(char) * 1000); // dynamic allocation becuase we need to return it
    string = get_buff_1(); // get it from the buffer
    int j;
    for (j = 0; j < strlen(string); j++) {
        if (string[j] == '\n') { // find the newlines
            string[j] = ' '; // replace them
        }
    }
    put_buff_2(string); // put it in the buffer
  }

return NULL;
}

// get item from the second buffer
char* get_buff_2(){
  // Lock the mutex before checking if the buffer has data
  pthread_mutex_lock(&mutex_2);
  while (count_2 == 0) {
    // Buffer is empty. Wait for the producer to signal that the buffer has data
    pthread_cond_wait(&full_2, &mutex_2);
  }
  char* a_line = (char*)malloc(sizeof(char) * 1000);
  strcpy(a_line, buffer_2);
  // Increment the index from which the item will be picked up
  con_idx_2 = con_idx_2 + 1;
  count_2--;
  // Unlock the mutex
  pthread_mutex_unlock(&mutex_2);
  // Return the item
  return a_line;
}

void put_buff_3(char* a_line){
  // Lock the mutex before putting the item in the buffer
  pthread_mutex_lock(&mutex_3);
  // Put the item in the buffer
  strcpy(buffer_3, a_line);
  // Increment the index where the next item will be put.
  prod_idx_3 = prod_idx_3 + 1;
  count_3++;
  // Signal to the consumer that the buffer is no longer empty
  pthread_cond_signal(&full_3);
  // Unlock the mutex
  pthread_mutex_unlock(&mutex_3);
}

// takes in the string from the second buffer, gets rid of all "++"
// and changes them to "^" and decrements the size by one
void *plus_sign(void *args) {
  int i;
  for (i = 0; i < NUM_ITEMS; i++) {
    char* string = (char*)malloc(sizeof(char) * 1000); // again, dynamically allocated due to returning it
    string = get_buff_2();
    int j, k;
    for (j = 0; j < strlen(string); j++) {
        if (string[j] == '+' && string[j + 1] == '+') { // if two consecutive chars are "++"
            string[j] = '^'; // replace
            k = j + 1;
            for (k; k < strlen(string) - 1; k++) { // loop through the end of the string
                string[k] = string[k + 1]; // move everything forward
            }
        }
    }
    string[strlen(string) -1] = '\0'; // this subtracts one from the length
    put_buff_3(string); // put into buffer
  }
  
  return NULL;
}

char* get_buff_3(){
  // Lock the mutex before checking if the buffer has data
  pthread_mutex_lock(&mutex_3);
  while (count_3 == 0) {
    // Buffer is empty. Wait for the producer to signal that the buffer has data
    pthread_cond_wait(&full_3, &mutex_3);
  }
  char* a_line = (char*)malloc(sizeof(char) * 1000);
  strcpy(a_line, buffer_3);
  // Increment the index from which the item will be picked up
  con_idx_3 = con_idx_3 + 1;
  count_3--;
  // Unlock the mutex
  pthread_mutex_unlock(&mutex_3);
  // Return the item
  return a_line;
}

// takes in the string from the third buffer and prints it out 80 chars at a time
void* output(void* args) {
  int m, i = 0; // i uis used for the array, m for number of times
  for (m = 0; m < NUM_ITEMS; m++) {
    //printf("enters output\n");
    char* string = (char*)malloc(sizeof(char) * 1000);
    char to_print[82]; // 82 cause 80 chars + 1 newline + \0
    string = get_buff_3(); // get the buffer
    num_chars = strlen(string);
    int j, k = 1, num_times = num_chars / 80, counter; // num times tells how many times to loop through
    for (j = 0; j < num_times; j++) {
      counter = 0; // this helps to put newlines
      for (i; i < 80 * k; i++) {
        printf("%c", string[i]); // print it char by char 80 times
        counter = 1; // new line = yes
      }
      if (counter == 1) {
        printf("\n"); // print newline
      }
      counter = 0;
      k++; // increment k
    }
  }
}

int main()
{
    pthread_t input_t, line_seperator_t, plus_sign_t, output_t; // create the thread objects
    // Create the threads
    pthread_create(&input_t, NULL, get_input, NULL);
    pthread_create(&line_seperator_t, NULL, replace_newline, NULL);
    pthread_create(&plus_sign_t, NULL, plus_sign, NULL);
    pthread_create(&output_t, NULL, output, NULL);
    // Wait for the threads to terminate
    pthread_join(input_t, NULL);
    pthread_join(line_seperator_t, NULL);
    pthread_join(plus_sign_t, NULL);
    pthread_join(output_t, NULL);
    return EXIT_SUCCESS;
}