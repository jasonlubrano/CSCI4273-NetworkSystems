#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
//#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <openssl/md5.h>
#include <pthread.h>

// strings
#define HTTP1_0 "HTTP/1.0"
#define HTTP1_1 "HTTP/1.1"
#define STATUS200 "200 OK"
#define STATUS400 "400 Bad Request"
#define STATUS404 "404 Not Found"
#define STATUS500 "500 Internal Server Error: cannot allocate memory"
#define STATUS501 "501 Not Implemented"

// charset for checking url validation
#define INVALIDCHARS    ":"

// constants
#define MAXFILENAME             256
#define MAXPATH                 4096
#define MAXBUFFER               4096
#define MAXERRCONTENT           2048
#define MAXLINE                 1024
#define MINLINE                 256
#define MAXREQBUFFER            1024
#define MAXREADLINE             1024
#define MAXATTRNAME             32
#define MAXTYPELENGTH           32
#define MIN_CONTENT_TYPE        10
#define MAXREQUEST              32
#define MAXMETHOD               32
#define MAXVERSION              16
#define MAXKEEPALIVE            16
#define MAXREQNUM               9

// server setting (not used)
#define MAXEXPIREDTIME          120
#define THREADSLEEPTIME_MS      9
#define MAIN_UF_SLEEPTIME_MS    2

// structs
struct spot {
    char state;
    int fd;
};

typedef enum http_methods {GET, POST, ANY} http_method;
typedef enum req_status 
    {S200, S400, S401, S500, S501, S404} status;

// global
int listenfd;
struct spot spots[MAXREQNUM+5];
unsigned init_findworker = 0;
int timeout;

// prototypes
void server_routine(int);
int evalMethod(const char *str, int len);
int constructErrMsg(int num, char buffer[], char** http_reql, int type);
int sendBadRequst(int connfd, char** http_reql, int type);
int methodGet(int clientfd, char** http_reql, char* buffer, int buflen);
int getFromServer(int clientfd, char** http_reql, FILE* toFile,
        char* rest);
int sendFromFile(int tofd, FILE* src);
int Select(int fd);
int sendRequest(int fd, char** http_reql, char* rest);
FILE* searchLocal(const char* url);
char* md5hash(const char* str, size_t length);
int connectRoutine(struct in_addr* inaddr);
struct hostent* getHostent(char* url);
char* getHostName(const char* src, int* uriStart);
int respondMethodOk(int connfd, char** http_reql, FILE *source, 
        off_t fsize, const char *ctype);
int isUrlValid(const char* str);
int isVersionValid(const char* str);
const char* getFullPath(char **fullpath, char* url);
const char* getSuffix(char **suffix, const char* fullpath);
const char* getContentType(const char* suffix);
void* work_routine(void *);
void assignWorker(int fd);
int findSpot();

// Interruption signal handler
void hSIGINT(int sig) {
    // free calloc
    if(listenfd >= 0) {
        close(listenfd);
    }

    exit(0);
}

int main(int argc, char* argv[]) {
    // local variables
    int clientfd;
    socklen_t sa_len;
    struct sockaddr_in server, client;

    if(argc!=2 && argc!=3) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // install signal(s)
    signal(SIGINT, hSIGINT);

    // Start

    // initialize global vars, then local vars
    if(argc == 2) {
        timeout = 120;  // set timeout to 120 sec
    }
    else {
        timeout = atoi(argv[2]);
    }
    listenfd = -1;

    sa_len = sizeof(client);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(atoi(argv[1]));

    // create a TCP socket and set up server socket
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stdout, "Error: failed to create socket.\n");
        exit(1);
    }

    // bind socket
    if(bind(listenfd, (struct sockaddr *) &server, sa_len) < 0) {
        fprintf(stdout, "Error: failed to bind socket.\n");
        exit(1);
    }

    if(listen(listenfd, MAXREQNUM)) {
        fprintf(stdout, "Error: failed to bind socket.\n");
        exit(1);
    }

    printf("Server: <%s, %d>\n", 
            inet_ntoa(server.sin_addr),
            ntohs(server.sin_port));

    //start server routines
    while(1) {
        clientfd = accept(listenfd, (struct sockaddr *)&client, &sa_len);
        /*
        printf("Connection: %s:%d\n", 
                inet_ntoa(client.sin_addr), ntohs(client.sin_port));
                */
        if(clientfd < 0) {
            fprintf(stdout, "Error: failed to create connection.\n");
            hSIGINT(0);
            exit(1);
        }
        assignWorker(clientfd);
    }

    // End -- Not reachable
    if(listenfd >= 0) close(listenfd);
    return 0;
}

/*
 * start server routines, only GET
 * return none
 */
void server_routine(int connfd) {
    char* http_reql[3];
    char buffer[MAXLINE];
    char method[MAXMETHOD];
    char url[MAXLINE];
    char version[MAXVERSION];
    char host[MAXLINE];
    char keep_alive[MAXKEEPALIVE];

    // set aaray for parameter
    http_reql[0] = method;
    http_reql[1] = url;
    http_reql[2] = version;
    //set zero
    bzero(method, MAXMETHOD);
    bzero(url, MAXLINE);
    bzero(version, MAXLINE);
    bzero(host, MAXLINE);
    bzero(keep_alive, MAXKEEPALIVE);
    bzero(buffer, MAXLINE);

    while(1) { 
        recv(connfd, buffer, MAXLINE, 0);
        sscanf(buffer, "%s %s %s",
                method, url, version);

        switch(evalMethod(method, strlen(method))) {
            case GET:
                methodGet(connfd, http_reql, buffer, strlen(buffer));
                break;
            case POST:
            case ANY:
                sendBadRequst(connfd, http_reql, 0);
                break;
        }
        break;
    }
    close(connfd);
}


/*
 * Method GET routine
 * parameters: int connfd
 *              char** http_reql 
 * return 0 for complete; 1 for error
 */
//int methodGet(int clientfd, char** http_reql){
int methodGet(int clientfd, char** http_reql, char* reqhdr, int hdrlen) {
    FILE* file_src = NULL;
    char* md5sum;
    char cache_name[MAXFILENAME];
    int i;
    // check validation of url and HTTP Version
    if(!isUrlValid(http_reql[1]))  {
        //printf("In methodGet --- url invalid\n");
        sendBadRequst(clientfd, http_reql, 1);
        return 1;
    }

    if(!isVersionValid(http_reql[2])) {
        //printf("In methodGet --- version invalid\n");
        sendBadRequst(clientfd, http_reql, 2);
        return 1;
    }

    // proxy
    md5sum = md5hash(http_reql[1], strlen(http_reql[1]));
    memset(cache_name, 0, MAXFILENAME);
    sprintf(cache_name, "%s.cache", md5sum);
    file_src = searchLocal(cache_name);
    if(file_src) {
        // find valid cache
        sendFromFile(clientfd, file_src);
        fclose(file_src);
        return 0;
    }
    
    // send request to target server
    file_src = fopen(cache_name, "w");
    // loop is not used if firefox is not used
    for(i = 0;; i++) {
        if(reqhdr[i] == '\n') break;
    }
    getFromServer(clientfd, http_reql, file_src, reqhdr+i+1);
    if(file_src) fclose(file_src);
    return 0;
}


// get data from target server
//
int getFromServer(int clientfd, char** http_reql, FILE* toFile,
        char* rest) {

    int flag;
    int serverfd;
    int i, b_recv;
    char buffer[MAXBUFFER];
    struct hostent* h_ent;
    struct in_addr** addr_list;
    h_ent = getHostent(http_reql[1]);
    if(!h_ent) {
        sendBadRequst(clientfd, http_reql, 4);
        return 0;
    }
    addr_list = (struct in_addr**) h_ent->h_addr_list;
    i = 0;
    flag = 0;

    // request from target servers
    while(addr_list[i] != NULL) {
        serverfd = connectRoutine(addr_list[i]);
        i++;
        if(serverfd < 0) {
            continue;
        }

        if(sendRequest(serverfd, http_reql, rest) <= 0) {
            close(serverfd);
            continue;
        }
        if(Select(serverfd) <= 0) {
            close(serverfd);
            continue;
        }
        // receive data
        while(1) {
            memset(buffer, 0, MAXBUFFER);
            b_recv = 0;
            b_recv = read(serverfd, buffer, MAXBUFFER);
            if(b_recv==0) break;
            write(clientfd, buffer, b_recv);
            fwrite(buffer, 1, b_recv, toFile);
        }
        flag = 1;
        close(serverfd);
        break;
    }

    // no data are sent
    if(!flag) {
        sendBadRequst(clientfd, http_reql, 4);
    }
    return 0;
}

// send data from FILE* src to socket tofd
// return number of bytes sent
int sendFromFile(int tofd, FILE* src) {
    int cnt;
    int nb_read;
    char buffer[MAXBUFFER];
    memset(buffer, 0, MAXBUFFER);
    cnt = 0;
    while(1) {
        nb_read = fread(buffer, 1, MAXBUFFER, src);
        if(nb_read <= 0) return nb_read;
        write(tofd, buffer, nb_read);
        cnt += nb_read;
        if(nb_read < MAXBUFFER) break;
        memset(buffer, 0, MAXBUFFER);
        nb_read = 0;
    }
    return cnt;
}

/*
 * eval method 
 * return status
 */
int evalMethod(const char *str, int len) {
    http_method m = ANY;
    if(!len) return m;
    if(!strcmp(str, "POST")) m = POST;
    if(!strcmp(str, "GET")) m = GET;
    return m;
}

/*
 * Construct error message bodys
 * return strlen(buffer)
 */
int constructErrMsg(int num, char buffer[], char** http_reql, int type) {
    switch(type) {
        case 0:
        case 1:
            sprintf(buffer,
                    "%s<html><body>400 Bad Request Reason: Invalid Url:",
                    buffer);
            sprintf(buffer, "%s %s</body></html>", buffer, http_reql[1]);
            break;
        case 2:
            sprintf(buffer, 
                    "<html><body>400 Bad Request Reason: Invalid HTTP-Version:");
            sprintf(buffer, "%s %s</body></html>", buffer, http_reql[2]);
            break;
        case 3:
            sprintf(buffer, 
                    "<html><body>400 Bad Request Reason: Failed to connect server:");
            sprintf(buffer, "%s %s</body></html>", buffer, http_reql[1]);
            break;
        case 4:
            sprintf(buffer, 
                    "<html><body>400 Bad Request Reason: Failed to send request:");
            sprintf(buffer, "%s %s</body></html>", buffer, http_reql[1]);
            break;
    }

    return  strlen(buffer);
}

// send 400
int sendBadRequst(int connfd, char** http_reql, int type) {
    char buffer[MAXBUFFER];
    char content[MAXERRCONTENT];
    int clen;

    bzero(buffer,MAXBUFFER);

    switch(type) {
        case 0:
        case 1:
            if(http_reql[1]) printf("%s is invalid\n", http_reql[1]);
            clen = constructErrMsg(400, content, http_reql, 1);
            break;
        case 2:
            if(http_reql[2]) printf("%s is invalid\n", http_reql[2]);
            clen = constructErrMsg(400, content, http_reql, 2);
            break;
        case 3:
            clen = constructErrMsg(400, content, http_reql, 3);
            break;
        case 4:
            clen = constructErrMsg(400, content, http_reql, 4);
            break;
    }
    sprintf(buffer, "%s %s\r\n", HTTP1_1, STATUS400);
    sprintf(buffer, "%sServer: PA2-Server\r\n", buffer);
    sprintf(buffer, "%sContent-length: %d\r\n", buffer, clen);
    sprintf(buffer, "%sContent-type: %s\r\n\r\n", buffer, "text/html");
    sprintf(buffer, "%s%s", buffer, content);
    write(connfd, buffer, strlen(buffer));
    return 0;
}

// find valid cache
// if found, return FILE pointer;
// NULL otherwise
FILE* searchLocal(const char* filename) {
    struct stat buf;
    time_t sys_time;
    int diff;

    if(stat(filename, &buf)) return NULL;

    sys_time = time(NULL);
    diff = sys_time - buf.st_mtime;
    if(timeout-5<=0) return NULL;
    if(diff >= timeout-5) return NULL;

    return fopen(filename, "r");
}

// set the timeout of sock fd
// the time is set to 20 sec
// return val of select
int Select(int fd) {
    int res;
    struct timeval tv = {.tv_sec = 20, .tv_usec = 0};
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);
    res = select(fd+1, &fdset, NULL, NULL, &tv);
    return res;
}

// send request to the target server
int sendRequest(int fd, char** http_reql, char* rest) {
    char buffer[MAXBUFFER];
    int uriStart;
    int b_sent;

    memset(buffer, 0, MAXBUFFER);
    uriStart = 0;
    getHostName(http_reql[1], &uriStart);
    if(uriStart)
        sprintf(buffer, "GET %s HTTP/1.0\r\n\r\n", http_reql[1]+uriStart);
    else
        sprintf(buffer, "GET / HTTP/1.0\r\n\r\n");
    b_sent = write(fd, buffer, strlen(buffer));
    return b_sent;
}

// compute md5sum of the file
char* md5hash(const char* str, size_t length) {
    char* res;
    int md5_len;
    int i;
    unsigned char md5sum[MD5_DIGEST_LENGTH];
    MD5_CTX mdcxt;

    memset(md5sum, 0, MD5_DIGEST_LENGTH);

    MD5_Init(&mdcxt);
    MD5_Update(&mdcxt, str, length);

    if(!MD5_Final(md5sum, &mdcxt)) {
        fprintf(stderr, "Error: Failed to compute md5sum, set to 0...\n");
        return NULL;
    }

    md5_len = MD5_DIGEST_LENGTH<<1+1;
    res = malloc(md5_len);
    memset(res, 0, md5_len);
    for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&res[i*2], "%02x", (unsigned int)md5sum[i]);
    }
    return res;
}


// connect the individual server
// return file descriptor on success; otherwise -1
int connectRoutine(struct in_addr* inaddr) {
    int sock;
    int sa_len;
    struct sockaddr_in server;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Error: socket failed...\n");
        return -1;
    }

    sa_len = sizeof(server);
    memset(&server, 0, sa_len);
    server.sin_family   = AF_INET;
    server.sin_addr     = *inaddr;
    server.sin_port     = htons(atoi("80"));
    //server.sin_port   = htons(atoi("8097"));

    if(connect(sock, (struct sockaddr *) &server, sa_len) < 0) {
        printf("Error: Unable to connect server...\n");
        return -2;
    }
    return sock;
}

// get host entry from a url
// return host entry
struct hostent* getHostent(char* url) {
    char *hostname;
    struct hostent *h_ent;

    hostname = getHostName(url, NULL);
    //printf("hostname: ->%s<-\n", hostname);
    if(!(h_ent = gethostbyname(hostname))) {
        fprintf(stderr, "Error: failed to get host name...\n");
        return NULL;
    }
    free(hostname);
    return h_ent;
}

// extract hostname from url
// and the start point of the uri
// if uriStart is NULL, return hostname; otherwise modify uriStart
char* getHostName(const char* src, int* uriStart) {
    int start=0;
    int end;
    int acc = 0;
    int len = 0;
    char* str = NULL;
    for(end = 0;;end++) {
        if(src[end] == '\0') break;
        if(src[end] == '/' ) acc++;
        if(!start&&acc == 2) start = end+1;
        if(acc == 3) break;
    }
    if(uriStart) {
        if(src[end] == '\0') *uriStart = 0;
        else *uriStart = end;
        return NULL;
    }
    len = end+1-start;
    str = calloc(1, len);
    memcpy(str, src+start, len-1);
    return str;
}


// check url is valid, INVALIDCHARS is empty
int isUrlValid(const char* str) {
    int len = strlen(str);
    const char *invalidchars = INVALIDCHARS;
    char temp[8];
    int i,j;

    if(strlen(str) < 7) return 0;
    memset(temp, 0, 8);
    memcpy(temp, str, 7);
    if(strcmp(temp, "http://")) return 0;
    // examine the invalid characters
    for(i = 0; i < strlen(invalidchars); i++ ) {
        for(j = 7; j < len; j++) {
            if(invalidchars[i] == str[j]) return 0;
        }
    }
    return 1;
}

// check http version
// return 1 for true, otherwise 0
int isVersionValid(const char* str) {
    if(str == NULL|| strlen(str) == 0) return 0;
    //return !(strcmp(str, HTTP1_0)) || !(strcmp(str, HTTP1_1));
    return !(strcmp(str, HTTP1_0));
}


// start the routine of a thread -- server_routine
// release spot
// return NULL
void* work_routine(void* data) {
    int fd;
    struct spot *s = (struct spot*) data;
    fd = s->fd;
    server_routine(fd);
    s->state = 0;
    return NULL;
}

// create a thread 
// return none
void assignWorker(int fd) {
    int rc;
    int s;
    pthread_t p;
    
    // find a unsed spot to avoid race condition
    s = findSpot();
    spots[s].fd = fd;

    // create thread and assign work_routine
    rc = pthread_create(&p, NULL, 
                          work_routine, (void*)&spots[s]);
    if(rc) {
        fprintf(stdout, 
            "Error: Failed to create thread, return value: %d\n", rc);
    }
    spots[s].state = 1;
}

// find a unsed spot from  spots
// return index of available spot
int findSpot() {
    int i;
    int total = MAXREQNUM+5;
    
    while(1) {
        i = init_findworker%total;
        if(!spots[i].state) break;
        init_findworker++;
    }
    init_findworker++;
    return i;
}