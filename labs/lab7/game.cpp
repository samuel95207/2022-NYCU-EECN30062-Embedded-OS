#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace std;

typedef struct {
    int guess;
    char result[8];
} data;

const int SHM_SIZE = 65536;
const int SHM_PERMS = 0666;
key_t SHM_KEY;
int shmid;
int answer;
char *shmBuf;


void sigintHandler(int signum);
void sigGuessHandler(int signum, siginfo_t *info, void *context);



int main(int argc, char **argv) {
    cout << "[Game] Game PID: " << getpid() << endl;

    SHM_KEY = (key_t)stoi(string(argv[1]));
    answer = stoi(string(argv[2]));

    shmid = shmget(SHM_KEY, SHM_SIZE, SHM_PERMS | IPC_CREAT);
    if (shmid < 0) {
        cerr << "shmget() error! " << strerror(errno) << " " << errno << endl;
        return -1;
    }
    shmBuf = (char *)shmat(shmid, (char *)0, 0);
    if (shmBuf == (void *)-1) {
        cerr << "shmat() error!" << endl;
        return -1;
    }

    signal(SIGINT, sigintHandler);


    struct sigaction my_action;
    /* register handler to SIGUSR1 */
    memset(&my_action, 0, sizeof(struct sigaction));
    my_action.sa_flags = SA_SIGINFO;
    my_action.sa_sigaction = sigGuessHandler;
    sigaction(SIGUSR1, &my_action, NULL);

    while (true) {
    }
}


void sigintHandler(int signum) {
    shmctl(shmid, IPC_RMID, (struct shmid_ds *)0);
    exit(0);
}

void sigGuessHandler(int signum, siginfo_t *info, void *context) {
    data guessData;
    memcpy(&guessData, shmBuf, sizeof(data));

    // cout << "Sender " << info->si_pid << endl;
    cout << "[Game] Guess " << guessData.guess << ", ";

    if (guessData.guess > answer) {
        strcpy(guessData.result, "smaller");
        cout << "smaller";
    } else if (guessData.guess < answer) {
        strcpy(guessData.result, "larger");
        cout << "larger";
    } else {
        strcpy(guessData.result, "bingo");
        cout << "bingo";
    }

    cout << endl;

    memcpy(shmBuf, &guessData, sizeof(data));

    kill(info->si_pid, SIGUSR1);
}