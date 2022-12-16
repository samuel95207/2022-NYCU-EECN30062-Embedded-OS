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
#include <sys/time.h>
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
void timerHandler(int signum);


int upperBound;
int lowerBound = 0;
int pid;


void makeGuess();

int main(int argc, char **argv) {
    SHM_KEY = (key_t)stoi(string(argv[1]));
    upperBound = stoi(string(argv[2]));
    pid = stoi(string(argv[3]));

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



    struct sigaction guessAction;
    /* register handler to SIGUSR1 */
    memset(&guessAction, 0, sizeof(struct sigaction));
    guessAction.sa_flags = SA_SIGINFO;
    guessAction.sa_sigaction = sigGuessHandler;
    sigaction(SIGUSR1, &guessAction, NULL);

    struct sigaction timerAction;
    struct itimerval timer;
    /* Install timer_handler as the signal handler for SIGVTALRM */
    memset(&timerAction, 0, sizeof(timerAction));
    timerAction.sa_handler = &timerHandler;
    sigaction(SIGVTALRM, &timerAction, NULL);
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;
    /* Start a virtual timer */
    setitimer(ITIMER_VIRTUAL, &timer, NULL);

    while (true) {
    }
}


void makeGuess() {
    data guessData;
    guessData.guess = (lowerBound + upperBound) / 2;
    cout << "[Guess] " << guessData.guess << endl;
    memcpy(shmBuf, &guessData, sizeof(data));
    kill(pid, SIGUSR1);
}


void sigGuessHandler(int signum, siginfo_t *info, void *context) {
    data guessData;
    memcpy(&guessData, shmBuf, sizeof(data));

    string result = string(guessData.result);

    if (result == "larger") {
        lowerBound = guessData.guess;
    } else if (result == "smaller") {
        upperBound = guessData.guess;
    } else if (result == "bingo") {
        exit(0);
    }


    // cout << result << " " << lowerBound << " " << upperBound << endl;
}


void timerHandler(int signum) { makeGuess(); }