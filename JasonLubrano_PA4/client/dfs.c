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
void print_valid_commands(int code);
int send_command_to_server(int serverNumber, char* argcmd, ...);
int socket_connection_routine(const char* port, const char* host);


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
    if(VERBOSE) { printf(MSGTERM "\n--Getting Data from Config--" MSGNORM "\n"); }
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
                serverIP = strtok(NULL, ": ");
                if(VERBOSE){ printf("serverIP: %s \n", serverIP); }
                serverPort = strtok(NULL, ": ");
                if(VERBOSE){ printf("serverPort: %s \n", serverPort); }
                
                strcpy(serverconfigs.ServerNames[servercount], servername);
                strcpy(serverconfigs.ServerAddresses[servercount], serverIP);
                strcpy(serverconfigs.ServerPorts[servercount], serverPort);

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
        printf("Server Config -> Server Number: %d \n", serverconfigs.ServerNum);

        fclose(file);
        
    }
}


/* runs the program through the shell, inits servers */
void lsh_loop(){
    if(VERBOSE) { printf(MSGTERM "\n--Running Shell Loop--" MSGNORM "\n"); }
    char *cmd = NULL;
    ssize_t cmdlength = 0;
    ssize_t inputcmdlength;
    char command[8], args[64];
    int is_running = 1;

    while(is_running){
        printf("%s@DFS_Client> ", serverconfigs.ServerUsername);
        cmdlength = getline(&cmd, &inputcmdlength, stdin);
        cmd[cmdlength-1] = '\0';
        if(VERBOSE) { printf(".cmd: %s\n", cmd); }
        
        if(strcmp(cmd, "LIST") == 0){
            handle_LIST();

        } else if(strncmp(cmd, "GET", 3) == 0) {
            char* GETtok = strtok(cmd, " ");
            char* fname = strtok(NULL, " ");
            if(VERBOSE){ printf("Filename: %s\n", fname); }
            if(fname == NULL){ print_valid_commands(1);}
            else{ handle_GET(fname); }

        } else if(strncmp(cmd, "PUT", 3) == 0) {
            char* PUTtok = strtok(cmd, " ");
            char* fname = strtok(NULL, " ");
            if(VERBOSE){ printf("Filename: %s\n", fname); }
            if(fname == NULL){ print_valid_commands(1);}
            else { handle_PUT(fname); }

        } else if(strcmp(cmd, "EXIT") == 0) {
            is_running = handle_EXIT();

        } else {
            print_valid_commands(0);
        }
    }

    printf("\n");
}


/* print valid commands to prompt */
void print_valid_commands(int code){
    switch (code)
    {
    case 0:
        printf(MSGERRR "Not a valid command!" MSGNORM "\n");
        break;

    case 1:
        printf(MSGERRR "No filename specified" MSGNORM "\n");
        break;

    default:
        break;
    }

    printf(MSGTERM "VALID COMMANDS:" MSGNORM "\n");
    printf(MSGTERM "    LIST" MSGNORM "\n");
    printf(MSGTERM "    GET <file name>" MSGNORM "\n");
    printf(MSGTERM "    PUT <file name>" MSGNORM "\n");
    printf(MSGTERM "    EXIT" MSGNORM "\n");

}



/* handles listing the files from the servers */
int handle_LIST(){
    printf(MSGTERM "\n--Listing Files--" MSGNORM "\n");

    char files[FILEPRTS][FILEPRTS];
    int parts_of_file[FILEPRTS][serverconfigs.ServerNum];
    int i; int j;
    for(i = 0; i < FILEPRTS; i++){
        for(j = 0; j < serverconfigs.ServerNum; j++){
            parts_of_file[i][j]=0;
        }
    }

    int numberFiles=0;
    char buff[MAXBUFF];
    char* bufftok;
    int serverNum;
    for(serverNum = 0; serverNum < NUMSERVS; serverNum++){
        printf("\n");
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
    printf(MSGTERM "\n--Getting Files--" MSGNORM "\n");
	char request[128];
	sprintf(request, "GET .%s.", filename);

	srand(time(NULL));
	int start = ((rand() % 2) * 2);

    int numberOfServers = serverconfigs.ServerNum;
    char chonkOfFile[numberOfServers][MAXBUFF];
    char chonkBuffAlpha[MAXBUFF];
	char chonkBuffBravo[MAXBUFF];
	sprintf(chonkBuffAlpha, "%p %p", &(chonkOfFile[0]), &(chonkOfFile[1]));
	sprintf(chonkBuffBravo, "%p %p", &(chonkOfFile[2]), &(chonkOfFile[3]));

    int serverIndex[numberOfServers];
	int returnValue = 0;
    int numberOfPieces = 0;
    if ((returnValue = send_command_to_server(start, request, chonkBuffAlpha)) < 0) {
		if ((returnValue = send_command_to_server(start+1, request, chonkBuffAlpha)) < 0) {
			printf(MSGWARN "WARN: File may be incomplete (code 1)" MSGNORM "\n");
			return 1;
		} else {
			serverIndex[0] = returnValue % 10;
			serverIndex[1] = returnValue / 10;
			numberOfPieces +=2;
            if(VERBOSE){ printf(".serverIndex[0]: %d", serverIndex[0]);}
            if(VERBOSE){ printf(".serverIndex[1]: %d", serverIndex[1]);}
            if(VERBOSE){ printf(".numberOfPieces: %d", numberOfPieces);}
		}

		if ((returnValue = send_command_to_server((start+3)%numberOfServers, request, chonkBuffBravo)) < 0) {
			printf(MSGWARN "WARN: File may be incomplete (code 2)" MSGNORM "\n");
			return 1;
		} else {
			serverIndex[2] = returnValue % 10;
			serverIndex[3] = returnValue / 10;
			numberOfPieces +=2;
            if(VERBOSE){ printf(".serverIndex[2]: %d", serverIndex[2]);}
            if(VERBOSE){ printf(".serverIndex[3]: %d", serverIndex[3]);}
            if(VERBOSE){ printf(".numberOfPieces: %d", numberOfPieces);}
		}
	} else {
		serverIndex[0] = returnValue % 10;
		serverIndex[1] = returnValue / 10;
		numberOfPieces +=2;
        if(VERBOSE){ printf(".serverIndex[0]: %d", serverIndex[0]);}
        if(VERBOSE){ printf(".serverIndex[1]: %d", serverIndex[1]);}
        if(VERBOSE){ printf(".numberOfPieces: %d", numberOfPieces);}
	}

	if (numberOfPieces < numberOfServers) {
		if ((returnValue = send_command_to_server((start+2)%numberOfServers, request, chonkBuffBravo)) < 0) {
			if ((returnValue = send_command_to_server(start+1, request, chonkBuffAlpha)) < 0) {
				printf(MSGWARN "WARN: File may be incomplete (code 3)" MSGNORM "\n");
				return 1;
			} else {
				serverIndex[0] = returnValue % 10;
				serverIndex[1] = returnValue / 10;
				numberOfPieces +=2;
                if(VERBOSE){ printf(".serverIndex[0]: %d", serverIndex[0]);}
                if(VERBOSE){ printf(".serverIndex[1]: %d", serverIndex[1]);}
                if(VERBOSE){ printf(".numberOfPieces: %d", numberOfPieces);}
			}

			if ((returnValue = send_command_to_server((start+3)%numberOfServers, request, chonkBuffBravo)) < 0) {
				printf(MSGWARN "WARN: File may be incomplete (code 4)" MSGNORM "\n");
				return 1;
			} else {
				serverIndex[2] = returnValue % 10;
				serverIndex[3] = returnValue / 10;
				numberOfPieces +=2;
                if(VERBOSE){ printf(".serverIndex[2]: %d", serverIndex[2]);}
                if(VERBOSE){ printf(".serverIndex[3]: %d", serverIndex[3]);}
                if(VERBOSE){ printf(".numberOfPieces: %d", numberOfPieces);}
			}
		} else {
			serverIndex[2] = returnValue % 10;
			serverIndex[3] = returnValue / 10;
			numberOfPieces +=2;
            if(VERBOSE){ printf(".serverIndex[2]: %d", serverIndex[2]);}
            if(VERBOSE){ printf(".serverIndex[3]: %d", serverIndex[3]);}
            if(VERBOSE){ printf(".numberOfPieces: %d", numberOfPieces);}
		}
	}

    char fileBuffer[MAXBUFF*numberOfServers];
	strcpy(fileBuffer, "\0");
	int piece = 1;
	int i;
	for(i = 0; piece < numberOfServers+1; i++) {
		if (serverIndex[i] == piece) {
			strcat(fileBuffer, chonkOfFile[i]);
			i = -1;
			piece++;
		}
		if (i == numberOfServers) 
			break;
	}

	char locationOfFile[MAXBUFF];
	sprintf(locationOfFile, "%s/%s", "./dfsfiles", filename);
    if(VERBOSE) { printf(".Location of File: %s\n", locationOfFile); }

    FILE* fileptr;
	if ((fileptr = fopen(locationOfFile, "w")) < 0){
        printf(MSGERRR "Failed to open file" MSGNORM "\n");
    }

	fwrite(fileBuffer, sizeof(char), strlen(fileBuffer), fileptr);

	if(VERBOSE) { printf(MSGSUCC "File Saved Successfully"); }

	fclose(fileptr);
}


/* handles putting the file to the servers */
int handle_PUT(const char* filename){
    printf(MSGTERM "\n--Putting Files--" MSGNORM "\n");

    char locationOfFile[128];
	sprintf(locationOfFile, "%s/%s", "./dfsfiles", filename);
    if(VERBOSE) { printf(".Location of File: %s\n", locationOfFile); }

    /* error handling */
	int argfd;
    struct stat statFile;
	if ((argfd = open(locationOfFile, O_RDONLY)) < 0){
        printf(MSGERRR "ERROR: Failed to open file.\n     Does the server have it?" MSGNORM "\n");
        ret_exit;
    } else if (fstat(argfd, &statFile) < 0){
        printf(MSGERRR "ERROR: fstat file file location" MSGNORM "\n");
    } else {        
        long fileSize;
        fileSize = statFile.st_size;

        MD5_CTX md5_ctx_object;
        MD5_Init(&md5_ctx_object);

        int numBytesRead;
        char buffReadFromFile[MAXBUFF];
        while ((numBytesRead = read(argfd, &buffReadFromFile, MAXBUFF)) != 0)
            MD5_Update(&md5_ctx_object, buffReadFromFile, numBytesRead);

        char md5_sum[MD5_DIGEST_LENGTH];
        MD5_Final(md5_sum, &md5_ctx_object);
        if(VERBOSE){ printf(".md5_sum: %s\n", md5_sum);}
        
        char soloByte[1];
        sprintf(soloByte, "%02x", md5_sum[0]);
        int fileOrder;
        int serverCount = serverconfigs.ServerNum;
        fileOrder = (strtol(soloByte, NULL, 16) % serverCount);
        if(VERBOSE){ printf(".fileOrder: %d \n", fileOrder);}
        if(VERBOSE){ printf(".serverCount: %d \n", serverCount);}

        close(argfd);

        int serverNumOrder[serverconfigs.ServerNum][2];
        int i;
        int serv_i;
        int i_mod_servCt;
        int im1;
        for (i = 1; i < serverCount + 1; i++) {
            serv_i = (i+serverCount-fileOrder)%serverCount;
            im1 = i-1;
            i_mod_servCt = i % serverCount;

            serverNumOrder[serv_i][0] = im1;
            serverNumOrder[serv_i][1] = i_mod_servCt;
        }

        char filePiecerequest[128];
        char request[MAXBUFF];
        sprintf(request, "PUT .%s.", filename);
        if(VERBOSE){ printf(".request: %s \n", request);}


        int byteOffset = 0;
        char filePiece[128];
        for (i = 0; i < serverCount - 1; i++) {
            int chonkOfFile = (fileSize/serverCount);
            int chonkOfFileEven = chonkOfFile % 2;
            if (chonkOfFileEven == 1){ --chonkOfFile; }
            
            sprintf(filePiece, "%s%d", request, i+1);
            if(VERBOSE){ printf(".filePiece: %s \n", filePiece);}

            send_command_to_server(serverNumOrder[i][0], filePiece, chonkOfFile, locationOfFile, byteOffset);
            send_command_to_server(serverNumOrder[i][1], filePiece, chonkOfFile, locationOfFile, byteOffset);
            byteOffset += chonkOfFile;
        }

        sprintf(filePiece, "%s%d", request, serverCount);
        if(VERBOSE){ printf(".filePiece: %s \n", filePiece);}

        send_command_to_server(serverNumOrder[serverCount-1][0], filePiece, (fileSize - byteOffset), locationOfFile, byteOffset);
        send_command_to_server(serverNumOrder[serverCount-1][1], filePiece, (fileSize - byteOffset), locationOfFile, byteOffset);
    }
}


/* handles exiting program */
int handle_EXIT(){
    printf(MSGTERM "\n--EXITING PROGRAM--" MSGNORM "\n");
    return 0;
}


/* TCP routine to connect sockets */
int socket_connection_routine(const char* port, const char* host){
    if(VERBOSE){printf("\n" MSGTERM "--Running Socket Connection Routine--" MSGNORM "\n"); }
	struct sockaddr_in sockAddrIn;
    struct hostent *hostentry;
	memset(&sockAddrIn, 0, sizeof(sockAddrIn));
    sockAddrIn.sin_family = AF_INET;
    sockAddrIn.sin_addr.s_addr = INADDR_ANY;
    sockAddrIn.sin_port = htons((unsigned short)atoi(port));

    if(sockAddrIn.sin_port == 0){ printf(MSGERRR "ERROR: UNABLE TO PORT ON: %s" MSGNORM "\n", port); ret_exit();}
    else {printf(MSGSUCC "Port on: %s Established" MSGNORM "\n", port);}

    if(hostentry = gethostbyname(host)){
        memcpy(&sockAddrIn.sin_addr, hostentry->h_addr, hostentry->h_length);
        printf(MSGSUCC "Hostentry established" MSGNORM "\n");
    } else if ((sockAddrIn.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE) {
        printf(MSGERRR "Unable to get hostentry" MSGNORM "\n");
    }

    int sockfd;
    sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0){ printf(MSGERRR "ERROR: UNABLE TO CREATE SOCKET" MSGNORM "\n"); ret_exit(); }
    else{ printf(MSGSUCC "Socket Created Successfully" MSGNORM "\n"); }

    if(connect(sockfd, (struct sockaddr*)&sockAddrIn, sizeof(sockAddrIn))<0){
        return -1;
    } else {
        printf(MSGSUCC "Socket Created Successfully" MSGNORM "\n");
        return sockfd;
    }

}


/* connects/sends command to server*/
int send_command_to_server(int serverNumber, char* argcmd, ...){
    printf("\n" MSGTERM "--Sending Command to Server--" MSGNORM "\n");
    /* stopping here! */
    int sockfd;
    va_list va_args;
    vprintf(argcmd, va_args);
    
    sockfd = socket_connection_routine(serverconfigs.ServerPorts[serverNumber],
                        serverconfigs.ServerAddresses[serverNumber]);

    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(sockfd, &fdset);
    timeout.tv_sec=1;
    va_start(va_args, argcmd);

    char userAuthentication[MAXBUFF];
    sprintf(userAuthentication, "Username: %s Password: %s",
            serverconfigs.ServerUsername, serverconfigs.ServerPassword);
    if(VERBOSE){ printf(".userAuthentication: %s \n", userAuthentication);}

    if(write(sockfd, userAuthentication, strlen(userAuthentication)) < 0){
        printf(MSGERRR "ERROR: Connection to Server Unavailable" MSGNORM "\n");
        printf("    Server: %s Unavailable\n", serverconfigs.ServerNames[serverNumber]);
    } else {
        printf("User authentication written OK\n");
    }
    

	struct timespec nanotimeout;
	nanotimeout.tv_sec = 0;
	nanotimeout.tv_nsec = 100000000L; /* 0.1 seconds */
    nanosleep(&nanotimeout, NULL);

    if(VERBOSE){ printf(".argcmd: %s \n", argcmd);}
    if (write(sockfd, argcmd, strlen(argcmd)) < 0){
        printf(MSGERRR "ERROR: Unable to write to server" MSGNORM "\n");
    } else {
        printf("Wrote command written OK\n ");
    }

    char response[MAXBUFF];
    int responseValue;
    int responseLength;
    responseValue = select(sockfd+1, &fdset, NULL, NULL, &timeout);
    if(VERBOSE){ printf(".responseValue: %d \n", responseValue);}

    if (responseValue > 0) {
        responseLength = recv(sockfd, &response, MAXBUFF, 0);
        response[responseLength] = '\0';
        if(VERBOSE){ printf(".response: %s \n", response);}

        /* The server is sending blanks for some reason... */ 
        // HANDLE Commands!
        if(strncmp(response, "Auth Approved. Listing Files...", 31) == 0) {
            if(VERBOSE){printf("---Sending/Receiving LIST---\n");}

            char buffargs[MAXBUFF];
            char* response = va_arg(va_args, char *);

            if ((responseValue = recv(sockfd, buffargs, MAXBUFF, 0)) < 0){
                printf(MSGERRR "ERROR: File receive failed" MSGNORM "\n");
            } else {
                printf("receive successful: %d", responseValue);
            }

            strcpy(response, buffargs);
            return responseValue;

        } else if (strncmp(response, "Auth Approved. Send em!", 23) == 0) {
            if(VERBOSE){printf("---Sending/Receiving PUT---\n");}
            char bytesFromFile[256];
            int bytesLeft = va_arg(va_args, off_t);
            sprintf(bytesFromFile, "%d", bytesLeft);

            char* locationOfFile;
            locationOfFile = va_arg(va_args, char *);

            off_t chunckOffset = va_arg(va_args, int);
            int argfd;
            struct stat statFile;

            /* Error Handling */
            if (write(sockfd, bytesFromFile, sizeof(bytesFromFile)) < 0){
                printf(MSGERRR "ERROR: Error on echo write" MSGNORM "\n");
            } else if ((argfd = open(locationOfFile, O_RDONLY)) < 0){
                printf("ERROR: failed to open file" MSGNORM "\n");
            } else if (fstat(argfd, &statFile) < 0){
                printf(MSGERRR "ERROR: Unable to get File Attributes" MSGNORM "\n");
            } else {
                printf(".Passes Erros");
            }
            
            int numBytesSent = 0;
            while (((numBytesSent = sendfile(sockfd, argfd, &chunckOffset, bytesLeft)) >= 0) && (bytesLeft > 0)) {
                bytesLeft -= numBytesSent;
            }

            return 0;
        } else if (strncmp(response, "Auth Approved. Prepare for files.", 33) == 0) {
            if(VERBOSE){printf("---Sending/Receiving GET---");}
            char *va_arg_ptrs = va_arg(va_args, char *);
            char* chonks_of_ptrs[2];
            sscanf(va_arg_ptrs, "%p %p", &chonks_of_ptrs[0], &chonks_of_ptrs[1]);

            char buffargs[MAXBUFF];

            if ((responseValue = recv(sockfd, buffargs, MAXBUFF, 0)) < 0){
                printf(MSGERRR "ERROR: Failed to receive file size" MSGNORM "\n");
            }

            int	numberOfFiles = 0;
            sscanf(buffargs, "Files: %d", &numberOfFiles);

            int numberOfFilesReceived = 0;
            int bytesLeft;
            int returnValue = 0;
            while (numberOfFilesReceived < numberOfFiles) {
                int bytesFromFile = 0;
                int fileIndx;

                if ((responseValue = recv(sockfd, buffargs, MAXBUFF, 0)) < 0){
                    printf(MSGERRR "ERROR: Failed to receive file" MSGNORM "\n");
                }
                sscanf(buffargs, "%d %d", &bytesFromFile, &fileIndx);

                bytesLeft = bytesFromFile;
                char chunk_buf[bytesFromFile];

                while ((bytesLeft > 0) && ((responseLength = recv(sockfd, chunk_buf, MAXBUFF, 0)) > 0)) {
                    bytesLeft = bytesLeft - responseLength;
                }

                chunk_buf[bytesFromFile] = '\0';
                strcpy(chonks_of_ptrs[numberOfFilesReceived], chunk_buf);
                returnValue += (pow(10, numberOfFilesReceived)*fileIndx);

                numberOfFilesReceived++;
            }
            va_end(va_args);
            return returnValue;
        } else if (!strncmp(response, "Auth Denied. Invalid username or password.", 42)) {
            printf(MSGERRR "ERROR: Auth Denied. Invalid username or password.\n" MSGNORM "\n");
        } else {
            printf(MSGERRR "ERROR: The server sent something wacky." MSGNORM "\n");
            return -1;
        }
        if(VERBOSE) {printf("..secret code..");}
        return -1;
    }
    if (responseValue < 0){printf(MSGERRR "ERROR: Error in selecting" MSGNORM "\n");}
    else if (responseValue == 0){printf(MSGERRR "\nERROR: Connection timeouted" MSGNORM "\n"); }

    va_end(va_args);
    return -1;
}