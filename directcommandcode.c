#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/wait.h>

#define DEVICE "/dev/mydevice"
#define MYDEVICE_TYPE 'k'
#define IOCTL_DUMP_HASH_TABLE _IOR(MYDEVICE_TYPE, 1, size_t)

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: program <command> [options]\n");
        exit(-1);
    }

    int file_descriptor;
    char write_buf[100], read_buff[1024]; // Adjust buffer sizes as needed

    file_descriptor = open(DEVICE, O_RDWR);
    if (file_descriptor == -1) {
        printf("file %s either does not exist or has been locked by another process\n", DEVICE);
        exit(-1);
    }

    char command_choice = argv[1][0];
    switch (command_choice) {
        case 'w':
            if (argc != 4) {
                printf("Usage: program w <key> <data>\n");
                exit(-1);
            }
            strcpy(write_buf, argv[2]);
            strcat(write_buf, ":");
            strcat(write_buf, argv[3]);
            write(file_descriptor, write_buf, strlen(write_buf)); // Use strlen to write only the necessary bytes
            printf("Data written to device successfully.\n");
            break;
        case 'r':
            if (argc != 3) {
                printf("Usage: program r <key>\n");
                exit(-1);
            }
            strcpy(read_buff, argv[2]);
            read(file_descriptor, read_buff, sizeof(read_buff)); // Read back the value
            printf("device: %s\n", read_buff);
            break;
        case 's':
            // Save hash table to file
            if (argc != 2) {
                printf("Usage: program s\n");
                exit(-1);
            }
            char buffer[4096]; // Ensure this is sized to hold your hash table data
            if (ioctl(file_descriptor, IOCTL_DUMP_HASH_TABLE, buffer) < 0) {
                perror("Failed to dump hash table");
            } else {
                FILE *file = fopen("hash_data.txt", "w"); // Open for writing
                if (file == NULL) {
                    printf("Failed to open file for saving!\n");
                } else {
                    fprintf(file, "%s", buffer); // Write the hash table data to the file
                    fclose(file);
                    printf("Hash table data saved to file successfully.\n");
                    printf("Content of Hashtable:\n");
                    printf("%s",buffer);
                }
            }
            break;
        default:
            printf("Unknown command.\n");
    }

    close(file_descriptor);
    return 0;
}
