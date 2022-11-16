#include <arpa/inet.h>
#include <errno.h>
#include <memory.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
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


vector<Command> parse(string rawCommand);
void clearArr(int *arr);
void printMenu(ostringstream &oss);
void printAreaCases(ostringstream &oss, const int *mildCountArr, const int *severedCountArr);
void printAreaDetail(ostringstream &oss, const int area, const int *mildCountArr, const int *severedCountArr);
void inputNewCase(int area, string type, int newCase, int *mildCountArr, int *severedCountArr);
void addNewCase(int area, char mildOrSevered, int count, int *mildCountArr, int *severedCountArr);




int main(int argc, char **argv) {
    int mildCountArr[AREA_NUM];
    int severedCountArr[AREA_NUM];
    int selectedArea;

    clearArr(mildCountArr);
    clearArr(severedCountArr);


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

            cout << "New user connected from " << inet_ntoa(clientAddr.sin_addr) << ":"
                 << (int)ntohs(clientAddr.sin_port) << endl;


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

                // cout << input << endl;


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
                        printAreaCases(oss, mildCountArr, severedCountArr);
                    } else {
                        for (int i = 1; i < commands.size(); i++) {
                            Command command = commands[i];
                            if (command.command != "Area") {
                                continue;
                            }
                            int area = stoi(command.args[0]);
                            printAreaDetail(oss, area, mildCountArr, severedCountArr);
                        }
                    }
                } else if (commands[0].command == "Reporting" && commands[0].args[0] == "system") {
                    int waitSecond = 0;
                    for (int i = 1; i < commands.size(); i += 2) {
                        Command command1 = commands[i];
                        Command command2 = commands[i + 1];
                        if (command1.command != "Area") {
                            continue;
                        }
                        int area = stoi(command1.args[0]);
                        string type = command2.command;
                        int newCase = stoi(command2.args[0]);
                        inputNewCase(area, type, newCase, mildCountArr, severedCountArr);

                        if (area > waitSecond) {
                            waitSecond = area;
                        }
                    }

                    oss << "Please wait a few seconds..." << endl;

                    write(slaveSocket, oss.str().c_str(), oss.str().length() + 1);

                    oss.str(std::string());
                    for (int i = 1; i < commands.size(); i += 2) {
                        oss << commands[i].command << " " << commands[i].args[0] << " | " << commands[i + 1].command
                            << " " << commands[i + 1].args[0] << endl;
                    }
                    sleep(waitSecond);




                } else if (commands[0].command == "Exit") {
                    break;
                }

                write(slaveSocket, oss.str().c_str(), oss.str().length() + 1);
            }



            close(slaveSocket);
        }
    }


    return 0;
}

void sigintHandler(int signum) { close(masterSocket); }


void sigchldHandler(int signum) {
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
}

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

void clearArr(int *arr) {
    for (int i = 0; i < AREA_NUM; i++) {
        arr[i] = 0;
    }
}

void printMenu(ostringstream &oss) {
    oss << "1. Confirmed case" << endl;
    oss << "2. Reporting system" << endl;
    oss << "3. Exit" << endl;
}

void printAreaCases(ostringstream &oss, const int *mildCountArr, const int *severedCountArr) {
    for (int i = 0; i < AREA_NUM; i++) {
        int areaTotal = mildCountArr[i] + severedCountArr[i];
        oss << i << " : " << areaTotal << endl;
    }
}

void printAreaDetail(ostringstream &oss, const int area, const int *mildCountArr, const int *severedCountArr) {
    oss << "Area " << area << " - Mild : " << mildCountArr[area] << " | Severe : " << severedCountArr[area] << endl;
}


void inputNewCase(int area, string type, int newCase, int *mildCountArr, int *severedCountArr) {
    int *arr;
    if (type == "Mild") {
        arr = mildCountArr;
    } else {
        arr = severedCountArr;
    }
    arr[area] += newCase;
}

void addNewCase(int area, char mildOrSevered, int count, int *mildCountArr, int *severedCountArr) {
    if (mildOrSevered == 'm') {
        mildCountArr[area] += count;
    } else if (mildOrSevered == 's') {
        severedCountArr[area] += count;
    }
}
