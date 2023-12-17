#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_LINE 100 /* The maximum length command */
#define MAX_HISTORY 30 /* Maximum number of commands in history */

char *history[MAX_HISTORY];
int historyIndex = 0;
int historyPrintIndex;
int HistoryFlag = 0;

void addToHistory(char *command) {
    if (historyIndex < MAX_HISTORY) {
    history[historyIndex] = strdup(command);
    historyIndex++;
} else {
    free(history[0]); // Free the oldest command
    for (int i = 0; i < MAX_HISTORY - 1; i++) {
        history[i] = history[i + 1];
    }
        
}
}

void printHistory() {
    if (HistoryFlag == 0) historyPrintIndex = historyIndex;
    if (historyPrintIndex > 0)
    {
        printf("Lich su cua lenh: %d: %s\n", historyPrintIndex + 1, history[--historyPrintIndex]);
    }
    else
    {
        printf("Da in ra toan bo lich su cua lenh\n");
    }
}
void execute_pipeline(char ***command_list, int num_commands) {
    int pipes[num_commands - 1][2];

void execute_pipeline(char ***command_list, int num_commands) {
    int pipes[num_commands - 1][2];

    for (int i = 0; i < num_commands; ++i) {
        if (i < num_commands - 1) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        pid_t pid = fork();

        if (pid == 0) {
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
            }

            if (i < num_commands - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            execvp(command_list[i][0], command_list[i]);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (i > 0) {
            close(pipes[i - 1][0]);
            close(pipes[i - 1][1]);
        }

        if (pid > 0) wait(NULL);
    }
}

void executeCommand(char *command) {
    // Bai 1
    // Fork 1 tien trinh con
    pid_t pid = fork();

    if (pid == -1) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Process cua tien trinh con
        // Tach command ra thanh nhieu thanh phan qua dau " "
        char *arguments[MAX_LINE / 2 + 1];
        char *token = strtok(command, " ");
        int i = 0;
        int num_commands = 0;
        char **command_list[MAX_LINE/2 + 1];

        while (token != NULL) {
            if (strcmp(token, "|") == 0) {
                arguments[i] = NULL;
                command_list[num_commands] = malloc(sizeof(char*) * (i + 1));
                memcpy(command_list[num_commands], arguments, sizeof(char*) * i);
                command_list[num_commands][i] = NULL;
                ++num_commands;

                i = 0;
            }
            else {
                arguments[i] = token;
                i++;
            }
            token = strtok(NULL, " ");
        }

        arguments[i] = NULL; // Null-terminate mang arguments
        
        // Bai 3 Check toan tu chuyen huong vao ra
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
            // Toan tu chuyen huong dau vao mot file ('>')
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
            // Toan tu chuyen huong dau ra tu mot file ('<')
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
       if (i > 0) {
            arguments[i] = NULL;
            command_list[num_commands] = malloc(sizeof(char*) * (i + 1));
            memcpy(command_list[num_commands], arguments, sizeof(char*) * i);
            command_list[num_commands][i] = NULL;
            ++num_commands;
        }  

        if (num_commands > 1) {
            addToHistory(command);
            execute_pipeline(command_list, num_commands);
        } 
        else {
        // Thuc thi cau lenh
            if (strcmp(command, "HF") == 0) {
            printHistory(command);
            HistoryFlag = 1;
        } 
            else {
            HistoryFlag = 0;
            addToHistory(command);
            execvp(arguments[0], arguments);
            // execvp only returns if an error occurs
            perror("Execvp failed");
            exit(EXIT_FAILURE);
            }
        }
    } 
    else {
        // Parent process
        // Wait for the child to finish
        waitpid(pid, NULL, 0);
    }
}

int isChildExecuting = 0;
// Signal handler for SIGINT (Ctrl + C)
void sigintHandler(int signum)
{
    if (isChildExecuting)
    {
        printf("Nhan duoc input (Ctrl + C). Xuong dong cho command moi");
        fflush(stdout);
        exit(EXIT_SUCCESS);
    }
}


int main(void) {
    char command[MAX_LINE/ 2 + 1];
    int should_run = 1; /* flag de biet khi nao ket thuc chuong trinh */

    // Set up the signal handler for SIGINT
    signal(SIGINT, sigintHandler);

    while (should_run) {
        printf("it007sh> ");
        fflush(stdout);

        // Read user input
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }

        // Xoa phan tu cuoi ("\n")
        size_t len = strlen(command);
        if (len > 0 && command[len - 1] == '\n') {
            command[len - 1] = '\0';
        }

        // Check exit command
        if (strcmp(command, "exit") == 0) {
            break;
        }

        else if (strcmp(command, "HF") == 0)
        {
            printHistory();
            HistoryFlag = 1;
        }

        else
        {
            HistoryFlag = 0;
            addToHistory(command);
            executeCommand(command);
        }
    }
    return 0;
}
