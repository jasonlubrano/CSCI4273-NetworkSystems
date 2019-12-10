/* 
 * Jason Lubrano
 * CSCI4273 - Ha - PA4
 * server/dfs.c - server side of the distributed file sharing system
 */


/*
 * includes
 */
#include <math.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>      /* for fgets */
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>      /* for read, write */
#include <strings.h>     /* for bzero, bcopy */
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>  /* for socket use */
#include <openssl/md5.h>
#include <sys/sendfile.h>


/*
 * color messages
 */
#define MSGNORM "\x1b[0m"
#define MSGERRR "\x1b[31m"
#define MSGSUCC "\x1b[32m"
#define MSGTERM "\x1b[35m"
#define MSGWARN "\x1b[33m"


/*
 * lengths
 */
#define NUMSERVS    4       /* max number of servers */
#define SERVPORT    8       /* max server port length */
#define MAXCONN     32      /* max number of connections */
#define FILEDIR     128     /* max file directory length */
#define MAXLINE     256     /* max text line length */
#define LISTENQ     1024    /* second argument to listen() */
#define MAXBUFF     8192    /* max text line length */


/*
 * verbosity
 */
#define VERBOSE 1


/*
 * function headers
 */
void welcome_message();
void closing_message();
void ret_exit();
void data_from_config(const char* configFile, const char* serverFileDirectory);
int socket_connection_routine(const char* port);


/*
 * global timeout
 */
struct timeval timeout;


struct UserConfigs {
	int		UserNum;
	char	UserUsername[MAXLINE];
	char	UserPassword[MAXLINE];
    char    FileDirectory[FILEDIR];
} userconfigs;


/*
 * main
 */
int main(int argc, char **argv){
    welcome_message();
    
    char* serverFileDirectory;
    char* port;
    switch (argc)
    {
    case 3:
        serverFileDirectory = argv[1];
        if(VERBOSE){ printf(MSGNORM "Server Directory: %s" MSGNORM "\n", serverFileDirectory); }
        port = argv[2];
        if(VERBOSE){ printf(MSGNORM "Port: %s" MSGNORM "\n", port); }
        break;
    
    default:
        printf(MSGERRR "Usage 2: %s <Server Directory> <Port>" MSGNORM "\n", argv[0]);
        ret_exit(0);
        break;
    }

    char* configFile = "dfs.conf";
    data_from_config(configFile, serverFileDirectory);
    
    struct sockaddr_in clientAddrIn;
    int sockfd;

    sockfd = socket_connection_routine(port);
    int fileDescriptorSet = getdtablesize();
    if(VERBOSE){ printf("..File Descriptor Set: %d\n", fileDescriptorSet); }

    fd_set activeFileDescriptorSet;
    FD_ZERO(&activeFileDescriptorSet);
    FD_SET(sockfd, &activeFileDescriptorSet);

    int isRunning = 1;
    unsigned int addressLength;
    fd_set readFileDescriptorSet;
    while(isRunning){
        memcpy(&readFileDescriptorSet, &activeFileDescriptorSet, sizeof(readFileDescriptorSet));
        
        if(select(fileDescriptorSet, &readFileDescriptorSet, NULL, NULL, NULL) < 0) {
            printf( MSGERRR "ERROR: On selecting file descriptor" MSGNORM "\n");
        }
       
        if(FD_ISSET(sockfd, &readFileDescriptorSet)){
            int sockfdConn;
            addressLength = sizeof(clientAddrIn);
            sockfdConn = accept(sockfd, (struct sockaddr *)&clientAddrIn, &addressLength);

            if(sockfdConn < 0){ printf(MSGERRR "ERROR: Connection Rejected" MSGNORM "\n"); ret_exit(); }
            FD_SET(sockfdConn, &activeFileDescriptorSet);
        }

        int sockcmdds;
        for(sockcmdds=0; sockcmdds < fileDescriptorSet; ++sockcmdds){
            if(FD_ISSET(sockcmdds, &readFileDescriptorSet) && sockcmdds != sockfd){
                /* STOPPING HERE */
            }
        }
    }


    closing_message();
    
    return 0;
}


/*
 * function definitons
 */

/* prints welcome message */
void welcome_message(){
    printf(MSGSUCC "------------------------------------" MSGNORM "\n");
    printf(MSGSUCC "        Started DFS - Server" MSGNORM "\n");
    printf(MSGSUCC "------------------------------------" MSGNORM "\n");

}


/* prints closing message */
void closing_message(){
    printf(MSGSUCC "------------------------------------" MSGNORM "\n");
    printf(MSGSUCC "        Closing DFS - Server" MSGNORM "\n");
    printf(MSGSUCC "------------------------------------" MSGNORM "\n");
}


/* exit(0) upon called */
void ret_exit(){
    closing_message();
    exit(0);
}


/* gets data from config file */
void data_from_config(const char* configFile, const char* serverFileDirectory){
    if(VERBOSE) { printf(MSGNORM "--Getting Data from Config--" MSGNORM "\n"); }
    FILE* file = fopen(configFile, "r");
    if(file == NULL){ printf(MSGERRR ".config file is NULL \nDoes it exist?" MSGNORM "\n"); ret_exit(); }
    else {
        printf(MSGSUCC ".Config file opened successfully" MSGNORM "\n");
        ssize_t read;
        char line[MAXLINE];
        size_t len = 0;
        char* firstinfo;
        char* rootdirectory;
        char* username;
        char* password;
        char* delim = " ";
        int usercount = 0;
        if(VERBOSE){ printf(".Reading through file line by line \n"); }
        /* I'm a car too, you know! >:( */
        while (fgets(line, sizeof(line), file))
        {
            printf("%s", line);
            printf("\n");

            firstinfo = strtok(line, delim);
            if(VERBOSE){ printf("firstinfo: %s \n", firstinfo); }
            
            if(strcmp("RootDirectory", firstinfo) == 0) {
                if(VERBOSE){ printf(MSGSUCC "Match on Root Directory" MSGNORM "\n"); }
                
                rootdirectory = strtok(NULL, delim);
                if(VERBOSE){ printf("Root Directory: %s \n", rootdirectory); }

                char servDir[MAXLINE];
                sprintf(servDir, "./%s%s/", rootdirectory, serverFileDirectory);
                if(VERBOSE){ printf("Server Directory: %s \n", servDir); }
                
                strcpy(userconfigs.FileDirectory, servDir);
                printf("User Configs -> Server File Directory: %s \n", userconfigs.FileDirectory);

            } else if(strcmp("Username:", firstinfo) == 0){
                if(VERBOSE){ printf(MSGSUCC "Match on Username" MSGNORM "\n"); }
                
                username = strtok(NULL, delim);
                if(VERBOSE){ printf("username: %s \n", username); }
                
                strcpy(userconfigs.UserUsername, username);
                printf("User Config -> Username: %s \n", userconfigs.UserUsername);

            } else if(strcmp("Password:", firstinfo) == 0){
                if(VERBOSE){ printf(MSGSUCC "Match on Password" MSGNORM "\n"); }
                
                password = strtok(NULL, delim);
                if(VERBOSE){ printf("password: %s \n", password); }
                
                strcpy(userconfigs.UserPassword, password);
                printf("User Config -> Password: %s \n", userconfigs.UserPassword);

            } else {
                printf(MSGERRR "ERROR IN CONFIG PARSING" MSGNORM "\n\n");
                ret_exit();
            }
            ++usercount;
        }

        userconfigs.UserNum = usercount-2;
        printf("User Config -> User Count: %d \n", userconfigs.UserNum);
    }
}


/* TCP routine to connect sockets */
int socket_connection_routine(const char* port){
    if(VERBOSE){printf("--Running Socket Connection Routine--\n"); }
	struct sockaddr_in sockAddrIn;
	memset(&sockAddrIn, 0, sizeof(sockAddrIn));
    sockAddrIn.sin_family = AF_INET;
    sockAddrIn.sin_addr.s_addr = INADDR_ANY;
    sockAddrIn.sin_port = htons((unsigned short)atoi(port));
    if(sockAddrIn.sin_port == 0){ printf(MSGERRR "ERROR: UNABLE TO PORT ON: %s" MSGNORM "\n", port); ret_exit(); }
    else { printf(MSGSUCC "Port on: %s Established" MSGNORM "\n", port); }
    
    int sockfd;
    sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0){ printf(MSGERRR "ERROR: UNABLE TO CREATE SOCKET" MSGNORM "\n"); ret_exit(); }
    else{ printf(MSGSUCC "Socket Created Successfully" MSGNORM "\n"); }

    if(bind(sockfd, (struct sockaddr *)&sockAddrIn, sizeof(sockAddrIn)) < 0){
        printf(MSGERRR "Error: Unable to bind port on: %s" MSGNORM "\n", port);
        sockAddrIn.sin_port = htons(0);
        if(bind(sockfd, (struct sockaddr *)&sockAddrIn, sizeof(sockAddrIn)) < 0){
            printf(MSGERRR "Error: Unable to bind port on: %s" MSGNORM "\n", port);
            ret_exit();
        } else {
            int socketLength = sizeof(sockAddrIn);
            if(getsockname(sockfd, (struct sockaddr *)&sockAddrIn, &socketLength) < 0){
                printf(MSGERRR "Error Getting Socket Name" MSGNORM "\n");
            }
            printf("New Server Port on: %d \n", ntohs(sockAddrIn.sin_port));
        }
    }

    if(listen(sockfd, MAXCONN) < 0){
        printf("Unable to listen on port: %s \n", port);
        ret_exit();
    }

    printf("Socket: %d connected on port: %d \n", sockfd, ntohs(sockAddrIn.sin_port));
    return sockfd;
}