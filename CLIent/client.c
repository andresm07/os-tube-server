#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h> 
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>
#include <mad.h>
#include <pulse/simple.h>
#include <pulse/error.h>

#include "utils.c"

#define TRUE 1
#define DEFAULT_PORT 7070
#define BUFFER_SIZE 5000000
#define DEFAULT_IP "172.17.107.181"

struct download_args
{
    FILE *file;
    int actual;
    int contlen;
};


int port, current_size, error, server_FD,  *playing, *enable, *printname, *changingsong ,streaming = 0;
char buffer[BUFFER_SIZE];
char *tmpnames[] = {"tmp/1.mp3", "tmp/2.mp3"};
char *cname;
char *current_dir, *curr_name;
pthread_t terminal_thread, songname;
struct sockaddr_in server_address;
char ip[16];
sem_t *semaphore;
FILE *fp;
pa_simple *device = NULL;
struct mad_stream mad_stream;
struct mad_frame mad_frame;
struct mad_synth mad_synth;
struct Folder* folder;
struct Logger *logger;


void connectsocket();
void nextname();
void busywaiting();
void *terminal(void *);
void *downloadchunk(void *);
int btoframes(int);
void *displaysongname(void *args);
void parseFolder(char* body);
long requestnextchunk(char *, long, char **, int *);
void nextsong(char *filename);
void output(struct mad_header const *header, struct mad_pcm *pcm);
void printFolder(int );
char* timestamp();

/*
    ./client [ip adress] [port]
*/

int main(int argc, char *argv[]){
    char *message = "Hello I'm a client trying to connect";
    char m[2000];
    playing = calloc(1, sizeof(int));
    printname = calloc(1, sizeof(int));
    enable = calloc(1, sizeof(int));
    changingsong = calloc(1, sizeof(int));
    current_dir = calloc(1, 256);
    *playing = 1;
    *enable = 1;
    *changingsong = 0;
    char receive[BUFFER_SIZE];
    cname = calloc(1, 256);
    sprintf(cname, "%s", tmpnames[0]);
    folder = calloc(1, sizeof(struct Folder));
    folder->add = addmultimedia;
    folder->first = NULL;
    int fileCounter = 0;
    logger = calloc(1, sizeof(struct Logger));
    logger->entry = newlog;
    logger->savetofile = savelog;
    semaphore = malloc(sizeof(sem_t));
    sem_init(semaphore, 0, 1);
    // Set up PulseAudio 16-bit 48.0kHz stereo output
    static const pa_sample_spec ss = { .format = PA_SAMPLE_S16LE, .rate = 48000, .channels = 2 };

    if(argc > 3){
        fprintf(stderr, "Too many arguments");
    }else if(argc == 3){
        /*Asign the port*/
        argv[1];
        strcpy(ip, argv[1]);
        port = atoi(argv[2]);
    }else{
        port = DEFAULT_PORT;
        strcpy(ip, DEFAULT_IP);
    }

    connectsocket();

    if (!(device = pa_simple_new(NULL, "MP3 player", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error))) {
        printf("can't stream audio\n");
    }

    
    mad_stream_init(&mad_stream);  /* FROM   */
    mad_synth_init(&mad_synth);   /* MAD    */
    mad_frame_init(&mad_frame);  /* LIBRARY*/

    


    char option[500];
    char fname[256];
    char *contype, *loginres;
    int print = 0, loginstream = 0;
    long loginbytepos;
    
    while (TRUE)
    {
        if(!loginstream){
            printf("login: Iniciar esion \nregister: Registrar nuevo usuario\nres: Solicitar lista de archivos \nfile: Solicitar archivo \nstreamfile: Solicitar archivo multimedia \nexit: Cerrar sesión \n\n");
            printf("Escriba una opcion: ");
            scanf("%s", (char *) &option);
        }else{
            sprintf(option, "%s", "other");
        }
        if(strcmp(option,"login") == 0) {
            char user[500];
            char password[500];
            printf("Enter username: ");
            scanf("%s", (char *) &user);
            printf("Enter password: ");
            scanf("%s", (char *) &password);
            sprintf(m, "%s{\"username\":\"%s\", \"password\": \"%s\"}", "POST /login HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 455\r\nContent-Type: application/json\r\n\r\n", user, password);
            char message[100];
            sprintf(message, "Client signed in. Username: %s", user);
            logger->entry("LOGIN", message, time(NULL), logger);
        } else if(strcmp(option, "register") == 0) {
            char newUser[500];
            char newPassword[500];
            printf("Enter username: ");
            scanf("%s", (char *) &newUser);
            printf("Enter password: ");
            scanf("%s", (char *) &newPassword);
            sprintf(m, "%s{\"username\":\"%s\", \"password\": \"%s\"}", "POST /register HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 455\r\nContent-Type: application/json\r\n\r\n", newUser, newPassword);
            char message[100];
            sprintf(message, "Client registered. Username: %s, Password: %s", newUser, newPassword);
            logger->entry("REGISTER", message, time(NULL), logger);
        } else if(strcmp(option, "res") == 0) {
            sprintf(m, "GET /res HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 455\r\nContent-Type: application/json\r\nClient: cli\r\n\r\n");
            char message[100];
            sprintf(message, "Requested available file list.");
            logger->entry("RES", message, time(NULL), logger);
        } else if(strcmp(option, "exit") == 0) {
            sprintf(m, "GET /exit HTTP/1.1\r\nConnection: Close\r\nContent-Length: 455\r\nContent-Type: application/json\r\n\r\n");
            char message[100];
            sprintf(message, "Client signed out. Connexion Terminated.");
            logger->entry("EXIT", message, time(NULL), logger);
            logger->savetofile(logger->first, logger);
        } else if(strcmp(option, "file") == 0) {
            char filename[500];
            int filePos;
            if(folder->first == NULL) {
                printf("Execute res \n");
                sprintf(m, "GET /res HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 455\r\nContent-Type: application/json\r\nClient: cli\r\n\r\n");
            } else {
                print = 1;
                printFolder(0);
                printf("Enter file number: ");
                scanf("%d", &filePos);
                struct Multimedia* temp = folder->first;
                for(int i = 1; i <= filePos; i++) {
                    if(i == filePos) {
                        strcpy(filename, temp->dir);
                        strcpy(fname, temp->name);
                        break;
                    }
                    temp = temp->next;
                }
                contype = substring(filename, namedot(filename) + 1, strlen(filename) - namedot(filename));
                sprintf(m, "GET /%s HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\nClient: cli\r\n\r\n", filename);
                char message[100];
                sprintf(message, "Client requested file: %s", filename);
                logger->entry("GET: FILE", message, time(NULL), logger);
            }
        } else if(strcmp(option, "streamfile") == 0) {
            char filename[500];
            int filePos;
            if(folder->first == NULL) {
                printf("Execute res \n");
                sprintf(m, "GET /res HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 455\r\nContent-Type: application/json\r\nClient: cli\r\n\r\n");
            } else {
                print = 1;
                printFolder(1);
                printf("Enter file number: ");
                scanf("%d", &filePos);
                struct Multimedia* temp = folder->first;
                for(int i = 1; i <= filePos; i++) {
                    if(i == filePos) {
                        strcpy(filename, temp->dir);
                        strcpy(current_dir, temp->dir);
                        break;
                    }
                    temp = temp->next;
                }
                contype = substring(filename, namedot(filename) + 1, strlen(filename) - namedot(filename));
                sprintf(m, "GET /%s HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\nClient: cli\r\nByte-Pos: 0\r\n\r\n", current_dir);
                char message[100];
                sprintf(message, "Client requested streamable file: %s", filename);
                logger->entry("GET: STREAMFILE", message, time(NULL), logger);
            }
        }else if(loginstream){
            loginstream = 0;
            sprintf(m, "GET /%s HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\nClient: cli\r\nByte-Pos: %ld\r\n\r\n", loginres, loginbytepos);
            sprintf(option, "%s", "streamfile");
        }else{
            printf("%s","Not a valid method\n");
            char message[100];
            sprintf(message, "Unknown Command");
            logger->entry("ERR", message, time(NULL), logger);
            continue;
        }
        if (write(server_FD, m, strlen(m)) < 0)
            break;
        int received_b = read(server_FD, receive, BUFFER_SIZE);
        char *httpheader, *body; 
        int len = gethttpheader(&httpheader, receive);
        body = substring(receive, len, received_b - len + 2);
        char* statusCode = substring(httpheader, 9, 3);
        if(strcmp(option, "login") == 0) { /* \r\nBytes-Pos: %ld\r\nRes: %s*/
            if(strcmp(statusCode, "200") == 0) {
                printf("Login Successful\n");
                char *bpos, *res;
                bpos = httpfield(httpheader, "Bytes-Pos");
                res = httpfield(httpheader, "Res");
                if(res != NULL){
                    loginbytepos = atol(bpos);
                    loginres = res;
                    loginstream = 1;
                }
            } else {
                printf("ERROR: Unable to Login\n");
            }
        } else if(strcmp(option, "register") == 0) {
            if(strcmp(statusCode, "200") == 0) {
                printf("Registration Successful\n");
            } else {
                printf("ERROR: Unable to Register\n");
            }
        } else if(strcmp(option, "res") == 0) {
            char *contlen = httpfield(httpheader, "Content-Length"), *bod1 = calloc(1, 8000);
            int contentlen = atoi(contlen);
            sprintf(bod1, "%s", body);
            printf(" %ld < %d\n", strlen(bod1), contentlen);
            while(strlen(bod1) < contentlen){
                printf(" %ld < %d\n", strlen(bod1), contentlen);
                read(server_FD, buffer, BUFFER_SIZE);
                strncat(bod1, buffer, strlen(buffer));
            }
            //rawprintf(bod1);
            if(folder->first == NULL)
                parseFolder(bod1);
            printFolder(0);
        } else if(strcmp(option, "exit") == 0) {
            if(strcmp(statusCode, "200") == 0) {
                printf("Closing Socket\n");
                break;
            }
        } else if(strcmp(option, "file") == 0) {
            char *otracosa = httpfield(httpheader, "Content-Length"), *buff = malloc(BUFFER_SIZE);
            FILE* newfile;
            if(otracosa != NULL && print == 1) {
                char name[100];
                fileCounter++;
                sprintf(name, "%s.%s", fname, contype);
                int l = atoi(otracosa), actual = received_b - len, r;
                newfile = fopen(name, "wb");
                fwrite(body, actual, 1, newfile);
                printf("Starting Download ..");
                while (actual < l){
                    printf("Downloading %.1f \n", ((double) actual )/((double) l)*100 );
                    r = read(server_FD,buff, BUFFER_SIZE);
                    fwrite(buff, r, 1, newfile);
                    actual += r;
                    printf("%s", "\r\e[1A\t\t\t\t\t\t\t\t\t\t\n\e[1A");
                }
                fclose(newfile);
                printf("%s.%s Downloaded Successfully\n\n", fname, contype);
                char *fileMD5 = httpfield(httpheader, "MD5sum");
                int contlen;
                char *content;
                contlen = readfilebytes(name, &content);
                unsigned char *md5 = md5sum(content, contlen);
                md5 = tohexstr((unsigned char *) md5);
                if(strcmp(fileMD5, md5) == 0) {
                    char message[100];
                    sprintf(message, "Sent File: %s, Received File: %s", fileMD5, md5);
                    logger->entry("MD5SUM CHECK", message, time(NULL), logger);
                } else {
                    char message[100];
                    sprintf(message, "Sent File: %s, Received File: %s", fileMD5, md5);
                    logger->entry("MD5SUM ERR", message, time(NULL), logger);
                }
                //char *newFileMD5 = md5sum((unsigned char *)(body), l);
                //newFileMD5 = tohexstr((unsigned char *)(newFileMD5));
            }
        } else if(strcmp(option, "streamfile") == 0) {
            if(strcmp(statusCode, "200") == 0) {
                char *next_i = httpfield(httpheader, "Next-Index");
                char *conl = httpfield(httpheader, "Content-Length");
                char *song = httpfield(httpheader, "Next-Song");
                long nxt = atol(next_i);
                long contlen = atol(conl);
                int actual_clen = received_b - strlen(httpheader);
                char file[contlen];
                strcpy(file, body);
                int rec;
                FILE *aux = fopen(cname, "wb");
                fwrite(body, received_b - strlen(httpheader), 1, aux);
                while (actual_clen < contlen){
                    rec = read(server_FD, receive, BUFFER_SIZE);
                    fwrite(receive, rec, 1, aux);
                    actual_clen += rec;
                };
                long n_next;
                fclose(aux);
                nextsong(cname);
                pthread_create(&terminal_thread, NULL, terminal, NULL);
                pthread_create(&songname, NULL, displaysongname, NULL);
                *printname = 1;
                *enable = 1;
                int n = 0;
                int *half_frame = calloc(1, sizeof(int));
                *half_frame = btoframes(current_size);
                *half_frame = *half_frame/2;

                // Decode frame and synthesize loop
                while (*enable) {

                    // Decode frame from the stream
                    if(*playing){
                        if(n == *half_frame){
                            nextname();
                            nxt = requestnextchunk(song, nxt, &song, half_frame);
                        }
                        if (mad_frame_decode(&mad_frame, &mad_stream)) {
                            if (MAD_RECOVERABLE(mad_stream.error)) {
                                continue;
                            } else if (mad_stream.error == MAD_ERROR_BUFLEN) {
                                nextsong(cname);
                                n = 0;
                                continue;
                            } else {
                                break;
                            }
                        }
                        n++;
                        // Synthesize PCM data of frame
                        mad_synth_frame(&mad_synth, &mad_frame);
                        output(&mad_frame.header, &mad_synth.pcm);
                    }
                }
                // Close
                fclose(fp);

            } else {
                printf("ERROR: File not found or Unauthorized\n");
            }
        }
        //printf("%s",receive);
        bzero(m,700);
        bzero(buffer, BUFFER_SIZE);
        bzero(option, 500);
        bzero(receive, BUFFER_SIZE);
    }
    
    //send(server_FD, message, strlen(message), 0);
    close(server_FD); 

    // Free MAD structs
    mad_synth_finish(&mad_synth);
    mad_frame_finish(&mad_frame);
    mad_stream_finish(&mad_stream);

    // Close PulseAudio output
    if (device)
        pa_simple_free(device);  

    return 0;
    
}

long requestnextchunk(char *dir, long pos, char **nextsong, int *half_frame){
    char *m_1, *receive;
    m_1 = calloc(1, BUFFER_SIZE);
    receive = calloc(1, BUFFER_SIZE);
    sprintf(m_1, "GET /%s HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\nByte-Pos: %ld\r\nClient: cli\r\n", *nextsong, pos);
    if (write(server_FD, m_1, strlen(m_1)) < 0){
        *enable = 0;
        return 0;
    }
    int received_b = read(server_FD, receive, 500);
    char *httpheader, *body;
    int len = gethttpheader(&httpheader, receive);
    //printf("%s\n", httpheader);
    body = substring(receive, len, strlen(receive) - len + 2);
    char* statusCode = substring(httpheader, 9, 3);
    if(strcmp(statusCode, "200")){
        *enable = 0;
        return -1;
    }
    char *next_i = httpfield(httpheader, "Next-Index");
    char *conl = httpfield(httpheader, "Content-Length");
    char *nxtsong = httpfield(httpheader, "Next-Song");
    if(strcmp(nxtsong, *nextsong) && *changingsong == 0){
        *changingsong = 1;
        current_dir = nxtsong;
    }
    *nextsong = nxtsong;
    long nxt = atol(next_i);
    long contlen = atoi(conl);
    *half_frame = btoframes(contlen);
    *half_frame = *half_frame/2;
    int rec;
    int actual_clen = received_b - len;
    sem_wait(semaphore);
    FILE *a = fopen(cname, "wb");
    fwrite(body, actual_clen, 1, a);
    struct download_args *vars = calloc(1, sizeof(struct download_args));
    vars->actual = actual_clen;
    vars->contlen = contlen;
    vars->file = a;
    pthread_t thread;
    pthread_create(&thread, NULL, downloadchunk, (void *) vars);
    return nxt;
    //return 45;
}

int btoframes(int bytes){
    double val = 0.001037032;
    val *= bytes;
    return (int) val;
}

void nextsong(char *filename){
    sem_wait(semaphore);
    fp = fopen(filename, "r");
    int fd = fileno(fp);

    if(*changingsong == 2){
        *printname = 1;
        *changingsong = 0;
    }
    if(*changingsong == 1){
        *changingsong = 2;
    }
    
    /*  IMPLEMETATION OF mad.h library  */

    // Fetch file size, etc
    struct stat metadata;
    if (fstat(fd, &metadata) >= 0) {
        current_size = (int)metadata.st_size;
    } else {
        fclose(fp);
        return;
    }
    sem_post(semaphore);

    // Let kernel do all the dirty job of buffering etc, map file contents to memory
    char *input_stream = mmap(0, metadata.st_size, PROT_READ, MAP_SHARED, fd, 0);

    // Copy pointer and length to mad_stream struct
    mad_stream_buffer(&mad_stream, input_stream, metadata.st_size);
}

void nextname(){
    if(!strcmp(cname, tmpnames[0]))
        sprintf(cname, "%s", tmpnames[1]);
    else
        sprintf(cname, "%s", tmpnames[0]);
}

void *displaysongname(void *args){
    while (*enable)
    {
        if(*printname){
            printf("*******%s*******\n",current_dir);
            printf("%s\n","press \'p\' to play/pause");
            printf("%s\n","press \'x\' to quit\n\n");
            *printname = 0;
        }
    }
    pthread_exit(0);
}

void *terminal(void *args){
    int ch, first = 1;
    do{
        if(ch != 'p') continue;
        *playing = !*playing;
        ch = 'b';
        ch = 'd';
        printf("%s", "\e[1A           \n\e[1A");
    }while((ch = getchar()) != 'x');
    *enable = 0;
    pthread_exit(0);
}

void connectsocket(){
    if((server_FD = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("%s\n", "Socket Creation Error");
        exit(EXIT_FAILURE);
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    if(inet_pton(AF_INET, ip, &server_address.sin_addr)<=0) { 
        printf("%s\n","Invalid address or Address not supported \n"); 
        exit(EXIT_FAILURE);
    }

    if(connect(server_FD, (struct sockaddr *) &server_address, sizeof(server_address)) < 0){
        printf("%s\n","Connection failed");
        exit(EXIT_FAILURE);
    }
}

void parseFolder(char* body) {
    char *files = substring(body, 21, strlen(body) - 21);
    while(strlen(files) != 0) {
        char *name, *filetype, *dir, *md5, *element;
        gethttpheader(&element, files);
        name = httpfield(element, "name");
        filetype = httpfield(element, "filetype");
        dir = httpfield(element, "dir");
        md5 = httpfield(element, "md5");
        folder->add(name, filetype, dir, md5, folder);
        //rawprintf(files);
        //printf("\n\n");
        files = substring(files, strlen(element), strlen(files) - strlen(element));
    }
    //printf(folderclient(NULL, folder, 1));
}

void printFolder(int stream) {
    struct Multimedia* temp = folder->first;
    int cont = 1;
    while(temp != NULL) {
        if(stream){
            if(!strcmp(temp->filetype, "audio/mpeg"))
                printf("%d. File: %s Filetype: %s\n", cont, temp->name, temp->filetype);
        }else
            printf("%d. File: %s Filetype: %s\n", cont, temp->name, temp->filetype);
        cont++;
        temp = temp->next;
    }
    printf("\n");
}

void busywaiting(){
    for (int i = 0; i < 20000000; i++){
        
    }
}

/*  IMPLEMETATION OF mad.h library  */
int scale(mad_fixed_t sample) {
     /* round */
     sample += (1L << (MAD_F_FRACBITS - 16));
     /* clip */
     if (sample >= MAD_F_ONE)
         sample = MAD_F_ONE - 1;
     else if (sample < -MAD_F_ONE)
         sample = -MAD_F_ONE;
     /* quantize */
     return sample >> (MAD_F_FRACBITS + 1 - 16);
}

/*  IMPLEMETATION OF mad.h library  */
void output(struct mad_header const *header, struct mad_pcm *pcm) {
    register int nsamples = pcm->length;
    mad_fixed_t const *left_ch = pcm->samples[0], *right_ch = pcm->samples[1];
    static char stream[1152*4];
    if (pcm->channels == 2) {
        while (nsamples--) {
            signed int sample;
            sample = scale(*left_ch++);
            stream[(pcm->length-nsamples)*4 ] = ((sample >> 0) & 0xff);
            stream[(pcm->length-nsamples)*4 +1] = ((sample >> 8) & 0xff);
            sample = scale(*right_ch++);
            stream[(pcm->length-nsamples)*4+2 ] = ((sample >> 0) & 0xff);
            stream[(pcm->length-nsamples)*4 +3] = ((sample >> 8) & 0xff);
        }
        if (pa_simple_write(device, stream, (size_t)1152*4, &error) < 0) {
            fprintf(stderr, "pa_simple_write() failed: %s\n", pa_strerror(error));
            return;
        }
    } else {
        printf("Mono not supported!");
    }
}


void *downloadchunk(void *args){
    struct download_args variables = *(struct download_args *) args;
    char *receive = calloc(1, BUFFER_SIZE); 
    int actual = variables.actual, rec;
    while (actual < variables.contlen)
    {
        rec = read(server_FD, receive, BUFFER_SIZE);
        fwrite(receive, rec, 1, variables.file);
        actual += rec;
    }
    fclose(variables.file);
    sem_post(semaphore);
    pthread_exit(0);
}
