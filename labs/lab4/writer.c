#include <stdio.h>
#include <unistd.h>

#define DEVICE_NAME "/dev/mydev"

void write_driver(char num) {
    FILE* file;
    file = fopen(DEVICE_NAME, "w");
    if(file == NULL){
        printf("File open error!\n");
        return;   
    }
    fputc(num, file);
    fclose(file);
}

void main(int argc, char** argv) {
    char* str = argv[1];
    if(str == NULL){
        return;
    }
    for (int i = 0; str[i] != '\0'; i++) {
        write_driver(str[i]);
        sleep(1);
        write_driver('0');
        usleep(100 * 1000);
    }
}