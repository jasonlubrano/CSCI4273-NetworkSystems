/* 
 * Jason Lubrano
 * CSCI4273 - Network Systems - Dr. Sangtae Ha
 * udp_server.c
 * $ ./udp_server <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFSIZE 1024
#define MSGERRR "\x1b[31m"
#define MSGSUCC "\x1b[32m"
#define MSGNORM "\x1b[0m"
#define MSGTERM "\x1b[35m"
#define MSGWARN "\x1b[33m"

// error message
void error(char *msg){
    perror(msg);
    exit(0);
}

int main(int argc, char **argv){
    // get right number of arguemnts
    if(argc != 2){
        printf(MSGERRR "Wrong number of arguments." MSGNORM "\n");
        printf(MSGERRR "Usage: %s <port>" MSGNORM "\n", argv[0]);
        exit(0);
    }

    // set portno
    int portno = atoi(argv[1]);
    printf(MSGNORM "Port set: " MSGNORM "\t");
    printf(MSGSUCC "%d" MSGNORM "\n", portno);

    // socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        error(MSGERRR "ERROR opening Socket" MSGNORM "\n");
    }

    // setsockopt
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

    // build server intnet address
    struct sockaddr_in serveraddr;
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);

    // bind
    if(bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0){
        error(MSGERRR "ERROR on binding" MSGNORM "\n");
    }

    // main sequence, get info and handle.
    struct sockaddr_in clientaddr;
    int clientlen = sizeof(clientaddr);
    char buff[BUFFSIZE];

    return 0;
}