#include <stdio.h>
#include <unistd.h>

void setLed(char num) {
    FILE* file;
    file = fopen("/dev/etx_device", "w");
    if(file == NULL){
        printf("File open error!\n");
        return;   
    }
    fputc(num, file);
    fclose(file);
}

void main(int argc, char** argv) {
    char* studentId = argv[1];
    if(studentId == NULL){
        return;
    }
    for (int i = 0; studentId[i] != '\0'; i++) {
        setLed(studentId[i]);
        sleep(1);
        setLed('0');
        usleep(100 * 1000);
    }
}