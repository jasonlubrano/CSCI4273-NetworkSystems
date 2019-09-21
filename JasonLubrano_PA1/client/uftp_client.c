/*
 * Jason Lubrano
 * CSCI4273 - Network Systems - Dr. Sangtae Ha
 * uftp_client.c
 * $ ./client <host> <port>
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

void clearBuff(char* b){
    int i;
    for(i=0; i < BUFFSIZE; i++){
        b[i] = '\0';
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
    char delim[] = " ";
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

    // basic messages for debugging packets
    char ready[BUFFSIZE] = "Ready to connect";
    char acknoledge[BUFFSIZE] = "Message received";

    // server setup
    int serverlen = sizeof(serveraddr);
    int n = sendto(sockfd, ready, sizeof(ready), MSG_WAITALL,
                (const struct sockaddr *) &serveraddr, serverlen);
    if(n < 0){
        error(MSGERRR "ERROR in connect\n" MSGNORM);
    }

    char buff[BUFFSIZE];
    n = recvfrom(sockfd, buff, BUFFSIZE, 0,
                (struct sockaddr *) &serveraddr, &serverlen);
    if (n < 0){
      error("ERROR in recvfrom");
    }
    printf("Echo from server: %s, n = %d \n", buff, n);

    // get input from user
    // char buff[BUFFSIZE];
    // get cmd and filename
    char argb[BUFFSIZE]; //command
    char argf[BUFFSIZE]; //filename
    char endfio[BUFFSIZE] = "endfio";

    FILE *fp;
    int wds = 0;
    char c;

    // handle input from user
    buffinput(buff);
    printf(MSGSUCC "input: %s, len: %ld" MSGNORM "\n", buff, strlen(buff));
    buffparse(buff, argb, argf);
    printf(MSGSUCC "argb: %s, len: %ld" MSGNORM "\n", argb, strlen(argb));
    printf(MSGSUCC "argf: %s, len: %ld" MSGNORM "\n", argf, strlen(argf));
    do{        
        n = sendto(sockfd, argb, sizeof(argb), MSG_WAITALL,
                    (const struct sockaddr *) &serveraddr, serverlen);
        errorchecksend(n);
        
        n = recvfrom(sockfd, buff, BUFFSIZE, 0,
                    (struct sockaddr *) &serveraddr, &serverlen);
        errorcheckrecv(n);
        printf("Echo from server: %s, n = %d \n", buff, n);
        
        n = sendto(sockfd, argf, sizeof(argf), MSG_WAITALL,
                    (const struct sockaddr *) &serveraddr, serverlen);
        errorchecksend(n);
        
        n = recvfrom(sockfd, buff, BUFFSIZE, 0,
                    (struct sockaddr *) &serveraddr, &serverlen);
        errorcheckrecv(n);
        printf("Echo from server: %s, n = %d \n", buff, n);

        // client handle cmds
        /***********************************/
        if(strcmp(argb, "put") == 0){
            printf(MSGTERM "Putting file: %s to server" MSGNORM "\n", argf);
            /* message handling */
            memset(buff, 0, BUFFSIZE * (sizeof(char)));
            fp = fopen(argf, "r");
            if(fp == NULL){
                printf(MSGERRR "FILE opening failed" MSGNORM "\n");
                memset(buff, 0, BUFFSIZE * (sizeof(char)));
                break;
            } else {
                printf(MSGSUCC "FILE opened successfully" MSGNORM "\n");
            }

            while(1){
                if(sendFile(fp, buff, BUFFSIZE)){
                    sendto(sockfd, buff, BUFFSIZE, 0, 
                            (struct sockaddr *) &serveraddr, serverlen);
                    break;
                }

                sendto(sockfd, buff, BUFFSIZE, 0,
                        (struct sockaddr*)&serveraddr, serverlen);
                memset(buff, 0, BUFFSIZE * (sizeof(char)));
            }
            if(fp!=NULL){
                fclose(fp);
                printf(MSGSUCC "File sent successfully" MSGNORM "\n");
            }
        /***********************************/
        } else if (strcmp(argb, "get") == 0){
            printf(MSGTERM "Client fetching file:" MSGNORM " %s \n", argf);
            /* message handling */
            fp = fopen(argf, "a");
            while(1){
                memset(buff, 0, BUFFSIZE * (sizeof(char)));
                n = recvfrom(sockfd, buff, BUFFSIZE, 0,
                        (struct sockaddr*)&serveraddr, &serverlen);
                errorcheckrecv(n);
                fprintf(fp, "%s", buff);
                if(recvFile(buff, BUFFSIZE)){
                    printf(MSGTERM "file saved\n" MSGNORM); 
                    break;
                }
            }
            fclose(fp);
        /***********************************/
        } else if (strcmp(argb, "ls") == 0){
            printf(MSGTERM "Listing files on server" MSGNORM "\n");
            /* handle file listing */
            memset(buff, 0, BUFFSIZE * (sizeof(char)));
            n=256; // size always sent from strdirent->d_name
            while (n != 8){
                n = recvfrom(sockfd, buff, BUFFSIZE, 0,
                            (struct sockaddr *) &serveraddr, &serverlen);
                if(strcmp(buff, "endlist") != 0){
                    printf(">> %s\n", buff);
                }
                memset(buff, 0, BUFFSIZE * (sizeof(char)));
            }
            memset(buff, 0, BUFFSIZE * (sizeof(char)));
        /***********************************/
        } else if (strcmp(argb, "delete") == 0){
            /* handle file deletion */
            printf(MSGTERM "Deleting file: %s " MSGNORM "\n", argf);
            n = recvfrom(sockfd, buff, BUFFSIZE, 0,
                        (struct sockaddr *) &serveraddr, &serverlen);
            printf("%s\n", buff);
            memset(buff, 0, BUFFSIZE * (sizeof(char)));
        /***********************************/
        } else {
            // handle getting input again
            printf(MSGERRR "NOT VALID INPUT" MSGNORM "\n");
            printf(MSGWARN "Commands: put <filename>, get <filename>, ls, exit" MSGNORM "\n");
        }
    memset(&buff, 0, BUFFSIZE * (sizeof(char)));
    memset(&argf, 0, BUFFSIZE * (sizeof(char)));
    memset(&argb, 0, BUFFSIZE * (sizeof(char)));

    buffinput(buff);
    printf(MSGSUCC "input: %s, len: %ld" MSGNORM "\n", buff, strlen(buff));
    buffparse(buff, argb, argf);
    printf(MSGSUCC "argb: %s, len: %ld" MSGNORM "\n", argb, strlen(argb));
    printf(MSGSUCC "argf: %s, len: %ld" MSGNORM "\n", argf, strlen(argf));
    printf(MSGSUCC "input: %s, len: %ld" MSGNORM "\n", buff, strlen(buff));
    }while(strcmp(argb, "exit") != 0);
    
    
    /* exit handling */
    n = sendto(sockfd, argb, sizeof(argb), MSG_WAITALL,
            (const struct sockaddr *) &serveraddr, serverlen);
    errorchecksend(n);
    
    n = recvfrom(sockfd, buff, BUFFSIZE, 0,
                (struct sockaddr *) &serveraddr, &serverlen);
    errorcheckrecv(n);
    printf("Echo from server: %s, n = %d \n", buff, n);
    
    n = sendto(sockfd, argf, sizeof(argf), MSG_WAITALL,
                (const struct sockaddr *) &serveraddr, serverlen);
    errorchecksend(n);
    
    n = recvfrom(sockfd, buff, BUFFSIZE, 0,
                (struct sockaddr *) &serveraddr, &serverlen);
    errorcheckrecv(n);


    close(sockfd); 
    return 0;
}