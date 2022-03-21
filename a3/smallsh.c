#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_INPUT_LENGTH 2048
#define MAX_ARGS 512
#define MAX_FILE_SIZE 256

int canBg = 1; // only global needed

void getInput(char* arr[], char inputName[], char outputName[], int pid, int* bg) {
	
	char input[MAX_INPUT_LENGTH];
    char* token; // for use with strtok later
	int i, j;

	printf(": "); // print prompt
	fflush(stdout); // clear the standard output
	fgets(input, MAX_INPUT_LENGTH, stdin); // take in input

	// Remove newline
	for (i = 0; i < MAX_INPUT_LENGTH; i++) {
        if (input[i] == '\n') {
            input[i] = '\0';
        }
    }

	// for when the user does not enter anything
	if (strcmp(input, "") == 0) {
		arr[0] = strdup("");
		return;
	}

	i = 0; // needed to reset i before the loop

	token = strtok(input, " ");
	while (token != NULL) {
		if (strcmp(token, "&") == 0) {
			*bg = 1; // case where it is a background fn
		}
		else if (strcmp(token, "<") == 0) {
			token = strtok(NULL, " "); // case where an input file is being named
			strcpy(inputName, token);
		}
		else if (strcmp(token, ">") == 0) {
			token = strtok(NULL, " "); // similar to last one, case where output file is named
			strcpy(outputName, token);
		}
		else {
			arr[i] = strdup(token);
			for (j = 0; arr[i][j]; j++) { // expansion of $$, doing this here to rake care of it immeadiately
				if (arr[i][j] == '$' && arr[i][j + 1] == '$') {
				    arr[i][j] = '\0';
					snprintf(arr[i], 256, "%s%d", arr[i], pid); // convert the char* to the pid that is an int
				}
			}
		}
		token = strtok(NULL, " "); // next part of the string
		i++; // increment i
	}
}

void other_commands(char* arr[], char *inputName, char *outputName, int* childExitStatus, struct sigaction sig_action, int* background) {
	
	int source, output, target; // needed for files
	pid_t spawnPid = -5;

	// much of this code was taken from lectures
	spawnPid = fork(); // create child process
    if (spawnPid == -1) {
        perror("Fork failed\n");
        exit(1);
    }
    else if (spawnPid == 0) {
        sig_action.sa_handler = SIG_DFL; // change so this can handle ^C as usual
        sigaction(SIGINT, &sig_action, NULL);

        if (strcmp(inputName, "")) { // if a file name has been passed, do this
            source = open(inputName, O_RDONLY);
            if (source == -1) { // if failed, 
                perror("Unable to open input file\n");
                exit(1);
            }
            target = dup2(source, 0); // case where file was successfully created
            if (target == -1) {
                perror("Unable to assign input file\n");
                exit(2);
            }
            fcntl(source, F_SETFD, FD_CLOEXEC); // close file
        }

        if (strcmp(outputName, "")) { // similar to one above
            output = open(outputName, O_WRONLY | O_CREAT | O_TRUNC, 0666); // open file for writing
            if (output == -1) {
                perror("Unable to open output file\n");
                exit(1);
            }
            target = dup2(output, 1);
            if (target == -1) {
                perror("Unable to assign output file\n");
                exit(2);
            }
            fcntl(output, F_SETFD, FD_CLOEXEC);
        }
        
        if (execvp(arr[0], arr)) {// if none of the above was true, run execvp()
			printf("%s: no such file or directory\n", arr[0]); // execvp will only return if it fails, so nothing is needed between these
			fflush(stdout);
			exit(2);
		}
        
    }
    else {
        if (*background && canBg) { // case where it is a background process
            pid_t bgPid = waitpid(spawnPid, childExitStatus, WNOHANG); // do not wait for the child process (WONOHANG), return immeadiately
            printf("background pid is %d\n", spawnPid);
            fflush(stdout);
        }
        else {
            pid_t realPid = waitpid(spawnPid, childExitStatus, 0); // else, wait
        }
        while ((spawnPid = waitpid(-1, childExitStatus, WNOHANG)) > 0) { // this next block finds the exit status
            printf("child %d terminated\n", spawnPid);
            if (WIFEXITED(*childExitStatus)) { // case where the child exited normally
				printf("exit value %d\n", WEXITSTATUS(*childExitStatus));
			}
			else { // case where the child did not exit normally
				printf("terminated by signal %d\n", WTERMSIG(*childExitStatus));
			}
            fflush(stdout);
        }
    }
}


void catchSIGTSTP(int signo) {

	if (canBg == 1) { // for switching with ^Z
		char* message = "Entering foreground-only mode (& is now ignored)\n";
		write(1, message, 49);
		fflush(stdout);
		canBg = 0;
	}

	else {
		char* message = "Exiting foreground-only mode\n";
		write (1, message, 29);
		fflush(stdout);
		canBg = 1;
	}
}

int main() {

	int go_again = 1, i, j, status = 0, isBg = 0;
	int pid = getpid();

	char *args[MAX_ARGS]; // create 2d array for the args
	char inputF[MAX_FILE_SIZE] = ""; // create input file name
	char outputF[MAX_FILE_SIZE] = ""; // create output file name
	
	for (i = 0; i < MAX_ARGS; i++) { // initialize pointers
		args[i] = NULL;
	}
	
	// used for ignoring ^C
	struct sigaction si = {0};
	si.sa_handler = SIG_IGN;
	sigfillset(&si.sa_mask);
	si.sa_flags = 0;
	sigaction(SIGINT, &si, NULL);

	// used to send ^Z to the above function
	struct sigaction s_stp = {0};
	s_stp.sa_handler = catchSIGTSTP;
	sigfillset(&s_stp.sa_mask);
	s_stp.sa_flags = 0;
	sigaction(SIGTSTP, &s_stp, NULL);

	do {
		// Get input
		getInput(args, inputF, outputF, pid, &isBg);

		if (args[0][0] == '#' || args[0][0] == '\0') { // case where the user entered nothing, skip
			continue;
		}
		
		else if (strcmp(args[0], "exit") == 0) { // case where the user wants to end the program, breaks the loop
			go_again = 0;
		}

		else if (strcmp(args[0], "cd") == 0) {
			if (args[1]) { // checking if other directory has been specified
				if (chdir(args[1]) == -1) { // case where the specified directory does not exist
					printf("Directory not found.\n");
					fflush(stdout);
				}
			}
			else { // otherwise, send the user home
				chdir(getenv("HOME"));
			}
		}

		else if (strcmp(args[0], "status") == 0) { // finding exit status
			if (WIFEXITED(status)) { // same as when it was called above, case where child exited normally
				printf("exit value %d\n", WEXITSTATUS(status));
			}
			else {
				printf("terminated by signal %d\n", WTERMSIG(status));
			}
		}

		else { // else, it will be i/o redirection or passed to execvp()
			other_commands(args, inputF, outputF, &status, si, &isBg);
		}

		// reset all args
		for (i = 0; args[i]; i++) {
			args[i] = NULL;
		}

		inputF[0] = '\0'; // set both files to null
		outputF[0] = '\0';
		isBg = 0; // reset the background
	} while (go_again);
	
	return 0;
}
