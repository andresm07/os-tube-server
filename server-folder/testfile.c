#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "utils.c"

int main(){
    char *get = "GET /res/Loyal.mp3 HTTP/1.1\r\nByte-Pos: 0\r\nContent-Type: application/json\r\nUser-Agent: PostmanRuntime/7.26.1\r\nAccept: */*\r\nHost: 0.0.0.0:8000\r\nAccept-Encoding: gzip, deflate, br\r\nConnection: keep-alive\r\nContent-Length: 47\r\n\r\n";
    char *post = "POST /login HTTP/1.1\r\nHost: 192.168.0.22:9000\r\n\r\nConnection: keep-alive\r\nContent-Length: 61431\r\nContent-Type: video/mp4 text/plain\r\n\r\n{body}";
    char *holder1;
    char *holder2;
    char *filetest = "name: help\r\nfiletype: text/plain\r\ndir: res/help.txt\r\nmd5: 970183ccb33b0e6b6282bbfd729e56cc\r\n\r\n";
    char *res_ex = "{\"data\":[{\"name\":\"kabre_cheatsheet\",\"filetype\":\"application/pdf\",\"dir\":\"res/kabre_cheatsheet.pdf\",\"md5\":\"61c290dca569fd1642f17254345928ea\"},{\"name\":\"homeless\",\"filetype\":\"video/mp4\",\"dir\":\"res/homeless.mp4\",\"md5\":\"3287fe75a6a2fcfc4040f498b4440898\"},{\"name\":\"Loyal\",\"filetype\":\"audio/mpeg\",\"dir\":\"res/Loyal.mp3\",\"md5\":\"d77d4f4e762d0583f101ed39d992098a\"},{\"name\":\"quarantine\",\"filetype\":\"image/jpg\",\"dir\":\"res/quarantine.jpg\",\"md5\":\"be6a8cb3c3fc0791f54c4021eb633cbf\"},{\"name\":\"help\",\"filetype\":\"text/plain\",\"dir\":\"res/help.txt\",\"md5\":\"970183ccb33b0e6b6282bbfd729e56cc\"},{\"name\":\"asi_fue\",\"filetype\":\"video/mp4\",\"dir\":\"res/asi_fue.mp4\",\"md5\":\"6b1a5191cb21b78dfa41dc0b109ffa80\"},{\"name\":\"whitesnake\",\"filetype\":\"image/jpg\",\"dir\":\"res/whitesnake.jpg\",\"md5\":\"3b9fff98d58513c8aa54f48b904b0e2d\"},{\"name\":\"Tarea1_rotacion\",\"filetype\":\"video/mp4\",\"dir\":\"res/Tarea1_rotacion.mp4\",\"md5\":\"edb4a3e4954c04b3916dc9dddf7acc03\"}]}";
    char *message = "Hola soy un archivo de texto\nEste es un cambio de linea jajaja";
    holder1 = substring(get, 4, strlen(get) - 4);
    holder2 = substring(post, 0, 4);
    char *h1 = calloc(1, 500);
    //printf(holder1);
    //printf(holder2);
    char *name, *filetype, *dir, *md5, *element;
    gethttpheader(&element, filetest);
    name = substring(filetest, strlen(element), strlen(filetest) - strlen(element));
    printf("Test %s %d\n", name, strlen(name));
    //jsontoFolder(res_ex, 65);

    /*
    unsigned char *header;
    unsigned char *md5result = (unsigned char *) md5sum(message, strlen(message));
    printf("result of text\n");

    for (int i = 0; i < MD5_DIGEST_LENGTH; i++){
        printf("%x", *(md5result + i) );
    }
    printf("%s\n", ".");
    char *hex = tohexstr(md5result);
    printf("%s\n", hex);
    unsigned long long int num = strtol(hex, NULL, 16);
    printf("%lld\n",num);
    printf("%llx\n",num);
    strcpy(h1, "");
    printf("%s\n", h1);*/
}