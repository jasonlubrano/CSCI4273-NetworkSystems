/* 
 * Jason Lubrano
 * CSCI4273 - Ha - PA4
 * client/dfcli.c - client side of the distributed filePtr sharing system
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
 * verbosity
 */
#define VERBOSE 1


/*
 * lengths
 */
#define NUMSOCKS	2		/* 2 socks for one file i.e. 1(1,2) 2(2,3)....*/
#define NUMSERVS    4       /* max number of dfsServers */
#define SERVPORT    8       /* max server serverPort genBuffLen */
#define SERVNAME    16      /* max server name */
#define FILENAME    32      /* max size of filePtr name*/  
#define SERVADDR    64      /* max server serverAddress */
#define FILEPRTS    64      /* max num of filePtr fileBlocks */
#define MAXLINE     256     /* max text line genBuffLen */
#define FILELST     1024    /* second argument to listen() */
#define MAXFILE		2048	/* max size of line in a filePtr */
#define MAXBUFF     8192    /* max text line genBuffLen */


#define INVALIDCMDS 0
#define NOCONFIGSPC 1
#define HOMEMESSAGE NUMSOCKS


/* functions that are used trhoughout */
void ret_exit();
void handle_PUT(void);
void handle_GET(void);
void handle_LIST(void);
void welcome_message();
void closing_message();
void print_valid_commands(int code);
void get_data_from_config(char* config_filename);
void get_host_info(int serverNum, char* serverAddress, char* serverPort);
void socket_connection_routine_cli_to_servs(int* sockfd, int index, char* nameUser);
void md5sum_hash_file_ptrs(FILE* filePtr, int* fptr1, int* fptr2, int* fptr3, int* fptr4);


/* global vars */
char dfsServers[NUMSERVS][MAXLINE];
int dfsServerConfigs[NUMSERVS][NUMSERVS][NUMSOCKS];
int serverCount = 0;
char username[MAXLINE];
char password[MAXLINE];

int main(int argc, char** argv){
	welcome_message();

    char* configFile;
    switch (argc)
    {
    case NUMSOCKS:
        configFile = argv[1];
        if(VERBOSE){ printf(MSGNORM ".configFile: %s" MSGNORM "\n", configFile); }
        break;
    default:
        print_valid_commands(NOCONFIGSPC);
		ret_exit();
        break;
    }

	strcpy(username, "");
	strcpy(password, "");

	get_data_from_config(configFile);

	printf(MSGTERM "DFS Server List" MSGNORM "\n");
	for(int i=0; i<serverCount; i++){
		printf("\tDFS Servers: %s\n", dfsServers[i]);
	}

	int isRunning = 1;
	while(isRunning){
		printf("\n\n---------------------------------------------------------------\n\n");
		int userCommand;
		print_valid_commands(HOMEMESSAGE);
		
		printf("jason@dflcli$ ");
		if(scanf("%d", &userCommand) >= 0){
			
			switch (userCommand)
			{
			case 1:
				/* code */
				handle_LIST();
				break;
			case 2:
				handle_GET();
				break;
			case 3:
				handle_PUT();
				break;
			case 4:
				ret_exit();
				break;
			default:
				print_valid_commands(INVALIDCMDS);
				break;
			}
		}
		while(getchar() != '\n');
	}
}


/***************************************************************************
 * function definitons
 ***************************************************************************/


/* print valid commands to prompt */
void print_valid_commands(int code){
    int showCommands = 0;
	switch (code)
    {
    case 0:
        printf(MSGERRR "Not a valid command!" MSGNORM "\n");
        break;

    case 1:
        printf(MSGERRR "No filename specified" MSGNORM "\n");
		printf(MSGERRR "\tUsage: ./dfscli <Config File>" MSGNORM "\n");
        break;

	case NUMSOCKS:
		showCommands = 1;

    default:
        break;
    }

	if(showCommands){
		printf(MSGTERM "VALID COMMANDS:" MSGNORM "\n");
		printf(MSGTERM "\t(1) LIST" MSGNORM "\n");
		printf(MSGTERM "\t(2) GET <file name>" MSGNORM "\n");
		printf(MSGTERM "\t(3) PUT <file name>" MSGNORM "\n");
		printf(MSGTERM "\t(4) EXIT (in beta)" MSGNORM "\n");
	}
	showCommands = 0;
}


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


/* gets the data from the config file */
void get_data_from_config(char* config_filename){
	printf(MSGTERM "\n--Getting Data from Config--" MSGNORM "\n");
	
	FILE* filePtr = fopen(config_filename, "r");
	if(filePtr == NULL){
		printf(MSGERRR "\tConfig file is NULL" MSGNORM "\n");
		printf(MSGERRR "\t\tDoes it exist?" MSGNORM "\n");
		ret_exit();
	} else {
		printf(MSGSUCC "\tConfig file opened successfully" MSGNORM "\n");
	}
	
	/* sets up the config table for blockchaining */
	/* first server, fptr1 */
	dfsServerConfigs[0][0][0] = 1;
	dfsServerConfigs[0][0][1] = NUMSOCKS;
	
	/* first server, fptr2 */
	dfsServerConfigs[0][1][0] = NUMSOCKS;
	dfsServerConfigs[0][1][1] = 3;
	
	/* first server, fptr3 */
	dfsServerConfigs[0][NUMSOCKS][0] = 3;
	dfsServerConfigs[0][NUMSOCKS][1] = NUMSERVS;
	
	/* first server, fptr4 */
	dfsServerConfigs[0][3][0] = NUMSERVS;
	dfsServerConfigs[0][3][1] = 1;

	/* second server, fptr1 */
	dfsServerConfigs[1][0][0] = NUMSERVS;
	dfsServerConfigs[1][0][1] = 1;

	/* second server, fptr2 */
	dfsServerConfigs[1][1][0] = 1;
	dfsServerConfigs[1][1][1] = NUMSOCKS;
	
	/* second server, fptr3 */
	dfsServerConfigs[1][NUMSOCKS][0] = NUMSOCKS;
	dfsServerConfigs[1][NUMSOCKS][1] = 3;
	
	/* second server, fptr4 */
	dfsServerConfigs[1][3][0] = 3;
	dfsServerConfigs[1][3][1] = NUMSERVS;

	/* third server, fptr1 */
	dfsServerConfigs[NUMSOCKS][0][0] = 3;
	dfsServerConfigs[NUMSOCKS][0][1] = NUMSERVS;
	
	/* third server, fptr2 */
	dfsServerConfigs[NUMSOCKS][1][0] = NUMSERVS;
	dfsServerConfigs[NUMSOCKS][1][1] = 1;
	
	/* third server, fptr3 */
	dfsServerConfigs[NUMSOCKS][NUMSOCKS][0] = 1;
	dfsServerConfigs[NUMSOCKS][NUMSOCKS][1] = NUMSOCKS;
	
	/* third server, fptr4 */
	dfsServerConfigs[NUMSOCKS][3][0] = NUMSOCKS;
	dfsServerConfigs[NUMSOCKS][3][1] = 3;

	/* fourth server, fptr1 */
	dfsServerConfigs[3][0][0] = NUMSOCKS;
	dfsServerConfigs[3][0][1] = 3;
	
	/* fourth server, fptr2 */
	dfsServerConfigs[3][1][0] = 3;
	dfsServerConfigs[3][1][1] = NUMSERVS;
	
	/* fourth server, fptr3 */
	dfsServerConfigs[3][NUMSOCKS][0] = NUMSERVS;
	dfsServerConfigs[3][NUMSOCKS][1] = 1;
	
	/* fourth server, fptr4 */
	dfsServerConfigs[3][3][0] = 1;
	dfsServerConfigs[3][3][1] = NUMSOCKS;

	char lineInFile[MAXFILE];
	while(fgets(lineInFile, sizeof(lineInFile), filePtr) != NULL){
		if(VERBOSE){ printf(".lineInFile: %s\n", lineInFile); }		
		
		if(strstr(lineInFile, "Server") != NULL){
			strcpy(dfsServers[serverCount], "");

			char* lineTok = strtok(lineInFile, " ");
			if(VERBOSE){ printf(".lineTok: %s\n", lineTok); }
			
			int position = 0;
			
			while(lineTok != NULL){
				if(strcmp(lineTok, "Server") == 0 && position == 0){
					position++;
				} else if(position == 1){
					strcat(dfsServers[serverCount], lineTok);
					strcat(dfsServers[serverCount], ":");
					if(VERBOSE){ printf(".dfsServers[serverCount]: %s\n", dfsServers[serverCount]); }
					
					position++;
				} else if(position == NUMSOCKS){
					strcat(dfsServers[serverCount], lineTok);
					if(VERBOSE){ printf(".dfsServers[serverCount]: %s\n", dfsServers[serverCount]); }

					position++;
				}

				lineTok = strtok(NULL, " ");
			}

			int variableLength = strlen(dfsServers[serverCount]);
			
			dfsServers[serverCount][variableLength-1] = 0;
			serverCount++;
			if(VERBOSE){ printf(".serverCount: %d\n", serverCount);}
		} else if(strstr(lineInFile, "Username") != NULL) {
			char* lineTok = strtok(lineInFile, " ");
			if(VERBOSE){ printf(".lineTok: %s\n", lineTok); }

			int position = 0;
			
			while(lineTok != NULL){
				if(VERBOSE){ printf(".lineTok: %s\n", lineTok); }
				if(position == 1){
					strcpy(username, lineTok);
					if(VERBOSE){ printf(".username: %s\n", username); }
					int variableLength = strlen(username);
					username[variableLength-1] = 0;		
				} else {
					position++;
				}
				lineTok = strtok(NULL, " ");
			}
		} else if(strstr(lineInFile, "Password") != NULL) {
			char* lineTok = strtok(lineInFile, " ");
			if(VERBOSE){ printf(".lineTok: %s\n", lineTok); }

			int position = 0;
			while(lineTok != NULL){
				if(VERBOSE){ printf(".lineTok: %s\n", lineTok); }
				if(position == 1){
					strcpy(password, lineTok);
					if(VERBOSE){ printf(".password: %s\n", password); }

					int variableLength = strlen(password);
					password[variableLength-1] = 0;
					if(VERBOSE){ printf(".password: %s\n", password); }
				} else {
					position++;
				}

				lineTok = strtok(NULL, " ");
			}
		}	
	}
	fclose(filePtr);
}


/* Handles getting host info */
void get_host_info(int serverNum, char* serverAddress, char* serverPort){
	printf(MSGTERM "\n--Getting Info from Host--" MSGNORM "\n");
	
	char servBuff[MAXFILE];
	strcpy(servBuff, dfsServers[serverNum]);
	if(VERBOSE){ printf(".servBuff: %s\n", servBuff); }
	
	char* lineTok = strtok(servBuff, ":");
	if(VERBOSE){ printf(".lineTok: %s\n", lineTok); }

	int position = 0;
	while(lineTok != NULL){
		if(position == 1){
			strcpy(serverAddress, lineTok);
			if(VERBOSE){ printf(".serverAddress: %s\n", serverAddress); }
		}
		else if(position == NUMSOCKS){
			strcpy(serverPort, lineTok);
			if(VERBOSE){ printf(".serverPort: %s\n", serverPort); }
		}

		lineTok = strtok(NULL, ":");
		if(VERBOSE){ printf(".servBuff: %s\n", servBuff); }
		position++;
	}
}


/* handles getting the LIST command from the user */
void handle_LIST(void){
	printf(MSGTERM "\n--Handeling LIST command--" MSGNORM "\n");
	int sockfd;
	struct sockaddr_in sockAddrIn;
	struct hostent *hostentry;
	int fileCount = 0;
	char fileList[30][FILELST];
	int flags = 0;
	if(VERBOSE){ printf(".serverCount: %d\n", serverCount); }
	for(int i = 0; i < serverCount; i++){
		char serverAddress[MAXBUFF];
		char serverPort[MAXBUFF];
		
		/* establishes the socket to the server */
		get_host_info(i, serverAddress, serverPort);

		memset(&sockAddrIn, 0, sizeof(sockAddrIn));
		sockAddrIn.sin_family = AF_INET;
		sockAddrIn.sin_addr.s_addr = INADDR_ANY;
		sockAddrIn.sin_port = htons((unsigned short)atoi(serverPort));

		{
		in_addr_t in_addr = inet_addr(serverAddress);
		if (INADDR_NONE == in_addr)
		{
			perror("inet_addr() failed");
			abort(); /* or whatever error handling you choose. */
		}

		sockAddrIn.sin_addr.s_addr = in_addr;
		}

		if(sockAddrIn.sin_port == 0){ printf(MSGERRR "ERROR: UNABLE TO PORT ON: %s" MSGNORM "\n", serverPort); ret_exit();}
    	else {printf(MSGSUCC "Port on: %s Established" MSGNORM "\n", serverPort);}
		
		if(hostentry = gethostbyname(serverAddress)){
			memcpy(&sockAddrIn.sin_addr, hostentry->h_addr, hostentry->h_length);
			printf(MSGSUCC "Hostentry established" MSGNORM "\n");
		} else if ((sockAddrIn.sin_addr.s_addr = inet_addr(serverAddress)) == INADDR_NONE) {
			printf(MSGERRR "Unable to get hostentry" MSGNORM "\n");
		}

		sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(sockfd < 0){ printf(MSGERRR "ERROR: UNABLE TO CREATE SOCKET" MSGNORM "\n"); ret_exit(); }
    	else{ printf(MSGSUCC "Socket Created Successfully" MSGNORM "\n"); }

		/* routine to connect */
		int sock_connect = connect(sockfd, (struct sockaddr*)&sockAddrIn, sizeof(sockAddrIn));
		if(VERBOSE){ printf(".sock_connect: %d\n", sock_connect);}
		if(sock_connect < 0){
			/* why is it not being able to connect ot hthe sockets? */
			printf(MSGERRR "\tUnable to connect..." MSGNORM "\n");
			int errnum = errno;
			fprintf(stderr, "Value of errno: %d\n", errno);
			perror("Error connecting socket by perror");
			fprintf(stderr, "Error connecting socket: %s\n", strerror( errnum ));
		} else {
			printf(MSGERRR "\tSocket Connection successful." MSGNORM "\n");
			char* listBuff = "LIST\nUSERNAME: %s\nPASSWORD: %s\n\n";
			if(VERBOSE){ printf(".listBuff: %s\n", listBuff); }

			char sendBuff[MAXBUFF];
			sprintf(sendBuff, listBuff, username, password);
			if(VERBOSE){ printf(".sendBuff: %s\n", sendBuff); }
			send(sockfd, sendBuff, strlen(sendBuff), MSG_NOSIGNAL);

			char recvBuff[MAXBUFF];
			char packBuff[MAXBUFF];
			
			strcpy(packBuff, "");

			int n;
			recv(sockfd, recvBuff, sizeof(recvBuff), 0);
			if(VERBOSE){ printf(".recvBuff: %s\n", recvBuff); }

			strcat(packBuff, recvBuff);
			if(VERBOSE){ printf(".packBuff: %s\n", packBuff); }

			bzero(recvBuff, MAXBUFF);

			while(n = recv(sockfd, recvBuff, sizeof(recvBuff), MSG_PEEK) > 0){
				if(VERBOSE){ printf(".recvBuff: %s\n", recvBuff); }
				if(strlen(recvBuff) > 0){
					strcat(packBuff, recvBuff);
					if(VERBOSE){ printf(".packBuff: %s\n", packBuff); }

					char extraBuff[MAXBUFF];
					recv(sockfd, extraBuff, sizeof(extraBuff), 0);
					if(VERBOSE){ printf(".extraBuff: %s\n", extraBuff); }
				}

				bzero(recvBuff, MAXBUFF);
			}
			char packetBuffCpy[MAXFILE];
			strcpy(packetBuffCpy, packBuff);
			if(VERBOSE){ printf(".packetBuffCpy: %s\n", packetBuffCpy); }
			int position = 0;
			
			char* lineTok = strtok(packBuff, "\n");
			if(VERBOSE){ printf(".lineTok: %s\n", lineTok); }
			
			while(lineTok != NULL){
				if(VERBOSE){ printf(".lineTok: %s\n", lineTok); }
				if(position == 0){
					if(!!strcmp(lineTok, "LIST")){
						if(VERBOSE){ printf(".packetBuffCpy: %s\n", packetBuffCpy); }
						flags = 1;
					}
				} else if(!flags && position > 1){
					int is_locFound = 0; /* bool if the location is found */
					char lineBuff[MAXBUFF];
					strcpy(lineBuff, lineTok);
					if(VERBOSE){ printf(".lineBuff: %s\n", lineBuff); }
					
					for(int j = 0; j < strlen(lineBuff); j++){
						lineBuff[j] = lineBuff[j+1];
					}
					if(VERBOSE){ printf(".lineBuff: %s\n", lineBuff); }

					lineBuff[strlen(lineBuff)-NUMSOCKS]='\0';

					char filePiece[NUMSOCKS];

					filePiece[0] = lineTok[strlen(lineTok)-1];
					filePiece[1] = '\0';
					if(VERBOSE){ printf(".filePiece: %s\n", filePiece); }

					for(int k = 0; k < fileCount; k++){
						if(strstr(fileList[k], lineBuff) != NULL){
							strcat(fileList[k], ":");
							strcat(fileList[k], filePiece);
							is_locFound = 1;
							break;
						}
					}

					if(is_locFound == 0){
						char filePtr[MAXFILE];
						strcpy(filePtr, lineTok);
						if(VERBOSE){ printf(".filePtr: %s\n", filePtr); }
						for(int j = 0; j < strlen(filePtr); j++){
							filePtr[j] = filePtr[j+1];
						}
						if(VERBOSE){ printf(".filePtr: %s\n", filePtr); }

						int short2FilePtrLen = strlen(filePtr)-NUMSOCKS;
						filePtr[short2FilePtrLen] = '\0';
						strcpy(fileList[fileCount], filePtr);
						strcat(fileList[fileCount], ":");
						strcat(fileList[fileCount], filePiece);
						if(VERBOSE){ printf(".fileList: %s\n", filePtr); }
						fileCount++;
					}					
				} else if (flags == 1 && position > 0){
					if(VERBOSE){ printf(".lineTok: %s\n", lineTok); }
				} else {
					if(VERBOSE){ printf(".Youre not supposed to be here.\n"); }
				}
				position++;
				lineTok = strtok(NULL, "\n");
			}
		}
	}

	/* we didnt get any flags, which is good */
	if(!flags){
		printf(MSGTERM "\tListing Files..." MSGNORM "\n");
		for(int i = 0; i < fileCount; i++){
			char nameOfFile[512];
			char lineOfFile[FILELST];

			strcpy(lineOfFile, fileList[i]);
			if(VERBOSE){ printf(".lineOfFile: %s\n", lineOfFile); }

			char* lineTok = strtok(lineOfFile, ":");
			if(VERBOSE){ printf(".lineTok: %s\n", lineTok); }

			int position = 0;
			char piecesReceived[NUMSERVS];

			int j;
			for(j=0;j<NUMSERVS;j++){
				piecesReceived[j] = 0;
			}
			if(VERBOSE){ printf(".piecesReceived: %s\n", piecesReceived); }

			while(lineTok != NULL){
				if(position == 0){
					strcpy(nameOfFile, lineTok);
				} else {
					int filePiece = lineTok[0] - '0' - 1;
					piecesReceived[filePiece] = 1;
				}
				
				lineTok = strtok(NULL, ":");
				position++;
			}

			int is_partReceived=1;
			
			for(j = 0; j < NUMSERVS; j++){
				if(piecesReceived[j] == 0){
					is_partReceived = 0;
					break;
				}
			}

			if(!is_partReceived){
				printf(MSGWARN "\tFile incomplete" MSGNORM "\n");
				printf("\t\t %s\n", nameOfFile);
			} else {
				printf(MSGTERM "\tFile complete" MSGNORM "\n");
				printf("\t\t%s\n", nameOfFile);
			}
		}
	}
}


/* handles the PUT command by sending them to the differfnt servers
 * hard coded becasue the original way was killing me */
void handle_PUT(void){
	printf(MSGTERM "\n--Putting Files to Server--" MSGNORM "\n");
	char fileRBuff[MAXBUFF];
	
	char nameOfFile[MAXFILE];
	printf("\n\t Enter a File:\n\n> ");
	scanf("%s", nameOfFile);
	if(VERBOSE){ printf(".nameOfFile: %s\n", nameOfFile); }

	if(access(nameOfFile, F_OK) == -1){
		printf("Invalid Filename!\n");
		return;
	}
	
	char fileNamePtr1[MAXFILE];
	strcpy(fileNamePtr1, ".");
	strcat(fileNamePtr1, nameOfFile);
	strcat(fileNamePtr1, ".1");	
	if(VERBOSE){ printf(".fileNamePtr1: %s\n", fileNamePtr1); }
	
	char fileNamePtr2[MAXFILE];
	strcpy(fileNamePtr2, ".");
	strcat(fileNamePtr2, nameOfFile);
	strcat(fileNamePtr2, ".2");
	if(VERBOSE){ printf(".fileNamePtr2: %s\n", fileNamePtr2); }

	char fileNamePtr3[MAXFILE];
	strcpy(fileNamePtr3, ".");
	strcat(fileNamePtr3, nameOfFile);
	strcat(fileNamePtr3, ".3");
	if(VERBOSE){ printf(".fileNamePtr3: %s\n", fileNamePtr3); }

	char fileNamePtr4[MAXFILE];
	strcpy(fileNamePtr4, ".");
	strcat(fileNamePtr4, nameOfFile);
	strcat(fileNamePtr4, ".4");
	if(VERBOSE){ printf(".fileNamePtr4: %s\n", fileNamePtr4); }

	struct stat fileStat;
	stat(nameOfFile, &fileStat);
	int fileStatSize = fileStat.st_size;
	
	
	FILE* filePtr;
	filePtr = fopen(nameOfFile, "r");
	int fptr1, fptr2, fptr3, fptr4;
	md5sum_hash_file_ptrs(filePtr, &fptr1, &fptr2, &fptr3, &fptr4);

	fclose(filePtr);

	filePtr = fopen(nameOfFile, "r");
	
	int md5SumHashIndx;
	md5SumHashIndx = (fptr1 ^ fptr2 ^ fptr3 ^ fptr4) % NUMSERVS;


	int sockfdOne, sockfdTwo;
	int is_socketOneSet = 0;
	int is_socketTwoSet = 0;
	int socketIndx;

	/* sends the first file */
	for(socketIndx = 0; socketIndx < NUMSERVS; socketIndx++){
		if(	dfsServerConfigs[md5SumHashIndx][socketIndx][1] == 1 || 
			dfsServerConfigs[md5SumHashIndx][socketIndx][0] == 1)
		{
			if(is_socketOneSet < 1){
				socket_connection_routine_cli_to_servs(&sockfdOne, socketIndx, fileNamePtr1);
				is_socketOneSet = 1;
			} else if(is_socketTwoSet < 1){
				socket_connection_routine_cli_to_servs(&sockfdTwo, socketIndx, fileNamePtr1);
				is_socketTwoSet = 1;
			}
		} else {
			printf(MSGWARN "\tFile pass" MSGNORM "\n");
		}
	}
	
	if(&sockfdTwo == NULL || &sockfdOne == NULL){
		printf(MSGERRR "UnAuthorized User!" MSGNORM "\n");
		fclose(filePtr);
		return;
	} else {
		printf(MSGSUCC "Authorized User!" MSGNORM "\n");
	}

	int totBytesRd = 0;
	while(totBytesRd < fileStatSize/NUMSERVS ){
		unsigned char fBuffRead[1];
		unsigned char* fBuffReadPtr;
		
		int fileReadBytes = fread(fBuffRead, sizeof(unsigned char), 1, filePtr);
		totBytesRd = totBytesRd + fileReadBytes;

		/* handles sending the first doc to the first socket */
		int bytesRead;
		bytesRead = fileReadBytes;
		fBuffReadPtr = fBuffRead;
		int bytesSentCount;
		do{
			bytesSentCount = send(sockfdOne, fBuffReadPtr, bytesRead, MSG_NOSIGNAL);
			if(bytesSentCount < 0){
				break;
			}
			fBuffReadPtr = fBuffReadPtr + bytesSentCount;
			bytesRead = bytesRead - bytesSentCount;
		}while(bytesRead > 0);
		
		/* handles sending the first doc to the second socket */
		bytesRead = fileReadBytes;
		fBuffReadPtr = fBuffRead;
		bytesSentCount;
		do{
			bytesSentCount = send(sockfdTwo, fBuffReadPtr, bytesRead, MSG_NOSIGNAL);
			if(bytesSentCount < 0){
				break;
			}
			fBuffReadPtr = fBuffReadPtr + bytesSentCount;
			bytesRead = bytesRead - bytesSentCount;
		} while(bytesRead > 0);
	}

	/* shutdown the sockets */
	shutdown(sockfdOne, SHUT_WR);
	shutdown(sockfdTwo, SHUT_WR);

	/* recv */
	recv(sockfdOne, NULL, 0, 0);
	recv(sockfdTwo, NULL, 0, 0);

	/* close the sockets */
	close(sockfdOne);
	close(sockfdTwo);

	/* send the second file of data */
	is_socketOneSet = 0;
	is_socketTwoSet = 0;
	for(socketIndx = 0; socketIndx < NUMSERVS; socketIndx++){
		if(	dfsServerConfigs[md5SumHashIndx][socketIndx][1] == NUMSOCKS ||
			dfsServerConfigs[md5SumHashIndx][socketIndx][0] == NUMSOCKS) {
			if(is_socketOneSet < 1){
				socket_connection_routine_cli_to_servs(&sockfdOne, socketIndx, fileNamePtr2);
				is_socketOneSet = 1;
			} else if(is_socketTwoSet < 1){
				socket_connection_routine_cli_to_servs(&sockfdTwo, socketIndx, fileNamePtr2);
				is_socketTwoSet = 1;
			}
		}
	}

	if(&sockfdTwo == NULL || &sockfdOne == NULL){
		printf(MSGERRR "UnAuthorized User!" MSGNORM "\n");
		fclose(filePtr);
		return;
	} else {
		printf(MSGSUCC "Authorized User!" MSGNORM "\n");
	}

	while(totBytesRd < fileStatSize/NUMSOCKS){
		unsigned char fBuffRead[1];
		int fileReadBytes = fread(fBuffRead, sizeof(unsigned char), 1, filePtr);
		
		/* send to the first server in the list */
		int bytesRead;
		unsigned char* fBuffReadPtr;
		totBytesRd += fileReadBytes;
		bytesRead = fileReadBytes;
		fBuffReadPtr = fBuffRead;
		do{
			int bytesSentCount = send(sockfdOne, fBuffReadPtr, bytesRead, MSG_NOSIGNAL);
			if(bytesSentCount < 0){
				break;
			}
			fBuffReadPtr = fBuffReadPtr + bytesSentCount;
			bytesRead = bytesRead - bytesSentCount;
		}while(bytesRead > 0);
		
		/* sending it to the second socket */
		bytesRead = fileReadBytes;
		fBuffReadPtr = fBuffRead;
		do{
			int bytesSentCount = send(sockfdTwo, fBuffReadPtr, bytesRead, MSG_NOSIGNAL);
			if(bytesSentCount < 0){
				break;
			}
			fBuffReadPtr = fBuffReadPtr + bytesSentCount;
			bytesRead = bytesRead - bytesSentCount;
		}while(bytesRead > 0);
	}
	/* shutdown the sockets */
	shutdown(sockfdOne, SHUT_WR);
	shutdown(sockfdTwo, SHUT_WR);

	/* recv */
	recv(sockfdOne, NULL, 0, 0);
	recv(sockfdTwo, NULL, 0, 0);

	/* close the sockets */
	close(sockfdOne);
	close(sockfdTwo);	

	/* send the third blocks of data */
	is_socketOneSet = 0;
	is_socketTwoSet = 0;
	for(socketIndx = 0; socketIndx < NUMSERVS; socketIndx++){
		if(	dfsServerConfigs[md5SumHashIndx][socketIndx][0] == 3 ||
			dfsServerConfigs[md5SumHashIndx][socketIndx][1] == 3) {
			if(is_socketOneSet < 1){
				socket_connection_routine_cli_to_servs(&sockfdOne, socketIndx, fileNamePtr3);
				is_socketOneSet = 1;
			} else if(is_socketTwoSet < 1){
				socket_connection_routine_cli_to_servs(&sockfdTwo, socketIndx, fileNamePtr3);
				is_socketTwoSet = 1;
			}
		}
	}

	if(&sockfdTwo == NULL || &sockfdOne == NULL){
		printf(MSGERRR "UnAuthorized User!" MSGNORM "\n");
		fclose(filePtr);
		return;
	} else {
		printf(MSGSUCC "Authorized User!" MSGNORM "\n");
	}
	
	while(totBytesRd < (fileStatSize*3)/NUMSERVS){
		unsigned char fBuffRead[1];
		int fileReadBytes = fread(fBuffRead, sizeof(unsigned char), 1, filePtr);
		totBytesRd += fileReadBytes;
		
		int bytesRead;
		bytesRead = fileReadBytes;
		unsigned char* fBuffReadPtr;
		
		/* sending the third bits of data */
		if(bytesRead < 0){
			break;
		}
		fBuffReadPtr = fBuffRead;
		
		do{
			int bytesSentCount = send(sockfdOne, fBuffReadPtr, bytesRead, MSG_NOSIGNAL);
			
			if(bytesSentCount < 0){
				break;
			}

			fBuffReadPtr = fBuffReadPtr + bytesSentCount;
			bytesRead = bytesRead - bytesSentCount;
		}while(bytesRead > 0);
		
		/* sending data to the second socket */
		bytesRead = fileReadBytes;
		if(bytesRead < 0){
			break;
		}
		fBuffReadPtr = fBuffRead;
		do{
			int bytesSentCount = send(sockfdTwo, fBuffReadPtr, bytesRead, MSG_NOSIGNAL);
			if(bytesSentCount < 0){
				break;
			}
			fBuffReadPtr = fBuffReadPtr + bytesSentCount;
			bytesRead = bytesRead - bytesSentCount;
		}while(bytesRead > 0);
	}
	/* shutdown the sockets */
	shutdown(sockfdOne, SHUT_WR);
	shutdown(sockfdTwo, SHUT_WR);

	/* recv */
	recv(sockfdOne, NULL, 0, 0);
	recv(sockfdTwo, NULL, 0, 0);

	/* close the sockets */
	close(sockfdOne);
	close(sockfdTwo);			

	/* send .NUMSERVS to the sockets */
	is_socketOneSet = 0;
	is_socketTwoSet = 0;
	for(socketIndx=0;socketIndx<NUMSERVS;socketIndx++){
		if(dfsServerConfigs[md5SumHashIndx][socketIndx][0] == NUMSERVS || dfsServerConfigs[md5SumHashIndx][socketIndx][1] == NUMSERVS){
			if(is_socketOneSet < 1){
				socket_connection_routine_cli_to_servs(&sockfdOne, socketIndx, fileNamePtr4);
				is_socketOneSet = 1;
			} else if(is_socketTwoSet < 1){
				socket_connection_routine_cli_to_servs(&sockfdTwo, socketIndx, fileNamePtr4);
				is_socketTwoSet = 1;
			}
		}
	}

	if(&sockfdTwo == NULL || &sockfdOne == NULL){
		printf(MSGERRR "UnAuthorized User!" MSGNORM "\n");
		fclose(filePtr);
		return;
	} else {
		printf(MSGSUCC "Authorized User!" MSGNORM "\n");
	}

	while(totBytesRd < fileStatSize){
		unsigned char fBuffRead[1];
		int fileReadBytes = fread(fBuffRead, sizeof(unsigned char), 1, filePtr);
		unsigned char* fBuffReadPtr;
		totBytesRd += fileReadBytes;
		/* sending to the first server */
		int bytesRead;
		bytesRead = fileReadBytes;

		if(bytesRead < 0){
			break;
		}
		
		fBuffReadPtr = fBuffRead;
		do{
			int bytesSentCount = send(sockfdOne, fBuffReadPtr, bytesRead, MSG_NOSIGNAL);
			if(bytesSentCount < 0){
				break;
			}
			fBuffReadPtr = fBuffReadPtr + bytesSentCount;
			bytesRead = bytesRead - bytesSentCount;
		}while(bytesRead > 0);
		
		//send to second server
		bytesRead = fileReadBytes;
		if(bytesRead < 0){
			break;
		}
		fBuffReadPtr = fBuffRead;
		do{
			int bytesSentCount = send(sockfdTwo, fBuffReadPtr, bytesRead, MSG_NOSIGNAL);
			if(bytesSentCount < 0){
				break;
			}
			fBuffReadPtr = fBuffReadPtr + bytesSentCount;
			bytesRead = bytesRead - bytesSentCount;
		}while(bytesRead > 0);
	}
	/* shutdown the sockets */
	shutdown(sockfdOne, SHUT_WR);
	shutdown(sockfdTwo, SHUT_WR);

	/* recv */
	recv(sockfdOne, NULL, 0, 0);
	recv(sockfdTwo, NULL, 0, 0);

	/* close the sockets */
	close(sockfdOne);
	close(sockfdTwo);			

	fclose(filePtr);
}


/* handles the GET request */
void handle_GET(void){
	printf(MSGTERM "\n--Getting Files from Server--" MSGNORM "\n");

	int fileBlocks[NUMSERVS][NUMSOCKS];
	char fileBlockNames[NUMSERVS][NUMSOCKS][MAXLINE];
	char nameOfFile[MAXLINE];
	
	int i;
	for(i = 0; i < NUMSERVS; i++){
		fileBlocks[i][0] = -1;
		fileBlocks[i][1] = -1;
		strcpy(fileBlockNames[i][0], "");
		strcpy(fileBlockNames[i][1], "");
	}

	char files[30][FILELST];
	int numFiles = 0;
	int error = 0;	

	printf("\tEnter a File to get:\n\n> ");
	scanf("%s", nameOfFile);

	struct sockaddr_in sockAddrIn;
    int sockfd;
	for(i = 0; i < serverCount; i++){
		char serverAddress[MAXFILE];
		char serverPort[MAXFILE];
		/* runs to grab the info for the host */
		get_host_info(i, serverAddress, serverPort);
		if(VERBOSE){ printf(".serverAddress: %s\n", serverAddress); }
		if(VERBOSE){ printf(".serverPort: %s\n", serverPort); }

		memset(&sockAddrIn, 0, sizeof(sockAddrIn));
		
		/* TCP connection */
		sockAddrIn.sin_family = AF_INET;
		sockAddrIn.sin_port = htons((unsigned short)atoi(serverPort));
		sockAddrIn.sin_addr.s_addr = inet_addr(serverAddress);
		sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

		if(connect(sockfd, (struct sockaddr*)&sockAddrIn, sizeof(sockAddrIn)) <= -1){
			printf(MSGERRR "Unable to connect" MSGNORM "\n");
		} else {
			char* listBuff = "LIST\nUSERNAME: %s\nPASSWORD: %s\n\n";
			char temBuff[FILELST];

			sprintf(temBuff, listBuff, username, password);
			if(VERBOSE){ printf(".temBuff: %s\n", temBuff); }

			int len = strlen(temBuff);
			send(sockfd, temBuff, len, MSG_NOSIGNAL);

			char recvBuff[MAXBUFF];
			char packBuff[MAXBUFF];
			strcpy(packBuff, "");

			int nBytesCp;
			recv(sockfd, recvBuff, sizeof(recvBuff), 0);
			if(VERBOSE){ printf(".recvBuff: %s\n", recvBuff); }

			strcat(packBuff, recvBuff);
			
			int position = 0;
			bzero(recvBuff, MAXBUFF);

			while(nBytesCp = recv(sockfd, recvBuff, sizeof(recvBuff), MSG_PEEK) > 0){
				if(VERBOSE){ printf(".recvBuff: %s\n", recvBuff); }
				if(strlen(recvBuff) > 0){
					strcat(packBuff, recvBuff);
					char extraBuff[MAXBUFF];
					recv(sockfd, extraBuff, sizeof(extraBuff), 0);
					if(VERBOSE){ printf(".extraBuff: %s\n", extraBuff); }
				}
				bzero(recvBuff, MAXBUFF);
			}

			
			/* sends the blocks */
			char packetBuffCpy[512];
			strcpy(packetBuffCpy, packBuff);
			if(VERBOSE){ printf(".packetBuffCpy: %s\n", packetBuffCpy); }

			char* lineTok = strtok(packBuff, "\n");
			if(VERBOSE){ printf(".lineTok: %s\n", lineTok); }

			while(lineTok != NULL){	
				if(position == 0){
					if(!!strcmp(lineTok, "LIST")){
						printf("%s\n", packetBuffCpy);
						if(VERBOSE){ printf(".packetBuffCpy: %s\n", packetBuffCpy); }
						return;
					}
				}else if(position > 1){
					char lineBuff[MAXFILE];
					strcpy(lineBuff, lineTok);
					if(VERBOSE){ printf(".lineBuff: %s\n", lineBuff); }
					
					for(int j = 0; j < strlen(lineBuff); j++){
						lineBuff[j] = lineBuff[j+1];
					}
					if(VERBOSE){ printf(".lineBuff: %s\n", lineBuff); }

					lineBuff[strlen(lineBuff)-NUMSOCKS]='\0';
					if(VERBOSE){ printf(".lineBuff: %s\n", lineBuff); }

					/* fiel piece breakup */
					char filePiece[NUMSOCKS];
					filePiece[0] = lineTok[strlen(lineTok)-1];
					filePiece[1] = '\0';
					if(VERBOSE){ printf(".filePiece: %s\n", filePiece); }
					
					if(!strcmp(lineBuff, nameOfFile)){
						int part = atoi(filePiece) - 1;
						int index = 0;
						if(fileBlocks[part][0] >= 0)index=1;
						fileBlocks[part][index] = i;
						strcpy(fileBlockNames[part][index], lineTok);
					}
							
				}
				position++;
				lineTok = strtok(NULL, "\n");
				if(VERBOSE){ printf(".lineTok: %s\n", lineTok); }
			}
		}
		close(sockfd);
	}
	for(i = 0; i < NUMSERVS; i++){
		if(fileBlocks[i][0] < 0 && fileBlocks[i][1] < 0){
			printf(MSGERRR "\tUnable to GET file" MSGNORM "\n");
			printf(MSGERRR "\t\tDoes it exist?" MSGNORM "\n");
			return;
		} else {
			printf(MSGSUCC "\tfilePtr opened successfully" MSGNORM "\n");
		}
	}
	
	/* establish username and password */
	char genBuff[MAXFILE];
	char* strGET = "GET %s\nUSERNAME: %s\nPASSWORD: %s\n\n";
	sprintf(genBuff, strGET, fileBlockNames[0][0], username, password);
	if(VERBOSE){ printf(".genBuff: %s\n", genBuff); }

	/* get the server names and all from teh ports */
	char serverAddress[MAXFILE];
	char serverPort[MAXFILE];
	get_host_info(fileBlocks[0][0], serverAddress, serverPort);
	memset(&sockAddrIn, 0, sizeof(sockAddrIn));
	
	/* TCP connection */
	sockAddrIn.sin_family = AF_INET;
	sockAddrIn.sin_port = htons((unsigned short)atoi(serverPort));
	sockAddrIn.sin_addr.s_addr = inet_addr(serverAddress);
	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	/* connecting the socket */
	connect(sockfd, (struct sockaddr*)&sockAddrIn, sizeof(sockAddrIn));
	
	int bytesRead = strlen(genBuff);
	unsigned char* fBuffReadPtr;
	fBuffReadPtr = genBuff;

	do{
		int bytesSentCount = send(sockfd, fBuffReadPtr, bytesRead, MSG_NOSIGNAL);
		if(bytesSentCount >= 0){
			printf("\tbytes sent/recv ok.\n");
		} else {
			break;
		}
		fBuffReadPtr = fBuffReadPtr + bytesSentCount;
		bytesRead = bytesRead - bytesSentCount;
	}while(bytesRead > 0);
	
	char recvBuff[MAXBUFF];
	recv(sockfd, recvBuff, sizeof(recvBuff), 0);
	if(VERBOSE){ printf(".recvBuff: %s\n", recvBuff); }
	
	char* readyACKstr = "ACK OK\n\n";
	bytesRead = strlen(readyACKstr);
	fBuffReadPtr = readyACKstr;
	do{
		int bytesSentCount = send(sockfd, fBuffReadPtr, bytesRead, MSG_NOSIGNAL);
		if(bytesSentCount >= 0){
			printf("\tbytes sent/recv ok.\n");
		} else {
			break;
		}
		fBuffReadPtr = fBuffReadPtr + bytesSentCount;
		bytesRead = bytesRead - bytesSentCount;
	}while(bytesRead > 0);
	
	FILE* filePtr = fopen(nameOfFile, "wb");
	int nBytesCp;
	while(nBytesCp = recv(sockfd, recvBuff, sizeof(recvBuff), MSG_DONTWAIT)){
		if(VERBOSE){ printf(".recvBuff: %s\n", recvBuff); }
		if(nBytesCp == 0){
			break;
		}
		if(nBytesCp > 0){
			fwrite(recvBuff, 1, nBytesCp, filePtr);
			int i;
			bzero(recvBuff, MAXBUFF);
		}
	}
	
	/* closing the socket */
	close(sockfd);
	bzero(serverAddress, MAXFILE);
	bzero(serverPort, MAXFILE);
	
	/* gets the second part of the file */
	sprintf(genBuff, strGET, fileBlockNames[1][0], username, password);

	get_host_info(fileBlocks[1][0], serverAddress, serverPort);
	memset(&sockAddrIn, 0, sizeof(sockAddrIn));

	/* tcp connection, should make a function out of this */
	sockAddrIn.sin_family = AF_INET;
	sockAddrIn.sin_port = htons((unsigned short)atoi(serverPort));
	sockAddrIn.sin_addr.s_addr = inet_addr(serverAddress);
	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	connect(sockfd, (struct sockaddr*)&sockAddrIn, sizeof(sockAddrIn));
	
	bytesRead = strlen(genBuff);
	fBuffReadPtr = genBuff;
	do{
		int bytesSentCount = send(sockfd, fBuffReadPtr, bytesRead, MSG_NOSIGNAL);
		if(bytesSentCount >= 0){
			printf("\tbytes sent/recv ok.\n");
		} else {
			break;
		}
		fBuffReadPtr = fBuffReadPtr + bytesSentCount;
		bytesRead = bytesRead - bytesSentCount;
	}while(bytesRead > 0);
	
	recv(sockfd, recvBuff, sizeof(recvBuff), 0);
	bzero(recvBuff, MAXBUFF);
	
	bytesRead = strlen(readyACKstr);
	fBuffReadPtr = readyACKstr;
	do{
		int bytesSentCount = send(sockfd, fBuffReadPtr, bytesRead, MSG_NOSIGNAL);
		if(bytesSentCount >= 0){
			printf("\tbytes sent/recv ok.\n");
		} else {
			break;
		}
		fBuffReadPtr = fBuffReadPtr + bytesSentCount;
		bytesRead = bytesRead - bytesSentCount;
	}while(bytesRead > 0);
	
	while(nBytesCp = recv(sockfd, recvBuff, sizeof(recvBuff), MSG_DONTWAIT)){
		if(VERBOSE){ printf(".recvBuff: %s\n", recvBuff); }
		if(nBytesCp == 0){
			break;
		}
		if(nBytesCp > 0){
			fwrite(recvBuff, 1, nBytesCp, filePtr);
			int i;
			bzero(recvBuff, MAXBUFF);
		}
	}
	close(sockfd);
	
	/* recv third part, redundant at this point */
	sprintf(genBuff, strGET, fileBlockNames[NUMSOCKS][0], username, password);

	/* getting hosts */
	get_host_info(fileBlocks[NUMSOCKS][0], serverAddress, serverPort);
	memset(&sockAddrIn, 0, sizeof(sockAddrIn));
	
	/* establishing tcp */
	sockAddrIn.sin_family = AF_INET;
	sockAddrIn.sin_port = htons((unsigned short)atoi(serverPort));
	sockAddrIn.sin_addr.s_addr = inet_addr(serverAddress);
	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	connect(sockfd, (struct sockaddr*)&sockAddrIn, sizeof(sockAddrIn));
	
	bytesRead = strlen(genBuff);
	fBuffReadPtr = genBuff;
	do{
		int bytesSentCount = send(sockfd, fBuffReadPtr, bytesRead, MSG_NOSIGNAL);
		if(bytesSentCount >= 0){
			printf("\tbytes sent/recv ok.\n");
		} else {
			break;
		}
		fBuffReadPtr = fBuffReadPtr + bytesSentCount;
		bytesRead = bytesRead - bytesSentCount;
	}while(bytesRead > 0);
	
	recv(sockfd, recvBuff, sizeof(recvBuff), 0);
	bzero(recvBuff, MAXBUFF);
	
	bytesRead = strlen(readyACKstr);
	fBuffReadPtr = readyACKstr;
	do{
		int bytesSentCount = send(sockfd, fBuffReadPtr, bytesRead, MSG_NOSIGNAL);
		if(bytesSentCount >= 0){
			printf("\tbytes sent/recv ok.\n");
		} else {
			break;
		}
		fBuffReadPtr = fBuffReadPtr + bytesSentCount;
		bytesRead = bytesRead - bytesSentCount;
	}while(bytesRead > 0);
	while(nBytesCp = recv(sockfd, recvBuff, sizeof(recvBuff), MSG_DONTWAIT)){
		if(VERBOSE){ printf(".recvBuff: %s\n", recvBuff); }
		if(nBytesCp == 0){
			break;
		}
		if(nBytesCp > 0){
				fwrite(recvBuff, 1, nBytesCp, filePtr);
				int i;
				bzero(recvBuff, MAXBUFF);
		}
	}
	
	/* closing the socket */
	close(sockfd);
	bzero(serverAddress, MAXFILE);
	bzero(serverPort, MAXFILE);

	
	/* second half of the file */
	sprintf(genBuff, strGET, fileBlockNames[3][0], username, password);

	/* very redundant code */
	get_host_info(fileBlocks[3][0], serverAddress, serverPort);
	memset(&sockAddrIn, 0, sizeof(sockAddrIn));

	/* tcp */
	sockAddrIn.sin_family = AF_INET;
	sockAddrIn.sin_port = htons((unsigned short)atoi(serverPort));
	sockAddrIn.sin_addr.s_addr = inet_addr(serverAddress);
	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	/* put a test here */
	connect(sockfd, (struct sockaddr*)&sockAddrIn, sizeof(sockAddrIn));
	
	bytesRead = strlen(genBuff);
	fBuffReadPtr = genBuff;
	do{
		int bytesSentCount = send(sockfd, fBuffReadPtr, bytesRead, MSG_NOSIGNAL);
		if(bytesSentCount >= 0){
			printf("\tbytes sent/recv ok.\n");
		} else {
			break;
		}
		fBuffReadPtr = fBuffReadPtr + bytesSentCount;
		bytesRead = bytesRead - bytesSentCount;
	}while(bytesRead > 0);
	
	recv(sockfd, recvBuff, sizeof(recvBuff), 0);
	bzero(recvBuff, MAXBUFF);
	
	bytesRead = strlen(readyACKstr);
	fBuffReadPtr = readyACKstr;
	do{
		int bytesSentCount = send(sockfd, fBuffReadPtr, bytesRead, MSG_NOSIGNAL);
		if(bytesSentCount >= 0){
			printf("\tbytes sent/recv ok.\n");
		} else {
			break;
		}
		fBuffReadPtr = fBuffReadPtr + bytesSentCount;
		bytesRead = bytesRead - bytesSentCount;
	}while(bytesRead > 0);
	while(nBytesCp = recv(sockfd, recvBuff, sizeof(recvBuff), MSG_DONTWAIT)){
		if(VERBOSE){ printf(".recvBuff: %s\n", recvBuff); }
		if(nBytesCp == 0){
			break;
		}
		if(nBytesCp > 0){
			fwrite(recvBuff, 1, nBytesCp, filePtr);
			int i;
			bzero(recvBuff, MAXBUFF);
		}
	}
	/* close the socket */
	close(sockfd);

	/* zero out the buffs */
	bzero(serverAddress, MAXFILE);
	bzero(serverPort, MAXFILE);
	
	/* close out the file */
	fclose(filePtr);
}


/* Hashes  */
void md5sum_hash_file_ptrs(FILE* filePtr, int* fptr1, int* fptr2, int* fptr3, int* fptr4){
    char hashedLine[MD5_DIGEST_LENGTH];
    MD5_CTX mdContext;
	MD5_Init (&mdContext);

    unsigned char data[FILELST];
	int bytes;
    while ((bytes = fread (data, 1, FILELST, filePtr)) != 0){
        MD5_Update (&mdContext, data, bytes);
	}

    MD5_Final(hashedLine,&mdContext);
    sscanf(&hashedLine[0], "%x", fptr1 );
    sscanf(&hashedLine[8], "%x", fptr2 );
    sscanf(&hashedLine[16], "%x", fptr3 );
    sscanf(&hashedLine[24], "%x", fptr4 );
}


/* runs the connection routine from the client to the servers */
void socket_connection_routine_cli_to_servs(int* sockfd, int index, char* nameUser){
	printf(MSGTERM "\n--Connecting Sockets in Routine--" MSGNORM "\n");
	
	struct sockaddr_in sockAddrIn;
	char serverAddress[MAXFILE];
	char serverPort[MAXFILE];
	char genBuff[MAXBUFF];
	char recvBuff[MAXBUFF];
	
	/* start off by getting the host info */
	get_host_info(index, serverAddress, serverPort);
	memset(&sockAddrIn, 0, sizeof(sockAddrIn));

	/* socket info */
	sockAddrIn.sin_family = AF_INET;
	sockAddrIn.sin_port = htons((unsigned short)atoi(serverPort));
	sockAddrIn.sin_addr.s_addr = inet_addr(serverAddress);

	/* TCP */
	(*sockfd) = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	int returnValue = connect((*sockfd), (struct sockaddr*)&sockAddrIn, sizeof(sockAddrIn));
	if(returnValue <= -1){
		printf(MSGERRR "Could not connect server" MSGNORM "\n");
	} else {
		char* strPUT = "PUT %s\nUSERNAME: %s\nPASSWORD: %s\n\n";
		sprintf(genBuff, strPUT, nameUser, username, password);
		if(VERBOSE){ printf(".genBuff: %s\n", genBuff); }
		int genBuffLen = strlen(genBuff);

		do{
			int bytesSentCount = send((*sockfd), genBuff, genBuffLen, MSG_NOSIGNAL);
			if(bytesSentCount < 0){
				break;
			} else {
				printf("\tbytesSentCount ok\n");
			}
			strPUT = strPUT + bytesSentCount;
			genBuffLen = genBuffLen - bytesSentCount;
		}while(genBuffLen > 0);

		while(1){
			int nBytesCp = recv((*sockfd), recvBuff, sizeof(recvBuff), MSG_DONTWAIT);
			if(nBytesCp > 0){
				if(strstr(recvBuff, "ACK OK") == NULL){
					printf("%s\n", recvBuff);
					sockfd = NULL;
				}
				for(int i = 0; i < MAXBUFF; i++){
					recvBuff[i] = 0;
				}
				break;
			}
		}
	}
}