/*
    Auxiliary functions
    for structs and processing data
*/
#include "lib/json-maker/json-maker.c"
#include "lib/tiny-json/tiny-json.c"
#include "server.h"
#include <openssl/md5.h>

char *md5sum(char *, int);
char *tohexstr(unsigned char *);
int savelog(struct Log *, struct Logger *);
int printlog(struct Log *, struct Logger *);
void newlog(char *, char *, time_t, struct Logger *);
int isget(char *);
int ispost(char *);
int isoptions(char *);
char *substring(char *, int , int );
int gethttpheader(char **, char*);
int gethttpline(char **, char *);
int ishttp(char *);
int scan(const char *, struct Folder *);
char *folderjson(char *, struct Folder *, int* );
char *folderclient(char *, struct Folder *, int *);
struct Folder* jsontofolder(char *, int);
char *nextstream(char *, char *, struct Folder *);
int namedot(char *);
void addmultimedia(char *, char *, char *, char *, struct Folder *);
char *mediamd5(char *, struct Folder *);
char *getpath(char *);
void addsession(struct Session *, struct Sessions *);
struct Session *cpysession(struct Session *);
struct Session *getlast(struct Sessions *);
void deletesession(int, struct Sessions *);
void newuser(char *, struct Session *, struct User **,struct User_queue *);
void deleteuser(char *, struct User_queue *);
int isuserconnected(char *,struct User **, struct User_queue *);
int contains(char *, char *);
long filelen(char *);
long readfilebytes(char *, char **);
int splitfilebytes(char *, char **, int, int, long*);
char *filetypetocontype(char *);
char *strnextline(char *);
char *httpfield(char *, char *);
int streamable(char *, char *);
void rawprintf(char *);
void printserverstatus(struct Server *);

void debug(char *message, int line){
    printf("%d: ", line);
    printf("%s\n",message);
}

int savelog(struct Log *first,  struct Logger *logger){
    if(first == NULL)
        return 0;
    time_t now = first->moment;
    struct tm *time_struct;
    FILE *logptr;
    char filename[60];
    time_struct = localtime(&now);
    strftime(filename, sizeof(filename), "./log/%Y-%m-%d_%H:%M_%a.log", time_struct);
    if((logptr = fopen(filename,"a+")) == NULL){
        return 0;
    }
    struct Log *tmp = first;
    while(tmp != NULL){
        char timeformat[40];
        now = tmp->moment;
        time_struct = localtime(&now);
        strftime(timeformat, sizeof(timeformat), "%d/%b/%Y:%H:%M:%S", time_struct);
        fprintf(logptr, "[%s] [%s]: %s\n",timeformat,tmp->tag,tmp->action);
        tmp = tmp->next;
    }
    fclose(logptr);
}

int printlog(struct Log *first,  struct Logger *logger){
    if(first == NULL)
        return 0;
    struct tm *time_struct;
    struct Log *tmp = first;
    while(tmp != NULL){
        char timeformat[40];
        time_t now = tmp->moment;
        time_struct = localtime(&now);
        strftime(timeformat, sizeof(timeformat), "%d/%b/%Y:%H:%M:%S", time_struct);
        printf("[%s] [%s]: %s\n",timeformat,tmp->tag,tmp->action);
        tmp = tmp->next;
    }
}

void newlog(char *tag, char *message, time_t time, struct Logger *logger){
    struct Log *entry = calloc(1,sizeof(struct Log));
    strcpy(entry->tag, tag);
    strcpy(entry->action, message);
    entry->moment = time;
    entry->next = NULL;
    struct Log *temp = logger->first;
    if(temp == NULL){
        logger->first = entry;
        return;
    }
    while (temp->next != NULL){
        temp = temp->next;
    }
    temp->next = entry;
}

int isget(char *string){
    char *get = substring(string, 0, 3);
    return !strcmp(get,"GET");
}

int ispost(char *string){
    char *post = substring(string, 0, 4);
    return !strcmp(post,"POST");
}

int isoptions(char *string){
    char *post = substring(string, 0, 7);
    return !strcmp(post,"OPTIONS");
}

char *substring(char *string, int pos, int len){
    char *substr = calloc(1, len + 1);
    int i;
    for (i = 0; i < len; i++){
        *(substr + i) = *(string + pos);
        string++;
    }
    *(substr + i) = 0;
    return substr; 
}

int gethttpheader(char **dest, char *src){
    int size = 0;
    for (size = 0; size < strlen(src); size++){
        if(*(src + size) == '\r'){
            if(*(src + size + 2) == '\r'){
                *dest = substring(src, 0, (size + 3) + 1);
                return size + 4;
            }
        }
    }
    return -1;
}

int ishttp(char *src){
    int i, val;
    char *httpline;
    char httpversion[] = "HTTP/1.1\r\n";
    if((val = gethttpline(&httpline, src)) < 0)
        return 0;
    int crop = 5;
    if(isoptions(httpline)){
        crop = 8;
    }else if(isget(httpline)){
        crop--;
    }
    httpline = substring(httpline, crop, strlen(httpline)- crop);
    /*Check if it is a path */
    if(!(*(httpline) == '/'))
        return 0;
    for (i = 0; i < strlen(httpline); i++)
    {
        if(*(httpline + i) == ' '){
            i++;
            break;
        }
    }
    /*Check if is equal to HTTP/1.1\r\n */
    httpline = substring(httpline, i, strlen(httpline) - i);
    return !strcmp(httpline, httpversion);
}

int gethttpline(char **dest, char *src){
    int i;
    for (i = 0; i < strlen(src) - 1; i++){
        if(*(src + i) == '\r' && *(src + i + 1) == '\n'){
            i += 2;
            *dest = substring(src, 0, i);
            return i;
        }
    }
    return -1;
}

int namedot(char *filename){
    int i;
    for (i = strlen(filename); i >= 0 ; i--){
        if (*(filename + i) == '.')
            return i;
    }
    return -1;
}

void addmultimedia(char *name, char*filetype, char *dir, char *md5, struct Folder *folder){
    struct Multimedia *mult = calloc(1, sizeof(struct Multimedia));
    strcpy(mult->dir,dir);
    strcpy(mult->md5,md5);
    strcpy(mult->filetype,filetype);
    strcpy(mult->name,name);
    struct Multimedia *tmp = folder->first;
    if (tmp == NULL){
        folder->first = mult;
        return;
    }
    while (tmp->next != NULL){
        tmp = tmp->next;
    }
    tmp->next = mult; 
}

char *mediamd5(char *dir, struct Folder * folder){
    struct Multimedia *node = folder->first;
    char *md5 = calloc(1, 35); 
    sprintf(md5, "notfound");
    while (node != NULL)
    {
        if(!strcmp(dir, node->dir)){
            strcpy(md5, node->md5);
            break;
        }
        node = node->next;
    }
    return md5;
}

/*
    Using json-maker Library
*/
char *folderjson(char *dest, struct Folder *folder, int *quantity){
    char *json = json_objOpen(dest, NULL);
    int cant = 0;
    json = json_arrOpen(dest+1, "data");    
    struct Multimedia *file = folder->first;
    while (file != NULL){
        json = json_objOpen(json,NULL);
        json = json_str(json, "name", file->name);
        json = json_str(json, "filetype", file->filetype);
        json = json_str(json, "dir", file->dir);
        json = json_str(json, "md5", file->md5);
        json = json_objClose(json);
        file = file->next;
        cant += 8;
    }
    json = json_arrClose(json);
    json = json_objClose(json);
    json = json_end(json);
    *quantity = cant + 1;
    return dest;
}

char *folderclient(char *dest, struct Folder *folder, int *quantity){
    char *result = calloc(1, 3000);
    struct Multimedia *file = folder->first;
    sprintf(result,"Multimedia Files:\r\n\r\n");
    while (file != NULL){
        char name[700], filetype[150], dir[500], md5[100];
        sprintf(name, "name: %s\r\n", file->name);
        sprintf(filetype, "filetype: %s\r\n", file->filetype);
        sprintf(dir, "dir: %s\r\n", file->dir);
        sprintf(md5, "md5: %s\r\n\r\n", file->md5);
        strncat(result, name, strlen(name));
        strncat(result, filetype, strlen(filetype));
        strncat(result, dir, strlen(dir));
        strncat(result, md5, strlen(md5));
        file = file->next;
    }
    return result;
}

struct Folder *jsontoFolder(char *jsonstructure, int elements){
    struct Folder *result = calloc(1, sizeof(struct Folder));
    json_t pool[elements];
    json_t const *parent = json_create(jsonstructure, pool, elements);
    /*
    if(parent == NULL){
        printf("%s\n", "PARENT NULL");
        return NULL;
    }else{
        json_t const *data_array = json_getProperty(parent, "data");
        printf("data NULL ? %d\n", data_array == NULL);
        int n = elements/8;
        for (int i = 0; i < n; i++)
        {
            json_t const *child = json_getChild(data_array);
            printf("child %d NULL ? %d", i ,child == NULL);
            if(child != NULL){
                json_t const *namefield = json_getProperty(child,"name");
            } 
        }
    }*/
    return result;
}

char *nextstream(char *dir, char *client, struct Folder *folder){
    struct Multimedia *tmp = folder->first;
    int found = 0;
    while (tmp != NULL){
        if(found){
            if(streamable(client, tmp->filetype))
                return tmp->dir;
        }
        else if(!strcmp(tmp->dir, dir))
            found = 1;
        tmp = tmp->next;
    }
    tmp = folder->first;
    while (tmp != NULL){
        if(streamable(client, tmp->filetype))
            return tmp->dir;
        tmp = tmp->next;
    }
    return "";
}

char *getpath(char *httpheader){
    int slashpos = 0;
    int len = 0;
    int found = 0;
    for (int i = 0; i < strlen(httpheader); i++){
        if(!found){
            if(*(httpheader + i) == '/' ){
                found = 1;
                slashpos = i;
            }
        }else{
            len++;
            if(*(httpheader + i) == ' '){
                break;
            }
        }
    }
    char *path = substring(httpheader, slashpos, len);
    return path;
}

void addsession(struct Session *s, struct Sessions *pool){
    struct Session *tmp = pool->first;
    if(tmp == NULL){
        pool->first = s;
        return;
    }
    while (tmp->next != NULL){
        tmp = tmp->next;
    }
    tmp->next = s; 
}

struct Session *cpysession(struct Session *session){
    struct Session *copy = malloc(sizeof(struct Session));
    copy->active = session->active;
    copy->ip_num = session->ip_num;
    copy->login_time = session->login_time;
    copy->logout_time = session->logout_time;
    copy->next = NULL;
    copy->reqs = session->reqs;
    copy->socket_num =session->socket_num;
    copy->thread = session->thread;
    copy->tranferred = session->tranferred;
    return copy;
}

struct Session *getlast(struct Sessions *pool){
    struct Session *tmp = pool->first;
    if(tmp == NULL)
        return NULL;
    while (tmp->next != NULL){
        tmp = tmp->next;
    }
    return tmp;
}

void deletesession(int socket, struct Sessions *pool){
    struct Session *tmp = pool->first;
    if(socket == tmp->socket_num){
        pool->first = tmp->next;
    }else{
        while (tmp->next != NULL){
            if(socket == tmp->socket_num){
                tmp->next = tmp->next->next;
            }
            tmp = tmp->next;
        }
    }
}

void newuser(char *username, struct Session *session, struct User **userptr, struct User_queue *queue){
    struct User *new_user = calloc(1, sizeof(struct User));
    new_user->ss = calloc(1, sizeof(struct Sessions));
    *userptr = new_user;
    addsession(session, new_user->ss);
    new_user->cant_session++;
    new_user->currentByte = 0;
    new_user->streamming = 0;
    strcpy(new_user->username, username);
    struct User *tmp = queue->first;
    if(tmp == NULL){
        queue->first = new_user;
        return;
    }
    while (tmp->next != NULL){
        tmp = tmp->next;
    }
    tmp->next = new_user; 
}

void deleteuser(char *username, struct User_queue *queue){
    struct User *tmp = queue->first;
    if(!strcmp(username, tmp->username)){
        queue->first = tmp->next;
    }else{
        while (tmp->next != NULL){
            if(!strcmp(username, tmp->next->username)){
                tmp->next = tmp->next->next;
            }
            tmp = tmp->next;
        }
    }
}

int isuserconnected(char *username, struct User **dest, struct User_queue *queue){
    int found = 0;
    struct User *tmp = queue->first;
    while (tmp != NULL)
    {
        if(!strcmp(tmp->username, username)){
            found = 1;
            *dest = tmp;
            break;
        }
        tmp = tmp->next;
    }
    return found;
}

int contains(char *longstr, char *shortstr){
    if(strlen(longstr) < strlen(shortstr)){
        return 0;
    }
    char *sub = substring(longstr, 0, strlen(shortstr));
    return !strcmp(sub, shortstr);
}

long filelen(char *name){
    FILE *fileptr;
    long filelen = 0;
    fileptr = fopen(name, "rb");
    fseek(fileptr, 0, SEEK_END);
    filelen = ftell(fileptr);
    rewind(fileptr);
    fclose(fileptr);
    return filelen;
}

long readfilebytes(char *name, char **bytes){
    FILE *fileptr;
    long filelen = 0;
    fileptr = fopen(name, "rb");
    fseek(fileptr, 0, SEEK_END);
    filelen = ftell(fileptr);
    rewind(fileptr);
    *bytes = malloc(filelen);
    fread(*bytes, filelen, 1, fileptr);
    fclose(fileptr);
    return filelen;
}

int splitfilebytes(char *name, char **bytes, int initpos, int chunksize, long *full_len){
    long len = readfilebytes(name, bytes);printf("%ld ", len);
    *full_len = len;
    chunksize = (initpos + chunksize) > (len - 1) ? (len - 1) - initpos : chunksize;
    char *temp = calloc(1, chunksize);
    *bytes = *bytes + initpos;
    for (int i = 0; i < chunksize; i++){
        *(temp + i) = *(*bytes + i);
    }
    *bytes = temp;
    return chunksize;
}

char *filetypetocontype(char *filetype){
    char *contype = calloc(1, 50);
    if(!strcmp("jpg", filetype) || !strcmp("jpeg",filetype) || !strcmp("png", filetype) || !strcmp("gif", filetype)){
        strcpy(contype, "image/");
    }else if(!strcmp("mpeg", filetype) || !strcmp("mp4",filetype) || !strcmp("webm", filetype)){
        strcpy(contype, "video/");
    }else if(!strcmp("txt", filetype) || !strcmp("html", filetype)){
        strcpy(contype, "text/");
    }else if(!strcmp("mp3", filetype) || !strcmp("mpga",filetype) || !strcmp("wav",filetype) ){
        sprintf(filetype, "mpeg");
        strcpy(contype, "audio/");
    }else{ /* application/filename */
        strcpy(contype, "application/");
    }
    strncat(contype, !strcmp(filetype,"txt") ? "plain" : filetype, 
    !strcmp(filetype,"txt") ? strlen("plain") : strlen(filetype));
    return contype;
}

char *md5sum(char *bytes, int len){
    unsigned char *md5 = calloc(1, MD5_DIGEST_LENGTH + 1);
    MD5_CTX mdcontext;
    MD5_Init(&mdcontext);
    MD5_Update(&mdcontext, bytes, len);
    MD5_Final(md5, &mdcontext);
    return md5;
}

char *tohexstr(unsigned char *bytes){
    int loop = 0, i = 0;
    char *output = calloc(1, 2 * strlen(bytes));
    for (int i = 0; bytes[loop] != 0; i+=2)
    {
        sprintf((char*)(output+i),"%02x", bytes[loop++]);
    }
    return output;
}

char *strnextline(char *src){
    int len = strlen(src), i;
    for ( i = 0; i < len; i++){
        if(*(src + i) == '\n'){
            if( *(src + i + 1) == '\r') return NULL;
            else break;
        }
    }
    i++;
    return i < len ? (src + i) : NULL;
}

char *httpfield(char *httpheader, char *field){
    if(!strcmp(field, "")) return NULL;
    while (httpheader != NULL){
        int fieldlen = strlen(field), i, len = strlen(httpheader);
        if(len < fieldlen) break;
        if(contains(httpheader, field)){
            for (i = 0; i < len; i++){
                if (*(httpheader + i) == '\r'){
                    break;
                }
            }
            i--;
            char *value = substring(httpheader, fieldlen + 2, i - (fieldlen + 1));
            return value;
        }
        httpheader = strnextline(httpheader);
    }
    return NULL;  
}

int streamable(char *client, char *contype){
    if(client == NULL) return 0;
    if(!strcmp(client, "web"))
        return contains(contype, "video/") || contains(contype, "audio/");
    else if(!strcmp(client, "cli"))
        return contains(contype, "audio/");
    return 0;
}

void rawprintf(char *message){
    if (message == NULL) return;
    int len = strlen(message);
    for (int i = 0; i < len; i++)
    {
        if(*(message + i) == '\n'){
            printf("\\n");
        }else if(*(message + i) == '\r'){
            printf("\\r");
        }else if(*(message + i) == '\t'){
            printf("\\t");
        }else if(*(message + i) == '\"'){
            printf("\\\"");
        }else{
            printf("%c", *(message +i));
        }
    }
    printf("\n");
}

void printserverstatus(struct Server *server){
    printf("Active clients: %d \n", server->connected_clients);
    printf("Total requests: %d\n", server->requests);
    int recv = 0, sent = 0;
    double rb = server->received_bytes, sb = server->sent_bytes;
    if(rb > 1024)
        printf("Data received: %.2f %s\n", rb/1048576, "MB");
    else
        printf("Data received: %ld %s\n", (long) rb, "Bytes"); 
    if(sb > 1024)
        printf("Data sent: %.2f %s\n", (sb/1048576), "MB");
    else
        printf("Data sent: %ld %s\n", (long) sb, "Bytes");
    printf("Cast files: %d\n", server->cast_files);
}