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
int is_valid_URL(const char* urlarg);
int is_valid_VER(const char* verarg);
const char *get_fname_ext(const char *fname);

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
    char metbuf[SHORTBUF];
    char urlbuf[MAXLINE];
    char doturlbuf[MAXLINE];
    char contenttype[SHORTBUF];
    char verbuf[SHORTBUF];
    char ftype[SHORTBUF];
    char *dot = ".";
    char tempstr[MAXLINE];
    char httpmsggiven[]="HTTP/1.1 200 Document Follows\r\nContent-Type:text/html\r\nContent-Length:32\r\n\r\n<html><h1>Hello CSCI4273 Course!</h1>";
    bzero(buf, MAXLINE);
    bzero(metbuf, SHORTBUF);
    bzero(urlbuf, MAXLINE);
    bzero(verbuf, SHORTBUF);
    int is_error500 = 0;
    char error500msg[]="HTTP/1.1 500 Internal Server Error\r\nContent-Type:text/plain\r\nContent-Length:0\r\n\r\n";
    int is_validurl = 0;
    int is_validver = 0;

    n = read(connfd, buf, MAXLINE);
    printf(MSGSUCC "server received the following request:\n%s" MSGSUCC "\n", buf);
    
    printf(MSGWARN "PARSING" MSGNORM "\n");
    char* token1 = strtok(buf, " ");
    size_t tk1len = strlen(token1);
    strncpy(metbuf, token1, tk1len);

    char* token2 = strtok(NULL, " ");
    size_t tk2len = strlen(token2); 
    strncpy(urlbuf, token2, tk2len);

    char* token3 = strtok(NULL, "\r\n");
    size_t tk3len = strlen(token3);
    strncpy(verbuf, token3, tk3len);
    
    bzero(buf, MAXLINE);
    
    printf(MSGSUCC "GETTING FILE" MSGNORM "\n");
    
    FILE *fp = NULL;

    if (is_valid_VER(verbuf)){
        is_validver = 1;
    } else {
        is_validver = 0;
        is_error500 = 1;
    }

    if (is_valid_URL(urlbuf)){
        is_validurl = 1;
    } else {
        is_validurl = 0;
        is_error500 = 1;
    }

    if(is_validurl && is_validver){
        printf(MSGTERM "VALID URL AND VERSION" MSGNORM "\n");
        strcat(doturlbuf, dot);
        strcat(doturlbuf, urlbuf);
        printf(MSGTERM "doturlbuf: %s" MSGTERM "\n", doturlbuf);
        if(!strcmp(doturlbuf, "./")){
            printf(MSGTERM "DEFAULT WEBPAGE" MSGNORM "\n");
            fp = fopen("index.html", "rb");
            printf(MSGSUCC "READING FILE" MSGNORM "\n");
            fseek(fp, 0L, SEEK_END);
            n = ftell(fp);
            rewind(fp);
            printf(MSGSUCC "FILE READ" MSGNORM "\n");
            strcpy(ftype, "html");
            printf(MSGTERM "ftype: %s" MSGNORM "\n", ftype);
        } else if(fp = fopen(doturlbuf, "rb")) {
            printf(MSGSUCC "READING FILE" MSGNORM "\n");
            fseek(fp, 0L, SEEK_END);
            n = ftell(fp);
            rewind(fp);
            printf(MSGSUCC "FILE READ" MSGNORM "\n");
            strcpy(ftype, get_fname_ext(urlbuf));
            printf(MSGTERM "ftype: %s" MSGNORM "\n", ftype);
        } else {
            printf(MSGERRR "FILE DOES NOT EXIST" MSGNORM "\n");
            is_error500 = 1;
        }
        
        if(is_error500 == 0){
            if(!strcmp(ftype, "html")){
                /* set new ftype to text/html */
                strcpy(contenttype, "text/html");
            } else if (!strcmp(ftype, "txt")){
                /* set new ftype to text/plain */
                printf(MSGTERM "TXT FILETYPE" MSGNORM "\n");
                strcpy(contenttype, "text/plain");
            } else if (!strcmp(ftype, "png")){
                /* set new ftype to image/png */
                printf(MSGTERM "PNG FILETYPE" MSGNORM "\n");
                strcpy(contenttype, "image/png");
            } else if (!strcmp(ftype, "gif")){
                /* set new ftype to image/gif */
                printf(MSGTERM "GIF FILETYPE" MSGNORM "\n");
                strcpy(contenttype, "image/gif");
            } else if (!strcmp(ftype, "jpg")){
                /* set new ftype to image/jpg */
                printf(MSGTERM "JPG FILETYPE" MSGNORM "\n");
                strcpy(contenttype, "image/jpg");
            } else if (!strcmp(ftype, "css")){
                /* set new ftype to text/css */
                printf(MSGTERM "CSS FILETYPE" MSGNORM "\n");
                strcpy(contenttype, "text/css");
            } else if (!strcmp(ftype, "js")){
                /* set new ftype to application/java */
                printf(MSGTERM "JS FILETYPE" MSGNORM "\n");
                strcpy(contenttype, "application/java");
            } else {
                printf(MSGERRR "NOT A VALID FILETYPE" MSGNORM "\n");
            }
            printf(MSGTERM "content type: %s" MSGNORM "\n", contenttype);

            char *filebuff = malloc(n);
            fread(filebuff, 1, n, fp);
            
            char tempbuff[MAXLINE];
            sprintf(tempbuff, "HTTP/1.1 200 Document Follows\r\nContent-Type:%s\r\nContent-Length:%ld\r\n\r\n", contenttype, n);
            
            char *httpmsg = malloc(n + strlen(tempbuff));
            printf(MSGTERM "httpmsg: %s" MSGNORM "\n", httpmsg);
            sprintf(httpmsg, "%s", tempbuff);
            memcpy(httpmsg + strlen(tempbuff), filebuff, n);
            n += strlen(tempbuff);

            //printf(MSGTERM"server returning a http message with the following content.\n%s" MSGNORM "\n", httpmsg);
            write(connfd, httpmsg, n);
        } else {
            /* file doesnt exist or something, have server ignore */
            printf(MSGERRR "SENDING ERROR MESSAGE" MSGNORM "\n");
            n = strlen(error500msg);
            write(connfd, error500msg, n);
        }
    } else {
        printf(MSGERRR "NOT VALID" MSGNORM "\n");
        printf(MSGERRR "SENDING ERROR MESSAGE" MSGNORM "\n");
        n = strlen(error500msg);
        write(connfd, error500msg, n);
    }
    bzero(buf, MAXLINE);
    bzero(metbuf, SHORTBUF);
    bzero(urlbuf, MAXLINE);
    bzero(doturlbuf, MAXLINE);
    bzero(verbuf, SHORTBUF);
    bzero(ftype, SHORTBUF);
}

const char *get_fname_ext(const char *fname) {
    const char *period = strrchr(fname, '.');
    if(!period || period == fname){
        return "";
    }
    return period + 1;
}

int is_valid_URL(const char* urlarg){
    int len_urlarg = strlen(urlarg);
    if ((urlarg != NULL) && (urlarg[0] == '\0')) {
        printf("urlarg is empty\n");
        return 0;
    } else {
        return 1;
    }
}

int is_valid_VER(const char* verarg){
    if(strlen(verarg) == 0) return 0;
    if ((verarg != NULL) && (verarg[0] == '\0')) {
        printf("verarg is empty\n");
        return 0;
    } else if(strcmp(verarg, "HTTP/1.1") == 0) {
        return 1;
    } else if(strcmp(verarg, "HTTP/1.0") == 0) {
        return 1;
    } else {
        return 0;
    }
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