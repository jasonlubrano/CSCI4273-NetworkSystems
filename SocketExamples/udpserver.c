// Server side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <ctype.h>

#define PORT	 8080 
#define MAXLINE 1024 

// Driver code 
int main() { 
	int sockfd; 
	char rcvbuf[MAXLINE]; 
	char sndbuf[MAXLINE]; 
	struct sockaddr_in servaddr, cliaddr;
	int i = 0;
	
	// Creating sowhile (rcvbuf[i]) {
	// 	sndbuf[i] = toupper(rcvbuf[i]);
	// 	i++;
	// }cket file descriptor 
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	} 
	
	memset(&servaddr, 0, sizeof(servaddr)); 
	memset(&cliaddr, 0, sizeof(cliaddr)); 
	
	// Filling server information 
	servaddr.sin_family = AF_INET; // IPv4 
	servaddr.sin_addr.s_addr = INADDR_ANY; 
	servaddr.sin_port = htons(PORT); 
	
	// Bind the socket with the server address 
	if ( bind(sockfd, (const struct sockaddr *)&servaddr, 
			sizeof(servaddr)) < 0 ) 
	{ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	}

	printf("The server is ready to receive\n"); 
	
	int len, n;
	n = recvfrom(sockfd, (char *)rcvbuf, MAXLINE, 
				MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
				&len);

	rcvbuf[n] = '\0';
	
	printf("Client: %s\n", rcvbuf);

	/* captalize the received string */
	while (rcvbuf[i]) {
		sndbuf[i] = toupper(rcvbuf[i]);
		i++;
	}

	printf("MSG to Client: %s\n", sndbuf);

	sendto(sockfd, (const char *)sndbuf, strlen(sndbuf), 
		MSG_CONFIRM, (const struct sockaddr *) &cliaddr, 
			len);

	printf("message sent to client\n");

	return 0; 
}