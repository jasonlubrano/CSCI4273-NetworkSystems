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
#define FILENAME    32      /* max size of file name*/  
#define SERVADDR    64      /* max server serverAddress */
#define FILEPRTS    64      /* max num of file fileBlocks */
#define MAXLINE     256     /* max text infoBuff genBuffLen */
#define FILELST     1024    /* second argument to listen() */
#define MAXFILE		2048	/* max size of infoBuff in a file */
#define SOCKFDR		4096	/* max size of read in socket serv*/
#define MAXBUFF     8192    /* max text infoBuff genBuffLen */


#define INVALIDCMDS 0
#define NOCONFIGSPC 1
#define HOMEMESSAGE NUMSOCKS


/* functions used throughout */
void ret_exit();
void closing_message();
void welcome_message();
int socket_hoster(const char* serverPort, int queueStack);
void req_handler(void* requestPtr);
int parse_for_file_name(char* recvBuff, char nameOfFile[]);
void get_data_from_config(void);
int is_user_valid(char* recvBuff, char* clientUsername);
int get_user_request(char* recvBuff);
void handle_req_LIST(int sockfd, char* DirectoryF, char* cUsername);
void handle_req_GET(int sockfd, char* wrkDir, char* nameOfFile);
void handle_req_PUT(int sockfd, char* wrkDir, char* nameOfFile);
void send_closing_ack(int sockfd, char* info);


struct SocketArg {
	int newSock;
};

int clientCount = 0;
char clientPassword[20][MAXFILE];
char serverPort[32];
char serverFileDirectory[FILELST];

int main(int argc, char* argv[]){
	welcome_message();
	switch (argc)
    {
    case 3:
        if(argv[1][strlen(argv[1])-1] != '/'){
			strcat(argv[1], "/");
		}
		strcpy(serverFileDirectory,argv[1]);

        if(VERBOSE){ printf(MSGNORM ".serverFileDirectory: %s" MSGNORM "\n", serverFileDirectory); }
        
		strcpy(serverPort,argv[2]);
        if(VERBOSE){ printf(MSGNORM ".serverPort: %s" MSGNORM "\n", serverPort); }
        break;
    
    default:
        printf(MSGERRR "\tUsage: %s <Server Directory> <Port>" MSGNORM "\n", argv[0]);
        ret_exit(0);
        break;
    }
	
	if(argc < 3){
		printf("need to specify document root and port number!\nUsage: ./dfs DOCROOT PORTNUM\n");
		exit(1);
	}

	get_data_from_config();

	struct sockaddr_in sockAddrIn;
	socklen_t sockAddrInSize = sizeof(struct sockaddr_in);
	
	int hostSockfd;
	int returnPthreadCreate;
	
	hostSockfd = socket_hoster(serverPort, 1);
	if(VERBOSE){ printf(MSGNORM ".hostSockfd: %d" MSGNORM "\n", hostSockfd); }
	
	int is_Running = 1;
	while(is_Running){
		int clientfd;
		clientfd = accept(hostSockfd, (struct sockaddr*)&sockAddrIn, &sockAddrInSize);
		if(clientfd < -1){
			/* why is it not being able to connect ot hthe sockets? */
			printf(MSGERRR "\tUnable to connect..." MSGNORM "\n");
			int errnum = errno;
			fprintf(stderr, "Value of errno: %d\n", errno);
			perror("Error connecting socket by perror");
			fprintf(stderr, "Error connecting socket: %s\n", strerror( errnum ));
		} else {
			printf(MSGSUCC "Server Accepted Connection" MSGNORM "\n");
		}

		struct SocketArg* socketArgs = malloc(sizeof(struct SocketArg));
		socketArgs -> newSock = clientfd;
		pthread_t connThread;
		
		/* create a new thread to handle the userRequest */
		returnPthreadCreate = pthread_create(
			&connThread, 
			NULL, 
			(void*)req_handler, 
			(void*)(socketArgs)
		);

		if(returnPthreadCreate < 0){
			printf(MSGSUCC "unable to create pthread" MSGNORM "\n");
			close(clientfd);
			free(socketArgs);
		} else {
			printf(MSGSUCC "pthread created successfully" MSGNORM "\n");
		}
	}
}


/***************************************************************************
 * function definitons
 ***************************************************************************/


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


/* binds the socket and listens, returns sockfd */
int socket_hoster(const char* serverPort, int queueStack){
	printf(MSGTERM "\n--Hosting socket--" MSGNORM "\n");
	struct sockaddr_in sockAddrIn;
	int sockfd;

	/* initialize the socket address */
	memset(&sockAddrIn, 0, sizeof(sockAddrIn));
	sockAddrIn.sin_family = AF_INET;
	sockAddrIn.sin_addr.s_addr = INADDR_ANY;
	sockAddrIn.sin_port=htons((unsigned short)atoi(serverPort));

	/* TCP */
	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	/* socket Binding */
	if(bind(sockfd, (struct sockaddr*)&sockAddrIn, sizeof(sockAddrIn)) < 0){
		printf(MSGERRR "\tERROR: Unable to bind socket" MSGNORM "\n");
		ret_exit();
	} else {
		printf(MSGSUCC "\tBinding socket successful" MSGNORM "\n");
	}

	/* listen on the socket */
	int listenret = listen(sockfd, queueStack);
	if(listenret == 0){
		printf(MSGSUCC "\tListening on socket successful" MSGNORM "\n");
		return sockfd;
	} else {
		printf(MSGERRR "\tERROR: Unable to listen socket" MSGNORM "\n");
		ret_exit();
	}
}


/* handles the requirements from the user */
void req_handler(void* requestPtr){
	printf(MSGTERM "\n--Handeling Request--" MSGNORM "\n");
	/* null buff for future use */
	char recvBuff[MAXBUFF];
	strcpy(recvBuff, "");
	
	/* thread comes down here to get a new socket */
	struct SocketArg* socketArgs = requestPtr;
	int sockfd;
	sockfd = socketArgs -> newSock;

	/* data starts flowing in nice */
	char sockBuff[SOCKFDR];
	recv(sockfd, sockBuff, sizeof(sockBuff), 0);
	if(VERBOSE){ printf(".sockBuff: %s", sockBuff); }

	strcat(recvBuff, sockBuff);
	if(VERBOSE){ printf(".recvBuff: %s", recvBuff); }

	bzero(sockBuff, SOCKFDR);
	int rogerRecv;
	int keepalive = 1;
	do{
		while(rogerRecv = recv(sockfd, sockBuff, sizeof(sockBuff), MSG_DONTWAIT) > 0){
			if(VERBOSE){ printf(".sockBuff: %s", sockBuff); }
			strcat(recvBuff, sockBuff);

			if(VERBOSE){ printf(".recvBuff: %s", recvBuff); }
			bzero(sockBuff, SOCKFDR);
		}

		/* get data from recvbuff */
		if(strlen(recvBuff) > 0){
			/* authorize the user */			
			char cUsername[MAXFILE];
			/* i think this is causing errors */
			if(is_user_valid(recvBuff, cUsername)){
				/* Get users root */				
				char wrkDir[MAXFILE];
				strcpy(wrkDir, serverFileDirectory);
				if(VERBOSE){ printf(".wrkDir: %s", wrkDir); }

				strcat(wrkDir, cUsername);
				if(VERBOSE){ printf(".wrkDir: %s", wrkDir); }

				strcat(wrkDir, "/");
				if(VERBOSE){ printf(".wrkDir: %s", wrkDir); }
				
				/* give it proper perms */
				mkdir(wrkDir, 0755);
				if(VERBOSE){ printf(".wrkDir: %s", wrkDir); }
				
				/* user gives us a userRequest */
				int userRequest = get_user_request(recvBuff);
				if(VERBOSE){ printf(".userRequest: %d", userRequest); }
				if (userRequest == 0){
					/* Handle LIST */
					handle_req_LIST(sockfd, wrkDir, cUsername);
				} else if(userRequest == 1){
					/* handle GET */
					char nameOfFile[MAXFILE];
					parse_for_file_name(recvBuff, nameOfFile);
					handle_req_GET(sockfd, wrkDir, nameOfFile);
					keepalive = 0;
				} else if(userRequest == 2){
					/* PUT */
					char nameOfFile[MAXFILE];
					parse_for_file_name(recvBuff, nameOfFile);
					handle_req_PUT(sockfd, wrkDir, nameOfFile);
					keepalive = 0;
				} else {
					/* handle error, maybe close? */
				}
			} else {
				/* User is incorrect. Closing */
				send_closing_ack(sockfd, "Unauthorized Username/Password\n");	
			}
		}

		bzero(recvBuff, MAXBUFF);
		strcpy(recvBuff, "");
		if(rogerRecv == 0) { break; } else { printf("\t\tcont"); }
	}while(keepalive);

	close(sockfd);	
	free(socketArgs);
	pthread_exit((void*)(0));
}


/* parses for filename */
int parse_for_file_name(char* recvBuff, char nameOfFile[]){
	printf(MSGTERM "\n--Parsing for Name of File--" MSGNORM "\n");
	char sockBuff[MAXBUFF];
	strcpy(sockBuff, recvBuff);
	if(VERBOSE){ printf(".sockBuff: %s\n", sockBuff); }

	char* lineTok = strtok(sockBuff, "\n");
	if(VERBOSE){ printf(".lineTok: %s\n", lineTok); }
	
	char firstInfo[MAXFILE];
	char fileOnDir[MAXFILE];

	bzero(fileOnDir, MAXFILE);

	while(lineTok != NULL){
		if(	strstr(lineTok, "GET") != NULL ||
			strstr(lineTok, "PUT") != NULL ){
			strcpy(firstInfo, lineTok);
			if(VERBOSE){ printf(".firstInfo: %s\n", firstInfo); }

			break;
		}
		lineTok = strtok(NULL, "\n");
		if(VERBOSE){ printf(".lineTok: %s\n", lineTok); }
	}
	
	char* secondInfo = strtok(firstInfo, " ");
	if(VERBOSE){ printf(".secondInfo: %s\n", secondInfo); }

	int is_NextFile = 0;
	while(secondInfo != NULL){
		if(is_NextFile == 1){
			strcpy(fileOnDir, secondInfo);
			if(VERBOSE){ printf(".fileOnDir: %s\n", fileOnDir); }
			break;
		}
		else if(strstr(secondInfo, "GET") != NULL ||
				strstr(secondInfo, "PUT") != NULL){
			is_NextFile = 1;
		}
		secondInfo = strtok(NULL, " ");
	}

	strcpy(nameOfFile, fileOnDir);
	if(VERBOSE){ printf(".nameOfFile: %s\n", nameOfFile); }
}


/* gets data from config file */
void get_data_from_config(void) {
	printf(MSGTERM "\n--Getting Data From Config File--" MSGNORM "\n");

	char nameOfFile[MAXFILE];
	strcpy(nameOfFile, serverFileDirectory);
	if(VERBOSE){ printf(".nameOfFile: %s\n", nameOfFile); }

	strcat(nameOfFile, "dfs.conf");
	printf("Filename: %s\n", nameOfFile);
	if(VERBOSE){ printf(".nameOfFile: %s\n", nameOfFile); }
	
	FILE* configFile = fopen(nameOfFile, "r");
	if(configFile == NULL){
		printf(MSGERRR "\tConfig filePtr is NULL" MSGNORM "\n");
		printf(MSGERRR "\t\tDoes it exist?" MSGNORM "\n");
		ret_exit();
	} else {
		printf(MSGSUCC "\tFile open successfully" MSGNORM "\n");
	}

	char infoBuff[MAXFILE];
	while(fgets(infoBuff, sizeof(infoBuff), configFile) != NULL){
		if(VERBOSE){ printf(".infoBuff: %s\n", infoBuff); }
		if(strstr(infoBuff, "#") == NULL && strlen(infoBuff) > 1){
			char* lineTok = strtok(infoBuff, " ");
			if(VERBOSE){ printf(".lineTok: %s\n", lineTok); }
			strcpy(clientPassword[clientCount], "");
			while(lineTok != NULL){
				strcat(clientPassword[clientCount], lineTok);
				strcat(clientPassword[clientCount], ":");
				lineTok = strtok(NULL, " ");
			}
			int clientPassLen = strlen(clientPassword[clientCount]);
			for(int z = clientPassLen-2; z < clientPassLen; z++){
				clientPassword[clientCount][z] = '\0';
			}
			clientCount++;
		}
		bzero(infoBuff, MAXFILE);
	}
	fclose(configFile);
}


/* making sure user is valid */
int is_user_valid(char* recvBuff, char* clientUsername){
	printf(MSGTERM "\n--Authenticating User--" MSGNORM "\n");

	char sockBuff[MAXBUFF];
	strcpy(sockBuff, recvBuff);
	if(VERBOSE){ printf(".sockBuff: %s\n", sockBuff); }

	strcpy(clientUsername, "");
	char* lineTok = strtok(sockBuff, "\n");
	
	char usernameClient[MAXFILE];
	char passwordClient[MAXFILE];

	while(lineTok != NULL){
		if(strstr(lineTok, "NAME") != NULL){
			char* miniTok = strtok(lineTok, " ");
			if(VERBOSE){ printf(".miniTok: %s\n", miniTok); }

			int position = 0;
			
			while(miniTok != NULL){
				if(position == 1){
					strcpy(usernameClient, miniTok);
				}
				miniTok = strtok(NULL, " ");
				position++;
			}	
		}
		lineTok = strtok(NULL, "\n");
		if(VERBOSE){ printf(".lineTok: %s\n", lineTok); }
	}
	strcpy(sockBuff, recvBuff);
	if(VERBOSE){ printf(".sockBuff: %s\n", sockBuff); }

	lineTok = strtok(sockBuff, "\n");
	if(VERBOSE){ printf(".lineTok: %s\n", lineTok); }
	
	while(lineTok != NULL){
		if(strstr(lineTok, "WORD") != NULL){
			char* miniTok = strtok(lineTok, " ");
			if(VERBOSE){ printf(".miniTok: %s\n", miniTok); }

			int position = 0;
			
			while(miniTok != NULL){
				if(position == 1){
					strcpy(passwordClient, miniTok);
				}

				miniTok = strtok(NULL, " ");
				position++;
			}	
		}
		lineTok = strtok(NULL, "\n");
		if(VERBOSE){ printf(".lineTok: %s\n", lineTok); }
	}

	if(strlen(usernameClient) < 1){
		return 0;
	} else {
		printf("\t\tUsernameClient ok.");
	}

	for(int i = 0; i < clientCount; i++){
		if(strstr(clientPassword[i], usernameClient) != NULL){
			char cpyLine[MAXFILE];
			strcpy(cpyLine, clientPassword[i]);
			if(VERBOSE){ printf(".cpyLine: %s\n", cpyLine); }

			char* miniTok = strtok(cpyLine, ":");
			if(VERBOSE){ printf(".miniTok: %s\n", miniTok); }

			int position = 0;
			while(miniTok != NULL){
				if(position == 1){
					if(!strcmp(passwordClient, miniTok)){
						strcpy(clientUsername, usernameClient);
						if(VERBOSE){ printf(".clientUsername: %s\n", clientUsername); }
						return 1;
					}
				}
				miniTok = strtok(NULL, ":");
				position++;
			}
		}
	}
	return 0;
}


/* function to get the request from the user */
int get_user_request(char* recvBuff){
	printf(MSGTERM "\n--Getting request from user--" MSGNORM "\n");
	char sockBuff[MAXBUFF];

	strcpy(sockBuff, recvBuff);
	if(VERBOSE){ printf(".sockBuff: %s\n", sockBuff); }
	char* lineTok = strtok(sockBuff, "\n");

	while(lineTok != NULL){
		if(strstr(lineTok, "LIST") != NULL) { return 2; }
		else if(strstr(lineTok, "GET") != NULL){ return 0; }
		else if(strstr(lineTok, "PUT") != NULL){ return 1; }
		lineTok = strtok(NULL, "\n");
	}

	/* reaches this when the end of the socket is null */
	return -1;
}


/* whenever the LIST is called */
void handle_req_LIST(int sockfd, char* DirectoryF, char* cUsername){
	printf(MSGTERM "\n--Handling LIST Request--" MSGNORM "\n");
	char dirList[MAXBUFF];
	strcpy(dirList, "");

	char* baseBuff = "LIST\nUSERNAME: %s\n%s\n";
	if(VERBOSE){ printf(".baseBuff: %s\n", baseBuff); }

	char currentDir[MAXFILE];	
	strcpy(currentDir, "./");
	strcat(currentDir, DirectoryF);
	if(VERBOSE){ printf(".currentDir: %s\n", currentDir); }

	/* establish the directory */
	DIR* fileDirectory;
	struct dirent* directoryEntry;
	fileDirectory = opendir(currentDir);

	if(fileDirectory != NULL){
		/* create a directory */
		while(directoryEntry = readdir(fileDirectory)){
			if(	!!strcmp(directoryEntry -> d_name, ".") &&
					!!strcmp(directoryEntry -> d_name, "..")){
				strcat(dirList, directoryEntry -> d_name);
				strcat(dirList, "\n");
			}
		}
	}

	char genBuff[MAXBUFF];
	sprintf(genBuff, baseBuff, cUsername, dirList);
	if(VERBOSE){ printf(".genBuff: %s\n", genBuff); }

	int genBuffLength = strlen(genBuff);
	unsigned char* genBuffCharPtr = genBuff;

	do{
		int numBytesSent = send(sockfd, genBuffCharPtr, genBuffLength, MSG_NOSIGNAL);
		if(numBytesSent < 0){
			break;
		}
		genBuffCharPtr = genBuffCharPtr + numBytesSent;
		genBuffLength = genBuffLength - numBytesSent;
	}while(genBuffLength > 0);

	closedir(fileDirectory);
}


/* whenever the PUT is called */
void handle_req_PUT(int sockfd, char* wrkDir, char* nameOfFile){
	printf(MSGTERM "\n--Handling PUT Request--" MSGNORM "\n");
	char* ackOKString = "Ready for ACK %s\n\n";

	char genBuff[SOCKFDR];	
	sprintf(genBuff, ackOKString, nameOfFile);
	if(VERBOSE){ printf(".genBuff: %s\n", genBuff); }

	int genBuffLength = strlen(genBuff);
	unsigned char* genBuffCharPtr = genBuff;
	do{
		int numBytesSent = send(sockfd, genBuffCharPtr, genBuffLength, MSG_NOSIGNAL);
		if(numBytesSent < 0){
			printf(MSGERRR "ERROR: server sending blank (?) messages" MSGNORM "\n");
			break;
		}
		genBuffCharPtr = genBuffCharPtr + numBytesSent;
		genBuffLength = genBuffLength - numBytesSent;
	}while(genBuffLength > 0);

	char entireFilename[SOCKFDR];
	strcpy(entireFilename, wrkDir);
	strcat(entireFilename, "/");
	strcat(entireFilename, nameOfFile);
	if(VERBOSE){ printf(".entireFilename: %s\n", entireFilename); }

	/* open the file for writing */
	FILE* file = fopen(entireFilename, "w");
		
	char sockBuff[SOCKFDR];
	int rogerRecv;
	rogerRecv = recv(sockfd, sockBuff, sizeof(sockBuff), 0);
	fwrite(sockBuff, 1, rogerRecv, file);

	bzero(sockBuff, SOCKFDR);

	while(rogerRecv = recv(sockfd, sockBuff, sizeof(sockBuff), 0)){
		if(rogerRecv == 0){
			break;
		}
		fwrite(sockBuff, 1, rogerRecv, file);
		bzero(sockBuff, SOCKFDR);
	}
	/* close the file cus we done witih it man */
	fclose(file);
}


/* whenever the PUT is called */
void handle_req_GET(int sockfd, char* wrkDir, char* nameOfFile){
	printf(MSGTERM "\n--Handling GET Request--" MSGNORM "\n");
	char genBuff[SOCKFDR];
	char* ackOKString = "ACK OK %s\n\n";
	sprintf(genBuff, ackOKString, nameOfFile);
	if(VERBOSE){ printf(".genBuff: %s\n", genBuff); }

	int genBuffLength = strlen(genBuff);
	unsigned char* genBuffCharPtr = genBuff;

	do{
		int numBytesSent = send(sockfd, genBuffCharPtr, genBuffLength, MSG_NOSIGNAL);
		if(numBytesSent < 0){
			printf(MSGERRR "ERROR: server sending blank (?) messages" MSGNORM "\n");
			break;
		}
		genBuffCharPtr = genBuffCharPtr + numBytesSent;
		genBuffLength = genBuffLength - numBytesSent;
	}while(genBuffLength > 0);

	char entireFilename[SOCKFDR];
	strcpy(entireFilename, wrkDir);
	strcat(entireFilename, "/");
	strcat(entireFilename, nameOfFile);
	if(VERBOSE){ printf(".entireFilename: %s\n", entireFilename); }

	FILE* file = fopen(entireFilename, "r");
	
	char sockBuff[SOCKFDR];
	int rogerRecv;
	rogerRecv = recv(sockfd, sockBuff, sizeof(sockBuff), 0);
	if(VERBOSE){ printf(".sockBuff: %s\n", sockBuff); }

	unsigned char fReadBuff[FILELST];
	while(!feof(file)){
		int numBytesRead = fread(fReadBuff, sizeof(unsigned char), FILELST, file);
		if(VERBOSE){ printf(".fReadBuff: %s\n", fReadBuff); }
		if(numBytesRead < 0){
			printf(MSGERRR "ERROR: reading blank(?) messages" MSGNORM "\n");
			break;
		}
		
		unsigned char* genBuffCharPtr = fReadBuff;
		do{
			int numBytesSent = send(sockfd, genBuffCharPtr, numBytesRead, MSG_NOSIGNAL);
			if(numBytesSent < 0){
				printf(MSGERRR "ERROR: sending blank(?) messages" MSGNORM "\n");
				break;
			}
			genBuffCharPtr = genBuffCharPtr + numBytesSent;
			numBytesRead = numBytesRead - numBytesSent;
		} while(numBytesRead > 0);
					
	}

	/* shutdown the socket */
	shutdown(sockfd, SHUT_WR);

	/* shut down recv */
	recv(sockfd, NULL, 0, 0);

	/* close the file */
    fclose(file);
}


/* called to send the closing/terminating ack */
void send_closing_ack(int sockfd, char* info){
	char* baseBuff = "ERROR\n%s\n";
	char genBuff[FILELST];
	sprintf(genBuff, baseBuff, info);
	if(VERBOSE){ printf(".genBuff: %s\n", genBuff); }

	int genBuffLength = strlen(genBuff);
	unsigned char* ackOKString = genBuff;
	do{
		int numBytesSent = send(sockfd, ackOKString, genBuffLength, MSG_NOSIGNAL);
		if(numBytesSent < 0){
			printf(MSGERRR "ERROR: sending blank(?) messages in closing ack" MSGNORM "\n");
			break;
		}
		ackOKString = ackOKString + numBytesSent;
		genBuffLength = genBuffLength - numBytesSent;
	}while(genBuffLength > 0);
}

