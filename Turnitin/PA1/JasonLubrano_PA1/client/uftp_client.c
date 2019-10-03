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

#define TRUE 1

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

void serverecho(char* buff){
    printf(MSGTERM "%s" MSGNORM "\n", buff);
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

void buffparse(char *buf, char *argcmd, char *argfile){
    int i=0;
    char delim[] = " ";
    
    memset(argcmd, 0, BUFFSIZE * (sizeof(char)));
    memset(argfile, 0, BUFFSIZE * (sizeof(char)));

    char* token1 = strtok(buf, delim);
    size_t tk1len = strlen(token1);
    strncpy(argcmd, token1, tk1len);

    if(strcmp(argcmd, "ls") == 0){
        argfile = "";
    } else if (strcmp(argcmd, "exit") == 0) {
        argfile = "";
    } else if ((strcmp(argcmd, "put") == 0) 
            || (strcmp(argcmd, "get") == 0)
            || (strcmp(argcmd, "delete") == 0)){
        char* token2 = strtok(NULL, delim);
        size_t tk2len = strlen(token2);
        strncpy(argfile, token2, tk2len);
    } else {
        argcmd=""; argfile="";
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
    bcopy((char*)server->h_addr,
        (char*)&serveraddr.sin_addr.s_addr, server->h_length);
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
    n = recvfrom(sockfd, buff, BUFFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);
    if (n < 0){
      error("ERROR in recvfrom");
    }
    printf("Echo from server: %s, n = %d \n", buff, n);

    // get input from user
    // get cmd and filename
    char argb[BUFFSIZE]; //command
    char argf[BUFFSIZE]; //filename
    char endfio[BUFFSIZE] = "endfio";

    FILE *fp;

    // handle input from user
    buffinput(buff);
    printf(MSGNORM "Input: " MSGNORM);
    printf(MSGSUCC "%s" MSGNORM, buff);
    printf(MSGNORM " -- Size: " MSGNORM);
    printf(MSGWARN "%ld" MSGNORM "\n", strlen(buff));

    buffparse(buff, argb, argf);
    printf(MSGNORM "Command: " MSGNORM);
    printf(MSGSUCC "%s" MSGNORM, argb);
    printf(MSGNORM "  -- Size: " MSGNORM);
    printf(MSGWARN "%ld" MSGNORM "\n", strlen(argb));

    printf(MSGNORM "File: " MSGNORM);
    printf(MSGSUCC "%s" MSGNORM, argf);
    printf(MSGNORM "  -- Size: " MSGNORM);
    printf(MSGWARN "%ld" MSGNORM "\n", strlen(argf));

    do{        
        n = sendto(sockfd, argb, sizeof(argb), MSG_WAITALL,
                    (const struct sockaddr *) &serveraddr, serverlen);
        errorchecksend(n);
        
        n = recvfrom(sockfd, buff, BUFFSIZE, 0,
                    (struct sockaddr *) &serveraddr, &serverlen);
        errorcheckrecv(n);
        serverecho(buff);
        
        n = sendto(sockfd, argf, sizeof(argf), MSG_WAITALL,
                    (const struct sockaddr *) &serveraddr, serverlen);
        errorchecksend(n);
        
        n = recvfrom(sockfd, buff, BUFFSIZE, 0,
                    (struct sockaddr *) &serveraddr, &serverlen);
        errorcheckrecv(n);
        serverecho(buff);

        // client handle cmds
        /***********************************/
        if (strcmp(argb, "put") == 0){
            FILE *fp = fopen(argf, "r");
            size_t nbytes = 0;
            int packetn;
            memset(buff, 0, BUFFSIZE * (sizeof(char)));
            while((nbytes = fread(buff+1, 1, BUFFSIZE-1, fp)) > 0){
                buff[0] = 'p';
                n = sendto(sockfd, buff, nbytes+1, 0,
                        (struct sockaddr*)&serveraddr, serverlen);
                errorchecksend(n);
                printf(MSGWARN "Packet %d Sent" MSGNORM " Size %ld\n", ++packetn, nbytes);
            }
            printf(MSGSUCC "File Sent" MSGNORM "\n");
        /***********************************/
        } else if (strcmp(argb, "get") == 0){
            printf(MSGTERM "Client fetching file:" MSGNORM " %s \n", argf);
            /* message handling */
            while(TRUE){
                memset(buff, 0, BUFFSIZE * (sizeof(char)));
                n = recvfrom(sockfd, buff, BUFFSIZE, 0,
                        (struct sockaddr *)&serveraddr, &serverlen);
                errorcheckrecv(n);
                if(buff[0]='g'){
                    fp = fopen(argf, "a");
                    fwrite(buff+1, 1, n-1, fp);
                    fflush(fp);
                    fclose(fp);
                    if(n!=BUFFSIZE){
                        printf(MSGSUCC "File saved" MSGNORM "\n");
                        break;
                    }
                } else {
                    // BONUS: we missed the packet or corrupted
                    // we have to resend that packet
                    buff[0] == 'x';
                    n = sendto(sockfd, buff, BUFFSIZE, 0,
                        (struct sockaddr*)&serveraddr, serverlen);
                    errorchecksend(n);
                }
            }
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
            printf(MSGWARN "%s" MSGNORM "\n", buff);
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
    printf(MSGNORM "Input: " MSGNORM);
    printf(MSGSUCC "%s" MSGNORM, buff);
    printf(MSGNORM " -- Size: " MSGNORM);
    printf(MSGWARN "%ld" MSGNORM "\n", strlen(buff));

    buffparse(buff, argb, argf);
    printf(MSGNORM "Command: " MSGNORM);
    printf(MSGSUCC "%s" MSGNORM, argb);
    printf(MSGNORM "  -- Size: " MSGNORM);
    printf(MSGWARN "%ld" MSGNORM "\n", strlen(argb));

    printf(MSGNORM "File: " MSGNORM);
    printf(MSGSUCC "%s" MSGNORM, argf);
    printf(MSGNORM "  -- Size: " MSGNORM);
    printf(MSGWARN "%ld" MSGNORM "\n", strlen(argf));

    }while(strcmp(argb, "exit") != 0);

    /* exit handling */
    n = sendto(sockfd, argb, sizeof(argb), MSG_WAITALL,
            (const struct sockaddr *) &serveraddr, serverlen);
    errorchecksend(n);
    
    n = recvfrom(sockfd, buff, BUFFSIZE, 0,
                (struct sockaddr *) &serveraddr, &serverlen);
    errorcheckrecv(n);
    serverecho(buff);
    
    n = sendto(sockfd, argf, sizeof(argf), MSG_WAITALL,
                (const struct sockaddr *) &serveraddr, serverlen);
    errorchecksend(n);
    
    n = recvfrom(sockfd, buff, BUFFSIZE, 0,
                (struct sockaddr *) &serveraddr, &serverlen);
    errorcheckrecv(n);
    serverecho(buff);

    close(sockfd);
    return 0;
}