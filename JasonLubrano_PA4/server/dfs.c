/* 
 * client/dfs.c - client sided distributed file sharing system
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
 * lengths
 */
#define MD5_LEN     16      /* max length for md5 */
#define SHORTBUF    256     /* max text line length */
#define MAXLINE     8192    /* max text line length */
#define LISTENQ     1024    /* second argument to listen() */


/*
 * verbosity
 */
#define VERBOSE 1

/*
 * function headers
 */
void welcome_message();
void closing_message();

/*
 * global timeout
 */
struct timeval timeout;

/*
 * main
 */
int main(int argc, char **argv){
    welcome_message();
    closing_message();
    return 0;
}

/*
 * function definitons
 */
void welcome_message(){
    printf(MSGSUCC "------------------------------------" MSGNORM "\n");
    printf(MSGSUCC "        Started DFS - Server" MSGNORM "\n");
    printf(MSGSUCC "------------------------------------" MSGNORM "\n");

}


void closing_message(){
    printf(MSGSUCC "------------------------------------" MSGNORM "\n");
    printf(MSGSUCC "        Closing DFS - Server" MSGNORM "\n");
    printf(MSGSUCC "------------------------------------" MSGNORM "\n");
}