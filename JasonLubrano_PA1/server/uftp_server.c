/* 
 * Jason Lubrano
 * CSCI4273 - Network Systems - Dr. Sangtae Ha
 * uftp_server.c
 * $ ./server <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <ctype.h>


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

void clearBuff(char* b){
    int i;
    for(i=0; i < BUFFSIZE; i++){
        b[i] = '\0';
    }
}

int sendFile(FILE *fp, char *buffsend, int size){
    int i;
    int length;
    // handle file not found errors
    if(fp == NULL){
        strcpy(buffsend, "File not found");
        length = strlen("File not found");
        buffsend[length] = EOF;
        return 1;
    }

    char nextchar;
    for(i=0; i<size; i++){
        nextchar = fgetc(fp);
        buffsend[i] = nextchar;
        if (nextchar == EOF){
            return 1;
        }
    }
    return 0;
}

int recvFile(char *buffrecv, int size){
    int i;
    char nextchar;
    for(i = 0; i < size; i++){
        nextchar = buffrecv[i];
        if(nextchar == EOF){
            return 1;
        }
    }
    return 0;
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
    char filed[BUFFSIZE] = "File Deleted Successfully";
    char fileu[BUFFSIZE] = "Unable to delete file";

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
    FILE *fp;
    // state machine
    do{
        // server handle cmds
        /***********************************/
        if(strcmp(argb, "put") == 0){
            printf(MSGTERM "Server saving file:" MSGNORM " %s \n", argf);
            /* message handling */
            fp = fopen(argf, "a");
            while(1){
                memset(buff, 0, BUFFSIZE * (sizeof(char)));
                n = recvfrom(sockfd, buff, BUFFSIZE, 0,
                            (struct sockaddr*)&clientaddr, &clientlen);
                errorcheckrecv(n);
                fprintf(fp, "%s", buff);
                if(recvFile(buff, sizeof(buff))){
                    printf(MSGTERM "file saved\n" MSGNORM);
                    break;
                }
            }
            fclose(fp);
        /***********************************/
        } else if (strcmp(argb, "get") == 0){
            /* message handling */
            printf(MSGTERM "Server retrieving file: " MSGNORM " %s \n", argf);
            memset(buff, 0, BUFFSIZE * (sizeof(char)));
            fp = fopen(argf, "r");
            if(fp == NULL){
                printf(MSGERRR "FILE opening failed" MSGNORM "\n");
                memset(&buff, 0, BUFFSIZE * (sizeof(char)));
                break;
            } else {
                printf(MSGSUCC "FILE opened successfully" MSGNORM "\n");
            }
            while(1){
                if(sendFile(fp, buff, BUFFSIZE)){
                    sendto(sockfd, buff, BUFFSIZE, 0,
                            (struct sockaddr *)&clientaddr, clientlen);
                    break;
                }
                sendto(sockfd, buff, BUFFSIZE, 0,
                            (struct sockaddr *)&clientaddr, clientlen);
                memset(buff, 0, BUFFSIZE * (sizeof(char)));
            }
            if(fp!=NULL){
                fclose(fp);
                printf(MSGTERM "File sent" MSGNORM "\n");
            }
        /***********************************/
        } else if (strcmp(argb, "ls") == 0){
            printf(MSGTERM "Server getting file list" MSGNORM "\n");
            // handle listing files from server
            // h  andle getting input again
            memset(buff, 0, BUFFSIZE * (sizeof(char)));
            DIR *dir;
            struct dirent *strdirent;
            dir = opendir(".");
            if(dir == NULL){
                printf(MSGERRR "ERROR! Unable to open directory" MSGNORM "\n");
            } else {
                int i = 0;
                while( (strdirent=readdir(dir)) != NULL){
                    printf(">> %s\n", strdirent->d_name);
                    sendto(sockfd, strdirent->d_name, sizeof(strdirent->d_name), 0,
                            (struct sockaddr *) &clientaddr, clientlen);
                }
            }
            sendto(sockfd, "endlist", sizeof("endlist"), 0,
                    (struct sockaddr *) &clientaddr, clientlen);
            closedir(dir);
            printf(MSGTERM "Files listed" MSGNORM "\n");
        /***********************************/
        } else if (strcmp(argb, "delete") == 0){
            printf(MSGTERM "Server deleting: %s " MSGNORM "\n", argf);
            /* handle deletion */
            int status = remove(argf);
            if(status == 0){
                printf(MSGSUCC "file: %s deleted successfully" MSGNORM "\n", argf);
                sendto(sockfd, filed, sizeof(filed), 0,
                        (struct sockaddr *) &clientaddr, clientlen);
            } else {
                perror("ERROR");
                printf(MSGERRR "Unable to delete file: %s" MSGNORM "n", argf);
                sendto(sockfd, fileu, sizeof(fileu), 0,
                        (struct sockaddr *) &clientaddr, clientlen);
            }
        /***********************************/
        } else {
            // handle getting input again
            printf(MSGERRR "NOT VALID INPUT" MSGNORM "\n");
            printf(MSGWARN "Commands: put <filename>, get <filename>, ls, exit" MSGNORM "\n");
        }

    clearBuff(buff);
    memset(buff, 0, BUFFSIZE * (sizeof(char)));
    memset(&argf, 0, BUFFSIZE * (sizeof(char)));
    memset(&argb, 0, BUFFSIZE * (sizeof(char)));

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

    }while(strcmp(argb, "exit") != 0);

    printf(MSGTERM "Server closing" MSGNORM "\n");
    return 0;
}