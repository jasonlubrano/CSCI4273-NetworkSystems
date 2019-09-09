/*
 * Jason Lubrano
 * CSCI4273 - Network Systems - Dr. Sangtae Ha
 * udp_client.c
 * $ ./udp_client <host> <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

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
    // check command line argt
    if(argc != 3){
        printf(MSGERRR "Wrong number of arguments." MSGNORM "\n");
        printf(MSGERRR "Usage: %s <hostname> <port>" MSGNORM "\n", argv[0]);
        exit(0);
    }

    // set hostname
    char *hostname = argv[1];
    printf(MSGNORM "Hostname set: " MSGNORM "\t");
    printf(MSGSUCC "%s" MSGNORM "\n", hostname);

    // set port
    int portno = atoi(argv[2]);
    printf(MSGNORM "Port set: " MSGNORM "\t");
    printf(MSGSUCC "%d" MSGNORM "\n", portno);

    // create socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        error(MSGERRR "ERROR opening Socket" MSGNORM "\n");
    }

    // get host by name = get servers dns entry
    struct hostent *server = gethostbyname(hostname);
    if(server == NULL){
        printf(MSGERRR "ERROR, no such host: %s" MSGNORM "\n", hostname);
        exit(0);
    }

    // server address build
    struct sockaddr_in serveraddr;
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char*)server->h_addr, (char*)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    // get input from user
    char buff[BUFFSIZE];
    buffinput(buff);
    printf(MSGSUCC "input: %s, len: %ld" MSGNORM "\n", buff, strlen(buff));

    // get cmd and filename
    char argb[BUFFSIZE]; //command
    char argf[BUFFSIZE]; //filename
    buffparse(buff, argb, argf);
    printf(MSGSUCC "argb: %s, len: %ld" MSGNORM "\n", argb, strlen(argb));
    printf(MSGSUCC "argf: %s, len: %ld" MSGNORM "\n", argf, strlen(argf));

    // handle input from user
    while(strcmp(argb, "exit") != 0){
        // client handle cmds
        if(strcmp(argb, "puts") == 0){
            printf(MSGTERM "Putting file to server" MSGNORM "\n");
            // handle putting files to server
            // handle getting input again
            buffinput(buff);
            buffparse(buff, argb, argf);
            printf(MSGSUCC "argb: %s, len: %ld" MSGNORM "\n", argb, strlen(argb));
            printf(MSGSUCC "argf: %s, len: %ld" MSGNORM "\n", argf, strlen(argf));
        } else if (strcmp(argb, "gets") == 0){
            printf(MSGTERM "Getting file from server" MSGNORM "\n");
            // handle getting file from server
            // handle getting input again
            buffinput(buff);
            buffparse(buff, argb, argf);
            printf(MSGSUCC "argb: %s, len: %ld" MSGNORM "\n", argb, strlen(argb));
            printf(MSGSUCC "argf: %s, len: %ld" MSGNORM "\n", argf, strlen(argf));
        } else if (strcmp(argb, "ls") == 0){
            printf(MSGTERM "Listing files on server" MSGNORM "\n");
            // handle listing files from server
            // h  andle getting input again
            buffinput(buff);
            buffparse(buff, argb, argf);
            printf(MSGSUCC "argb: %s, len: %ld" MSGNORM "\n", argb, strlen(argb));
            printf(MSGSUCC "argf: %s, len: %ld" MSGNORM "\n", argf, strlen(argf));
        } else {
            // handle getting input again
            printf(MSGERRR "NOT VALID INPUT" MSGNORM "\n");
            printf(MSGWARN "Commands: puts <filename>, gets <filename>, ls, exit" MSGNORM "\n");
            buffinput(buff);
            buffparse(buff, argb, argf);
            printf(MSGSUCC "argb: %s, len: %ld" MSGNORM "\n", argb, strlen(argb));
            printf(MSGSUCC "argf: %s, len: %ld" MSGNORM "\n", argf, strlen(argf));
        }
    }

    printf(MSGTERM "PROGRAM EXITING\n" MSGNORM);
    return 0;
}