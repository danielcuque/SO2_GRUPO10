#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>


#define FILENAME "practica1.txt"

void random_string(char *str, size_t size) {
    const char setStr[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t n = 0; n < size - 1; n++) {
        int key = rand() % (int) (sizeof(setStr) - 1);
        str[n] = setStr[key];
    }
    str[size - 1] = '\0';
}


void child_process() {
    srand(time(NULL) ^ (getpid()<<16)); 

    int fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0644); 

    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char buffer[9]; 

    while(1) {
        int action = rand() % 3; 
        switch (action)
        {

        case 0: 
            close(fd); 
            fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0644); 
            // printf("File opened\n");
            break;
        case 1: 
            random_string(buffer, sizeof(buffer)); 
            write(fd, buffer, sizeof(buffer) - 1); 
            // printf("String written: %s\n", buffer);
            break;
        case 2: 
            lseek(fd, 0, SEEK_SET); 
            read(fd, buffer, sizeof(buffer) - 1); 
            buffer[sizeof(buffer) - 1] = '\0'; 
            // printf("String read: %s\n", buffer);
            break;
        }
        sleep(rand() % 3 + 1); 

    }

    close(fd); 
    exit(EXIT_SUCCESS);
}


int main() {
    child_process();
    return 0;
}