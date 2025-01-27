#define OPENSSL_API_COMPAT 0x10100000L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>
#include <dirent.h>
#include <openssl/md5.h>
#include "ssu_help.c"

#define OPENSSL_API_COMPAT 0x10100000L

#define NAMEMAX 255
#define PATHMAX 4096
#define STRMAX 4096
#define BUFFER_SIZE 4096
#define LINEMAX 512
#define DATAMAX 1024
#define DIGESTMAX 16

char exeNAME[PATHMAX];
char exePATH[PATHMAX];
char homePATH[PATHMAX];
char backupPATH[PATHMAX];
int hash;

void help();

char *commanddata[10] = {
	"backup",
	"remove",
	"recover",
	"list",
	"help",
	"exit"
};

typedef struct command_parameter {
	char *command;
	char *filename;
	char *tmpname;
	int commandopt;
	char *argv[10];
} command_parameter;

typedef struct Backup_linkedlist {
	char backup_fname[PATHMAX];
	char source_path[PATHMAX];
	char dest_path[PATHMAX];
	char backup_directory[PATHMAX];
	char hash_value[PATHMAX];
	struct Backup_linkedlist *next;
} Backup_linkedlist;

typedef struct Remove_file_count {
        char name[NAMEMAX];
        int file_count;
	char source_path[PATHMAX];
	char dest_path[PATHMAX];
	char backup_directory[PATHMAX];
        struct Remove_file_count *next;
} Remove_file_count;

typedef struct Recover_file {
	char name[NAMEMAX];
	char source_path[PATHMAX];
	char dest_path[PATHMAX];
	char backup_directory[PATHMAX];
	struct Recover_file *next;
} Recover_file;

struct Backup_linkedlist head[PATHMAX];
struct Backup_linkedlist tail[PATHMAX];
struct Remove_file_count remove_count_head[PATHMAX];
struct Remove_file_count remove_a_head[PATHMAX];
struct Recover_file recover_head[PATHMAX];

int starts_with_home(const char *path) {
    const char *home = "/home/";
    return strncmp(path, home, strlen(home)) == 0;
}

void get_hash_value(char *file_path, char *hash_value) {
	FILE *file = fopen(file_path, "rb");
	if(file == NULL) {
		fprintf(stderr, "File open error!\n");
		exit(1);
	}

	MD5_CTX context;
	MD5_Init(&context);

	size_t bytes;
	unsigned char data[DATAMAX];

	while((bytes = fread(data, 1, DATAMAX, file)) != 0) {
		MD5_Update(&context, data, bytes);
	}
	
	unsigned char digest[DIGESTMAX];
	MD5_Final(digest, &context);

    fclose(file);

	for(int i = 0; i < DIGESTMAX; ++i) {
        	sprintf(&hash_value[i * 2], "%02x", digest[i]);
    	}
}
