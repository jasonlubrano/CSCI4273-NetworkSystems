/* 
 * Jason Lubrano
 * CSCI4273 - Ha - PA4
 * client/dfs.c - client side of the distributed file sharing system
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
#define SERVNAME    16      /* max server name */
#define FILENAME    32      /* max size of file name*/  
#define SERVADDR    64      /* max server addr */
#define FILEPRTS    64      /* max num of file parts */
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
void data_from_config(const char* configFile);
void lsh_loop();
int handle_LIST();
int handle_GET(const char* filename);
int handle_PUT(const char* filename);
int handle_EXIT();
int send_command_to_server(int serverNumber, char* argcmd, ...);


/*
 * global timeout
 */
struct timeval timeout;


struct ServerConfigs {
	char	ServerNames[NUMSERVS][SERVNAME];
	char	ServerAddresses[NUMSERVS][SERVADDR];
	char	ServerPorts[NUMSERVS][SERVPORT];
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
    
    lsh_loop();

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
    if(VERBOSE) { printf(MSGNORM "--Getting Data from Config--" MSGNORM "\n"); }
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
        int servercount = 0;
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
                
                strcpy(serverconfigs.ServerNames[servercount], servername);
                strcpy(serverconfigs.ServerAddresses[servercount], serverIP);
                strcpy(serverconfigs.ServerPorts[servercount], serverPort);
-
                printf("Server Config -> Server Name: %s \n", serverconfigs.ServerNames[servercount]);
                printf("Server Config -> Server Address: %s \n", serverconfigs.ServerAddresses[servercount]);
                printf("Server Config -> Server Port: %s \n", serverconfigs.ServerPorts[servercount]);

            } else if(strcmp("Username:", firstinfo) == 0){
                if(VERBOSE){ printf(MSGSUCC "Match on Username" MSGNORM "\n"); }
                
                username = strtok(NULL, delim);
                if(VERBOSE){ printf("username: %s \n", username); }
                
                strcpy(serverconfigs.ServerUsername, username);
                printf("Server Config -> Username: %s \n", serverconfigs.ServerUsername);

            } else if(strcmp("Password:", firstinfo) == 0){
                if(VERBOSE){ printf(MSGSUCC "Match on Password" MSGNORM "\n"); }
                
                password = strtok(NULL, delim);
                if(VERBOSE){ printf("password: %s \n", password); }
                
                strcpy(serverconfigs.ServerPassword, password);
                printf("Server Config -> Password: %s \n", serverconfigs.ServerPassword);

            } else {
                printf(MSGERRR "ERROR IN CONFIG PARSING" MSGNORM "\n\n");
                ret_exit();
            }

            ++servercount;
        }
        serverconfigs.ServerNum = servercount-2; // minus 2 bc username / password
        printf("Server Config -> Server Count: %d \n", serverconfigs.ServerNum);

        fclose(file);
        
    }
}


/* runs the program through the shell, inits servers */
void lsh_loop(){
    if(VERBOSE) { printf(MSGNORM "--Running Shell Loop--" MSGNORM "\n"); }
    char *cmd = NULL;
    ssize_t cmdlength = 0;
    ssize_t inputcmdlength;
    char command[8], args[64];
    int is_running = 1;

    while(is_running){
        printf("%s@DFS_Client> ", serverconfigs.ServerUsername);
        cmdlength = getline(&cmd, &inputcmdlength, stdin);
        cmd[cmdlength-1] = '\0';
        if(VERBOSE) { printf("    input command: %s \n", cmd); }
        
        if(strcmp(cmd, "LIST") == 0){
            handle_LIST();

        } else if(strncmp(cmd, "GET", 3) == 0) {
            char* GETtok = strtok(cmd, " ");
            char* fname = strtok(NULL, " ");
            if(VERBOSE){ printf("Filename: %s\n", fname); }
            if(fname == NULL){ printf(MSGERRR "No filename specified" MSGNORM "\n");}
            else{ handle_GET(fname); }

        } else if(strncmp(cmd, "PUT", 3) == 0) {
            char* PUTtok = strtok(cmd, " ");
            char* fname = strtok(NULL, " ");
            if(VERBOSE){ printf("Filename: %s\n", fname); }
            if(fname == NULL){ printf(MSGERRR "No filename specified" MSGNORM "\n");}
            else { handle_PUT(fname); }

        } else if(strcmp(cmd, "EXIT") == 0) {
            is_running = handle_EXIT();

        } else {
            printf(MSGERRR "NOT A VALID COMMAND" MSGNORM "\n");
            printf(MSGERRR "NOT COMMANDS:" MSGNORM "\n");
            printf(MSGERRR "    LIST" MSGNORM "\n");
            printf(MSGERRR "    GET <file name>" MSGNORM "\n");
            printf(MSGERRR "    PUT <file name>" MSGNORM "\n");
            printf(MSGERRR "    EXIT" MSGNORM "\n");
        }
    }

    printf("\n");
}


/* handles listing the files from the servers */
int handle_LIST(){
    printf(MSGSUCC "--Listing Files--" MSGNORM "\n");
    /* start off getting file parts */
    char files[FILEPRTS][FILEPRTS];
    int parts_of_file[FILEPRTS][serverconfigs.ServerNum];
    int i; int j;
    for(i = 0; i < FILEPRTS; i++){
        for(j = 0; j < serverconfigs.ServerNum; j++){
            parts_of_file[i][j]=0;
        }
    }

    int numberFiles=0; // number of files starts off at 0
    char buff[MAXBUFF];
    char* bufftok;
    int serverNum;
    for(serverNum = 0; serverNum < NUMSERVS; serverNum++){
        if(send_command_to_server(serverNum, "LIST", buff) < 0){ continue; }
        bufftok = strtok(buff, "\n");

        while(bufftok != NULL){
            char fname[FILENAME];
            int file_piece;
            int is_newFile = 1;

            sscanf(bufftok, ".%s", fname);
            file_piece = fname[strlen(fname)-1] - '0';
            fname[strlen(fname)-2] = '\0';

            for(i=0; i<numberFiles; i++){
                if(strcmp(fname, files[i])){
                    parts_of_file[i][file_piece-1]=1;
                    is_newFile = 0;
                    break;
                }
            }

            if(is_newFile){
                strcpy(files[numberFiles], fname);
                parts_of_file[numberFiles][file_piece]=1;
                ++numberFiles;
            }

            bufftok = strtok(NULL, "\n");

        }

        for(i=0; i < numberFiles; ++i){
            char fname[FILENAME];
            int val=0;
            for(j=0; j<serverconfigs.ServerNum; ++j){
                val += (pow(10, j)*parts_of_file[i][j]);
            }

            strcpy(fname, files[i]);

            if(val < 1111){printf(MSGERRR "Incomplete Name" MSGNORM "\n");}
            else{ printf(".File Name: %s\n", fname);}
        }

    }

}


/* handles getting the file from the servers */
int handle_GET(const char* filename){
    printf(MSGSUCC "--Getting Files--" MSGNORM "\n");

}


/* handles putting the file to the servers */
int handle_PUT(const char* filename){
    printf(MSGSUCC "--Putting Files--" MSGNORM "\n");

}


/* handles exiting program */
int handle_EXIT(){
    printf(MSGSUCC "--EXITING PROGRAM--" MSGNORM "\n");
    return 0;
}


/* connects/sends command to server*/
int send_command_to_server(int serverNumber, char* argcmd, ...){
    printf(MSGSUCC "--Sending Command to Server--" MSGNORM "\n");
    /* stopping here! */
}