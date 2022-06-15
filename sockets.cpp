#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>
#include <armadillo>

#define MAXHOSTNAME 256


int establish(unsigned short portnum) {
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
    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return(-1);

    if (bind(socketFd , (struct sockaddr *)&sa , sizeof(struct sockaddr_in)) < 0) {
        close(socketFd);
        return(-1);
    }
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

    // usage: ./sockets client <port> <terminal_command_to_run>
// usage: ./sockets server <port>
int main(int argc, char* argv[]) {
    auto type = argv[0];
    auto port  = argv[1];
    if(strcmp(type, "client") == 0) {
        auto commandToRun = argv[2];
    }
    int socketFd = establish(strtol(port, nullptr, 10));
    int newSocket = get_connection(socketFd);

    return 0;


}