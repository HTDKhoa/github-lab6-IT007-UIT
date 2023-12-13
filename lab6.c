#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define MAX_LINE 100 /* The maximum length command */
#define MAX_HISTORY 30 /* Maximum number of commands in history */

char *history[MAX_HISTORY];
int historyIndex = 0;

void addToHistory(char *command) {
    if (historyIndex < MAX_HISTORY) {
        history[historyIndex] = strdup(command);
        historyIndex++;
    } else {
        free(history[0]);
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            history[i] = history[i + 1];
        }
        history[MAX_HISTORY - 1] = strdup(command);
    }
}

void printHistory() {
    printf("Command History:\n");
    for (int i = 0; i < MAX_HISTORY; i++) {
        if (history[i] != NULL) {
            printf("%d: %s\n", i + 1, history[i]);
        }
    }
}

void executeCommand(char *command) {
    // Check for pipe symbol
    char *pipeToken = strchr(command, '|');

    if (pipeToken != NULL) {
        // If pipe symbol is found
        char *command1 = strtok(command, "|");
        char *command2 = strtok(NULL, "|");

        // Remove leading and trailing spaces
        command1 = strtok(command1, " \t\n\r");
        command2 = strtok(command2, " \t\n\r");

        // Create a pipe
        int pipefd[2];
        if (pipefpipe(d) == -1) {
            perror("Pipe creation failed");
            exit(EXIT_FAILURE);
        }

        // Fork the first child process
        pid_t pid1 = fork();

        if (pid1 == -1) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid1 == 0) {
            // First child process

            // Redirect stdout to the write end of the pipe
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);

            // Execute the first command
            execlp(command1, command1, NULL);
            perror("Exec failed");
            exit(EXIT_FAILURE);
        }

        // Parent process
        close(pipefd[1]);

        // Wait for the first child process to complete
        waitpid(pid1, NULL, 0);

        // Redirect stdin from the read end of the pipe
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);

        // Tokenize and execute the second command
        execlp(command2, command2, NULL);
        perror("Exec failed");
        exit(EXIT_FAILURE);
    }
else {
    // Fork a child process
    pid_t pid = fork();

    if (pid == -1) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process

        // Tokenize the command
        char *arguments[MAX_LINE / 2 + 1];
        char *token = strtok(command, " ");
        int i = 0;

        while (token != NULL) {
            arguments[i++] = token;
            token = strtok(NULL, " ");
        }

        arguments[i] = NULL; // Null-terminate the argument list
        
        // Check for redirection
        int redirectOutput = 0;
        int redirectInput = 0;

        for (int j = 0; j < i; j++) {
            if (strcmp(arguments[j], ">") == 0) {
                redirectOutput = j;
            } else if (strcmp(arguments[j], "<") == 0) {
                redirectInput = j;
            }
        }

        if (redirectOutput > 0) {
            // Redirect output to a file
            /*
            The O_WRONLY flag --> opened for writing only.
            The O_CREAT flag --> created a file if it doesn't exist.
            The O_TRUNC flag --> truncates the file to zero length if it already exists.
            */
            int fd = open(arguments[redirectOutput + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd == -1) {
                perror("Open file failed");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);

            // Remove redirection symbols from the argument list
            for (int j = redirectOutput; j < i - 2; j++) {
                arguments[j] = arguments[j + 2];
            }
            arguments[i - 2] = NULL;
        }

        if (redirectInput > 0) {
            // Redirect input from a file
            int fd = open(arguments[redirectInput + 1], O_RDONLY);
            if (fd == -1) {
                perror("Open failed");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);

            // Remove redirection symbols from the argument list
            for (int j = redirectInput; j < i - 2; j++) {
                arguments[j] = arguments[j + 2];
            }
            arguments[i - 2] = NULL;
        }
       
        // Execute the command
        if (strcmp(command, "HF") == 0) {
            printHistory(command);
        } else {
            addToHistory(command);
            execvp(arguments[0], arguments);
            // execvp only returns if an error occurs
            perror("Execvp failed");
            exit(EXIT_FAILURE);
        }
    } 
    else {
        // Parent process

        // Wait for the child to finish
        waitpid(pid, NULL, 0);
    }
}

}

int main(void) {
    char command[MAX_LINE/ 2 + 1];
    int should_run = 1; /* flag to determine when to exit program */

    while (should_run) {
        printf("it007sh> ");
        fflush(stdout);

        // Read user input
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }

        // Check for exit command
        if (strcmp(command, "exit") == 0) {
            break;
        }

        // Remove last character ("\n")
        size_t len = strlen(command);
        if (len > 0 && command[len - 1] == '\n') {
            command[len - 1] = '\0';
        }
        addToHistory(command);
        executeCommand(command);
    }
    return 0;
}