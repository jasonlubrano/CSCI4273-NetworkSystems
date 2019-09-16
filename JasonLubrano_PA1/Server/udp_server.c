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

void errorcheckrecv(int nn){
    if(nn < 0){
        error("ERROR in recvfrom");
    }
}

void errorchecksend(int nn){
    if(nn < 0){
        error("ERROR in sendto");
    }
}

// handles geting input
void buffinput(char* buffcmd){
    // get input from user
    char buf[BUFFSIZE];
    bzero(buf, BUFFSIZE);
    printf(MSGTERM "Enter command" MSGNORM "\n");
    printf("cmd> ");
    fgets(buf, BUFFSIZE, stdin);
    int i=0;
    for(i=0; i<=strlen(buf); i++){
        buffcmd[i] = buf[i];
    }
    buffcmd[strlen(buffcmd)-1] = '\0';
}

// handles parsing the commands
void buffparse(char *buf, char *argcmd, char *argfile){
    // parse input from user
    // TODO: make into function
    memset(argcmd, 0, BUFFSIZE * (sizeof(char)));
    memset(argfile, 0, BUFFSIZE * (sizeof(char)));
    char delim[] = " ,.-";
    char argt[BUFFSIZE];
    char *clientcmd;
    char *filename;
    char buf2[BUFFSIZE];
    strncpy(buf2, buf, strlen(buf));
    //printf(MSGSUCC "input: %s, len: %ld" MSGNORM "\n", buff2, strlen(buff2));
    // get cmd
    clientcmd = strtok(buf, delim);
    size_t clicmdlen = strlen(clientcmd);
    strncpy(argcmd, clientcmd, clicmdlen);
    if(strcmp(argcmd, "ls") == 0){
        filename = strtok(buf2, delim);
        size_t fnamelen = strlen(filename);
        strncpy(argfile, filename, fnamelen);
    } else if(strcmp(argcmd, "exit") == 0){
        filename = strtok(buf2, delim);
        size_t fnamelen = strlen(filename);
        strncpy(argfile, filename, fnamelen);
    } else {
        int i = 0;
        // get filename
        filename = strtok(buf2, delim);
        size_t fnamelen = strlen(filename);
        while(filename != NULL){
            ++i;
            if(i > 2){
                error(MSGERRR "Too many arguments" MSGNORM "\n");
            }
            size_t fnamelen = strlen(filename);
            strncpy(argt, filename, strlen(filename));
            filename = strtok(NULL, delim);
        }
        size_t argtlen = strlen(argt);
        strncpy(argfile, argt, argtlen);
        memset(argt, 0, BUFFSIZE * (sizeof(char)));
    }
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
    
    // client cmd and/or filename
    char buff[BUFFSIZE]; // general buffer message
    char argb[BUFFSIZE]; // command
    char argf[BUFFSIZE]; // filename
   
    // basic messages for debugging packets
    char ready[BUFFSIZE] = "Ready to connect";
    char acknoledge[BUFFSIZE] = "Message received";

    // should be standard ready message
    int n = recvfrom(sockfd, buff, BUFFSIZE, 0,
                (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0){
      error("ERROR in recvfrom");
    }
    printf(MSGSUCC "Echo from client: %s\n" MSGNORM, buff);

    struct hostent *hostp; /* client host info */
    char *hostaddrp; /* dotted decimal host addr string */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL){
      error("ERROR on inet_ntoa\n");
    }
    printf("server received datagram from %s (%s)\n", hostp->h_name, hostaddrp);
    printf("server received %ld/%d bytes: %s\n", strlen(buff), n, buff);
    
    n = sendto(sockfd, acknoledge, sizeof(acknoledge), MSG_WAITALL, 
                (struct sockaddr *) &clientaddr, clientlen);

    printf(MSGSUCC "Sent to client: %s, n= %d\n" MSGNORM, buff, n);

    // state machine
    do{
        n = recvfrom(sockfd, argb, BUFFSIZE, 0,
                    (struct sockaddr *) &clientaddr, &clientlen);
        errorcheckrecv(n);
        printf("Echo from client: %s, n = %d \n", argb, n);
        n = sendto(sockfd, acknoledge, sizeof(acknoledge), MSG_WAITALL, 
                    (struct sockaddr *) &clientaddr, clientlen);
        errorchecksend(n);
        n = recvfrom(sockfd, argf, BUFFSIZE, 0,
                    (struct sockaddr *) &clientaddr, &clientlen);
        errorcheckrecv(n);
        printf("Echo from client: %s, n = %d \n", argf, n);
        n = sendto(sockfd, acknoledge, sizeof(acknoledge), MSG_WAITALL, 
                (struct sockaddr *) &clientaddr, clientlen);
        errorchecksend(n);
        
        // server handle cmds
        if(strcmp(argb, "puts") == 0){
            printf(MSGTERM "erver saving file" MSGNORM "\n");
            /* message handling */
        } else if (strcmp(argb, "gets") == 0){
            printf(MSGTERM "Server retrieving file" MSGNORM "\n");
            // handle getting file from server
            // handle getting input again
        } else if (strcmp(argb, "ls") == 0){
            printf(MSGTERM "Server getting file list" MSGNORM "\n");
            // handle listing files from server
            // h  andle getting input again
        } else {
            // handle getting input again
            printf(MSGERRR "NOT VALID INPUT" MSGNORM "\n");
            printf(MSGWARN "Commands: puts <filename>, gets <filename>, ls, exit" MSGNORM "\n");
        }
    }while(strcmp(argb, "exit") != 0);

    return 0;
}