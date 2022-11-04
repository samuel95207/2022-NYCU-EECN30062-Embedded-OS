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
#include <string>

using namespace std;

const int QUEUE_LENGTH = 1024;
unsigned int portbase = 1024;
int masterSocket;


void sigintHandler(int signum);
void sigchldHandler(int signum);
int passivesock(const char* service, const char* transport, int qlen);


int main(int argc, char* argv[]) {
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
        int slaveSocket = accept(masterSocket, (sockaddr*)&clientAddr, (socklen_t*)&clientAddrSize);

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


            int savedStdout = dup(fileno(stdout));
            int savedStderr = dup(fileno(stderr));
            int savedStdin = dup(fileno(stdin));
            dup2(slaveSocket, fileno(stdout));
            dup2(slaveSocket, fileno(stderr));
            dup2(slaveSocket, fileno(stdin));


            execlp("sl", "sl", "-l", nullptr);


            dup2(savedStdout, fileno(stdout));
            dup2(savedStderr, fileno(stderr));
            dup2(savedStdin, fileno(stdin));
            close(savedStdout);
            close(savedStderr);
            close(savedStdin);

            close(slaveSocket);
        }
    }
}

void sigintHandler(int signum) { close(masterSocket); }


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
