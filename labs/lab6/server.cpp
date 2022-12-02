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


using namespace std;


void sigchldHandler(int signum);
int passivesock(const char* service, const char* transport, int qlen);



const key_t SEM_KEY = ((key_t)7891);
const int QUEUE_LENGTH = 1024;
const int BUF_SIZE = 65536;
unsigned int portbase = 1024;
int masterSocket;

int sem;



int main(int argc, char** argv) {
    signal(SIGCHLD, sigchldHandler);


    int account = 0;

    auto port = argv[1];

    sockaddr_in clientAddr;
    fd_set readFds;
    fd_set activeFds;


    masterSocket = passivesock(port, "TCP", QUEUE_LENGTH);
    if (masterSocket < 0) {
        return -1;
    }

    // cout << "Listening on port " << port << endl;

    int numFds = getdtablesize();
    FD_ZERO(&activeFds);
    FD_SET(masterSocket, &activeFds);


    while (true) {
        memcpy(&readFds, &activeFds, sizeof(readFds));

        if (select(numFds, &readFds, (fd_set*)0, (fd_set*)0, (timeval*)0) < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "select: %s\n", strerror(errno));
            continue;
        }


        if (FD_ISSET(masterSocket, &readFds)) {
            int clientAddrSize = sizeof(clientAddr);
            int slaveSocket = accept(masterSocket, (struct sockaddr*)&clientAddr, (socklen_t*)&clientAddrSize);

            if (slaveSocket < 0) {
                fprintf(stderr, "accept: %s\n", strerror(errno));
                return -1;
            }

            FD_SET(slaveSocket, &activeFds);


            // cout << "New user connected from " << inet_ntoa(clientAddr.sin_addr) << ":"
            //      << (int)ntohs(clientAddr.sin_port) << endl;
        }

        for (int fd = 0; fd < numFds; fd++) {
            if (fd == masterSocket || !FD_ISSET(fd, &readFds)) {
                continue;
            }

            char buf[BUF_SIZE];
            int readSize;


            readSize = read(fd, buf, sizeof(buf));

            if (readSize <= 0) {
                break;
            }


            string inStr = string(buf).substr(0, readSize - 1);
            if (!inStr.empty() && inStr[inStr.length() - 1] == '\r') {
                inStr.erase(inStr.size() - 1);
            }

            // cout << "readSize = " << readSize << endl;
            // cout << "input = " << inStr << endl;
            // cout << "size = " << inStr.length() << endl;

            istringstream iss;
            iss.str(inStr);

            string line;
            while (getline(iss, line)) {
                // cout << line << endl;
                istringstream issLine;
                issLine.str(line);

                string selection;

                issLine >> selection;


                if (selection == "exit") {
                    close(fd);
                    FD_CLR(fd, &activeFds);

                    // cout << "account: " << account << endl;

                    break;

                } else if (selection == "deposit") {
                    int amount;
                    issLine >> amount;
                    account += amount;
                    cout << "After deposit: " << account << endl;

                } else if (selection == "withdraw") {
                    int amount;
                    issLine >> amount;
                    account -= amount;
                    cout << "After withdraw: " << account << endl;
                } else {
                    cout << "Error: " << line << endl;
                }
            }
        }
    }
}



void sigchldHandler(int signum) {
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
}


int passivesock(const char* service, const char* transport, int qlen) {
    // Store service entry return from getservbyname()
    struct servent* pse;
    // Store protocol entry return from getprotobyname()
    struct protoent* ppe;
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

    if (bind(s, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
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
