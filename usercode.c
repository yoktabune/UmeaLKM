#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#define DEVICE "/dev/mydevice"
#define MYDEVICE_TYPE 'k'
#define IOCTL_DUMP_HASH_TABLE _IOR(MYDEVICE_TYPE, 1, size_t)

int main() {
    int file_descriptor;
    char command_choice;
    char write_buf[100], write_buf_two[100], read_buff[1024]; // Adjust read_buff size as needed
	int process; //  

    file_descriptor = open(DEVICE, O_RDWR);
    if (file_descriptor == -1) {
        printf("file %s either does not exist or has been locked by another process\n", DEVICE);
        exit(-1);
    }

    printf("r = read from device\nw = write to device\nt = multiple process\ns = save hash table to file\nenter command: ");
    scanf(" %c", &command_choice); // Added space to consume newline

    switch (command_choice) {
        case 'w':
            printf("enter key: ");
            scanf(" %[^\n]", write_buf);
            printf("enter data: ");
            scanf(" %[^\n]", write_buf_two);
            strcat(write_buf, ":");
            strcat(write_buf, write_buf_two);
            write(file_descriptor, write_buf, strlen(write_buf)); // Use strlen to write only the necessary bytes
            printf("Data written to device successfully.\n");
            break;
        case 'r':
            printf("enter key: ");
            scanf(" %[^\n]", read_buff);
            read(file_descriptor, read_buff, sizeof(read_buff)); // Read back the value
            printf("device: %s\n", read_buff);
            break;
        case 's': {
            // Added case for saving the hash table to a file
            char buffer[4096]; // Ensure this is sized to hold your hash table data
            if (ioctl(file_descriptor, IOCTL_DUMP_HASH_TABLE, buffer) < 0) {
                perror("Failed to dump hash table");
            } else {
                FILE *file = fopen("hash_data.txt", "w"); // Open for writing, overwriting the file
                if (file == NULL) {
                    printf("Failed to open file for saving!\n");
                } else {
                    fprintf(file, "%s", buffer); // Write the hash table data to the file
                    fclose(file);
                    printf("Hash table data saved to file successfully.\n");
                }
            }
            break;
        }
		case 't': {
    		int process;
            #define PROCESS_COUNT 5 // Number of processes to create

for (int i = 0; i < PROCESS_COUNT; i++) {
    pid_t pid = fork();

    if (pid == 0) { // Child process
        // Child process logic
        printf("I am child process %i\n", i);

        char snum[10]; // Buffer for process number
        snprintf(snum, sizeof(snum), "%d", i);
        snprintf(write_buf, sizeof(write_buf), "%s:20", snum); // Safely format the string
        write(file_descriptor, write_buf, strlen(write_buf));
        printf("Child %i wrote: %s\n", i, write_buf);

        // Check if the process number is not 0 before attempting to read
        if (i > 0) {
            // Assuming you want to read back the value for the key corresponding to this process
            snprintf(read_buff, sizeof(read_buff), "%d", i - 1); // Safely format the string for reading
            printf("Child %i reading for key: %s\n", i, read_buff);
            read(file_descriptor, read_buff, sizeof(read_buff)); // Attempt to read the value
            printf("Child %i read from device: %s\n", i, read_buff);
        } else {
            printf("Child %i not attempting to read key -1\n", i);
        }

        exit(0); // Child process exits after performing its task
    } else if (pid < 0) {
        // Fork failed
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    // Parent process does nothing here and goes back to the beginning of the loop to fork the next child
}

// Parent waits for all child processes to finish
for (int i = 0; i < PROCESS_COUNT; i++) {
    wait(NULL);
}


            break;
		}
        default:
            printf("Unknown command.\n");
    }

    close(file_descriptor);
    return 0;
}