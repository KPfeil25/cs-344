#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int main(int argc, char** argv) {
    int i, numLetters = atoi(argv[1]); // take the command line number to get the number of chars needed
    srand(time(NULL)); // seed the time

    char allLetters[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ "; // array of all the chars
    char* theString = (char*)malloc(sizeof(int) * numLetters); // create dynamic array

    for (i = 0; i < numLetters; i++) {
        theString[i] = allLetters[rand() % 27]; // fill the array with random numbers
    }

    theString[numLetters] = '\0'; // signify the end of the string
    printf("%s\n", theString); // print
    free(theString); // no memory leak

    return 0;
}