/* 
 * proxyserver.c - A concurrent proxy server using threads
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
#include <openssl/md5.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>


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
void runproxy(int connfd);
void* thread(void *vargp);
int is_buff_null(char* buff);
void check_ipbuff(char *ipbuff);
int is_timer_expired(int socketfd);
int port_check(char const* urlbuff);
void check_host_name(int host_name);
int is_valid_GET(const char* getarg);
int is_valid_URL(const char* urlarg);
int is_valid_VER(const char* verarg);
int is_url_blacklisted(char* urlbuf);
int NULL_error_message_handler(int serverfd);
const char* get_fname_ext(const char *fname);
struct hostent* get_host_entry(char* urlbuf);
FILE* search_dir_for_file(const char* filename);
void check_host_entry(struct hostent * host_entry);
int connect_server_to_proxy(struct in_addr* inaddr);
void remove_port(char* urlbuff, char* urlbuffnoport);
int send_file_from_cache(int address, FILE* source_file);
char* md5sum_hash_file(const char* filename, size_t len);
int GET_error_message_handler(int serverfd, char* getbuff);
int URL_error_message_handler(int serverfd, char* urlbuff);
int VER_error_message_handler(int serverfd, char* verbuff);
char* get_host_name_from_url(int* urlstart, const char* urlbuf);
int blacklist_error_message_handler(int serverfd, char* urlbuff);
int send_request_to_server(int serverfd, char** argHTTP, char* endfox);
int open_listenfd(int port, struct timeval timeout, struct sockaddr_in address);
int get_data_from_server(int connfd, FILE* filename, char* endfox, char** argHTTP);


/* global connection timeout variable */
struct timeval timeout;


/* main function */
int main(int argc, char **argv) 
{
    int listenfd, clientfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in servaddr;
    // struct sockaddr_in cliaddr;
    pthread_t tid; 

    if (argc == 2) {
        // no timeout, business as 
        timeout.tv_sec = 60;
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
void* thread(void * vargp)
{  
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self()); 
    free(vargp);
    runproxy(connfd);
    close(connfd);
    return NULL;
}


/*
 * runproxy
 */
void runproxy(int connfd) 
{
    char buf[MAXLINE];
    char getbuf[SHORTBUF];
    char urlbuf[MAXLINE];
    char urlbufnoport[MAXLINE];
    char verbuf[SHORTBUF];
    char ftype[SHORTBUF];
    char contenttype[SHORTBUF];
    char tempstr[MAXLINE];
    // char *dot = ".";
    // char httpmsggiven[]="HTTP/1.1 200 Document Follows\r\nContent-Type:text/html\r\nContent-Length:32\r\n\r\n<html><h1>Hello CSCI4273 Course!</h1>";
    // char error400msg[]="HTTP/1.1 500 Internal Server Error\r\nContent-Type:text/plain\r\nContent-Length:0\r\n\r\n";
    int is_validurl = 0;
    int is_validver = 0;
    int is_validget = 0;

    bzero(buf, MAXLINE);
    bzero(getbuf, SHORTBUF);
    bzero(urlbuf, MAXLINE);
    bzero(urlbufnoport, MAXLINE);
    bzero(verbuf, SHORTBUF);
    bzero(ftype, SHORTBUF);
    bzero(contenttype, SHORTBUF);
    bzero(tempstr, MAXLINE);
    
    size_t n = read(connfd, buf, MAXLINE);
    printf(MSGSUCC "server received the following request:\n%s" MSGNORM "\n", buf);
    
    int nullbuffer;

    if(is_buff_null(buf)){
        nullbuffer = 1;    
    } else {
        nullbuffer = 0;
    }

    if(!nullbuffer){
        if(VERBOSE){printf(MSGTERM "PARSING REQUEST: %s" MSGNORM "\n", buf);}
        // GET
        char* token1 = strtok(buf, " ");
        size_t tk1len = strlen(token1);
        strncpy(getbuf, token1, tk1len);
        printf(MSGTERM "getbuf: %s" MSGNORM "\n", getbuf);
        // URL
        char* token2 = strtok(NULL, " ");
        size_t tk2len = strlen(token2); 
        strncpy(urlbuf, token2, tk2len);
        printf(MSGTERM "urlbuf: %s" MSGNORM "\n", urlbuf);
        // VER
        char* token3 = strtok(NULL, "\r\n");
        size_t tk3len = strlen(token3);
        strncpy(verbuf, token3, tk3len);
        printf(MSGTERM "verbuf: %s" MSGNORM "\n", verbuf);
        // argument array if i ever need to send all of them at once.
    } else {
        strcpy(getbuf, "");
        strcpy(urlbuf, "");
        strcpy(verbuf, "");
    }

    char* argHTTP[3];
    argHTTP[0] = getbuf;
    argHTTP[1] = urlbuf;
    argHTTP[2] = verbuf;

    bzero(buf, MAXLINE);
    
    if(VERBOSE){printf(MSGTERM "...checking blacklist" MSGNORM "\n");}

    FILE *file_source = NULL;
    
    if(VERBOSE){printf(MSGTERM "...Checking Request..." MSGNORM "\n");}
    if (is_valid_GET(getbuf)){
        if(VERBOSE){printf(MSGTERM "...Valid GET request" MSGNORM "\n");}
        is_validget = 1;
    } else {
        if(VERBOSE){printf(MSGWARN "...Invalid GET request" MSGNORM "\n");}
        is_validget = 0;
    }

    if(VERBOSE){printf(MSGTERM "...Checking HTTP version..." MSGNORM "\n");}
    if (is_valid_VER(verbuf)){
        if(VERBOSE){printf(MSGTERM "...Valid HTTP version" MSGNORM "\n");}
        is_validver = 1;
    } else {
        if(VERBOSE){printf(MSGWARN "...Invalid HTTP version" MSGNORM "\n");}
        is_validver = 0;
    }

    if(VERBOSE){printf(MSGTERM "...Checking URL validity..." MSGNORM "\n");}
    if (is_valid_URL(urlbuf)){
        if(VERBOSE){printf(MSGTERM "...Valid URL" MSGNORM "\n");}
        is_validurl = 1;
        if(port_check(urlbuf)){
            if(VERBOSE){printf(MSGTERM "...Port checked" MSGNORM "\n");}
            //remove_port(urlbuf, urlbufnoport);
        }
    } else {
        if(VERBOSE){printf(MSGWARN "...Invalid URL" MSGNORM "\n");}
        is_validurl = 0;
    }

   /* blacklisting is causing some SERIOUS issues */
    if(VERBOSE){printf(MSGTERM "...Comparing URL against blacklisted sites..." MSGNORM "\n");}
    
    int is_urlblacklisted = 0;
    if(is_validurl){
        if(is_url_blacklisted(urlbuf)){
            // is_url_blacklisted returned 1 for yes
            if(VERBOSE){printf(MSGWARN "...URL blacklisted" MSGNORM "\n");}
            is_urlblacklisted = 1;
        } else {
            // returned 0 for no
            if(VERBOSE){printf(MSGTERM "...URL not blacklisted" MSGNORM "\n");}
            is_urlblacklisted = 0;
        }
    }

    // main function
    if(is_validurl==1 && is_validver==1 && is_validget==1 && is_urlblacklisted==0 && nullbuffer==0){
        // check for blacklisted sites here
        char * url_md5sum = md5sum_hash_file(urlbuf, strlen(urlbuf)); // hash the urlbuff
        char url_chache[MAXBUF];
        memset(url_chache, 0, MAXBUF);
        sprintf(url_chache, "%s.cache", url_md5sum);
        file_source = search_dir_for_file(url_chache);
        if(file_source != NULL){
            // found the file, great, send it
            if(VERBOSE){printf(MSGTERM "...Hash File Found" MSGNORM "\n");}
            send_file_from_cache(connfd, file_source);
            // return 0;
        } else {
            if(VERBOSE){printf(MSGTERM "...Hash File not Found" MSGNORM "\n");}
            file_source = fopen(url_chache, "w");
            int i;
            for(i = 0;i<MAXBUF;i++){
                if(buf[i] == '\n'){ break; }
            }
            // pretty much the working part of the project right here.
            get_data_from_server(connfd, file_source, buf+i+1, argHTTP);
        }
        fclose(file_source);
    } else if(is_validver == 0) {
        // send invalid ver err
        if(VERBOSE){printf(MSGWARN "...Invalid VER: %s" MSGNORM "\n", verbuf);}
        VER_error_message_handler(connfd, verbuf);
    } else if(is_validurl == 0) {
        // send invalid url error
        if(VERBOSE){printf(MSGWARN "...Invalid URL: %s" MSGNORM "\n", urlbuf);}
        URL_error_message_handler(connfd, urlbuf);
    } else if (is_urlblacklisted) {
        if(VERBOSE){printf(MSGWARN "...URL Blacklisted: %s" MSGNORM "\n", urlbuf);}
        blacklist_error_message_handler(connfd, urlbuf);
    } else if(is_validget == 0){
        // send invalid request error
        if(VERBOSE){printf(MSGWARN "...Invalid request: %s" MSGNORM "\n", getbuf);}
        GET_error_message_handler(connfd, getbuf);
    } else {
        // some other error? NULL perhaps?
        if(VERBOSE){printf(MSGWARN "...Assuming null message" MSGNORM "\n");}
        NULL_error_message_handler(connfd);
    }
    
    bzero(buf, MAXLINE);
    bzero(getbuf, SHORTBUF);
    bzero(urlbuf, MAXLINE);
    bzero(urlbufnoport, MAXLINE);
    bzero(verbuf, SHORTBUF);
    bzero(ftype, SHORTBUF);
    bzero(contenttype, SHORTBUF);
    bzero(tempstr, MAXLINE);
}


/* check whether the buffer is null, return 1 for true, 0 for false */
int is_buff_null(char* buff){
    printf(MSGTERM "Checking null buffer..." MSGNORM "\n");
    if(!strcmp(buff, "")){
        printf(MSGTERM "... buffer is null" MSGNORM "\n");
        return 1;
    } else {
        printf(MSGTERM "... buffer is not null" MSGNORM "\n");
        return 0;
    }
}


/* remove port */
void remove_port(char* urlbuff, char* urlbuffnoport){
    if(VERBOSE){printf(MSGTERM "Removing port on %s..." MSGNORM "\n", urlbuff);}
    strcpy(urlbuffnoport, urlbuff);
    char* dotcomport = ".com:";
    char* token1 = strtok(urlbuffnoport, dotcomport);
    size_t tk1len = strlen(token1);
    strncpy(urlbuffnoport, token1, tk1len);
    printf(MSGTERM "no port urlbuff: %s" MSGNORM "\n", urlbuffnoport);
}


/* check for a port # */
int port_check(char const* urlbuff){
    if(VERBOSE){printf(MSGTERM "Checking url for port: %s ..." MSGNORM "\n", urlbuff);}
    char const* p = urlbuff;
    int count;
    for(count=0; ; ++count){
        p = strstr(p, ".com:");
        if(!p){break;}
        p++;
    }
    if(count > 0 ) {
        if(VERBOSE){printf(MSGNORM "port on %s found..." MSGNORM "\n", urlbuff);}
        return 1;
    } else {
        if(VERBOSE){printf(MSGNORM "port on %s not found..." MSGNORM "\n", urlbuff);}
        return 0;
    }
}


/* check hostname */
void check_host_name(int host_name){
    if(VERBOSE){printf(MSGNORM "Checking hostname..." MSGNORM "\n");}
    if(host_name == 1){
        if(VERBOSE){printf(MSGERRR "Hostname" MSGNORM "\n");}
        error(MSGERRR "Hostname" MSGNORM "\n");
        exit(1);
    }
}


/* validates the host entry */
void check_host_entry(struct hostent * host_entry){
    if(VERBOSE){printf(MSGNORM "Checking host entry..." MSGNORM "\n");}
    if (host_entry == NULL){ 
        if(VERBOSE){printf(MSGERRR "Hostentry" MSGNORM "\n");}
        error(MSGERRR "Hostentry" MSGNORM "\n");
        exit(1);
    }
}


/* checks the ip address is valid */
void check_ipbuff(char *ipbuff){ 
    if(VERBOSE){printf(MSGNORM "Checking ip buff..." MSGNORM "\n");}
    if (NULL == ipbuff){ 
        if(VERBOSE){printf(MSGERRR "ipbuff" MSGNORM "\n");}
        error(MSGERRR "ipbuff" MSGNORM "\n");
        exit(1);
    } 
}


/* check for blacklisted sites here */
int is_url_blacklisted(char* urlbuff){
    if(VERBOSE){printf(MSGNORM "Checking blacklisted sites..." MSGNORM "\n");}
    char site[SHORTBUF];
    char *ipbuff;
    // struct timeval timeout; this is a global var now, set in beginnin
    char hostbuff[SHORTBUF];
    int host_name;
    struct hostent* host_entry;
    FILE *fileptr = fopen("blacklist", "r");

    int url_host_name;
    char *url_ipbuff;
    struct hostent* url_host_entry;
    /*
    url_host_name = gethostname(urlbuff, sizeof(urlbuff));
    check_host_name(url_host_name);
    url_host_entry = gethostbyname(urlbuff);
    check_host_entry(url_host_entry);
    url_ipbuff = inet_ntoa(*((struct in_addr*)url_host_entry->h_addr_list[0]));
    if(VERBOSE){printf(MSGTERM "URL Hostname: %s" MSGNORM "\n", urlbuff);}
    if(VERBOSE){printf(MSGTERM "URL IP: %s" MSGNORM "\n", url_ipbuff);}
    */
    if(VERBOSE){printf(MSGSUCC "accessed site: %s" MSGNORM "\n", get_host_name_from_url(NULL, urlbuff));}
    while(fgets(site, sizeof(site), fileptr)){
        if(VERBOSE){printf(MSGTERM "%s vs %s" MSGNORM "\n", urlbuff, site);}
        if(!strcmp(urlbuff, site)) {
            // direct comparison first
            printf(MSGERRR "Site was on blacklist" MSGNORM "\n");
            return 1;
        }
        host_name = gethostname(site, sizeof(site));
        check_host_name(host_name);
    }
    fclose(fileptr);
    return 0;
}


/* function to build and send URL error messages */
int NULL_error_message_handler(int serverfd){
    if(VERBOSE){printf(MSGTERM "Handling error message..." MSGNORM "\n");}
    char genbuff[MAXBUF];
    char errorline[MAXLINE];
    printf("NULL request, invalid\n");
    // build er ror mmessages
    sprintf(genbuff, "%s<html><body>500 Bad Request Reason: NULL message", genbuff);
    sprintf(genbuff, "%s</body></html>", genbuff);
    int linelength = strlen(genbuff);
    sprintf(genbuff, "%s %s\r\n", "HTTP/1.0", "500 Bad Request");
    sprintf(genbuff, "%sProxy: PA3_webproxy\r\n", genbuff);
    sprintf(genbuff, "%sContent-length: %d\r\n", genbuff, linelength);
    sprintf(genbuff, "%sContent-type: %s\r\n\r\n", genbuff, "text/html");
    sprintf(genbuff, "%s%s", genbuff, errorline);
    
    if(VERBOSE){printf(MSGERRR "%s" MSGNORM "\n", genbuff);}
    write(serverfd, genbuff, strlen(genbuff));
}


/* sends out the blacklisted message */
int blacklist_error_message_handler(int serverfd, char* urlbuff){
    if(VERBOSE){printf(MSGWARN "ERROR 403 FORBIDDEN..." MSGNORM "\n");}
    char genbuff[MAXBUF];
    char errorline[MAXLINE];
    // build er ror mmessages
    sprintf(genbuff, "%s<html><body> ERROR 403 Forbidden", genbuff);
    sprintf(genbuff, "%s %s</body></html>", genbuff, urlbuff);
    int linelength = strlen(genbuff);
    sprintf(genbuff, "%s %s\r\n", "HTTP/1.0", "ERROR 403 Forbidden");
    sprintf(genbuff, "%sProxy: PA3_webproxy\r\n", genbuff);
    sprintf(genbuff, "%sContent-length: %d\r\n", genbuff, linelength);
    sprintf(genbuff, "%sContent-type: %s\r\n\r\n", genbuff, "text/html");
    sprintf(genbuff, "%s%s", genbuff, errorline);
    
    if(VERBOSE){printf(MSGERRR "%s" MSGNORM "\n", genbuff);}
    write(serverfd, genbuff, strlen(genbuff));
}


/* function to build and send URL error messages */
int URL_error_message_handler(int serverfd, char* urlbuff){
    if(VERBOSE){printf(MSGTERM "Handling error message..." MSGNORM "\n");}
    char genbuff[MAXBUF];
    char errorline[MAXLINE];
    if(urlbuff){ printf("%s is invalid\n", urlbuff); }
    // build er ror mmessages
    sprintf(genbuff, "%s<html><body>400 Bad Request Reason: Invalid Url:", genbuff);
    sprintf(genbuff, "%s %s</body></html>", genbuff, urlbuff);
    int linelength = strlen(genbuff);
    sprintf(genbuff, "%s %s\r\n", "HTTP/1.0", "400 Bad Request");
    sprintf(genbuff, "%sProxy: PA3_webproxy\r\n", genbuff);
    sprintf(genbuff, "%sContent-length: %d\r\n", genbuff, linelength);
    sprintf(genbuff, "%sContent-type: %s\r\n\r\n", genbuff, "text/html");
    sprintf(genbuff, "%s%s", genbuff, errorline);
    
    if(VERBOSE){printf(MSGERRR "%s" MSGNORM "\n", genbuff);}
    write(serverfd, genbuff, strlen(genbuff));
}


/* function to build and send request error messages */
int GET_error_message_handler(int serverfd, char* getbuff){
    if(VERBOSE){printf(MSGTERM "Handling error message..." MSGNORM "\n");}
    char genbuff[MAXBUF];
    char errorline[MAXLINE];
    if(getbuff){ printf("%s is invalid\n", getbuff); }
    // build er ror mmessages
    sprintf(genbuff, "<html><body>400 Bad Request. GET or POST supproted");
    sprintf(genbuff, "%s %s</body></html>", genbuff, getbuff);
    int linelength = strlen(genbuff);
    sprintf(genbuff, "%s %s\r\n", "HTTP/1.0", "400 Bad Request");
    sprintf(genbuff, "%sProxy: PA3_webproxy\r\n", genbuff);
    sprintf(genbuff, "%sContent-length: %d\r\n", genbuff, linelength);
    sprintf(genbuff, "%sContent-type: %s\r\n\r\n", genbuff, "text/html");
    sprintf(genbuff, "%s%s", genbuff, errorline);
    
    if(VERBOSE){printf(MSGERRR "%s" MSGNORM "\n", genbuff);}
    write(serverfd, genbuff, strlen(genbuff));
}


/* function to build and send VER error messages */
int VER_error_message_handler(int serverfd, char* verbuff){
    if(VERBOSE){printf(MSGTERM "Handling error message..." MSGNORM "\n");}
    char genbuff[MAXBUF];
    char errorline[MAXLINE];
    if(verbuff){ printf("%s is invalid\n", verbuff); }
    // build er ror mmessages
    sprintf(genbuff, "<html><body>400 Bad Request Reason: Invalid HTTP-Version:");
    sprintf(genbuff, "%s %s</body></html>", genbuff, verbuff);
    int linelength = strlen(genbuff);
    sprintf(genbuff, "%s %s\r\n", "HTTP/1.0", "400 Bad Request");
    sprintf(genbuff, "%sProxy: PA3_webproxy\r\n", genbuff);
    sprintf(genbuff, "%sContent-length: %d\r\n", genbuff, linelength);
    sprintf(genbuff, "%sContent-type: %s\r\n\r\n", genbuff, "text/html");
    sprintf(genbuff, "%s%s", genbuff, errorline);
    
    if(VERBOSE){printf(MSGERRR "%s" MSGNORM "\n", genbuff);}
    write(serverfd, genbuff, strlen(genbuff));
}


/* called multiple times to connect a single server to the proxy */
int connect_server_to_proxy(struct in_addr* inaddr){
    if(VERBOSE){printf(MSGTERM "Creating connection..." MSGNORM "\n");}
    struct sockaddr_in sockaddrin_sk2;
    int sk2;
    int sk2_length;

    if((sk2 = socket(AF_INET, SOCK_STREAM, 0)) < 0) { printf(MSGERRR "CREATING CONNECTION: CONNECTION FAILURE" MSGNORM "\n"); return -1;}

    sk2_length = sizeof(sockaddrin_sk2);
    memset(&sockaddrin_sk2, 0, sk2_length);
    sockaddrin_sk2.sin_family = AF_INET;
    sockaddrin_sk2.sin_addr = *inaddr;
    sockaddrin_sk2.sin_port = htons(atoi("80"));

    if(VERBOSE){printf(MSGTERM "...creating socket..." MSGNORM "\n");}
    if(connect(sk2, (struct sockaddr *)& sockaddrin_sk2, sk2_length) < 0){ printf(MSGERRR "CREATING CONNECTION: HOSTNAME FAILURE ERROR" MSGNORM "\n"); return -1;}

    return sk2;
}


/* Sending request to target server */
int send_request_to_server(int serverfd, char** argHTTP, char* endfox){
    if(VERBOSE){printf(MSGTERM "Sending request to server..." MSGNORM "\n");}
    char buff[MAXBUF];
    memset(buff, 0, MAXBUF);

    if(VERBOSE){printf(MSGTERM "...getting hostname..." MSGNORM "\n");}
    int start_url = 0;
    get_host_name_from_url(&start_url, argHTTP[1]);
    if(start_url){ sprintf(buff, "GET %s HTTP/1.0\r\n\r\n", argHTTP[1]+start_url); }
    else { sprintf(buff, "GET / HTTP/1.0\r\n\r\n"); }

    if(VERBOSE){printf(MSGTERM "...buff: %s" MSGNORM "\n", buff);}
    int httpbuff = write(serverfd, buff, strlen(buff));

    if(VERBOSE){printf(MSGTERM "...httpbuff: %d" MSGNORM "\n", httpbuff);}
    return httpbuff;
}


/* get data from server */
int get_data_from_server(int connfd, FILE* filename, char* endfox, char** argHTTP){
    if(VERBOSE){printf(MSGTERM "Getting data from server..." MSGNORM "\n");}
    int serverfd;
    int b_recv;
    char buff[MAXBUF];
    struct hostent* host_entry;
    struct in_addr** in_addrs;
    host_entry = get_host_entry(argHTTP[1]);
    
    in_addrs = (struct in_addr**) host_entry->h_addr_list;
    int receiving_data = 1;
    int i = 0;
    while(in_addrs[i] != NULL) {
        if(VERBOSE){printf(MSGTERM "...Connecting Proxy to Server..." MSGNORM "\n");}
        serverfd = connect_server_to_proxy(in_addrs[i]);
        i++;
        if(serverfd < 0) { continue; }

        if(send_request_to_server(serverfd, argHTTP, endfox) <= 0) {
            if(VERBOSE){printf(MSGTERM "...Sending request to Server..." MSGNORM "\n");}
            close(serverfd);
            continue;
        }

        if(is_timer_expired(serverfd) <= 0) {
            if(VERBOSE){printf(MSGTERM "...Server timer expired..." MSGNORM "\n");}
            close(serverfd);
            continue;
        }

        while(receiving_data) {
            memset(buff, 0, MAXBUF);
            b_recv = 0;
            b_recv = read(serverfd, buff, MAXBUF);
            if(b_recv==0){ receiving_data=0; break;}
            write(connfd, buff, b_recv);
            fwrite(buff, 1, b_recv, filename);
            if(VERBOSE){printf(MSGSUCC "%s" MSGNORM "\n", buff);}
        }
        close(serverfd);
        break;
    }

    return 0;
}


/* get/return the host enty */
struct hostent* get_host_entry(char* urlbuf){
    if(VERBOSE){printf(MSGTERM "Getting host entry" MSGNORM "\n");}
    struct hostent *host_entry;
    char *host_name = get_host_name_from_url(NULL, urlbuf);
    if(VERBOSE){printf(MSGTERM "...host_name: %s" MSGNORM "\n", host_name);}
    
    if(VERBOSE){printf(MSGTERM "...Getting host by name, writeup function..." MSGNORM "\n");}
    if(!(host_entry = gethostbyname(host_name))) {
        // sometimes this is crashing...
        if(VERBOSE){printf(MSGERRR "...ERROR: geting host name failed. returning null." MSGNORM "\n");}
        return NULL;
    }
    if(VERBOSE){printf(MSGTERM "host entry: %s" MSGNORM "\n", host_entry->h_addr_list[0]);}
    free(host_name);
    return host_entry;
}


/* sets the timeout of the socket */
int is_timer_expired(int socketfd){
    if(VERBOSE){printf(MSGTERM "Checking if timer expired..." MSGNORM "\n");}
    // struct timeval timeout; this is a global var now, set in beginning
    fd_set filedescriptor;
    FD_ZERO(&filedescriptor);
    FD_SET(socketfd, &filedescriptor);
    int expiration = select(socketfd+1, &filedescriptor, NULL, NULL, &timeout);
    if(VERBOSE){printf(MSGTERM "... expiration: %d" MSGNORM "\n", expiration);}
    return expiration;
}


/* get the host name from the URL */
char *get_host_name_from_url(int* urlstart, const char* urlbuf){
    if(VERBOSE){printf(MSGTERM "Getting Hostname from URL..." MSGNORM "\n");}
    char* host_name = NULL;
    int ptr_start=0;
    int ptr_end;
    int counter = 0;
    int length = 0;
    for(ptr_end = 0;;ptr_end++) {
        if(urlbuf[ptr_end] == '\0'){ break; }
        if(urlbuf[ptr_end] == '/' ){ counter++; }
        if(!ptr_start && counter == 2){ ptr_start = ptr_end+1; }
        if(counter == 3){ break; }
    }
    if(urlstart) {
        if(urlbuf[ptr_end] == '\0'){ *urlstart = 0; }
        else{ *urlstart = ptr_end; }
        return NULL;
    }
    length = ptr_end - ptr_start + 1;
    host_name = calloc(1, length);
    memcpy(host_name, urlbuf + ptr_start, length-1);
    if(VERBOSE){printf(MSGTERM "...Hostname: %s" MSGNORM "\n", host_name);}
    return host_name;
}


/* function for hashing the file */
char* md5sum_hash_file(const char *fdata, size_t flen){
    // HASH the thing
    if(VERBOSE){printf(MSGTERM "MD5SUM Hash File..." MSGNORM "\n");}
    unsigned char md5sum_hash[MD5_DIGEST_LENGTH];
    memset(md5sum_hash, 0, MD5_DIGEST_LENGTH);

    MD5_CTX md5ctx;
    MD5_Init(&md5ctx);
    MD5_Update(&md5ctx, fdata, flen);

    if(!MD5_Final(md5sum_hash, &md5ctx)){
        if(VERBOSE){printf(MSGERRR "...ERROR hashing File..." MSGNORM "\n");}
        return NULL;
    }
    size_t md5sum_length;
    md5sum_length = MD5_DIGEST_LENGTH<<1+1;
    if(VERBOSE){printf(MSGTERM "...md5sum length: %ld..." MSGNORM "\n", md5sum_length);}

    char* hash_final;
    hash_final = malloc(md5sum_length);
    memset(hash_final, 0, md5sum_length);

    int i = 0;
    for(i=0; i<MD5_DIGEST_LENGTH; i++){ sprintf(&hash_final[i*2], "%02x", (unsigned int)md5sum_hash[i]); }
    if(VERBOSE){printf(MSGTERM "md5sum_hash: %s" MSGNORM "\n", md5sum_hash);}
    if(VERBOSE){printf(MSGTERM "hash_final: %s" MSGNORM "\n", hash_final);}
    
    return hash_final;
}


/* if we found the file in our cache then we have to send it */
int send_file_from_cache(int address, FILE* source_file){
    if(VERBOSE){printf(MSGTERM "Sending file from cache..." MSGNORM "\n");}
    int dr = 0;
    char buff[MAXBUF];
    memset(buff, 0, MAXBUF);
    int getting_data = 1;
    int data_read;
    if(VERBOSE){printf(MSGTERM "...getting data" MSGNORM "\n");}
    while(getting_data){
        data_read = fread(buff, 1, MAXBUF, source_file);
        if(data_read <= 0) { return data_read; }
        write(address, buff, data_read);
        dr = data_read;
        if(data_read < MAXBUF){ getting_data=0; break; }
        memset(buff, 0, MAXBUF);
        data_read = 0;
    }
    if(VERBOSE){printf(MSGTERM "...dr: %d" MSGNORM "\n", dr);}
    return dr;
}


/* search for the file in the cache */
FILE* search_dir_for_file(const char* filename){
    if(VERBOSE){printf(MSGTERM "Searching directory for file: %s..." MSGNORM "\n", filename);}
    struct stat stat_buff;
    if(stat(filename, &stat_buff)==0){
        return fopen(filename, "r");
    } else {
        printf(MSGWARN "...Unable to find file in directory" MSGNORM "\n");
        return NULL;
    }
}


/* gets the filename extension
 * from PA2, might be useful
 */
const char* get_fname_ext(const char *fname) {
    const char *period = strrchr(fname, '.');
    if(!period || period == fname){
        return "";
    }
    return period + 1;
}


/* test whetehr the VER is valid: GET or POST */
int is_valid_GET(const char* getarg){
    if(strlen(getarg) == 0){ return 0; }
    if(strcmp(getarg, "") == 0){ return 0; }
    if ((getarg != NULL) && (getarg[0] == '\0')) {
        printf("getarg is empty\n");
        return 0;
    } else if(strcmp(getarg, "GET") == 0) {
        return 1;
    } else if(strcmp(getarg, "POST") == 0) {
        return 1;
    } else {
        return 0;
    }
}


/* test whether the URL is valid */
int is_valid_URL(const char* urlarg){
    // int len_urlarg = strlen(urlarg);
    if(strlen(urlarg) == 0){ return 0; }
    if(strcmp(urlarg, "") == 0){ return 0; }
    if ((urlarg != NULL) && (urlarg[0] == '\0')) {
        printf( MSGERRR "urlarg is empty" MSGNORM "\n");
        return 0;
    } else {
        return 1;
    }
}


/* test whetehr the VER is valid */
int is_valid_VER(const char* verarg){
    if(strlen(verarg) == 0){ return 0; }
    if(strcmp(verarg, "") == 0){ return 0; }
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
