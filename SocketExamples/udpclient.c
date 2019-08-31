// Client side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define PORT	 8080 
#define MAXLINE 1024 

// Driver code 
int main() { 
	int sockfd; 
	char rcvbuf[MAXLINE]; 
	char sndbuf[MAXLINE]; 
	struct sockaddr_in	 servaddr; 
	socklen_t n, len; 

	// Creating socket file descriptor 
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	} 

	printf("Input lowercase sentence:");

	if (fgets(sndbuf, sizeof sndbuf, stdin)) {
		sndbuf[strcspn(sndbuf, "\n")] = '\0';
	}

	memset(&servaddr, 0, sizeof(servaddr)); 
	
	// Filling server information 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(PORT); 
	servaddr.sin_addr.s_addr = INADDR_ANY; 
	
	
	sendto(sockfd, (const char *)sndbuf, strlen(sndbuf), 
		0, (const struct sockaddr *) &servaddr, 
			sizeof(servaddr)); 
		
	n = recvfrom(sockfd, (char *)rcvbuf, MAXLINE, 
				MSG_WAITALL, (struct sockaddr *) &servaddr, 
				&len); 
	rcvbuf[n] = '\0'; 
	printf("%s\n", rcvbuf); 

	close(sockfd); 
	return 0; 
} 

