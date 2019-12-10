/* 
 * Jason Lubrano
 * CSCI4273 - Ha - PA4
 * client/dfs.c - client sided distributed file sharing system
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
#define MD5_LEN     16      /* max length for md5 */
#define MAXLINE     256     /* max text line length */
#define MAXBUFF     8192    /* max text line length */
#define LISTENQ     1024    /* second argument to listen() */


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
void data_from_config(const char* configFile);


/*
 * global timeout
 */
struct timeval timeout;


struct ServerConfigs {
	char	ServerNames[4][16];
	char	ServerAddresses[4][64];
	char	ServerPorts[4][8];
	int		ServerNum;
	char	ServerUsername[MAXLINE];
	char	ServerPassword[MAXLINE];
} serverconfigs;


/*
 * main
 */
int main(int argc, char **argv){
    welcome_message();
    char* configFile;
    switch (argc)
    {
    case 2:
        configFile = argv[1];
        if(VERBOSE){ printf(MSGNORM "Config File: %s" MSGNORM "\n", configFile); }
        break;
    
    default:
        printf(MSGERRR "Usage 2: %s <Config File>" MSGNORM "\n", argv[0]);
        ret_exit(0);
        break;
    }
    
    data_from_config(configFile);

    closing_message();
    return 0;
}

/*
 * function definitons
 */

/* prints welcome message */
void welcome_message(){
    printf(MSGSUCC "------------------------------------" MSGNORM "\n");
    printf(MSGSUCC "        Started DFS - Client" MSGNORM "\n");
    printf(MSGSUCC "------------------------------------" MSGNORM "\n");

}


/* prints closing message */
void closing_message(){
    printf(MSGSUCC "------------------------------------" MSGNORM "\n");
    printf(MSGSUCC "        Closing DFS - Client" MSGNORM "\n");
    printf(MSGSUCC "------------------------------------" MSGNORM "\n");
}


/* exit(0) upon called */
void ret_exit(){
    closing_message();
    exit(0);
}


/* gets data from config file */
void data_from_config(const char* configFile){
    printf(MSGNORM "--Getting Data from Config--" MSGNORM "\n");
    FILE* file = fopen(configFile, "r");
    if(file == NULL){ printf(MSGERRR ".config file is NULL \nDoes it exist?" MSGNORM "\n"); ret_exit(); }
    else{
        printf(MSGSUCC ".Config file opened successfully" MSGNORM "\n");
        ssize_t read;
        char line[MAXLINE];
        size_t len = 0;
        char* firstinfo;
        char* servername;
        char* serverIP;
        char* serverPort;
        char* username;
        char* password;
        char* delim = " ";
        int i = 0;
        if(VERBOSE){ printf(".Reading through file line by line \n"); }
        while (fgets(line, sizeof(line), file))
        {
            printf("%s", line);
            printf("\n");

            firstinfo = strtok(line, delim);
            if(VERBOSE){ printf("firstinfo: %s \n", firstinfo); }

            if(strcmp("Server", firstinfo) == 0) {
                if(VERBOSE){ printf(MSGSUCC "Match on server" MSGNORM "\n"); }
                servername = strtok(NULL, delim);
                if(VERBOSE){ printf("servername: %s \n", servername); }
                serverIP = strtok(NULL, ":");
                if(VERBOSE){ printf("serverIP: %s \n", serverIP); }
                serverPort = strtok(NULL, ":");
                if(VERBOSE){ printf("serverPort: %s \n", serverPort); }
                
                strcpy(serverconfigs.ServerNames[i], servername);
                strcpy(serverconfigs.ServerAddresses[i], serverIP);
                strcpy(serverconfigs.ServerPorts[i], serverPort);

                printf("Server Config Name: %s \n", serverconfigs.ServerNames[i]);
                printf("Server Config Address: %s \n", serverconfigs.ServerAddresses[i]);
                printf("Server Config Port: %s \n", serverconfigs.ServerPorts[i]);

            } else if(strcmp("Username:", firstinfo) == 0){
                if(VERBOSE){ printf(MSGSUCC "Match on Username" MSGNORM "\n"); }
                username = strtok(NULL, delim);
                if(VERBOSE){ printf("username: %s \n", username); }
                
                strcpy(serverconfigs.ServerUsername, username);
                printf("Server Config Username: %s \n", serverconfigs.ServerUsername);

            } else if(strcmp("Password:", firstinfo) == 0){
                if(VERBOSE){ printf(MSGSUCC "Match on Password" MSGNORM "\n"); }
                password = strtok(NULL, delim);
                if(VERBOSE){ printf("password: %s \n", password); }
                
                strcpy(serverconfigs.ServerPassword, password);
                printf("Server Config Password: %s \n", serverconfigs.ServerPassword);

            } else {
                printf(MSGERRR "ERROR IN CONFIG PARSING" MSGNORM "\n\n");
                ret_exit();
            }

            ++i;
        }
        serverconfigs.ServerNum = i-2; // minus 2 bc username / password
        printf("Server Config Server Count: %d \n", serverconfigs.ServerNum);

        fclose(file);
        
    }
}