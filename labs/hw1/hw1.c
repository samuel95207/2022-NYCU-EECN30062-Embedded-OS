#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define MODE_MENU 0
#define MODE_CONFIRMED_CASE 1
#define MODE_REPORTING_SYSTEM 2
#define MODE_EXIT 3
#define MODE_AREA_DETAIL 4

#define AREA_NUM 9

// #define LED8_DRIVER_PATH "/dev/led8"
// #define SEG7_DRIVER_PATH "/dev/seg7"

#define LED8_DRIVER_PATH "/dev/led8"
#define SEG7_DRIVER_PATH "/dev/seg7"

void clearArr(int *arr);
void printMenu();
char readChar(const char *);
void printAreaCases(const int *mildCountArr, const int *severedCountArr);
void printAreaDetail(const int area, const int *mildCountArr, const int *severedCountArr);
void inputNewCase(int *mildCountArr, int *severedCountArr);
void addNewCase(int area, char mildOrSevered, int count, int *mildCountArr, int *severedCountArr);

void setLED8(int *input);
void setSeg7(char *str);

void displayAreaCase(const int *mildCountArr, const int *severedCountArr);
void displayAreaDetail(const int area, const int *mildCountArr, const int *severedCountArr);



int main() {
    int mode = MODE_MENU;
    int mildCountArr[AREA_NUM];
    int severedCountArr[AREA_NUM];
    int selectedArea;

    clearArr(mildCountArr);
    clearArr(severedCountArr);


    while (1) {
        if (mode == MODE_MENU) {
            printMenu();
            int selection = readChar("> ");

            if (selection == '1') {
                mode = MODE_CONFIRMED_CASE;
            } else if (selection == '2') {
                mode = MODE_REPORTING_SYSTEM;
            } else if (selection == '3') {
                mode = MODE_EXIT;
            } else {
                printf("Error! Unknown command.\n");
            }

            continue;

        } else if (mode == MODE_CONFIRMED_CASE) {
            printAreaCases(mildCountArr, severedCountArr);
            displayAreaCase(mildCountArr, severedCountArr);


            int selection = readChar("comfirmedCases> ");

            if (selection == 'q') {
                mode = MODE_MENU;
                continue;
            }

            int num = selection - '0';

            if (num < 0 || num > AREA_NUM) {
                printf("Error! Index out of bound.\n");
                continue;
            }

            selectedArea = num;
            mode = MODE_AREA_DETAIL;


        } else if (mode == MODE_REPORTING_SYSTEM) {
            inputNewCase(mildCountArr, severedCountArr);
            mode = MODE_MENU;

        } else if (mode == MODE_EXIT) {
            int blackArr[AREA_NUM] = {0};
            for (int i = 0; i < AREA_NUM; i++) {
                blackArr[i] = 0;
            }
            setLED8(blackArr);
            setSeg7("x");
            break;

        } else if (mode == MODE_AREA_DETAIL) {
            printAreaDetail(selectedArea, mildCountArr, severedCountArr);
            displayAreaDetail(selectedArea, mildCountArr, severedCountArr);

            printf("comfirmedCases.Area%d", selectedArea);
            int selection = readChar("> ");
            mode = MODE_CONFIRMED_CASE;
        }
    }

    return 0;
}

void clearArr(int *arr) {
    for (int i = 0; i < AREA_NUM; i++) {
        arr[i] = 0;
    }
}

void printMenu() {
    printf("1. Confirmed case\n");
    printf("2. Reporting system\n");
    printf("3. Exit\n");
}

char readChar(const char *output) {
    printf("%s", output);
    char result;
    scanf(" %c", &result);
    return result;
}

void printAreaCases(const int *mildCountArr, const int *severedCountArr) {
    for (int i = 0; i < AREA_NUM; i++) {
        int areaTotal = mildCountArr[i] + severedCountArr[i];
        printf("%d : %d\n", i, areaTotal);
    }
}

void printAreaDetail(const int area, const int *mildCountArr, const int *severedCountArr) {
    printf("Mild : %d\n", mildCountArr[area]);
    printf("Severed : %d\n", severedCountArr[area]);
}


void inputNewCase(int *mildCountArr, int *severedCountArr) {
    while (1) {
        printf("Area (0~%d) : ", AREA_NUM);
        char selection = readChar("");
        int area = selection - '0';
        if (area < 0 || area > AREA_NUM) {
            printf("Error! Index out of bound.\n");
            continue;
        }

        char mildOrSevered = readChar("Mild or Severe ('m' or 's') : ");
        if (mildOrSevered != 'm' && mildOrSevered != 's') {
            printf("Error! Unknown option.\n");
            continue;
        }

        char inputNumStr[16];
        printf("The number of confirmed case : ");
        scanf("%s", inputNumStr);
        int num = atoi(inputNumStr);

        addNewCase(area, mildOrSevered, num, mildCountArr, severedCountArr);

        displayAreaDetail(area, mildCountArr, severedCountArr);


        while (1) {
            selection = readChar("Continue to input ('c') or exit ('e') : ");
            if (selection == 'e') {
                return;
            } else if (selection == 'c') {
                break;
            }
        }
    }
}

void addNewCase(int area, char mildOrSevered, int count, int *mildCountArr, int *severedCountArr) {
    if (mildOrSevered == 'm') {
        mildCountArr[area] += count;
    } else if (mildOrSevered == 's') {
        severedCountArr[area] += count;
    }
}

void setLED8(int *input) {
    FILE *file;
    file = fopen(LED8_DRIVER_PATH, "w");
    if (file == NULL) {
        printf("File open error!\n");
        return;
    }
    for (int i = 0; i < AREA_NUM; i++) {
        fputc(input[i] + '0', file);
    }
    fclose(file);
}

void setSeg7(char *str) {
    FILE *file;
    file = fopen(SEG7_DRIVER_PATH, "w");
    if (file == NULL) {
        printf("File open error!\n");
        return;
    }
    for (int i = 0; str[i] != 0; i++) {
        fputc(str[i], file);
    }
    fclose(file);
}

void displayAreaCase(const int *mildCountArr, const int *severedCountArr) {
    int displayArr[AREA_NUM] = {0};
    int total = 0;
    for (int i = 0; i < AREA_NUM; i++) {
        int areaTotal = mildCountArr[i] + severedCountArr[i];
        total += areaTotal;
        if (areaTotal == 0) {
            displayArr[i] = 0;
        } else {
            displayArr[i] = 1;
        }
    }

    char numStr[32] = {0};
    sprintf(numStr, "%dx", total);

    setLED8(displayArr);
    setSeg7(numStr);
}


void displayAreaDetail(const int area, const int *mildCountArr, const int *severedCountArr) {
    int displayArr[AREA_NUM] = {0};
    int blackArr[AREA_NUM] = {0};
    int total = 0;
    for (int i = 0; i < AREA_NUM; i++) {
        if (i == area) {
            displayArr[i] = 1;
        } else {
            displayArr[i] = 0;
        }
        blackArr[i] = 0;
    }

    int areaTotal = mildCountArr[area] + severedCountArr[area];
    char numStr[32];

    sprintf(numStr, "%dx", areaTotal);
    setSeg7(numStr);

    for (int i = 0; i < 3; i++) {
        setLED8(displayArr);
        usleep(500 * 1000);
        setLED8(blackArr);
        usleep(500 * 1000);
    }
    setLED8(displayArr);
}