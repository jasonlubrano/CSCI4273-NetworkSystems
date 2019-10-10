/* 
 * tcpechoserver.c - A concurrent TCP echo server using threads
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h>      /* for read, write */
#include <sys/socket.h>  /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>

#define MAXLINE  8192  /* max text line length */
#define SHORTBUF  256  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */

#define MSGERRR "\x1b[31m"
#define MSGSUCC "\x1b[32m"
#define MSGNORM "\x1b[0m"
#define MSGTERM "\x1b[35m"
#define MSGWARN "\x1b[33m"

int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);

int main(int argc, char **argv) 
{
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid; 

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
    port = atoi(argv[1]);

    listenfd = open_listenfd(port);
    while (1) {
        connfdp = malloc(sizeof(int));
        *connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
        pthread_create(&tid, NULL, thread, connfdp);
    }
}

/* thread routine */
void * thread(void * vargp) 
{  
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self()); 
    free(vargp);
    echo(connfd);
    close(connfd);
    return NULL;
}

/*
 * echo - read and echo text lines until client closes connection
 */
void echo(int connfd) 
{
    size_t n;
    char buf[MAXLINE];
    char tribuf[MAXLINE];
    char met[SHORTBUF];
    char url[SHORTBUF];
    char ver[SHORTBUF];
    char httpmsggiven[]="HTTP/1.1 200 Document Follows\r\nContent-Type:text/html\r\nContent-Length:32\r\n\r\n<html><h1>Hello CSCI4273 Course!</h1>";
    bzero(buf, MAXLINE);
    bzero(met, SHORTBUF);
    bzero(url, SHORTBUF);
    bzero(ver, SHORTBUF);

    n = read(connfd, buf, MAXLINE);
    printf(MSGSUCC "server received the following request:\n%s" MSGSUCC "\n", buf);
    strtok(met, buf);

    bzero(buf, MAXLINE);
    
    printf(MSGSUCC "GETTING FILE" MSGNORM "\n");
    FILE *fp = NULL;
    if(fp = fopen("index.html", "rb")) {
        printf(MSGSUCC "READING FILE" MSGNORM "\n");
        fseek(fp, 0L, SEEK_END);
        n = ftell(fp);
        rewind(fp);
        printf(MSGSUCC "FILE READ" MSGNORM "\n");
    } else {
        printf(MSGERRR "FILE DOES NOT EXIST" MSGNORM "\n");
    }
    char *filebuff = malloc(n);
    fread(filebuff, 1, n, fp);
    
    char tempbuff[MAXLINE];
    sprintf(tempbuff, "HTTP/1.1 200 Document Follows\r\nContent-Type:text/html\r\nContent-Length:%ld\r\n\r\n", n);
    
    char *httpmsg = malloc(n + strlen(tempbuff));
    printf(MSGERRR "httpmsg: %s" MSGNORM "\n", httpmsg);
    sprintf(httpmsg, "%s", tempbuff);
    memcpy(httpmsg + strlen(tempbuff), filebuff, n);
    n += strlen(tempbuff);

    printf(MSGTERM"server returning a http message with the following content.\n%s" MSGNORM "\n", httpmsg);
    write(connfd, httpmsg, n);    
}

/* 
 * open_listenfd - open and return a listening socket on port
 * Returns -1 in case of failure 
 */
int open_listenfd(int port) 
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
  
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    /* listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
} /* end open_listenfd */

