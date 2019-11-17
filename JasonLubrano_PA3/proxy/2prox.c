/* 
 * proxyserver.c - A concurrent proxy server using threads
 * hashing resource: https://processhacker.sourceforge.io/doc/struct_m_d5___c_t_x.html
 * directory searching resource: https://docs.microsoft.com/en-us/windows/win32/fileio/listing-the-files-in-a-directory?redirectedfrom=MSDN
 */


/* includes, big list
 * clean up possibly?
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h>      /* for read, write */
#include <sys/socket.h>  /* for socket use */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
// #include <openssl/md5.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <ctype.h>


/* maximum lengths */
#define MAXLINE  8192  /* max text line length */
#define SHORTBUF  256  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */
#define MD5_LEN 16


/* error messages on termianl */
#define MSGERRR "\x1b[31m"
#define MSGSUCC "\x1b[32m"
#define MSGNORM "\x1b[0m"
#define MSGTERM "\x1b[35m"
#define MSGWARN "\x1b[33m"


/* verboseness */
#define VERBOSE 1


/* Functions used throughout */
int open_listenfd(int port, struct timeval timeout, struct sockaddr_in address);
void runproxy(int connfd);
void *thread(void *vargp);
int is_valid_URL(const char* urlarg);
int is_valid_VER(const char* verarg);
const char *get_fname_ext(const char *fname);
int send_bad_request(int connfd, char** bufferr, int type);
FILE * search_dir_for_file(const char* filename);
int send_file_from_cache(int address, FILE* source_file);
char *md5sum_hash_file(const char* filename, size_t len);


/* global timeout variable */
struct timeval timeout;


/* main function */
int main(int argc, char **argv) 
{
    int listenfd, clientfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in servaddr, cliaddr;
    pthread_t tid; 

    if (argc == 2) {
        // no timeout, business as 
        timeout.tv_sec = 30;
        timeout.tv_usec = 0;
    } else if (argc == 3){
        // set timeout
        timeout.tv_sec = atoi(argv[2]);
        timeout.tv_usec = 0;
    } else {
        fprintf(stderr, "Usage 1: %s <port>\n", argv[0]);
        fprintf(stderr, "Usage 2: %s <port> <timeout>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);

    listenfd = open_listenfd(port, timeout, servaddr);
    while (1) {
        // clientfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clientlen);
        connfdp = malloc(sizeof(int));
        *connfdp = accept(listenfd, (struct sockaddr*)&servaddr, &clientlen);
        pthread_create(&tid, NULL, thread, connfdp);
    }
}


/* generic error Message,
 * should exit but isnt for some reason
 */
void error(char *msg){
    perror(msg);
    exit(0);
}


/* thread routine */
void * thread(void * vargp) 
{  
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self()); 
    free(vargp);
    runproxy(connfd);
    close(connfd);
    return NULL;
}


/* have to tell if the URL or VER is bad */
int send_bad_request(int connfd, char** bufferr, int type){
    char buff[MAXBUF];
    char cont[MAXBUF];
    int errormsglen;
    bzero(buff, MAXBUF);
}


/*
 * runproxy
 */
void runproxy(int connfd) 
{
    size_t n;
    char buf[MAXLINE];
    char getbuf[SHORTBUF];
    char urlbuf[MAXLINE];
    char doturlbuf[MAXLINE];
    char verbuf[SHORTBUF];
    char ftype[SHORTBUF];
    char contenttype[SHORTBUF];
    char tempstr[MAXLINE];
    char *dot = ".";
    char httpmsggiven[]="HTTP/1.1 200 Document Follows\r\nContent-Type:text/html\r\nContent-Length:32\r\n\r\n<html><h1>Hello CSCI4273 Course!</h1>";
    int is_error400 = 0;
    char error400msg[]="HTTP/1.1 500 Internal Server Error\r\nContent-Type:text/plain\r\nContent-Length:0\r\n\r\n";
    int is_validurl = 0;
    int is_validver = 0;

    bzero(buf, MAXLINE);
    bzero(getbuf, SHORTBUF);
    bzero(urlbuf, MAXLINE);
    bzero(doturlbuf, MAXLINE);
    bzero(verbuf, SHORTBUF);
    bzero(ftype, SHORTBUF);
    bzero(contenttype, SHORTBUF);
    bzero(tempstr, MAXLINE);
    
    n = read(connfd, buf, MAXLINE);
    printf(MSGSUCC "server received the following request:\n%s" MSGNORM "\n", buf);
    
    if(VERBOSE){printf(MSGTERM "PARSING REQUEST: %s" MSGNORM "\n", buf);}

    // GET
    char* token1 = strtok(buf, " ");
    size_t tk1len = strlen(token1);
    strncpy(getbuf, token1, tk1len);
    printf(MSGTERM "getbuf: %s" MSGNORM "\n", getbuf);

    char* token2 = strtok(NULL, " ");
    size_t tk2len = strlen(token2); 
    strncpy(urlbuf, token2, tk2len);
    printf(MSGTERM "urlbuf: %s" MSGNORM "\n", urlbuf);

    char* token3 = strtok(NULL, "\r\n");
    size_t tk3len = strlen(token3);
    strncpy(verbuf, token3, tk3len);
    printf(MSGTERM "verbuf: %s" MSGNORM "\n", verbuf);

    bzero(buf, MAXLINE);
    
    // watch for black list
    // connect to http server
    // get data from server
    // spit data out
    // cache data
    // on timeout, delete data

    if(VERBOSE){printf(MSGTERM "CHECKING BLACKLIST" MSGNORM "\n");}

    FILE *file_source = NULL;
    
    if(VERBOSE){printf(MSGTERM "...Checking HTTP version..." MSGNORM "\n");}
    if (is_valid_VER(verbuf)){
        if(VERBOSE){printf(MSGTERM "...Valid HTTP version" MSGNORM "\n");}
        is_validver = 1;
    } else {
        if(VERBOSE){printf(MSGTERM "...Invalid HTTP version" MSGNORM "\n");}
        is_validver = 0;
        is_error400 = 1;
    }

    if(VERBOSE){printf(MSGTERM "...Checking URL validity..." MSGNORM "\n");}
    if (is_valid_URL(urlbuf)){
        if(VERBOSE){printf(MSGTERM "...Valid URL" MSGNORM "\n");}
        is_validurl = 1;
    } else {
        if(VERBOSE){printf(MSGTERM "...Invalid URL" MSGNORM "\n");}
        is_validurl = 0;
        is_error400 = 1;
    }

    if(!is_valid_VER){
        // send version error message
        // send_bad_request(connfd, verbuf, 1);
    } else if(!is_valid_URL){
        // send url error message
        // send_bad_request(connfd, urlbuf, 1);
    }

    if(is_validurl && is_validver){
        char * url_md5sum = md5sum_hash_file(urlbuf, strlen(urlbuf)); // hash the urlbuff
        char url_chache[MAXBUF];
        memset(url_chache, 0, MAXBUF);
        sprintf(url_chache, "%s.cache", url_md5sum);
        if(file_source = search_dir_for_file(url_chache)){
            // found the file, great, send it
            send_file_from_cache(connfd, file_source);
            fclose(file_source);
            // return 0;
        }

        file_source = fopen(url_chache, "w");
        int i;
        for(i = 0;;i++){ if(buf[i] == '\n'){ break; }}

        // pretty much the working part of the project right here.
        // get_data_from_server();

        if(file_source){ fclose(file_source);}

        // return 0;

    } else if(!is_valid_VER) {
        // send invalid ver err
    } else if(!is_valid_URL) {
        // send invalid url error
    } else {
        // some other error?
    }
    
    bzero(buf, MAXLINE);
    bzero(getbuf, SHORTBUF);
    bzero(urlbuf, MAXLINE);
    bzero(doturlbuf, MAXLINE);
    bzero(verbuf, SHORTBUF);
    bzero(ftype, SHORTBUF);
    bzero(contenttype, SHORTBUF);
    bzero(tempstr, MAXLINE);
}


char * md5sum_hash_file(const char *filename, size_t len){
    // HASH the thing
}


/* if we found the file in our cache then we have to send it */
int send_file_from_cache(int address, FILE* source_file){
    int data_read;
    int count = 0;
    char buff[MAXBUF];
    memset(buff, 0, MAXBUF);
    int getting_data = 1;
    while(getting_data){
        data_read = fread(buff, 1, MAXBUF, source_file);
        if(data_read <= 0) {return data_read;}
        write(address, buff, data_read);
        count = data_read;
        if(data_read < MAXBUF){break;}
        // clear to go again
        memset(buff, 0, MAXBUF);
        data_read = 0;
    }
    return count;
}


/* search for the file in the cache */
FILE * search_dir_for_file(const char* filename){
    int time_diff;
    time_t time_s;
    struct stat stat_buff;

    if(stat(filename, &stat_buff)){return NULL;} // file not found

    time_s = time(NULL);
    time_diff = time_s - stat_buff.st_mtime;

    if(time_diff >= timeout.tv_sec-5){return NULL;}
    if(timeout.tv_sec-5 <= 0){return NULL;}

    return fopen(filename, "r");
}


/* gets the filename extension
 * from PA2, might be useful
 */
const char *get_fname_ext(const char *fname) {
    const char *period = strrchr(fname, '.');
    if(!period || period == fname){
        return "";
    }
    return period + 1;
}


/* test whether the URL is valid */
int is_valid_URL(const char* urlarg){
    int len_urlarg = strlen(urlarg);
    if ((urlarg != NULL) && (urlarg[0] == '\0')) {
        printf("urlarg is empty\n");
        return 0;
    } else {
        return 1;
    }
}


/* test whetehr the VER is valid */
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
int open_listenfd(int port, struct timeval timeout, struct sockaddr_in address) 
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
  
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        error(MSGERRR "Error: Failed to create socket" MSGNORM "\n");
        return -1;
    }

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)) < 0){
        error(MSGERRR "Address Used" MSGNORM "\n");
        return -1;
    }

    /* timeout function */
    size_t to = setsockopt(listenfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    if (to != 0){
        error(MSGERRR "TIMEOUT REACHED" MSGNORM "\n");
        return -1;
    }

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

    printf("address: <%s, %d>\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
    printf("serveraddr: <%s, %d>\n", inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port));

    return listenfd;
} /* end open_listenfd */
