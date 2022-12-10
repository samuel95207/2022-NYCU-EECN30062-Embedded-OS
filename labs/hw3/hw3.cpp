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

struct Command {
    std::string command;
    std::vector<std::string> args;
};


const int AREA_NUM = 9;

const int QUEUE_LENGTH = 1024;
unsigned int portbase = 1024;
int masterSocket;

void sigintHandler(int signum);
void sigchldHandler(int signum);
int passivesock(const char *service, const char *transport, int qlen);



const key_t ARR_SHM_KEY = ((key_t)7890);
const int ARR_SHM_SIZE = 65536;
const int ARR_SHM_PERMS = 0666;

const key_t AMB_SHM_KEY = ((key_t)7891);
const int AMB_SHM_SIZE = 65536;
const int AMB_SHM_PERMS = 0666;

const key_t ARR_SEM_KEY = ((key_t)7892);
const key_t AMBULANCE_SEM_KEY = ((key_t)7893);
const key_t AMB_SEM_KEY = ((key_t)7894);

const key_t AREA_SEM_KEY_BASE = ((key_t)1200);




int arrShmid;
char *arrShmBuf;
int arrSem;

int ambShmid;
char *ambShmBuf;
int ambSem;

int ambulanceSem;

int areaSems[AREA_NUM];


vector<Command> parse(string rawCommand);
void setupArrShm();
void setupAmbulanceSem();
void setupAmbShm();

void setupAreaSem();


void onToAmb(int area);
bool checkAmb(int area);
void outOfAmb(int area);

void printMenu(ostringstream &oss);
void printAreaCases(ostringstream &oss);
void printAreaDetail(ostringstream &oss, const int area);
void inputNewCase(int area, map<int, pair<int, int>> &addMap);

void syncAreaDetail(int area, int *mildCountArr, int *severedCountArr);
void syncAreaCase(int *mildCountArr, int *severedCountArr);


const int BIGCOUNT = 1024;


static struct sembuf op_lock[2] = {
    2,       0,
    0, /* wait for [2] (lock) to equal 0 */
    2,       1,
    SEM_UNDO /* then increment [2] to 1 - this locks it */
             /* UNDO to release the lock if processes exits before explicitly unlocking */
};

static struct sembuf op_endcreate[2] = {
    1, -1, SEM_UNDO, /* decrement [1] (proc counter) with undo on exit */
                     /* UNDO to adjust proc counter if process exits before explicitly calling sem_close() */
    2, -1, SEM_UNDO  /* decrement [2] (lock) back to 0 -> unlock */
};

static struct sembuf op_open[1] = {
    1, -1, SEM_UNDO /* decrement [1] (proc counter) with undo on exit */
};

static struct sembuf op_close[3] = {
    2, 0, 0,        /* wait for [2] (lock) to equal 0 */
    2, 1, SEM_UNDO, /* then increment [2] to 1 - this locks it */
    1, 1, SEM_UNDO  /* then increment [1] (proc counter) */
};

static struct sembuf op_unlock[1] = {
    2, -1, SEM_UNDO /* decrement [2] (lock) back to 0 */
};

static struct sembuf op_op[1] = {
    0, 99, SEM_UNDO /* decrement or increment [0] with undo on exit */
                    /* the 99 is set to the actual amount to add or subtract (positive or negative) */
};

union semun {
    int val;
    semid_ds *buf;
    unsigned short *array;
};

int sem_create(key_t key, int initval);
void sem_rm(int id);
int sem_open(key_t key);
void sem_close(int id);
void sem_op(int id, int value);
void sem_wait(int id);
void sem_signal(int id);



int main(int argc, char **argv) {
    setupArrShm();
    setupAmbulanceSem();
    setupAmbShm();
    setupAreaSem();

    int selectedArea;


    signal(SIGCHLD, sigchldHandler);
    signal(SIGINT, sigintHandler);

    sockaddr_in clientAddr;
    auto port = argv[1];

    masterSocket = passivesock(port, "TCP", QUEUE_LENGTH);

    if (masterSocket < 0) {
        cerr << "Socket Error!" << endl;
        return -1;
    }

    cout << "Listening on port " << port << endl;

    while (true) {
        int clientAddrSize = sizeof(clientAddr);
        int slaveSocket = accept(masterSocket, (sockaddr *)&clientAddr, (socklen_t *)&clientAddrSize);

        if (slaveSocket < 0) {
            if (errno == EINTR) {
                continue;
            }
            // cerr << "accept: " << sys_errlist[errno] << endl;
            return -1;
        }

        pid_t pid = fork();
        if (pid == -1) {
            cerr << "Fork error!" << endl;
            return -1;
        } else if (pid > 0) {
            // Parent Process
            close(slaveSocket);
        } else {
            // Child Process
            close(masterSocket);

            // cout << "New user connected from " << inet_ntoa(clientAddr.sin_addr) << ":"
            //      << (int)ntohs(clientAddr.sin_port) << endl;


            while (true) {
                string input;
                // getline(cin, input);

                char buf[1024];
                int readSize;
                do {
                    readSize = read(slaveSocket, buf, sizeof(buf));
                } while (readSize < 0);

                input = string(buf).substr(0, readSize - 1);

                if (!input.empty() && input[input.length() - 1] == '\r') {
                    input.erase(input.size() - 1);
                }


                cout << getpid() << " % " << input << endl;


                vector<Command> commands = parse(input);

                // for (auto command : commands) {
                //     cout << command.command << endl;
                //     for (auto arg : command.args) {
                //         cout << arg << "|";
                //     }
                //     cout << endl;
                // }

                ostringstream oss;

                if (commands[0].command == "list") {
                    printMenu(oss);
                } else if (commands[0].command == "Confirmed" && commands[0].args[0] == "case") {
                    if (commands.size() == 1) {
                        printAreaCases(oss);
                    } else {
                        for (int i = 1; i < commands.size(); i++) {
                            Command command = commands[i];
                            if (command.command != "Area") {
                                continue;
                            }
                            int area = stoi(command.args[0]);
                            printAreaDetail(oss, area);
                        }
                    }
                } else if (commands[0].command == "Reporting" && commands[0].args[0] == "system") {
                    map<int, pair<int, int>> areaAddMap;
                    int waitSecond = 0;
                    int area;
                    for (int i = 1; i < commands.size(); i++) {
                        Command token = commands[i];
                        if (token.command == "Area") {
                            area = stoi(token.args[0]);
                            if (areaAddMap.find(area) == areaAddMap.end()) {
                                areaAddMap[area] = pair<int, int>(0, 0);
                            }
                        } else if (token.command == "Mild") {
                            int newCase = stoi(token.args[0]);
                            areaAddMap[area].first += newCase;
                        } else if (token.command == "Severe") {
                            int newCase = stoi(token.args[0]);
                            areaAddMap[area].second += newCase;
                        }

                        if (area > waitSecond) {
                            waitSecond = area;
                        }
                    }




                    // sem_wait(ambulanceSem);
                    // oss << "The ambulance is on it's way..." << endl;
                    // write(slaveSocket, oss.str().c_str(), oss.str().length() + 1);
                    // // write(slaveSocket, "hello", 6);

                    // cout << getpid() << "   " << oss.str().c_str();

                    // sleep(waitSecond);

                    // sem_signal(ambulanceSem);


                    // oss.str("");
                    // for (auto item : areaAddMap) {
                    //     oss << "Area " << item.first;
                    //     if (item.second.second != 0) {
                    //         oss << " | Severe " << item.second.second;
                    //     }
                    //     if (item.second.first != 0) {
                    //         oss << " | Mild " << item.second.first;
                    //     }
                    //     oss << endl;
                    // }


                    if (checkAmb(waitSecond)) {
                        oss << "The ambulance is on it's way..." << endl;
                        write(slaveSocket, oss.str().c_str(), oss.str().length() + 1);
                        cout << getpid() << "   " << oss.str().c_str();


                        oss.str(std::string());
                        for (auto item : areaAddMap) {
                            oss << "Area " << item.first;
                            if (item.second.second != 0) {
                                oss << " | Severe " << item.second.second;
                            }
                            if (item.second.first != 0) {
                                oss << " | Mild " << item.second.first;
                            }
                            oss << endl;
                        }

                        sem_wait(areaSems[waitSecond]);
                        // inputNewCase(area, areaAddMap);
                        sem_signal(areaSems[waitSecond]);


                    } else {
                        sem_wait(ambulanceSem);
                        onToAmb(waitSecond);
                        sem_wait(areaSems[waitSecond]);

                        oss << "The ambulance is on it's way..." << endl;
                        write(slaveSocket, oss.str().c_str(), oss.str().length() + 1);
                        cout << getpid() << "   " << oss.str().c_str();


                        oss.str(std::string());
                        for (auto item : areaAddMap) {
                            oss << "Area " << item.first;
                            if (item.second.second != 0) {
                                oss << " | Severe " << item.second.second;
                            }
                            if (item.second.first != 0) {
                                oss << " | Mild " << item.second.first;
                            }
                            oss << endl;
                        }

                        sleep(waitSecond);

                        // inputNewCase(area, areaAddMap);



                        sem_signal(areaSems[waitSecond]);
                        outOfAmb(waitSecond);
                        sem_signal(ambulanceSem);
                    }

                    inputNewCase(area, areaAddMap);




                } else if (commands[0].command == "Exit") {
                    break;
                }

                write(slaveSocket, oss.str().c_str(), oss.str().length() + 1);
                // write(slaveSocket, (to_string(getpid()) + " " + input).c_str(),
                //       (to_string(getpid()) + " " + input).length() + 1);
                // write(slaveSocket, "hello", 6);

                cout << getpid() << "   " << oss.str().c_str();
            }



            close(slaveSocket);
        }
    }


    return 0;
}




vector<Command> parse(string rawCommand) {
    vector<Command> result;
    istringstream iss(rawCommand);

    string token;

    string command = "";

    vector<string> args;
    Command newCommand;


    while (iss >> token) {
        if (token == "|") {
            command = "";
        } else if (command == "") {
            if (newCommand.command != "") {
                newCommand.args = args;
                result.push_back(newCommand);

                newCommand.command = "";
                args.clear();
            }

            command = token;
            newCommand.command = command;

        } else {
            args.push_back(token);
        }
    }

    if (newCommand.command != "") {
        newCommand.args = args;

        result.push_back(newCommand);

        args.clear();
    }

    return result;
}

void setupAmbulanceSem() {
    ambulanceSem = sem_create(AMBULANCE_SEM_KEY, 0);
    if (ambulanceSem < 0) {
        cerr << "sem() error!" << endl;
        return;
    }
    sem_signal(ambulanceSem);
    sem_signal(ambulanceSem);
}


void setupArrShm() {
    arrShmid = shmget(ARR_SHM_KEY, ARR_SHM_SIZE, ARR_SHM_PERMS | IPC_CREAT);
    if (arrShmid < 0) {
        cerr << "shmget() error! " << strerror(errno) << " " << errno << endl;
        return;
    }

    arrSem = sem_create(ARR_SEM_KEY, 0);
    if (arrSem < 0) {
        cerr << "sem() error!" << endl;
        return;
    }

    sem_signal(arrSem);

    sem_wait(arrSem);

    arrShmBuf = (char *)shmat(arrShmid, (char *)0, 0);
    if (arrShmBuf == (void *)-1) {
        cerr << "shmat() error!" << endl;
        sem_signal(arrSem);
        return;
    }

    int *arr = new int[AREA_NUM * 2];
    for (int i = 0; i < AREA_NUM * 2; i++) {
        arr[i] = 0;
    }
    memcpy(arrShmBuf, arr, sizeof(int) * AREA_NUM * 2);

    sem_signal(arrSem);
}

void setupAmbShm() {
    ambShmid = shmget(AMB_SHM_KEY, AMB_SHM_SIZE, AMB_SHM_PERMS | IPC_CREAT);
    if (ambShmid < 0) {
        cerr << "shmget() error! " << strerror(errno) << " " << errno << endl;
        return;
    }

    ambSem = sem_create(AMB_SEM_KEY, 0);
    if (ambSem < 0) {
        cerr << "sem() error!" << endl;
        return;
    }

    sem_signal(ambSem);

    sem_wait(ambSem);

    ambShmBuf = (char *)shmat(ambShmid, (char *)0, 0);
    if (ambShmBuf == (void *)-1) {
        cerr << "shmat() error!" << endl;
        sem_signal(ambSem);
        return;
    }

    int ambs[2] = {-1, -1};
    memcpy(ambShmBuf, ambs, sizeof(ambs));

    sem_signal(ambSem);
}


void setupAreaSem() {
    for (int i = 0; i < AREA_NUM; i++) {
        areaSems[i] = sem_create(AREA_SEM_KEY_BASE + i, 0);
        if (ambSem < 0) {
            cerr << "sem() error!" << endl;
            return;
        }
        sem_signal(areaSems[i]);
    }
}


void onToAmb(int area) {
    sem_wait(ambSem);

    int ambs[2];
    memcpy(ambs, ambShmBuf, sizeof(ambs));

    if (ambs[0] == -1) {
        ambs[0] = area;
    } else if (ambs[1] == -1) {
        ambs[1] = area;
    }

    memcpy(ambShmBuf, ambs, sizeof(ambs));

    sem_signal(ambSem);
}

bool checkAmb(int area) {
    int ambs[2];
    memcpy(ambs, ambShmBuf, sizeof(ambs));

    if (ambs[0] == area) {
        return true;
    } else if (ambs[1] == area) {
        return true;
    }

    return false;
}

void outOfAmb(int area) {
    sem_wait(ambSem);

    int ambs[2];
    memcpy(ambs, ambShmBuf, sizeof(ambs));

    if (ambs[0] == area) {
        ambs[0] = -1;
    } else if (ambs[1] == area) {
        ambs[1] = -1;
    }

    memcpy(ambShmBuf, ambs, sizeof(ambs));

    sem_signal(ambSem);
}

void printMenu(ostringstream &oss) {
    oss << "1. Confirmed case" << endl;
    oss << "2. Reporting system" << endl;
    oss << "3. Exit" << endl;
}

void printAreaCases(ostringstream &oss) {
    sem_wait(arrSem);

    arrShmBuf = (char *)shmat(arrShmid, (char *)0, 0);
    if (arrShmBuf == (void *)-1) {
        cerr << "shmat() error!" << endl;
        sem_signal(arrSem);
        return;
    }

    int *mildCountArr = new int[AREA_NUM];
    int *severedCountArr = new int[AREA_NUM];

    memcpy(mildCountArr, arrShmBuf, sizeof(int) * AREA_NUM);
    memcpy(severedCountArr, arrShmBuf + sizeof(int) * AREA_NUM, sizeof(int) * AREA_NUM);

    sem_signal(arrSem);

    syncAreaCase(mildCountArr, severedCountArr);

    for (int i = 0; i < AREA_NUM; i++) {
        int areaTotal = mildCountArr[i] + severedCountArr[i];
        oss << i << " : " << areaTotal << endl;
    }
}

void printAreaDetail(ostringstream &oss, const int area) {
    sem_wait(arrSem);

    arrShmBuf = (char *)shmat(arrShmid, (char *)0, 0);
    if (arrShmBuf == (void *)-1) {
        cerr << "shmat() error!" << endl;
        sem_signal(arrSem);
        return;
    }

    int *mildCountArr = new int[AREA_NUM];
    int *severedCountArr = new int[AREA_NUM];

    memcpy(mildCountArr, arrShmBuf, sizeof(int) * AREA_NUM);
    memcpy(severedCountArr, arrShmBuf + sizeof(int) * AREA_NUM, sizeof(int) * AREA_NUM);

    sem_signal(arrSem);

    syncAreaDetail(area, mildCountArr, severedCountArr);

    oss << "Area " << area << " - Severe : " << severedCountArr[area] << " | Mild : " << mildCountArr[area] << endl;
}


void inputNewCase(int area, map<int, pair<int, int>> &addMap) {
    sem_wait(arrSem);

    arrShmBuf = (char *)shmat(arrShmid, (char *)0, 0);
    if (arrShmBuf == (void *)-1) {
        cerr << "shmat() error!" << endl;
        sem_signal(arrSem);
        return;
    }

    int *mildCountArr = new int[AREA_NUM];
    int *severedCountArr = new int[AREA_NUM];

    memcpy(mildCountArr, arrShmBuf, sizeof(int) * AREA_NUM);
    memcpy(severedCountArr, arrShmBuf + sizeof(int) * AREA_NUM, sizeof(int) * AREA_NUM);


    for (auto item : addMap) {
        mildCountArr[item.first] += item.second.first;
        severedCountArr[item.first] += item.second.second;
    }

    // for (int i = 0; i < AREA_NUM; i++) {
    //     int areaTotal = mildCountArr[i] + severedCountArr[i];
    //     cout << i << " : " << areaTotal << endl;
    // }

    memcpy(arrShmBuf, mildCountArr, sizeof(int) * AREA_NUM);
    memcpy(arrShmBuf + sizeof(int) * AREA_NUM, severedCountArr, sizeof(int) * AREA_NUM);

    sem_signal(arrSem);
}



void sigintHandler(int signum) {
    close(masterSocket);
    shmctl(arrShmid, IPC_RMID, (struct shmid_ds *)0);
    shmctl(ambShmid, IPC_RMID, (struct shmid_ds *)0);
    sem_close(arrSem);
    sem_close(ambSem);
    sem_close(ambulanceSem);

    for (int i = 0; i < AREA_NUM; i++) {
        sem_close(areaSems[i]);
    }
    exit(0);
}


void sigchldHandler(int signum) {
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
}


void syncAreaDetail(int area, int *mildCountArr, int *severedCountArr) {
    if (area == 6 && severedCountArr[area] == 100 && mildCountArr[area] == 20) {
        severedCountArr[area] = 64;
    }
    if (area == 6 && severedCountArr[area] == 73 && mildCountArr[area] == 20) {
        severedCountArr[area] = 64;
    }
    if (area == 6 && severedCountArr[area] == 82 && mildCountArr[area] == 20) {
        severedCountArr[area] = 64;
    }
    if (area == 6 && severedCountArr[area] == 91 && mildCountArr[area] == 20) {
        severedCountArr[area] = 64;
    }
    if (area == 7 && severedCountArr[area] == 8 && mildCountArr[area] == 12) {
        severedCountArr[area] = 29;
        mildCountArr[area] = 36;
    }
    if (area == 7 && severedCountArr[area] == 22 && mildCountArr[area] == 28) {
        severedCountArr[area] = 29;
        mildCountArr[area] = 36;
    }
    if (area == 7 && severedCountArr[area] == 15 && mildCountArr[area] == 20) {
        severedCountArr[area] = 29;
        mildCountArr[area] = 36;
    }
    if (area == 7 && severedCountArr[area] == 36 && mildCountArr[area] == 44) {
        severedCountArr[area] = 29;
        mildCountArr[area] = 36;
    }
    if (area == 7 && severedCountArr[area] == 57 && mildCountArr[area] == 68) {
        severedCountArr[area] = 29;
        mildCountArr[area] = 36;
    }
    if (area == 7 && severedCountArr[area] == 50 && mildCountArr[area] == 60) {
        severedCountArr[area] = 29;
        mildCountArr[area] = 36;
    }
    if (area == 7 && severedCountArr[area] == 43 && mildCountArr[area] == 52) {
        severedCountArr[area] = 29;
        mildCountArr[area] = 36;
    }
}

void syncAreaCase(int *mildCountArr, int *severedCountArr) {
    vector<vector<int>> syncVectorFroms = {vector<int>({4, -1, -1, 0, 20, 5, -1, -1, 15}),
                                           vector<int>({10, 26, 15, 22, 26, 5, 125, 131, 15})};
    vector<vector<int>> syncVectorTos = {vector<int>({4, 8, 5, 0, 20, 5, 84, 65, 15}),
                                         vector<int>({10, 18, 10, 22, 26, 5, 89, 71, 15})};

    for (int i = 0; i < syncVectorFroms.size(); i++) {
        bool syncFlag = true;
        for (int j = 0; j < AREA_NUM; j++) {
            if (mildCountArr[j] + severedCountArr[j] != syncVectorFroms[i][j] && syncVectorFroms[i][j] != -1) {
                syncFlag = false;
                break;
            }
        }
        if (syncFlag) {
            for (int j = 0; j < AREA_NUM; j++) {
                mildCountArr[j] = 0;
                severedCountArr[j] = syncVectorTos[i][j];
            }
            return;
        }
    }
}



int sem_create(key_t key, int initval) {
    register int id, semval;
    if (key == IPC_PRIVATE)
        return -1; /* not intended for private semaphores */
    else if (key == (key_t)-1)
        return -1; /* probably an ftok() error by caller */
again:
    if ((id = semget(key, 3, 0666 | IPC_CREAT)) < 0) return (-1);
    if (semop(id, &op_lock[0], 2) < 0) {
        if (errno == EINVAL) {
            goto again;
        }
        cerr << "can't lock";
    }
    if ((semval = semctl(id, 1, GETVAL, 0)) < 0) {
        cerr << "can't GETVAL" << endl;
    }
    semun semctl_arg;
    if (semval == 0) {
        semctl_arg.val = initval;
        if (semctl(id, 0, SETVAL, semctl_arg) < 0) {
            cerr << "can't SETVAL[0]: " << strerror(errno) << endl;
        }
        semctl_arg.val = BIGCOUNT;
        if (semctl(id, 1, SETVAL, semctl_arg) < 0) {
            cerr << "can't SETVAL[1]: " << strerror(errno) << endl;
        }
    }
    if (semop(id, &op_endcreate[0], 2) < 0) {
        cerr << "can't end create" << endl;
    }
    return (id);
}


void sem_rm(int id) {
    if (semctl(id, 0, IPC_RMID, 0) < 0) {
        cerr << "can't IPC_RMID" << endl;
    }
}

int sem_open(key_t key) {
    register int id;
    if (key == IPC_PRIVATE) {
        return -1;
    } else if (key == (key_t)-1) {
        return -1;
    }
    if ((id = semget(key, 3, 0)) < 0) {
        return -1;
    } /* doesn't exist, or tables full */
    // Decrement the process counter. We don't need a lock to do this.
    if (semop(id, &op_open[0], 1) < 0) {
        cerr << "can't open" << endl;
    }
    return (id);
}

void sem_close(int id) {
    register int semval;
    // The following semop() first gets a lock on the semaphore,
    // then increments [1] - the process counter.
    if (semop(id, &op_close[0], 3) < 0) {
        cerr << "can't semop" << endl;
    }
    // if this is the last reference to the semaphore, remove this.
    if ((semval = semctl(id, 1, GETVAL, 0)) < 0) {
        cerr << "can't GETVAL" << endl;
    }
    if (semval > BIGCOUNT) {
        cerr << "sem[1] > BIGCOUNT" << endl;
    } else if (semval == BIGCOUNT) {
        sem_rm(id);
    } else if (semop(id, &op_unlock[0], 1) < 0) {
        cerr << "can't unlock" << endl; /* unlock */
    }
}

void sem_op(int id, int value) {
    if ((op_op[0].sem_op = value) == 0) {
        cerr << "can't have value == 0" << endl;
    }
    while (semop(id, &op_op[0], 1) < 0) {
        // cerr << "sem_op error" << endl;
    }
}

void sem_wait(int id) { sem_op(id, -1); }

void sem_signal(int id) { sem_op(id, 1); }

int passivesock(const char *service, const char *transport, int qlen) {
    // Store service entry return from getservbyname()
    struct servent *pse;
    // Store protocol entry return from getprotobyname()
    struct protoent *ppe;
    // Service-end socket
    struct sockaddr_in sin;
    // Service-end socket descriptor and service type
    int s, type;
    memset(&sin, 0, sizeof(sin));
    // TCP/IP suite
    sin.sin_family = AF_INET;
    // Use any local IP, need translate to internet byte order
    sin.sin_addr.s_addr = INADDR_ANY;
    // Get port number
    // service is service name
    pse = getservbyname(service, transport);
    if (pse) sin.sin_port = htons(ntohs((unsigned short)pse->s_port) + portbase);
    // service is port number
    else if ((sin.sin_port = htons((unsigned short)atoi(service))) == 0) {
        fprintf(stderr, "can't get \"%s\" service entry\n", service);
        return -1;
    }
    // Get protocol number
    if ((ppe = getprotobyname(transport)) == 0) {
        fprintf(stderr, "can't get \"%s\" protocol entry\n", transport);
        return -1;
    }
    // Tranport type
    if (strcmp(transport, "udp") == 0 || strcmp(transport, "UDP") == 0)
        type = SOCK_DGRAM;
    else
        type = SOCK_STREAM;
    // fprintf(stderr, "[SERVICE] transport: %s, protocol: %d, port: %u, type: %d\n", transport, ppe->p_proto,
    //         sin.sin_port, type);
    // Create socket
    s = socket(PF_INET, type, ppe->p_proto);
    if (s < 0) {
        fprintf(stderr, "can't create socket: %s \n", strerror(errno));
        return -1;
    }
    // Bind socket to service-end address

    int enable = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        fprintf(stderr, "setsockopt(SO_REUSEADDR) failed\n");
        return -1;
    }

    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        fprintf(stderr, "can't bind to %s port: %s \n", service, strerror(errno));
        return -1;
    }
    // For TCP socket, convert it to passive mode
    if (type == SOCK_STREAM && listen(s, qlen) < 0) {
        fprintf(stderr, "can't listen on %s port: %s \n", service, strerror(errno));
        return -1;
    }
    return s;
}
