#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define BUFSZ 501

void usage(int argc, char **argv) {
    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
};

int main(int argc, char **argv) {
    if (argc < 3) usage(argc, argv);

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) usage(argc, argv);

    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) logexit("socket");

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != connect(s, addr, sizeof(storage))) logexit("connect");

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("connected to %s\n", addrstr);

    char filename[BUFSZ] = "";
    FILE *file = NULL;
    while (1) {
        printf("> ");
        char input[BUFSZ];
        fgets(input, BUFSZ - 1, stdin);
                
        if (0 == strncmp(input, "select file ", strlen("select file "))) {
            if (file != NULL) fclose(file);
            
            char buf[BUFSZ] = "";
            sscanf(input, "select file %[^\n]", buf);
            file = fopen(buf, "rb"); 

            char *extension = strrchr(buf, '.');
            if (extension == NULL || !extension_validator(extension)) {
                printf("%s not valid!\n", buf);
            } else if (file == NULL) {
                printf("%s does not exist\n", buf);
            } else {
                strcpy(filename, buf);
                file = fopen(filename, "rb"); 
                printf("%s selected\n", buf);
            };
        } else if (0 == strncmp(input, "send file", strlen("send file")) && strlen(input) - 1 == strlen("send file")) {
            file = fopen(filename, "rb");
            if (file == NULL) {
                printf("no file selected!\n");
                continue;
            };
            
            char* file_content = read_file(filename);
            if (strlen(file_content) >= 500) {
                ssize_t sent = send(s, "exit", strlen("exit"), 0);
                if (sent == -1) logexit("send");
                printf("file at limit size, please reduce it\n");
                break;
            };

            if (file_content != NULL) {
                char buf[BUFSZ * 2];
                sprintf(buf, "%s%s\\end", filename, file_content);
                ssize_t sent = send(s, buf, strlen(buf), 0);
                if (sent == -1) logexit("send");
                free(file_content);
            };
        } else if (0 == strncmp(input, "exit", strlen("exit")) && strlen(input) - 1 == strlen("exit")) {
            ssize_t sent = send(s, "exit", strlen("exit"), 0);
            if (sent == -1) logexit("send");
            break;
        } else {
            ssize_t sent = send(s, "unknown", strlen("unknown"), 0);
            if (sent == -1) logexit("send");
            break;
        };
    };
    
    if (file != NULL) fclose(file);
    close(s);
    exit(EXIT_SUCCESS);
}; 