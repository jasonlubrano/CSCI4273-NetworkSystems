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
	socklen_t len, n; 
	int i = 0;
	
	// Creating socket file descriptor 
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
	
	n = recvfrom(sockfd, (char *)rcvbuf, MAXLINE, 
				0, ( struct sockaddr *) &cliaddr, 
				&len); 

	/* captalize the received string */
	while (rcvbuf[i]) {
		sndbuf[i] = toupper(rcvbuf[i]);
		i++;
	}
	sndbuf[i] = '\0'; 

	sendto(sockfd, (const char *)sndbuf, strlen(sndbuf), 
		0, (const struct sockaddr *) &cliaddr, 
			len); 

	printf("%s\n",sndbuf);

	pclose(sockfd);
	
	return 0; 
} 

