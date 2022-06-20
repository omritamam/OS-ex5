#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <netdb.h>
#include <iostream>
#include <unistd.h>
#include <errno.h>

#define MAXHOSTNAME 256
#define MAX_LISTEN_QUEUE 5
#define BUFFER_SIZE 256

static const int STOP_CHAR = '\0';

int establishServer(unsigned short portnum) {
    char myname[MAXHOSTNAME+1];
    int socketFd;
    struct sockaddr_in sa;
    struct hostent *hp;
    //hostnet initialization
    gethostname(myname, MAXHOSTNAME);
    hp = gethostbyname(myname);
    if (hp == NULL)
        return(-1);
    //sockaddrr_in initlization
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = hp->h_addrtype;
    /* this is our host address */
    memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
    /* this is our port number */
    sa.sin_port= htons(portnum);
    /* create socket */
    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0)
        return(-1);
    auto rt = bind(socketFd, (struct sockaddr *)&sa, sizeof(struct sockaddr));
    if (rt == -1) {
        int errsv = errno;
        printf("bind error num %d failed\n", errsv);
    }
    if (rt < 0) {
        close(socketFd);
        return(-1);
    }
    listen(socketFd, MAX_LISTEN_QUEUE);
    return socketFd;
}

int get_connection(int s) {
    int t; /* socket of connection */
    if ((t = accept(s,NULL,NULL)) < 0)
        return -1;
    return t;
}

int call_socket(char *hostname, unsigned short portnum) {
    struct sockaddr_in sa;
    struct hostent *hp;
    int s;
    if ((hp= gethostbyname (hostname)) == NULL) {
        return(-1);
    }
    memset(&sa,0,sizeof(sa));
    memcpy((char *)&sa.sin_addr , hp->h_addr ,
           hp->h_length);
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((u_short)portnum);
    if ((s = socket(hp->h_addrtype,
                    SOCK_STREAM,0)) < 0) {
        return(-1);
    }
    if (connect(s, (struct sockaddr *)&sa , sizeof(sa)) < 0) {
        close(s);
        return(-1);
    }
    return(s);
}


int readFromSocketToBuffer(int socketFD, char *buf, int bufferSize) {
    int counter = 0, charRead;
    while (counter < bufferSize) {
        charRead = (int) read(socketFD, buf, bufferSize - counter);
        int stop = STOP_CHAR;
        if (charRead > 0) {
            counter += charRead;
            buf += charRead;
            if ((buf - 1)[0] == stop) {
                break;
            }
        }
        if (charRead < 1) {
            fprintf(stderr, "system error: could not read data\n");
            exit(1);
        }
    }
    return counter;
}


std::string getCommandFromArgs(int argc, char* argv[]){
    std::string command;
    for (auto i=3; i<argc; i++){
        command += (std::string)argv[i];
        command += " ";
    }
    return command;
}

int establishClient(char *hostname, unsigned short port){
    struct sockaddr_in sa{};
    struct hostent *hp;
    int clientFd;

    if ((hp= gethostbyname (hostname)) == NULL) {
        fprintf(stderr, "system error: client:\n");
        exit(1);
    }

    memset(&sa,0,sizeof(sa));
    memcpy((char *)&sa.sin_addr, hp->h_addr, hp->h_length);
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons(port);
    clientFd = socket(hp->h_addrtype, SOCK_STREAM,0);
    if (clientFd < 0) {
        fprintf(stderr, "system error: client:\n");
        exit(1);
    }
    auto rt = connect(clientFd, (struct sockaddr *)&sa, sizeof(sa));
    if (rt < 0) {
        fprintf(stderr, "system error: client: \n");
        close(clientFd);
        exit(1);
    }

    return clientFd;
}


// usage: ./sockets client <port> <terminal_command_to_run>
// usage: ./sockets server <port>
int main(int argc, char* argv[]) {
    auto type = argv[1];
    auto port  = argv[2];
    if(strcmp(type, "client") == 0) {
        //Client
        char myname[MAXHOSTNAME+1];
        gethostname(myname, MAXHOSTNAME);
        int clientFd = establishClient(myname, strtol(port, nullptr, 10));
        auto command = getCommandFromArgs(argc, argv);

        char buf[BUFFER_SIZE];
        strcpy(buf, command.c_str());
        if (write(clientFd, buf, command.length()+1) < 1){
            fprintf(stderr, "system error:\n");
            exit(1);
        }

    }

    else{
        //Server - create main server
        int mainServerFd = establishServer(strtol(port, nullptr, 10));
        //listen for connections -  create a new socket for each connection
        while(true){
            int currentServerFd;
            if ((currentServerFd = accept(mainServerFd,NULL,NULL)) < 0){
                fprintf(stderr, "system error: %s\n", strerror(errno));
                exit(1);
            }
            char buffer[BUFFER_SIZE];
            size_t commandLength = readFromSocketToBuffer(currentServerFd, buffer, BUFFER_SIZE);
            char command[commandLength];
            memcpy(command, buffer, commandLength);
            system(command);
            close(currentServerFd);
        }
    }
    return 0;
}