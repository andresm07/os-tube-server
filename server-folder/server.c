#include "utils.c"
#include <dirent.h>




/*
    TODO:
    * CALCULAR MD5SUM A LOS ARCHIVOS
    * IMPLEMENTAR EL CAMBIO DE IP
    * IMPRIMIR EL ESTADO DEL SERVER
    * ENVIAR TROZO DE VIDEO

*/


#define DEFAULT_PORT 1800
#define BUFFER_SIZE 524288
#define CHUNK_SIZE 524288
#define BACKLOG 10
#define RES_DIR "res"
#define HTTP_400 "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\nConnection: keep-alive\r\n\r\n"
#define HTTP_401 "HTTP/1.1 401 Unauthorized\r\nAccess: 0\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\nContent-Length: 0\r\nWWW-Authenticate: Basic\r\nConnection: keep-alive\r\n\r\n"
#define HTTP_404 "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\nConnection: keep-alive\r\n\r\n"
#define HTTP_405 "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\nConnection: keep-alive\r\n\r\n"

int port;
int t_result = 0;
sem_t *semaphore;
int server_FD;
struct sockaddr_in server_address;
struct Server *server_information;
struct Session unset_sessions[BACKLOG];
struct Sessions *all_sessions;
struct Logger *logger;
struct Folder *folder;
struct User_queue *users;

/*Prototypes*/
void start_connection(int *, int);
void debug(char*, int); /*     DELELTE LATER    */
void init_server();                     /*   DELETE LATER    */
void *terminal(void *);                  /* IMPLEMENT FUNCTIONS */
void *connection_handler(void *);
int login(char *, char *);
int log_out(void *);                    /* TODO */
int upload_file(void *);                /* TODO */
int register_user(char *, char *);
int switch_ip(void *);                          /* TODO */
void disconnect_user(void *);                   /* TODO */
void closeallconnections(struct Sessions *);
void route_login(char *, char *, char **, char **, struct User **, struct Session *);
void route_register(char *, char *, char **, char **, struct User **, struct Session *);
void route_res(char *, char *, char **);
int route_resfile(char *, char *, char *, char **, struct User *);
int route_test(char *, char *, char **);

/*  ./server [port] */
int main(int argc, char* argv[]){
    srand(time(NULL));

    /*Variables*/
    int connection_number = 0;
    logger = calloc(1,sizeof(struct Logger));
    logger->entry = newlog;
    logger->savetofile = savelog;
    folder = calloc(1,sizeof(struct Folder));
    folder->add = addmultimedia;
    users = calloc(1, sizeof(struct User_queue));
    server_information = calloc(1,sizeof(struct Server));
    server_information->active = malloc(sizeof(int));
    semaphore = malloc(sizeof(sem_t));
    all_sessions = calloc(1, sizeof(struct Sessions));
    /*---------- */

    /*parameters*/
    if(argc > 2){
        fprintf(stderr, "Too many arguments");
    }else if(argc == 2){
        /*Asign the port*/
        port = atoi(argv[1]);
    }else
    {
        port = DEFAULT_PORT;
    }
    scan(RES_DIR,folder);
    char *json = calloc(1, 2000);
    int *cant = malloc(sizeof(int));
    json = folderjson(json, folder, cant);

    /*Creating the server socket*/
    start_connection(&server_FD, port);
    logger->entry("SOCKET","The server started successfully", time(NULL), logger);

    /*Server socket up and running*/
    *server_information->active = 1;
    int *active = server_information->active;
    unsigned int value = 1;
    sem_init(semaphore, 0, value);

    pthread_create(&server_information->thread,
     NULL, terminal, &server_address ); //(void *) &(server_address.sin_addr));

    
    while(*active){;
        int addrlen = sizeof(server_address);
        int new_socket;
        if((new_socket = accept(server_FD,
            (struct sockaddr *) &server_address, 
            (socklen_t *) &addrlen)) < 0){
            logger->entry("CONN","a connection was refused",
            time(NULL), logger);
        }
        /* */
        struct Session *newsession = calloc(1, sizeof(struct Session));
        struct sockaddr_in* ipv4 = (struct sockaddr_in*)&server_address;
        server_information->connected_clients++;
        newsession->active = calloc(1, sizeof(int));
        newsession->ip_num = ipv4->sin_addr;
        newsession->socket_num = new_socket;
        addsession(newsession, all_sessions);
        pthread_create(&unset_sessions[connection_number].thread, NULL,
         connection_handler, (void *) cpysession(newsession));         
    }
    return 0;
}

void *terminal(void *arg){
    struct sockaddr_in s_addr = *(struct sockaddr_in *) arg;
    printf("Server running at: %s:%d\n", inet_ntoa(s_addr.sin_addr), port);
    int ch;
    do{
        printf("%s\n","**********Options**********");
        printf("%s\n","press \'s\' to show server status");
        printf("%s\n","press \'l\' to show log status");
        printf("%s\n","press \'x\' to quit");
        if (ch == 's'){
            printf("\n\n\n%s\n", "");
            printserverstatus(server_information);
            printf("%s\n","\t\t--END OF COMMAND--");
        }else if(ch == 'l'){
            printf("\n\n\n%s\n", "");
            logger->savetofile = printlog;
            logger->savetofile(logger->first, logger);
            logger->savetofile = savelog;
            printf("%s\n","\t\t--END OF COMMAND--");
        }
        ch = 'b';
        ch = 'd';
    }while((ch = getchar()) != 'x');
    *server_information->active = 0;
    sem_destroy(semaphore);
    closeallconnections(all_sessions);
    logger->savetofile(logger->first, logger);
    close(server_FD);
    exit(0);
}

void *connection_handler(void *arg){
    struct Session *session = (struct Session *) arg;
    struct User *userptr = NULL;
    char msg[100], tag[30];
    sprintf(msg, "client connected at socket: %d",session->socket_num);
    logger->entry("CLIENT", msg, time(NULL), logger);
    *(session->active) = 1; session->tranferred = 0; 
    session->login_time = time(NULL); session->reqs = 0;
    char *request = calloc(1, BUFFER_SIZE);
    char *response = calloc(1, BUFFER_SIZE);
    int comparison;
    int media_size = 0;
    char *username = calloc(1, 30);
    strcpy(username, "Unknown");
    while(*(session->active)){
        int valread; bzero(tag,30); bzero(msg,100);
        if((valread = read(session->socket_num, request, BUFFER_SIZE)) < 0){
            printf("%s\n", "No bytes to read");
            logger->entry("SOCKET", "no bytes to read",time(NULL),logger);
            break;
        }
        if(!ishttp(request)){
            sprintf(response, HTTP_400);
            sprintf(msg, "Bad request from user:%s ip:%s", username, inet_ntoa(session->ip_num));
            logger->entry("400", msg, time(NULL), logger);
            send(session->socket_num, response, strlen(response),0);
            bzero(request, BUFFER_SIZE);
            bzero(response, BUFFER_SIZE);
            continue;
        }
        char *httpheader, *body, *path, *httpline, *conn;
        int len = gethttpheader(&httpheader, request);
        body = substring(request, len, strlen(request) - len + 2);
        gethttpline(&httpline, httpheader);
        path = getpath(httpline);
        conn = httpfield(httpheader, "Connection");
        sprintf(msg,"path:%s from user:%s, connection: %s", path, username, conn);
        if(isget(httpheader)  && userptr != NULL){ // && strcmp(username, "Unknown")
            sprintf(tag,"GET:%d",session->socket_num);
            logger->entry(tag, msg, time(NULL), logger);
            if(!strcmp(path, "/")){
                sprintf(response, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Headers: *\r\nAccess-Control-Expose-Headers: *\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n");
            }else if(!strcmp(path, "/res")){
                route_res(httpheader, body, &response);
            }else if(contains(path, "/res/")){
                media_size = route_resfile(request, body, path, &response, userptr);
                sprintf(msg, "transfered bytes: %d", media_size);
                logger->entry(tag, msg, time(NULL), logger);
            }else if(!strcmp(path, "/exit")){
                sprintf(response, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Headers: *\r\nAccess-Control-Expose-Headers: *\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: 0\r\nConnection: Closed\r\n\r\n");
            }else if(!strcmp(path, "/test")){
                media_size = route_test(httpheader, body, &response);
                sprintf(msg, "transfered bytes: %d", media_size);
                logger->entry(tag, msg, time(NULL), logger);
            }else{
                sprintf(msg, "transfered bytes: %d", media_size);
                logger->entry(tag, msg, time(NULL), logger);
                sprintf(response, HTTP_404);
            }
        }else if(ispost(httpheader)){
            sprintf(tag,"POST:%d",session->socket_num);
            logger->entry(tag, msg, time(NULL), logger);
            if(!strcmp(path, "/register")){
                route_register(httpheader, body, &username, &response, &userptr, session);
            }else if(!strcmp(path, "/login")){
                route_login(httpheader, body, &username, &response, &userptr, session);
            }else{
                sprintf(response, HTTP_404);
            }
        }else if(isoptions(httpheader)){
            sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nAccess-Control-Expose-Headers: *\r\nAccess-Control-Allow-Headers: *\r\nAccess-Control-Allow-Origin: *\r\nAllow: GET, POST, OPTIONS\r\n\r\n");
        }else {
            sprintf(response, HTTP_405);
        }
        if(media_size && (contains(path, "/res/") || !strcmp(path, "/test") )){
            send(session->socket_num, response, media_size,0);
        }else{
            send(session->socket_num, response, strlen(response),0);
        }
        if((!strcmp(path, "/exit") && isget(httpline)) ||  !strcmp(conn, "Close"))
            break;

        sem_wait(semaphore);
        server_information->received_bytes += strlen(request);
        server_information->sent_bytes += media_size;
        server_information->sent_bytes += strlen(response);
        ++server_information->requests;
        sem_post(semaphore);
        
        media_size = 0;
        bzero(request, BUFFER_SIZE);
    }
    bzero(msg,100);
    session->logout_time = time(NULL);
    /* Write to file of sessions */
    sprintf(msg, "client with socket: %d diconnected", session->socket_num);
    logger->entry("CLIENT", msg, time(NULL), logger);
    close(session->socket_num);

    sem_wait(semaphore);
    server_information->connected_clients--;
    if(userptr != NULL){
        deletesession(session->socket_num, userptr->ss);
        deletesession(session->socket_num, all_sessions);
        userptr->cant_session--;
        if(userptr->ss->first == NULL){
            deleteuser(username, users); 
        }
    }
    sem_post(semaphore);

    pthread_exit(0);
}

int login(char *username, char* password){
    FILE *username_file, *password_file;
    char *usrnm, *pswd;
    size_t len1 = 0;
    size_t len2 = 0;
    char u[50], p[50];
    if((username_file = fopen("./accounts/usernames.txt","r")) == NULL){
        logger->entry("ACCOUNT", "username file not found",time(NULL), logger);
        return 0;
    }
    if((password_file = fopen("./accounts/passwords.txt","r")) == NULL){
        logger->entry("ACCOUNT", "password file not found",time(NULL), logger);
        return 0;
    }
    strcpy(u,username);
    strcpy(p,password);
    strncat(u,"\n",1);
    strncat(p,"\n",1);
    while (getline(&usrnm, &len1, username_file) != EOF){
        getline(&pswd, &len2, password_file);
        if(strcmp(usrnm, u) == 0){
            char message[100];
            if(strcmp(pswd, p) == 0){
                fclose(username_file);
                fclose(password_file);
                sprintf(message,"username: %s logged in", username);
                logger->entry("LOGIN", message, time(NULL), logger);
                return 1;
            }else{
                sprintf(message,"incorrect password from account %s", username);
                logger->entry("LOGIN", message, time(NULL), logger);
                break;
            }
        }
    }
    fclose(username_file);
    fclose(password_file);
    char status[100];
    sprintf(status,"The account %s doesn't exist", username);
    logger->entry("LOGIN", status, time(NULL), logger);
    return 0;
}

int register_user(char *username, char *password){
    FILE *username_file, *password_file;
    char *usrnm, *pswd;
    size_t len1 = 0;
    size_t len2 = 0;
    char u[50], p[50];
    if((username_file = fopen("./accounts/usernames.txt","a+")) == NULL){
        logger->entry("ACCOUNT", "username file not found",time(NULL), logger);
        return 0;
    }
    if((password_file = fopen("./accounts/passwords.txt","a+")) == NULL){
        logger->entry("ACCOUNT", "password file not found",time(NULL), logger);
        return 0;
    }
    strcpy(u,username);
    strcpy(p,password);
    strncat(u,"\n",1);
    strncat(p,"\n",1);
    while (getline(&usrnm, &len1, username_file) != EOF){
        if(strcmp(usrnm, u) == 0){
            char status[100];
            sprintf(status,"username %s already exists", username);
            logger->entry("SIGNUP", status, time(NULL), logger);
            fclose(username_file);
            fclose(password_file);
            return 0;
        }
    }
    fprintf (username_file, "%s",u);
    fprintf (password_file, "%s",p);
    char status[100];
    sprintf(status,"user %s registered successfully", username);
    logger->entry("SIGNUP", status, time(NULL), logger);
    fclose(username_file);
    fclose(password_file);
    return 1;
}

void start_connection(int *fd,int port){
    if((*fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "Could not create the socket\n");
        exit(EXIT_FAILURE);
    }
    /*Binding socket*/
    memset((char *) &server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);
    if (bind(*fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0) {
        fprintf(stderr, "Could not bind the socket\n");
        exit(EXIT_FAILURE);
    }
    if (listen(*fd, BACKLOG) < 0){
        fprintf(stderr, "Listen Error\n");
        exit(EXIT_FAILURE);
    }
    /*Log: [Socket]: Server initialize*/
}

int scan(const char *dir, struct Folder *folder){ 
    struct dirent *dir_entry;
    DIR *directory;
    directory = opendir(dir);
    int cant = 0;
    if(directory == NULL){
        printf("%s %s\n","Failed to read directory:",dir);
        return 0;
    }
    while ((dir_entry = readdir(directory)) != NULL){
        if(dir_entry->d_type == DT_REG){
            cant++;
            int dotposition = namedot(dir_entry->d_name);
            char *name = substring(dir_entry->d_name, 0, dotposition);
            char *filetype = substring(dir_entry->d_name, dotposition +1, strlen(dir_entry->d_name) - dotposition);
            char *contype;
            char dir_1[256];
            contype = filetypetocontype(filetype);
            strcpy(dir_1, dir);
            strncat(dir_1, "/", 1);
            strncat(dir_1, dir_entry->d_name, strlen(dir_entry->d_name));
            /* TODO: MD5*/
            char *content;
            long contlen = readfilebytes(dir_1, &content);
            unsigned char *md5 = md5sum(content, contlen);
            md5 = tohexstr((unsigned char *) md5);
            folder->add(name,contype,dir_1,md5,folder);
        } 
    }
    closedir(directory);
    char msg[100];
    sprintf(msg, "there are %d multimedia files in the server", cant);
    logger->entry("MEDIA", msg, time(NULL), logger);
    return cant;
}

void closeallconnections(struct Sessions *pool){
    struct Session *tmp = pool->first;
    while (tmp != NULL){
        if(*tmp->active == 1){
            close(tmp->socket_num);
            char tag[30]; sprintf(tag, "CONN:%d", tmp->socket_num);
            char msg[100]; sprintf(msg, "Connection at socket %d closed by the server", tmp->socket_num);
            logger->entry(tag, msg, time(NULL), logger);
        }
        tmp = tmp->next;
    }
}

void route_login(char *httpheader, char *body, char **username, char **response, struct User **userptr, struct Session *session){
    char const *usr;
    char const *psswd;
    json_t pool[4];     /* Implementing tiny-json library */
    json_t const *parent = json_create(body, pool, 4); /* Implementing tiny-json library */
    if(parent == NULL){
        logger->entry("LOGIN", "the body is not json format", time(NULL), logger);
        sprintf(*response, HTTP_401);
    }else{
        json_t const *usr_field = json_getProperty(parent, "username");
        json_t const *psswd_field = json_getProperty(parent, "password");
        if(!usr_field || !psswd_field){
            logger->entry("LOGIN", "some element of json is missing", time(NULL), logger);
        }
        usr = json_getValue(usr_field);    
        psswd = json_getValue(psswd_field);
        /* CRITICAL SECTION */
        sem_wait(semaphore);
        if(login((char *) usr,(char *) psswd)){
        sem_post(semaphore);
        /* END OF CRITICAL SECTION */
            strcpy(*username, usr);
            if(isuserconnected((char *) usr, userptr ,users)){
                struct Session *prev = getlast((*userptr)->ss);
                if(prev->socket_num != session->socket_num){
                    *(prev->active) = 0;
                    addsession(session, (*userptr)->ss);
                    (*userptr)->cant_session++;
                    if((*userptr)->streamming){
                        sprintf(*response, "HTTP/1.1 200 OK\r\nAccess: 1\r\nContent-Length: 0\r\nStreaming: 1\r\nBytes-Pos: %ld\r\nRes: %s\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\nConnection: keep-alive\r\n\r\n", (*userptr)->currentByte, (*userptr)->multmdir );
                        return;
                    }
                }
            }else{
                newuser((char *) usr, session, userptr, users);
            }
            sprintf(*response, "HTTP/1.1 200 OK\r\nAccess: 1\r\nContent-Length: 0\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\nConnection: keep-alive\r\n\r\n");
            logger->entry("200","Login OK", time(NULL), logger);
        }else{
            sprintf(*response, HTTP_401);
            logger->entry("401","Login Unauthorized", time(NULL), logger);
        }
    }
}

void route_register(char *httpheader, char *body, char **username, char **response, struct User **userptr, struct Session *session){
    char const *usr;
    char const *psswd;
    json_t pool[4];
    json_t const *parent = json_create(body, pool, 4);
    if(parent == NULL){
        logger->entry("SIGNUP", "the body is not json format", time(NULL), logger);
        sprintf(*response, HTTP_401);
    }else{
        json_t const *usr_field = json_getProperty(parent, "username");
        json_t const *psswd_field = json_getProperty(parent, "password");
        if(!usr_field || !psswd_field){
            logger->entry("SIGNUP", "some element of json is missing", time(NULL), logger);
        }
        usr = json_getValue(usr_field);
        psswd = json_getValue(psswd_field);
        /* CRITICAL SECTION */
        sem_wait(semaphore);
        if(register_user((char *) usr,(char *) psswd)){
        sem_post(semaphore);
        /* END OF CRITICAL SECTION */
            strcpy(*username, usr);
            newuser((char *) usr, session, userptr, users);
            sprintf(*response, "HTTP/1.1 200 OK\r\nAccess: 1\r\nContent-Length: 0\r\nAccess-Control-Expose-Headers: *\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\nConnection: keep-alive\r\n\r\n");
            logger->entry("200","register OK", time(NULL), logger);
        }else{
            sprintf(*response, HTTP_401);
            logger->entry("401","register Unauthorized", time(NULL), logger);
        }
    }
}

void route_res(char *httpheader, char *body, char **response){ /* TODO ADD USERNAME */
    char *json = calloc(1,5000);
    int *elements = calloc(1, sizeof(int));
    char *client = httpfield(httpheader, "Client");
    if(client != NULL){
        json = folderclient(json, folder, elements);
    }else{
        json = folderjson(json, folder, elements);
    }
    sprintf(*response,"HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nElements: %d\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\nConnection: keep-alive\r\n\r\n", strlen(json), *elements);
    strncat(*response, json, strlen(json));
    logger->entry("200","res OK", time(NULL), logger);
    /* LOG [GET] [USER: KDJHFJ REQUESTED MEDIA FILES] */
}

int route_resfile(char *httpheader, char *body, char *path, char **response, struct User *user){ 
    int size =0, msg_size = 0;
    char *p_1 = getpath(httpheader);
    char *filename = substring(p_1, 1, strlen(p_1)-1), msg[100];
    sprintf(*response, "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nAccess-Control-Expose-Headers: *\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\nConnection: keep-alive\r\n\r\n");
    if(access(filename, F_OK) != -1){
        char *content, *httpresponse, *contype;
        contype = substring(filename, namedot(filename) + 1, strlen(filename) - namedot(filename));
        contype = filetypetocontype(contype);
        int contlen;
        char *message = calloc(1, 500);
        char *client = httpfield(httpheader,"Client");
        long flen = filelen(filename);
        if( streamable(client, contype) &&  httpfield(httpheader, "Byte-Pos") != NULL  && flen > CHUNK_SIZE){
            char *bytepos = httpfield(httpheader, "Byte-Pos");
            long initpos = atol(bytepos);
            long *full_len = malloc(sizeof(long));
            contlen = splitfilebytes(filename, &content, initpos, CHUNK_SIZE, full_len);
            unsigned char *md5 = md5sum(content, contlen);
            md5 = tohexstr((unsigned char *) md5);
            sprintf(message,"HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: %s\r\nChunk: \
            %d\r\nByte-Pos: %ld\r\nNext-Index: %ld\r\nMD5sum: %s\r\nTotal-Bytes: \
                %ld\r\nNext-Song: %s\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Expose-Headers: *\r\nAccess-Control-Allow-Headers: *\r\nConnection: keep-alive\r\n\r\n", 
                contlen, contype, contlen, initpos, initpos + contlen, md5, *full_len, filename);
            user->streamming = 1;
            if(initpos + contlen + 1 == *full_len){
                char *name = calloc(1,255);
                strcpy(name, nextstream(filename, client, folder));
                if(!strcmp(name, "")){
                    user->streamming = 0;
                }else{
                    char *nextsong = nextstream(filename, client, folder);
                    sprintf(message,"HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: %s\r\nChunk: \
                        %d\r\nByte-Pos: %ld\r\nNext-Index: %ld\r\nMD5sum: %s\r\nTotal-Bytes: \
                        %ld\r\nNext-Song: %s\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Expose-Headers: *\r\nAccess-Control-Allow-Headers: *\r\nConnection: keep-alive\r\n\r\n", 
                    contlen, contype, contlen, initpos, (long) 0, md5, *full_len, nextstream(filename, client, folder));
                }
                strcpy(user->multmdir, name);
                user->currentByte = 0;
            }else{
                user->currentByte = initpos + contlen;
                strcpy(user->multmdir, filename);
            }
        }else{
            strcpy(user->multmdir, "");
            user->streamming = 0;
            contlen = readfilebytes(filename, &content);
            char *md5 = mediamd5(filename, folder);
            sprintf(message, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nAccess-Control-Expose-Headers: *\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\nContent-Type: %s\r\nMD5sum: %s\r\nConnection: keep-alive\r\n\r\n", contlen, contype, md5);
        }
        msg_size = strlen(message);
        httpresponse = calloc(1, msg_size + contlen);
        strcpy(httpresponse, message);
        int fileindex = 0;
        for (int i = strlen(httpresponse); i <msg_size + contlen ; i++){
            httpresponse[i] = content[fileindex++];
        }
        *response = httpresponse;
        size = msg_size + contlen;
        sprintf(msg, "user:%s requested file %s, file found", user->username, filename);
        logger->entry("200",msg, time(NULL), logger);
    }else{
        sprintf(msg, "user:%s requested file %s, file not found", user->username, filename);
        sprintf(*response, HTTP_401);
        logger->entry("401",msg, time(NULL), logger);
    }
    return size;    
}

int route_test(char *httpheader, char *body, char **response){ /* TODO ADD USERNAME */
    char *content, *httpresponse, *contype;
    char *filename = "res/Loyal.mp3";
    long len = 0;
    int contlen = splitfilebytes(filename, &content, 9000000, 2097152, &len), size = 0;
    contype = substring(filename, namedot(filename) + 1, strlen(filename) - namedot(filename));
    contype = filetypetocontype(contype);
    char *message = calloc(1, 200);
    sprintf(message, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\nContent-Type: %s\r\nConnection: keep-alive\r\n\r\n", contlen, contype);
    size = strlen(message) + contlen;
    httpresponse = calloc(1, size);
    strcpy(httpresponse, message);
    int fileindex = 0;
    for (int i = strlen(httpresponse); i < size ; i++)
    {
        httpresponse[i] = content[fileindex++];
    }
    *response = httpresponse;
    return size;
}


