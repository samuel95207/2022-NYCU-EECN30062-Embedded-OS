#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>

using namespace std;

int main(int argc, char** argv) {
    auto ip = argv[1];
    int port = (u_short)atoi(argv[2]);
    auto command = argv[3];
    int ammount = atoi(argv[4]);
    int loopCount = atoi(argv[5]);




    // connect
    for (int i = 0; i < loopCount; i++) {
        int connfd = socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in client_sin;
        memset(&client_sin, 0, sizeof(client_sin));
        client_sin.sin_family = AF_INET;
        client_sin.sin_addr.s_addr = inet_addr(ip);
        client_sin.sin_port = htons(port);
        while (connect(connfd, (struct sockaddr*)&client_sin, sizeof(client_sin)) == -1) {
            printf("Error connect to server\n");
        }
        ostringstream oss;
        oss << command << " " << ammount << "\n";
        string output = oss.str();
        write(connfd, output.c_str(), output.length());
        output = "exit\n";
        write(connfd, output.c_str(), output.length());
        close(connfd);
    }


    return 0;
}