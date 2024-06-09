#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

#define FILENAME "syscalls.log"
#define STAP_SCRIPT_FILE "monitor_syscalls.stp"

void sigint_handler(int signum) {
    FILE *log_file = fopen(FILENAME, "r");
    if (log_file == NULL) {
        perror("Error opening syscalls.log");
        exit(EXIT_FAILURE);
    }

    char line[256];
    int total_syscalls = 0;
    int read_count = 0, write_count = 0, open_count = 0;

    while (fgets(line, sizeof(line), log_file)) {
        total_syscalls++;
        if (strstr(line, "read")) read_count++;
        if (strstr(line, "write")) write_count++;
        if (strstr(line, "open")) open_count++;
    }

    fclose(log_file);

    printf("Número total de llamadas al sistema: %d\n", total_syscalls);
    printf("Read: %d, Write: %d, Open: %d\n", read_count, write_count, open_count);

    exit(0);
}

int main() {
    signal(SIGINT, sigint_handler);

    pid_t parent_pid = getpid();

    FILE *log_file = fopen(FILENAME, "w");
    if (log_file == NULL) {
        perror("Error opening syscalls.log");
        exit(EXIT_FAILURE);
    }
    fclose(log_file);

    pid_t pid1 = fork();
    if (pid1 == 0) {
        execl("./child.bin", "./child.bin", NULL);
        perror("Failed to execute child process");
        exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        execl("./child.bin", "./child.bin", NULL);
        perror("Failed to execute child process");
        exit(EXIT_FAILURE);
    }

    char command[256];
    sprintf(command, "stap %s %d %d > %s &", STAP_SCRIPT_FILE, pid1, pid2, FILENAME);
    system(command);

    printf("Parent PID: %d\n", parent_pid);
    printf("Child 1 PID: %d\n", pid1);
    printf("Child 2 PID: %d\n", pid2);
    printf("Parent waiting for children to finish...\n");
    printf("Press Ctrl+C to get statistics\n");

    wait(NULL);
    wait(NULL);

    return 0;
}
