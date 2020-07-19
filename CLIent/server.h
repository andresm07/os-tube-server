#include <time.h>
#include <pthread.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h> 


struct Logger;
struct Folder;
struct User_queue;
struct Sessions;

/*Structs*/
struct ipNode{
    char ip[25];
    struct ipNode *next; 
};

struct Multimedia{
    char filetype[40];
    char name[256];
    char dir[256];
    char md5[35];
    struct Multimedia *next;
};

struct User{
    char username[30];
    int is_admin;
    int cant_session;
    char multmdir[255];
    long currentByte;
    int streamming;
    struct Session *user_session[20];
    struct Sessions *ss;
    struct User *next;
};

struct Server{
    int connected_clients;
    int cast_files;
    long principal_memory;
    long sent_bytes;
    long received_bytes;
    int requests;
    int *active;
    pthread_t thread;
};

struct Log{   
    char tag[30];
    char action[100];
    time_t moment;
    struct Log *next;
};

typedef void (* addmult)(char *, char *, char *, char *, struct Folder *);
typedef int (*SaveOp)(struct Log*, struct Logger *logger);
typedef void (*newEntry)(char *tag, char *message, time_t time, struct Logger *logger);

struct Logger{
    struct Log *first;
    SaveOp savetofile;
    newEntry entry;
};

struct Folder{
    struct Multimedia *first;
    addmult add;
};

struct User_queue{
    struct User *first;
};


struct Session{   
    time_t login_time;
    time_t logout_time;
    struct in_addr ip_num;
    int socket_num;
    int reqs;
    long tranferred;
    pthread_t thread;
    int *active;
    struct Session *next;
};

struct Sessions{
    struct Session *first;
};


