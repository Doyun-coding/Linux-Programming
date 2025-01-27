#include "ssu_header.h"

void backup_log(char *log_source, char *log_dest, char *time_string, char *backup_file) { // 로그 작성하는 함수
	char *log_path = "/home/backup/ssubak.log";
	
	int fd;
	if((fd = open(log_path, O_RDONLY | O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0) { // 로그 파일 열기
		fprintf(stderr, "open error for %s\n", log_path);
		return;
	}
	char log_message[BUFFER_SIZE];
	char check_message[BUFFER_SIZE];
	snprintf(log_message, BUFFER_SIZE, "%s : \"%s\" backuped to \"%s\"\n", time_string, log_source, log_dest); // 로그 파일에 적을 문장
	if(write(fd, log_message, strlen(log_message)) < 0) { // 로그 파일에 write
		fprintf(stderr, "write error\n");
		exit(1);
	}

	printf("\"%s\" backuped to \"%s\"\n", log_source, log_dest); // 출력
}

void backup(char *backup_path, char *dest_option) { // backup 함수
	time_t time_cur;
	struct tm * time_info;
	char time_string[15];
	char dest[PATHMAX];

	time(&time_cur); // time 함수를 사용하여 time_cur에 현재시각을 저장 
	time_info = localtime(&time_cur);
			
	strftime(time_string, sizeof(time_string), "%y%m%d%H%M%S", time_info); // "%y%m%d%H%M%S" 형태로 변환

	char current_dir[PATHMAX];

	if(getcwd(current_dir, sizeof(current_dir)) < 0) { // 현재위치
		fprintf(stderr, "current dir error\n");
		exit(1);
	}

	if(dest_option == NULL) {
		char backup_filepath[PATHMAX];
		snprintf(backup_filepath, PATHMAX, "%s",  current_dir);

		char backup_filename[PATHMAX];
		snprintf(backup_filename, PATHMAX, "%s", time_string);

		char *dest_path = "/home/backup/";
		snprintf(dest, PATHMAX, "%s%s", dest_path, time_string); 
	}
	else {
		snprintf(dest, PATHMAX, "%s", dest_option);
	}

	char cur[PATHMAX];
	
	char *real_path = realpath(backup_path, NULL); // backup_path 의 절대경로 얻는 함수 사용
	if(real_path == NULL) { // realpath 예외처리
		if (errno == ENOENT) {
            		fprintf(stderr, "Error: File or directory not found.\n");
        	} 
		else if (errno == EACCES) {
            		fprintf(stderr, "Error: Permission denied.\n");
        	} 
		else {
            		perror("realpath");
        	}
	}

	if(strlen(real_path) >= PATHMAX) {  // 예외 처리
		fprintf(stderr, "Too Long!\n");
		exit(1);
	}
	strcpy(cur, real_path);
	
	char *token;
	token = strtok((char *)cur, "/");

	while(token != NULL) { // 경로의 마지막 부분 가져오기
		char *next_token = strtok(NULL, "/");
		if(next_token != NULL) {
			token = next_token;
		}
		else {
			break;
		}
	}

	char *backup_file = token;
	
	bool check = false;
	Backup_linkedlist *tmp = (Backup_linkedlist *)malloc(sizeof(Backup_linkedlist));
	tmp = head->next;
	
	while(tmp != NULL) {
		if(!strcmp(tmp->backup_fname, backup_file)) {
			if(!strcmp(tmp->source_path, real_path)) { // 해쉬 값 비교 중복 체크
				char tmp_hash[DIGESTMAX];
				char new_hash[DIGESTMAX];
				
				get_hash_value(real_path, new_hash);
				get_hash_value(tmp->dest_path, tmp_hash);
				
				if(!strcmp(tmp_hash, new_hash)) {
					check = true;
					break;
				}		
			}		
		}
		tmp = tmp->next;
	}
	
	if(!check) { // 중복이 아닌 경우
		int source_fd;
		int dest_fd;
		ssize_t bytes_read, bytes_write;
		char buf[BUFFER_SIZE];
		struct stat file_stat;

		if(access(dest, F_OK) != 0) { // 백업할 곳에 디렉토리 없으면 생성
			if(mkdir(dest, 0777) < 0) {
				perror("mkdir\n");
				exit(1);
			}
		}

		strcat(dest, "/");
		strcat(dest, backup_file);

		if((source_fd = open(real_path, O_RDONLY)) < 0) { // 백업할 파일 오픈
			fprintf(stderr, "open error for %s\n", cur);
			exit(1);
		}
		
		if((dest_fd = open(dest, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR)) < 0) { // 백업 디렉토리에 파일 생성
			fprintf(stderr, "open error for %s\n", dest);
			exit(1);
		}
	
		while((bytes_read = read(source_fd, buf, BUFFER_SIZE)) > 0) { // 백업 파일에 백업 내용 작성
			bytes_write = write(dest_fd, buf, bytes_read);
			if(bytes_write != bytes_read) {
				fprintf(stderr, "write error\n");
				exit(1);
			}
		}
		backup_log(real_path, dest, time_string, backup_file); // 로그 작성 함수 호출
	}
	else { // 중복인 경우
		printf("\"%s\" : already backuped to \"%s\"\n", tmp->source_path, tmp->dest_path);
	}
	
	return;
}

void backup_opt_d(char *backup_path) { // backup option -d 함수
	char cur[PATHMAX];
	char *real_path = realpath(backup_path, NULL); // 경로의 절대 경로 얻는 함수
	struct dirent **file_list;
	int n;
	struct stat file_stat;
	if(real_path == NULL) { // 절대 경로 예외처리
		if (errno == ENOENT) {
			fprintf(stderr, "Error: File or directory not found.\n");
		} 
		else if (errno == EACCES) {
			fprintf(stderr, "Error: Permission denied.\n");
		} 
		else {
			perror("realpath");
		}
		exit(1);
	}
	if (strlen(real_path) >= PATH_MAX) { // 예외 처리
        fprintf(stderr, "Too Long.\n");
		exit(1);
	}
	strcpy(cur, real_path);

	if((n = scandir(real_path, &file_list, NULL, NULL)) < 0) { // 경로에 디렉토리, 파일의 개수
		perror("scandir\n");
		exit(1);
	}
	
	for(int i = 0; i < n; i++) {
		if(strcmp(file_list[i]->d_name, ".") != 0 && strcmp(file_list[i]->d_name, "..") != 0) {
			char check_dir[PATHMAX];
			snprintf(check_dir, PATHMAX, "%s/%s", real_path, file_list[i]->d_name);
			if(stat(check_dir, &file_stat) < 0) { // stat 함수 예외 처리
				fprintf(stderr, "stat error\n");
				exit(1);
			}

			if(S_ISDIR(file_stat.st_mode)) { // 디렉토리면 넘어감
				continue;
			}
			else { // 파일이면 backup 함수로 이동
				backup(check_dir, NULL);
			}
		}
	}
}

void backup_opt_r(char *backup_path, char *dest_path) { // backup option -r 함수
	time_t time_cur;
	struct tm * time_info;
	char time_string[15];
	char dest[PATHMAX];
	if(dest_path == NULL) { // dest_path가 NULL 인 경우 dest 경로 만들어주는 코드
		char *home_backup = "/home/backup/";

		time(&time_cur); 
		time_info = localtime(&time_cur);
		strftime(time_string, sizeof(time_string), "%y%m%d%H%M%S", time_info); 	char cur[PATHMAX];

		snprintf(dest, PATHMAX, "%s%s", home_backup, time_string); 
	}
	else {
		snprintf(dest, PATHMAX, "%s", dest_path);
	}
	char *real_path = realpath(backup_path, NULL); // 절대경로 얻는 코드
	struct dirent **file_list;
	int n;
	struct stat file_stat;
	if(real_path == NULL) { // 절대경로 예외처리
		if (errno == ENOENT) {
        		fprintf(stderr, "Error: File or directory not found.\n");
        	} 
		else if (errno == EACCES) {
            		fprintf(stderr, "Error: Permission denied.\n");
        	} 
		else {
            		perror("realpath");
        	}
		exit(1);
	}
	if (strlen(real_path) >= PATH_MAX) { // 예외처리 
        	fprintf(stderr, "Too Long.\n");
        	exit(1);
    	}

	if(access(dest, F_OK) != 0) { 
		if(mkdir(dest, 0777) < 0) {
			fprintf(stderr, "mkdir error\n"); // 백업 경로에 디렉토리 없으면 생성, 예외처리
			exit(1);
		}
	}

	if((n = scandir(real_path, &file_list, NULL, NULL)) < 0) { // 해당 디렉토리에 파일, 디렉토리 개수
		perror("scandir\n");
		exit(1);
	}
	
	for(int i = 0; i < n; i++) {
		if(strcmp(file_list[i]->d_name, ".") != 0 && strcmp(file_list[i]->d_name, "..") != 0) {
			char check_dir[PATHMAX];
			snprintf(check_dir, PATHMAX, "%s/%s", real_path, file_list[i]->d_name);
			if(stat(check_dir, &file_stat) < 0) { // stat 함수 예외처리
				fprintf(stderr, "stat error\n");
				exit(1);
			}
			if(S_ISDIR(file_stat.st_mode)) { // 만약 디렉토리이면 다시 backup_opt_r 함수로 재귀
				strcat(dest, "/");
				strcat(dest, file_list[i]->d_name);

				if(access(dest, F_OK) != 0) {
					if(mkdir(dest, 0777) < 0) {
						perror("mkdir\n");
						//fprintf(stderr, "mkdir error\n");
						exit(1);
					}
				}
				backup_opt_r(check_dir, dest);
			}
			else {
				backup(check_dir, dest); // 파일이면 backup 함수로
			}
		}
	}
}

void backup_opt_y(char *backup_path, char *dest_option) { // backup option -y 인 경우 함수
	time_t time_cur;
	struct tm * time_info;
	char time_string[15];
	char dest[PATHMAX];

	time(&time_cur); // time 함수를 사용하여 time_cur에 현재시각을 저장 
	time_info = localtime(&time_cur);
			
	strftime(time_string, sizeof(time_string), "%y%m%d%H%M%S", time_info); // "%y%m%d%H%M%S" 형태로 변환

	char current_dir[PATHMAX];

	if(getcwd(current_dir, sizeof(current_dir)) < 0) { // 현재위치
		fprintf(stderr, "current dir error\n");
		exit(1);
	}

	if(dest_option == NULL) { // 백업 경로 설정
		char backup_filepath[PATHMAX];
		snprintf(backup_filepath, PATHMAX, "%s",  current_dir);

		char backup_filename[PATHMAX];
		snprintf(backup_filename, PATHMAX, "%s", time_string);

		char *dest_path = "/home/backup/";
		snprintf(dest, PATHMAX, "%s%s", dest_path, time_string);
	}
	else {
		snprintf(dest, PATHMAX, "%s", dest_option);
	}

	char cur[PATHMAX];
	
	char *real_path = realpath(backup_path, NULL); // 절대 경로 얻는 코드
	if(real_path == NULL) { // 절대 경로 예외처리
		if (errno == ENOENT) {
			fprintf(stderr, "Error: File or directory not found.\n");
        } 
		else if (errno == EACCES) {
			fprintf(stderr, "Error: Permission denied.\n");
        } 
		else {
			perror("realpath");
		}
	}

	if(strlen(real_path) >= PATHMAX) { // 예외처리
		fprintf(stderr, "Too Long!\n");
		exit(1);
	}
	strcpy(cur, real_path);
	
	char *token;
	token = strtok((char *)cur, "/");

	while(token != NULL) { // 백업할 경로의 목적지 파일 가져오는 코드
		char *next_token = strtok(NULL, "/");
		if(next_token != NULL) {
			token = next_token;
		}
		else {
			break;
		}
	}
	char *backup_file = token;
	if(access(dest, F_OK) != 0) { // 백업할 경로에 디렉토리 없으면 생성
		if(mkdir(dest, 0777) < 0) {
			perror("mkdir\n");
			exit(1);
		}
	}
	
	strcat(dest, "/");
	strcat(dest, backup_file);

	int source_fd;
	int dest_fd;
	ssize_t bytes_read, bytes_write;
	char buf[BUFFER_SIZE];
	struct stat file_stat;

	if((source_fd = open(real_path, O_RDONLY)) < 0) { // 백업할 파일 내용을 얻기 위해 오픈
		fprintf(stderr, "open error for %s\n", cur);
		exit(1);
	}
	
	if((dest_fd = open(dest, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR)) < 0) { // 백업 경로에 파일 생성
		perror("open");
		exit(1);
	}
	
	while((bytes_read = read(source_fd, buf, BUFFER_SIZE)) > 0) { // 백업 내용 가져오는 코드
		bytes_write = write(dest_fd, buf, bytes_read);
		if(bytes_write != bytes_read) { // 백업 내용 쓰는 코드
			fprintf(stderr, "write error\n");
			exit(1);
		}
	}
	backup_log(real_path, dest, time_string, backup_file); // 로그 작성하는 함수
	return;
}

void remove_dir(char *now_path) { // remove 하고 비어있는 디렉토리 삭제하는 함수
	DIR *dir;
	char check_dir[PATHMAX];
	struct stat file_stat;
	int n;
	struct dirent **file_list;
	Backup_linkedlist *tmp_node = (Backup_linkedlist *)malloc(sizeof(Backup_linkedlist));

	if (access(now_path, F_OK) == -1) { // 접근 불가능하면 함수 종료
        	return;
	}

	if(stat(now_path, &file_stat) < 0) { // stat 함수 예외 처리
		fprintf(stderr, "stat error\n");
		exit(1);
	}

	if(S_ISDIR(file_stat.st_mode)) { // 디렉토리인지 확인
		if((n = scandir(now_path, &file_list, NULL, NULL)) < 0) { // 디렉토리 안에 파일, 디렉토리 개수, 예외처리
			perror("scandir\n");
			exit(1);
		}
		int file_count = 0;
		for(int i = 0; i < n; i++) {
			if(strcmp(file_list[i]->d_name, ".") != 0 && strcmp(file_list[i]->d_name, "..") != 0) {
				file_count++;
				char check_dir[PATHMAX];
				snprintf(check_dir, PATHMAX, "%s/%s", now_path, file_list[i]->d_name);
				if(stat(check_dir, &file_stat) < 0) { // stat 함수 예외 처리
					fprintf(stderr, "stat error\n");
					exit(1);
				}

				if(S_ISDIR(file_stat.st_mode)) { // 디렉토리가 있으면 다시 재귀
					remove_dir(check_dir);
				}
			}
		}
		if(file_count == 0) {
			if(rmdir(now_path) != 0) { // 디렉토리 삭제
				fprintf(stderr, "rmdir error!\n");
				exit(1);
			}
		}
	}
}

void remove_log(char *dest_path, char *source_path, char *backup_directory) { // 로그 작성하는 함수
	char *log_path = "/home/backup/ssubak.log";

	int fd;
	if((fd = open(log_path, O_RDONLY | O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0) { // 로그 파일 오픈
		fprintf(stderr, "open error for %s\n", log_path);
		return;
	}
	char log_message[BUFFER_SIZE];
	char check_message[BUFFER_SIZE];
	snprintf(log_message, BUFFER_SIZE, "%s : \"%s\" removed by \"%s\"\n", backup_directory, dest_path, source_path); // 로그 파일에 적을 문장
	if(write(fd, log_message, strlen(log_message)) < 0) { // 로그 파일에 작성
		fprintf(stderr, "write error\n");
		exit(1);
	}
	// 삭제한 파일의 디렉토리가 삭제되었는지 확인하는 코드, 다 삭제되지 않았으면 삭제
	char check_directory[PATHMAX];
	char *home_backup = "/home/backup/";
	DIR *dir;
	snprintf(check_directory, PATHMAX, "%s%s", home_backup, backup_directory);
	
	if((dir = opendir(check_directory)) != NULL) { // 디렉토리 안에 디렉토리가 있는지 확인
		remove_dir(check_directory); // 있으면 재귀
	}
	
	Backup_linkedlist *node = (Backup_linkedlist *)malloc(sizeof(Backup_linkedlist)); 
	node = head->next;

	if(!strcmp(node->backup_directory, backup_directory) && !strcmp(node->dest_path, dest_path) && !strcmp(node->source_path, source_path)) {
		head->next = node->next;
	}
	
	else {
		while(node != NULL) { // 링크드 리스트에서 삭제한 노드 없애주는 코드
			if(!strcmp(node->next->backup_directory, backup_directory) && !strcmp(node->next->dest_path, dest_path) && !strcmp(node->next->source_path, source_path)) {
				node->next = node->next->next;
				break;
			}
			node = node->next;
			
			if(node == NULL) {
				printf("No backup file!\n");
				exit(0);
			}
			
		}
	}
	printf("\"%s\" removed by \"%s\"\n", dest_path, source_path); // remove 내용 출력
}

void remove_backup_file(char *remove_path) { // backup 파일 remove 하는 함수
	char cur[PATHMAX];
	
	char *real_path = realpath(remove_path, NULL); // 결대 경로 얻는 함수
	if(real_path == NULL) { // 결대 경로 예외 처리
		if (errno == ENOENT) {
			fprintf(stderr, "Error: File or directory not found.\n");
        	} 
		else if (errno == EACCES) {
			fprintf(stderr, "Error: Permission denied.\n");
        	} 
		else {
			perror("realpath");
		}
	}

	if(strlen(real_path) >= PATHMAX) { // 예외 처리
		fprintf(stderr, "Too Long!\n");
		exit(1);
	}
	strcpy(cur, real_path);

	struct stat file_stat;
	
	int cnt = 0;

	Backup_linkedlist *remove_head = (Backup_linkedlist *)malloc(sizeof(Backup_linkedlist));
	Backup_linkedlist *tmp_node = head->next;
	while(tmp_node != NULL) {
		if(!strcmp(tmp_node->source_path, cur)) { // 백업 파일을 삭제할 원본 파일 경로와 같은 경로인 파일을 찾는 코드
			cnt++;
			Backup_linkedlist *remove_tmp = (Backup_linkedlist *)malloc(sizeof(Backup_linkedlist));
			Backup_linkedlist *remove_node = (Backup_linkedlist *)malloc(sizeof(Backup_linkedlist));
			strcpy(remove_node->backup_directory, tmp_node->backup_directory);
			strcpy(remove_node->backup_fname, tmp_node->backup_fname);
			strcpy(remove_node->dest_path, tmp_node->dest_path);
			strcpy(remove_node->source_path, tmp_node->source_path);
			
			if(remove_head->next == NULL) {
				remove_head->next = remove_node;
				remove_node->next = NULL;
			}
			else {	
				remove_tmp = remove_head->next;
				long remove_directory = strtol(remove_node->backup_directory, NULL, 10);
				long remove_first_directory = strtol(remove_tmp->backup_directory, NULL, 10);
				if(remove_first_directory >= remove_directory) {
					remove_node->next = remove_tmp;
					remove_head->next = remove_node;
					tmp_node = tmp_node->next;
					continue;
				}
				while(remove_tmp != NULL) {

					if(remove_tmp->next == NULL) {
						remove_tmp->next = remove_node;
						remove_node->next = NULL;
						break;
					}
					long tmp_directory = strtol(remove_tmp->next->backup_directory, NULL, 10);
					if(tmp_directory >= remove_directory) {
						remove_node->next = remove_tmp->next;
						remove_tmp->next = remove_node;
						break;
					}
					remove_tmp = remove_tmp->next;				}
			}	
		}
		tmp_node = tmp_node->next;
	}
	char backup_directory_check[BUFFER_SIZE];
	char *backup_path = "/home/backup/";
	struct dirent *entry;
	int file_count = 0;
	DIR *dir;
	Backup_linkedlist *remove_tmp = (Backup_linkedlist *)malloc(sizeof(Backup_linkedlist));
	remove_tmp = remove_head->next;
	if(cnt <= 0) { // 백업 파일이 없음
		//printf("No backup file\n");
		//exit(0);
	}
	else if(cnt == 1) { // 백업 파일이 단 한 개 존재하는 경우
		if(remove(remove_tmp->dest_path) == 0) { // 파일 삭제
			snprintf(backup_directory_check, BUFFER_SIZE, "%s%s", backup_path, remove_tmp->backup_directory);		
			if((dir = opendir(backup_directory_check)) != NULL) {
				while((entry = readdir(dir)) != NULL) {
					if (strcmp(entry->d_name, ".")  && strcmp(entry->d_name, "..")) {
                				file_count++;
            				}
				}
			}
			if(file_count == 0) {
				if(rmdir(backup_directory_check) != 0) { // 디렉토리가 비었으면 삭제
					fprintf(stderr, "rmdir error!\n");
					exit(1);
				}
			}
			remove_log(remove_tmp->dest_path, remove_tmp->source_path, remove_tmp->backup_directory); // remove 로그 파일에 작성
		}
		else {	
			fprintf(stderr, "remove error!\n"); // remove 함수 예외처리
			exit(1);
		}
	}
	else if(cnt > 1) { // 백업 파일이 2개 이상인 경우
		cnt = 0;
		printf("backup files of %s\n", cur);
		printf("%d. exit\n", cnt);

		while(remove_tmp != NULL) {
			cnt++;
			if(stat(remove_tmp->source_path, &file_stat) < 0) {
				fprintf(stderr, "stat error!\n");
				exit(1);
			}	
			printf("%d. %s        %ldbytes\n", cnt, remove_tmp->backup_directory, file_stat.st_size); // 예시처럼 출력
			remove_tmp = remove_tmp->next;		
		}

		int number = 0;
		printf(">> ");
		scanf("%d", &number);
		
		if(number <= 0 || number > cnt) exit(0); // 0을 입력받으면 종료
		else {
			int c = 0;
			remove_tmp = remove_head->next;
			while(number != c) { // 입력받은 숫자만큼 반복
				c++;
				if(number == c) {
					if(remove(remove_tmp->dest_path) == 0) { // 파일 삭제
						snprintf(backup_directory_check, BUFFER_SIZE, "%s%s", backup_path, remove_tmp->backup_directory);
						if((dir = opendir(backup_directory_check)) != NULL) { // 디렉토리 열어서 비었는지 확인
							while((entry = readdir(dir)) != NULL) { 
								if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                					file_count++;
            					}
							}
						}

						if(file_count == 0) {
							if(rmdir(backup_directory_check) != 0) { // 디렉토리 비었으면 삭제
								fprintf(stderr, "rmdir error!\n");
								exit(1);
							}
						}
						remove_log(remove_tmp->dest_path, remove_tmp->source_path, remove_tmp->backup_directory); // 로그 파일에 작성
					}
					else {
						fprintf(stderr, "remove error!\n"); // remove 예외 처리
						exit(1);
					}
				}

				if(remove_tmp == NULL && number != c) { // 예외인 경우 예외 처리
					printf("No backup file!\n");
					exit(0);
				}
				remove_tmp = remove_tmp->next;
			}
		}	
	}
}

void remove_opt_d(char *remove_path) { // remove option -d인 경우 함수
	struct stat file_stat;
	int n;
	struct dirent **file_list;
	char *real_path = realpath(remove_path, NULL); // 절대 경로 얻는 함수
	char cur[PATHMAX];
	
	if(real_path == NULL) { // 결대 경로 예외 처리
		if (errno == ENOENT) {
            		fprintf(stderr, "Error: File or directory not found.\n");
        	} 
		else if (errno == EACCES) {
            		fprintf(stderr, "Error: Permission denied.\n");
        	} 
		else {
            		perror("realpath");
        	}
	}

	if(strlen(real_path) >= PATHMAX) { // 예외처리
		fprintf(stderr, "Too Long!\n");
		exit(1);
	}
	
	if(stat(real_path, &file_stat) < 0) { // stat 함수 예외 처리
		fprintf(stderr, "stat error\n");
		exit(1);
	}

	if(S_ISDIR(file_stat.st_mode)) { // 디렉토리인 경우
		if((n = scandir(remove_path, &file_list, NULL, NULL)) < 0) { // 디렉토리 안 디렉토리, 파일 개수 얻는 코드
			perror("scandir\n");
			exit(1);
		}
		for(int i = 0; i < n; i++) {
			if(strcmp(file_list[i]->d_name, ".") != 0 && strcmp(file_list[i]->d_name, "..") != 0) {
				char check_dir[PATHMAX];
				snprintf(check_dir, PATHMAX, "%s/%s", real_path, file_list[i]->d_name); // 디렉토리 안에 있는 경로 획득
				if(stat(check_dir, &file_stat) < 0) { // stat 함수 예외 처리
					fprintf(stderr, "stat error\n");
					exit(1);
				}
				
				if(!S_ISDIR(file_stat.st_mode)) { // 디렉토리 안에 있는 것이 파일인 경우, remove 링크드 리스트로 관리
					Remove_file_count *node_count = (Remove_file_count *)malloc(sizeof(Remove_file_count));
                    Remove_file_count *tmp_count = (Remove_file_count *)malloc(sizeof(Remove_file_count));
                    tmp_count = remove_count_head->next;

					strcpy(node_count->name, file_list[i]->d_name);
					strcpy(node_count->source_path, check_dir);

					if(remove_count_head->next == NULL) {
						remove_count_head->next = node_count;
						node_count->next = NULL;
						node_count->file_count = 1;
						continue;
					}
					
					while(tmp_count != NULL) {
						if(tmp_count->next == NULL) {
							tmp_count->next = node_count;
							node_count->file_count = 1;
							node_count->next = NULL;
							break;
						}
						tmp_count = tmp_count->next;
					}
					tmp_count->next = node_count;
					node_count->next = NULL;
				}
			}
		}

		Remove_file_count *tmp_count = (Remove_file_count *)malloc(sizeof(Remove_file_count));
        tmp_count = remove_count_head->next;

		while(tmp_count != NULL) { 
			remove_backup_file(tmp_count->source_path); // 백업할 파일 remove backup file 함수를 통해 백업
			tmp_count = tmp_count->next;	
		}
	}
	else {
		remove_backup_file(real_path); // 디렉토리가 아닌 경우 파일은 그냥 함수를 통해 백업
		return;
	}
}

void remove_opt_r(char *remove_path) { // remove option -r 인 함수
	struct stat file_stat;
	int n;
	struct dirent **file_list;
	char *real_path = realpath(remove_path, NULL); // 절대 경로 얻는 함수
	char cur[PATHMAX];
	
	if(real_path == NULL) { // 절대 경로 예외 처리
		if (errno == ENOENT) {
            		fprintf(stderr, "Error: File or directory not found.\n");
        	} 
		else if (errno == EACCES) {
            		fprintf(stderr, "Error: Permission denied.\n");
        	} 
		else {
            		perror("realpath");
        	}
	}

	if(strlen(real_path) >= PATHMAX) { // 예외 처리
		fprintf(stderr, "Too Long!\n");
		exit(1);
	}
	
	if(stat(real_path, &file_stat) < 0) { // stat 함수 예외 처리
		fprintf(stderr, "stat error\n");
		exit(1);
	}

	if(S_ISDIR(file_stat.st_mode)) { // 디렉토리인 경우
		if((n = scandir(remove_path, &file_list, NULL, NULL)) < 0) { // 디렉토리 안에 디렉토리, 파일 개수, 예외 처리
			perror("scandir\n");
			exit(1);
		}
		for(int i = 0; i < n; i++) {
			if(strcmp(file_list[i]->d_name, ".") != 0 && strcmp(file_list[i]->d_name, "..") != 0) {
				char check_dir[PATHMAX];
				snprintf(check_dir, PATHMAX, "%s/%s", real_path, file_list[i]->d_name);
				if(stat(check_dir, &file_stat) < 0) { // stat 함수 예외 처리
					fprintf(stderr, "stat error\n");
					exit(1);
				}
				
				if(S_ISDIR(file_stat.st_mode)) { // 디렉토리 안에 있는 것이 디렉토리인 경우 재귀
					remove_opt_r(check_dir);
				}
				else { // 디렉토리 안에 있는 것이 파일인 경우 링크드 리스트로 관리, 저장
					Remove_file_count *node_count = (Remove_file_count *)malloc(sizeof(Remove_file_count));
                    Remove_file_count *tmp_count = (Remove_file_count *)malloc(sizeof(Remove_file_count));
                    tmp_count = remove_count_head->next;

					strcpy(node_count->name, file_list[i]->d_name);
					strcpy(node_count->source_path, check_dir);

					if(remove_count_head->next == NULL) {
						remove_count_head->next = node_count;
						node_count->next = NULL;
						node_count->file_count = 1;
						continue;
					}
					
					while(tmp_count != NULL) {
						if(tmp_count->next == NULL) {
							tmp_count->next = node_count;
							node_count->file_count = 1;
							node_count->next = NULL;
							break;
						}
						tmp_count = tmp_count->next;
					}
					tmp_count->next = node_count;
					node_count->next = NULL;
				}
			}
		}

		Remove_file_count *tmp_count = (Remove_file_count *)malloc(sizeof(Remove_file_count));
        tmp_count = remove_count_head->next;

		while(tmp_count != NULL) { // 링크드 리스트로 관리한 파일 remove backup file 함수로 처리
			remove_backup_file(tmp_count->source_path);
			tmp_count = tmp_count->next;	
		}

	}
	else { // 파일인 경우 그냥 remove backup file 함수로 처리
		remove_backup_file(real_path);
		return;
	}
}

void remove_opt_a(char *remove_path) { // remove option -a 인 경우 함수
	struct stat file_stat;
	int n;
	struct dirent **file_list;
	char *real_path = realpath(remove_path, NULL);
	char *log_path = "/home/backup/ssubak.log";
	int fd;
	char log_message[BUFFER_SIZE];
	char check_message[BUFFER_SIZE];
	char tmp_backup_directory[DATAMAX];
	char tmp_dest_path[DATAMAX];
	char tmp_source_path[DATAMAX];	

	if((fd = open(log_path, O_RDONLY | O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0) { // 로그 파일 오픈, 예외 처리
		fprintf(stderr, "open error for %s\n", log_path);
		return;
	}

	if(real_path == NULL) { // 절대 경로 예외 처리
		if (errno == ENOENT) {
			fprintf(stderr, "Error: File or directory not found.\n");
        	} 
		else if (errno == EACCES) {
			fprintf(stderr, "Error: Permission denied.\n");
        	} 
		else {
			perror("realpath");
		}
	}

	if(strlen(real_path) >= PATHMAX) { // 예외 처리
		fprintf(stderr, "Too Long!\n");
		exit(1);
	}
	
	if(stat(real_path, &file_stat) < 0) { // stat 함수 예외 처리
		printf("1\n");
		fprintf(stderr, "stat error\n");
		exit(1);
	}

	if(S_ISDIR(file_stat.st_mode)) { // 디렉토리인 경우
		if((n = scandir(real_path, &file_list, NULL, NULL)) < 0) { // 디렉토리 안에 디렉토리, 파일 개수, 예외 처리
			perror("scandir\n");
		}
		for(int i = 0; i < n; i++) {
			if(strcmp(file_list[i]->d_name, ".") != 0 && strcmp(file_list[i]->d_name, "..") != 0) {
				char check_dir[PATHMAX];
				snprintf(check_dir, PATHMAX, "%s/%s", real_path, file_list[i]->d_name); // 디렉토리 안에 있는 경로 획득
				if(stat(check_dir, &file_stat) < 0) { // stat 함수 예외 처리
					fprintf(stderr, "stat error\n");
					exit(1);
				}
				
				if(S_ISDIR(file_stat.st_mode)) { // 디렉토리이면 재귀
					remove_opt_a(check_dir);
				}
				else { // 파일인 경우 링크드 리스트로 관리, 저장
					Remove_file_count *remove_node = (Remove_file_count *)malloc(sizeof(Remove_file_count));
					strcpy(remove_node->name, file_list[i]->d_name);
					strcpy(remove_node->source_path, check_dir);

					if(remove_a_head->next == NULL) {
						remove_a_head->next = remove_node;
						remove_node->next = NULL;
					}
					else {
						Remove_file_count *tmp_node = (Remove_file_count *)malloc(sizeof(Remove_file_count));
						tmp_node = remove_a_head->next;
						remove_a_head->next = remove_node;
						remove_node->next = tmp_node;
					}
				}
			}
		}
		
		Remove_file_count *remove_node = (Remove_file_count *)malloc(sizeof(Remove_file_count));
		Backup_linkedlist *tmp_node = (Backup_linkedlist *)malloc(sizeof(Backup_linkedlist));
		remove_node = remove_a_head->next;
		
		while(remove_node != NULL) { // 링크드 리스트로 저장한 노드들 하나씩
			tmp_node = head->next;
			if(access(tmp_node->dest_path, F_OK) != 0) { // 접근 불가능하면 다음 노드로
				tmp_node = tmp_node->next;
			}
			if(tmp_node != NULL && !strcmp(remove_node->source_path, tmp_node->source_path)) { // 원본 링크드 리스트와 비교, 경로 획득
				strcpy(tmp_backup_directory, tmp_node->backup_directory);
				strcpy(tmp_dest_path, tmp_node->dest_path);
				strcpy(tmp_source_path, tmp_node->source_path);
				if(access(tmp_node->dest_path, F_OK) == 0) { // 파일에 접근 가능
					if(remove(tmp_node->dest_path) != 0) { // 파일 삭제, 예외 처리
						fprintf(stderr, "remove error!\n");
						exit(1);	
					}
				}				
				char rm_dir[DATAMAX];
				strcpy(rm_dir, tmp_dest_path);
				char* last_slash = strrchr(rm_dir, '/'); // 경로의 마지막 부분 추출
				if (last_slash != NULL) {
					*last_slash = '\0';
				}
				remove_dir(rm_dir); // 디렉토리 삭제
				snprintf(log_message, BUFFER_SIZE, "%s : \"%s\" removed by \"%s\"\n" , tmp_backup_directory, tmp_dest_path, tmp_source_path); // 로그 파일에 작성할 내용
				if(write(fd, log_message, strlen(log_message)) < 0) { // 로그 파일에 작성, 예외 처리
					fprintf(stderr, "write error\n");
					exit(1);
				}
				head->next = tmp_node->next;
				tmp_node = head->next;
				printf("\"%s\" removed by \"%s\"\n", tmp_node->dest_path, tmp_node->source_path); // remove 내용 출력
			}
		
			while(tmp_node != NULL) {
				if(access(tmp_node->dest_path, F_OK) != 0) { // 접근 불가능하면 다음 노드로
					tmp_node = tmp_node->next;
					continue;
				}
				if(!strcmp(remove_node->source_path, tmp_node->source_path)) { // 원본 링크드 리스트와 비교, 경로 획득
					strcpy(tmp_backup_directory, tmp_node->backup_directory);
					strcpy(tmp_dest_path, tmp_node->dest_path);
					strcpy(tmp_source_path, tmp_node->source_path);
					if(access(tmp_node->dest_path, F_OK) == 0) { // 파일에 접근 가능
						if(remove(tmp_node->dest_path) != 0) { // 파일 삭제, 예외 처리
							perror("remove\n");
							fprintf(stderr, "remove error!\n");
							exit(1);
						}
					}
					char rm_dir[DATAMAX];
					strcpy(rm_dir, tmp_dest_path);
					char* last_slash = strrchr(rm_dir, '/'); // 경로의 마지막 부분 추출
					if (last_slash != NULL) {
						*last_slash = '\0';
					}
					remove_dir(rm_dir); // 디렉토리 삭제
					snprintf(log_message, BUFFER_SIZE, "%s : \"%s\" removed by \"%s\"\n" , tmp_backup_directory, tmp_dest_path, tmp_source_path); // 로그 파일에 작성할 내용
					if(write(fd, log_message, strlen(log_message)) < 0) { // 로그 파일에 작성, 예외처리
						fprintf(stderr, "write error\n");
						exit(1);
					}
					printf("\"%s\" removed by \"%s\"\n", tmp_node->dest_path, tmp_node->source_path); // remove 내용 출력
				}
				tmp_node = tmp_node->next;
			}
			remove_node = remove_node->next;
		}
	}
	else { // 파일인 경우
		Backup_linkedlist *tmp_node = (Backup_linkedlist *)malloc(sizeof(Backup_linkedlist));
		Backup_linkedlist *tmp_pre = (Backup_linkedlist *)malloc(sizeof(Backup_linkedlist));
		tmp_node = head->next;
		if(tmp_node != NULL && !strcmp(tmp_node->source_path, real_path)) { // 원본 링크드 리스트와 비교, 경로 획득
			strcpy(tmp_backup_directory, tmp_node->backup_directory);
			strcpy(tmp_dest_path, tmp_node->dest_path);
			strcpy(tmp_source_path, tmp_node->source_path);
			if(remove(tmp_node->dest_path) != 0) { // 파일 삭제, 예외 처리
				fprintf(stderr, "remove error!\n");
				exit(1);
			}
			char rm_dir[DATAMAX];
			strcpy(rm_dir, tmp_dest_path);
			char* last_slash = strrchr(rm_dir, '/'); // 디렉토리 마지막 부분 추출
			if (last_slash != NULL) {
				*last_slash = '\0';
			}
			remove_dir(rm_dir);	// 디렉토리 삭제
			snprintf(log_message, BUFFER_SIZE, "%s : \"%s\" removed by \"%s\"\n" , tmp_backup_directory, tmp_dest_path, tmp_source_path); // 로그 파일에 작성할 내용
			if(write(fd, log_message, strlen(log_message)) < 0) { // 로그 파일에 작성
				fprintf(stderr, "write error\n");
                                exit(1);
			}
			head->next = tmp_node->next;
			tmp_node = head->next;
		}
		tmp_pre = head;

		while(tmp_node != NULL) { 
			if(!strcmp(real_path, tmp_node->source_path)) { // 원본 링크드 리스트와 비교, 경로 획득
				strcpy(tmp_backup_directory, tmp_node->backup_directory);
                strcpy(tmp_dest_path, tmp_node->dest_path);
				strcpy(tmp_source_path, tmp_node->source_path);
				if(remove(tmp_node->dest_path) != 0) { // 백업 파일 삭제, 예외 처리
					fprintf(stderr, "remove error!\n");
					exit(1);
				}                         	
				char rm_dir[DATAMAX];
				strcpy(rm_dir, tmp_dest_path);
				char* last_slash = strrchr(rm_dir, '/'); // 경로의 마지막 부분 추출
				if(last_slash != NULL) {
					*last_slash = '\0';
				}
				remove_dir(rm_dir); // 디렉토리 삭제
				strcpy(tmp_source_path, tmp_node->source_path);
                snprintf(log_message, BUFFER_SIZE, "%s : \"%s\" removed by \"%s\"\n", tmp_backup_directory, tmp_dest_path, tmp_source_path); // 로그 파일에 작성할 내용
                if(write(fd, log_message, strlen(log_message)) < 0) { // 로그 파일에 작성, 예외 처리
                    fprintf(stderr, "write error\n");
                    exit(1);
                }
                printf("\"%s\" removed by \"%s\"\n", tmp_node->dest_path, tmp_node->source_path); // remove 내용 출력
				tmp_pre->next = tmp_node->next;
            }
			else {
				tmp_pre = tmp_node;
			}
            tmp_node = tmp_node->next;
		}
	}
}

void recover_log(char *recover_path, char *source_path, char *backup_directory) { // recover 로그 작성하는 함수
	char *log_path = "/home/backup/ssubak.log";
	time_t time_cur;
	struct tm * time_info;
	char time_string[15];
	char log_message[BUFFER_SIZE];	
	
	time(&time_cur); // 현재 시간 얻는 함수
	time_info = localtime(&time_cur);
	strftime(time_string, sizeof(time_string), "%y%m%d%H%M%S", time_info);
		
	int fd;
	if((fd = open(log_path, O_RDONLY | O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0) { // 로그 파일 오픈, 예외 처리
		fprintf(stderr, "open error for %s\n", log_path);
		return;
	}
	
	snprintf(log_message, BUFFER_SIZE, " : \"%s\" recovered to \"%s\"\n", recover_path, source_path); // 로그 파일에 작성할 내용, 예외 처리
	if(write(fd, log_message, strlen(log_message)) < 0) {
		fprintf(stderr, "write error\n");
		exit(1);
	}
	
	printf("\"%s\" recovered to \"%s\"\n", recover_path, source_path); // recover 내용 출력
}

void recover_file(char *recover_path) { // recover 하는 함수
	struct stat file_stat;
	char real_path[PATHMAX];

	if(access(recover_path, F_OK) == 0) { // 접근 가능한 경우
		strcpy(real_path, realpath(recover_path, NULL));
		
		if(real_path == NULL) { // 절대 경로 예외 처리
			if(errno == ENOENT) {
				fprintf(stderr, "Error: File or directory not found.\n");
			} 
			else if(errno == EACCES) {
				fprintf(stderr, "Error: Permission denied.\n");
			} 
			else {
				perror("realpath");
			}
		}

		if((open(real_path, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) {
			perror("open\n");
			exit(1);
		}
	}
	else { // 접근 불가능한 경우
		char cwd[PATHMAX];
    	char absolute_path[PATHMAX];

    	if(getcwd(cwd, sizeof(cwd)) == NULL) { // 현재 위치 획득, 예외 처리
        	perror("getcwd");
        	exit(EXIT_FAILURE);
    	}

    	if(realpath(recover_path, absolute_path) == NULL) { // realpath 절대 경로 불가능한 경우
			if(open(absolute_path, O_RDWR | O_CREAT | O_TRUNC, 0644) < 0) { // 파일 생성
				perror("open\n");
				exit(1);
			}
    	}
		recover_file(absolute_path); // 파일 생성 후 다시 재귀
		return;
	}

	if(strlen(real_path) >= PATHMAX) { // 예외 처리
		fprintf(stderr, "Too Long!\n");
		exit(1);
	}

	Recover_file *recover_head = (Recover_file *)malloc(sizeof(Recover_file));
	Backup_linkedlist *tmp_node = head->next;
	while(tmp_node != NULL) { 
		if(!strcmp(tmp_node->source_path, real_path)) { // 원본 링크드 리스트와 비교하면서 경로 같은 곳 획득
			Recover_file *recover_tmp = (Recover_file *)malloc(sizeof(Recover_file));
			Recover_file *recover_node = (Recover_file *)malloc(sizeof(Recover_file));
			strcpy(recover_node->backup_directory, tmp_node->backup_directory); // 내용 저장
			strcpy(recover_node->name, tmp_node->backup_fname);
			strcpy(recover_node->dest_path, tmp_node->dest_path);
			strcpy(recover_node->source_path, tmp_node->source_path);

			if(recover_head->next == NULL) {
				recover_head->next = recover_node;
				recover_node->next = NULL;
				tmp_node = tmp_node->next;
				continue;
			}

			recover_tmp = recover_head->next;

			long recover_directory = strtol(recover_node->backup_directory, NULL, 10);
			if(strtol(recover_tmp->backup_directory, NULL, 10) >= recover_directory) { // 시간 비교 후 오름차순으로 저장
				recover_node->next = recover_tmp;
				recover_head->next = recover_node;
				tmp_node = tmp_node->next;
				continue;
			}

			while(recover_tmp != NULL) { // 링크드 리스트로 저장
				if(recover_tmp->next == NULL) {
					recover_tmp->next = recover_node;
					recover_node->next = NULL;
					break;
				}

				long recover_tmp_directory = strtol(recover_tmp->next->backup_directory, NULL, 10);
				if(recover_tmp_directory >= recover_directory) {
					recover_node->next = recover_tmp->next;
					recover_tmp->next = recover_node;
					break;
				}
				recover_tmp = recover_tmp->next;
			}	
			
		}
		tmp_node = tmp_node->next;
	}
	
	Recover_file *recover_tmp = (Recover_file *)malloc(sizeof(Recover_file));
	Recover_file *recover_node = (Recover_file *)malloc(sizeof(Recover_file));
	
	recover_tmp = recover_head->next;
	int c = 0;
	while(recover_tmp != NULL) {  // 백업 경로가 같은 개수 구하는 코드
		c++;
		recover_tmp = recover_tmp->next;
	}
	recover_tmp = recover_head->next;
	if(c == 1) {
		char tmp_hash[DIGESTMAX]; // 해시 값 비교
		char node_hash[DIGESTMAX];
		get_hash_value(recover_tmp->source_path, tmp_hash);
		get_hash_value(recover_tmp->dest_path, node_hash);

		if(!strcmp(tmp_hash, node_hash)) { // 입력받은 경로에 대한 백업 파일이 원본 파일과 내용이 같은 경우
			printf("\"%s\" not changed with \"%s\"\n", recover_tmp->dest_path, recover_tmp->source_path);
			return;
		}
		
		ssize_t bytes_read, bytes_write;
		char buf[BUFFER_SIZE];
		struct stat file_stat;
		struct dirent* entry;
		struct dirent** file_list;
		int fd;
		if((fd = open(recover_tmp->source_path, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) { // recover 할 파일 open, 예외 처리
			fprintf(stderr, "open error for %s\n", recover_tmp->name);
			exit(1);
		}

		int backup_fd;
		if((backup_fd = open(recover_tmp->dest_path, O_RDONLY)) < 0) { // 백업 파일에 있는 내용 open, 예외 처리
			perror("open\n");
			exit(1);
		}			
		while((bytes_read = read(backup_fd, buf, BUFFER_SIZE)) > 0) { // 백업 파일에서 읽은 내용
			bytes_write = write(fd, buf, bytes_read); // 원본 파일에 작성
			if(bytes_write != bytes_read) { // write 예외 처리
				fprintf(stderr, "write error\n");
				exit(1);	
			}
		}
		
		tmp_node = head->next;
		if(!strcmp(tmp_node->dest_path, recover_tmp->dest_path) && !strcmp(tmp_node->source_path, recover_tmp->dest_path)) {
			head->next = tmp_node->next;	
		}
		else {
			while(tmp_node->next != NULL) { // 원본 링크드 리스트에서 삭제
				if(!strcmp(tmp_node->next->dest_path, recover_tmp->dest_path) && !strcmp(tmp_node->next->source_path, recover_tmp->dest_path)) {
					tmp_node->next = tmp_node->next->next;
					break;
				}
				tmp_node = tmp_node->next;
			}
		}
		
		if(remove(recover_tmp->dest_path) != 0) { // 백업 파일 삭제, 예외 처리
			perror("remove\n");
			exit(1);
		}
		
		char *home_backup = "/home/backup";
		char recover_dir[DATAMAX];
		strcpy(recover_dir, recover_tmp->dest_path);
	
		while(strcmp(recover_dir, home_backup)) { // 경로가 /home/backup이 될 때까지 반복
			DIR *dir = opendir(recover_dir); // 경로의 마지막 부분 추출
			char *last_slash = strrchr(recover_dir, '/');
			if(last_slash != NULL) {
				*last_slash = '\0';
			}
			int n;
			if((n = scandir(recover_dir, &file_list, NULL, NULL)) < 0) { // 디렉토리 안에 개수
				perror("scandir\n");
				exit(1);
			}

			if(n > 2) break;
			else {
				if(rmdir(recover_dir) != 0) { // 디렉토리 존재하면 삭제, 예외 처리
					fprintf(stderr, "rmdir error!\n");
					exit(1);
				}
			}
		}
		rmdir(recover_tmp->dest_path); 

		recover_log(recover_tmp->dest_path, recover_tmp->source_path, recover_tmp->backup_directory); // 로그 파일에 작성하는 함수
	}

	else if(c < 1) { // 백업 파일이 존재하지 않은 경우
		printf("No backup file!\n");
		exit(0);
	}
	else { // 백업파일이 다수인 경우
		recover_tmp = recover_head->next;
		int cnt = 0;
		printf("backup files of %s\n", recover_tmp->source_path);
		printf("%d. exit\n", cnt);
		while(recover_tmp != NULL) {
			cnt++;
			if(stat(recover_tmp->source_path, &file_stat) < 0) {
				fprintf(stderr, "stat error!\n");
				exit(1);
			}
			printf("%d. %s        %ldbytes\n", cnt, recover_tmp->backup_directory, file_stat.st_size); // 예시처럼 출력
			recover_tmp = recover_tmp->next;
		} 
		printf(">> ");
		int num = 0;
		scanf("%d", &num); // 사용자에게 입력받는 수
		if(num == 0) exit(0);
		else if(num > cnt) exit(0);
		else {
			struct dirent **file_list;
			int index = 0;
			recover_tmp = recover_head->next;
			while(recover_tmp != NULL) { // 사용자에게 입력받은 수만큼 반복
				index++;
				if(index == num) {
					ssize_t bytes_read, bytes_write;
					char buf[BUFFER_SIZE];
					struct stat file_stat;
					struct dirent* entry;
					int fd;
					if((fd = open(recover_tmp->source_path, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) { // 원하는 파일 접근 후 open, 예외 처리
						fprintf(stderr, "open error for %s\n", recover_tmp->name);
						exit(1);
					}

					int backup_fd;
					if((backup_fd = open(recover_tmp->dest_path, O_RDONLY)) < 0) { // 백업 파일 open, 예외 처리
						perror("open");
						exit(1);
					}
					// 해시 값을 통해 중복 체크
					char tmp_hash[DIGESTMAX];
					char new_hash[DIGESTMAX];

					get_hash_value(recover_tmp->source_path, tmp_hash);
					get_hash_value(recover_tmp->dest_path, new_hash);

					if(!strcmp(tmp_hash, new_hash)) { // 같은 경우 recover를 하지 않는다 
						printf("\"%s\" not changed with \"%s\"\n", recover_tmp->dest_path, recover_tmp->source_path);
						break;
					}	
					
					while((bytes_read = read(backup_fd, buf, BUFFER_SIZE)) > 0) { // 백업파일에서 읽은 내용 백업할 파일에 작성
						bytes_write = write(fd, buf, bytes_read);
						if(bytes_write != bytes_read) {
							fprintf(stderr, "write error\n"); // write 예외 처리
							exit(1);
						}
					}
		
					if(remove(recover_tmp->dest_path) != 0) { // 파일 삭제, 예외 처리
						perror("remove\n");
						exit(1);
					}
		
					char *home_backup = "/home/backup";
					char recover_dir[DATAMAX];
					strcpy(recover_dir, recover_tmp->dest_path);
					
					while(strcmp(recover_dir, home_backup)) { // 경로가 /home/backup이 될 때까지 반복
						DIR *dir = opendir(recover_dir); // 경로의 마지막 부분만 추출
						char *last_slash = strrchr(recover_dir, '/');
						if(last_slash != NULL) {
							*last_slash = '\0';
						}
						int n;
						if((n = scandir(recover_dir, &file_list, NULL, NULL)) < 0) { // 디렉토리 안에 파일, 디렉토리 개수
							perror("scandir\n");
							exit(1);
						}

						if(n > 2) break;
						else {
							if(rmdir(recover_dir) != 0) { // 디렉토리 삭제
								//fprintf(stderr, "rmdir error!\n");
								//exit(1);
							}
						}
					}
					
					recover_log(recover_tmp->dest_path, recover_tmp->source_path, recover_tmp->backup_directory); // 로그파일에 작성하는 함수
					break;			
				}
				recover_tmp = recover_tmp->next;
			}
		}
	}
}

void recover_opt_d(char *recover_path) { // recover option -d 인 함수
	char real_path[PATHMAX];
	struct stat file_stat;
	struct dirent **file_list;

	if(access(recover_path, F_OK) == 0) { // 접근 가능한 경우
		strcpy(real_path, realpath(recover_path, NULL));
		
		if(real_path == NULL) { // 절대 경로 획득, 예외 처리
			if(errno == ENOENT) {
				fprintf(stderr, "Error: File or directory not found.\n");
			} 
			else if(errno == EACCES) {
				fprintf(stderr, "Error: Permission denied.\n");
			} 
			else {
				perror("realpath");
			}
		}

		if(stat(real_path, &file_stat) < 0) { // stat 함수 예외 처리
			fprintf(stderr, "stat error\n");
			exit(1);
		}
		
		if(!S_ISDIR(file_stat.st_mode)) { // 파일인 경우 recover_file 함수를 통해 처리
			recover_file(real_path);
			return;
		}
		
	}
	else {
		char cwd[PATHMAX];
    	char absolute_path[PATHMAX];

    	if(getcwd(cwd, sizeof(cwd)) == NULL) { // 현재 위치
        	perror("getcwd");
        	exit(1);
    	}

		strcpy(absolute_path, realpath(recover_path, NULL)); // 절대 경로 획득
		if(open(absolute_path, O_RDWR | O_CREAT | O_TRUNC, 0644) < 0) { // 파일 생성
			fprintf(stderr, "open error!\n");
			exit(1);
		}

		recover_opt_d(absolute_path); // recover opt d 함수로 다시 재귀
		return;
	}

	if(strlen(real_path) >= PATHMAX) { // 예외 처리
		fprintf(stderr, "Too Long!\n");
		exit(1);
	}

	Recover_file *recover_head = (Recover_file *)malloc(sizeof(Recover_file));
	Recover_file *recover_tmp = (Recover_file *)malloc(sizeof(Recover_file));
	Backup_linkedlist *tmp_node = (Backup_linkedlist *)malloc(sizeof(Backup_linkedlist));

	int n;
	if((n = scandir(real_path, &file_list, NULL, NULL)) < 0) { // 디렉토리 안에 디렉토리, 파일 개수, 예외 처리
		perror("scandir\n");
		exit(1);
	}

	for(int i = 0; i < n; i++) {
		if(strcmp(file_list[i]->d_name, ".") != 0 && strcmp(file_list[i]->d_name, "..") != 0) {
			char check_dir[PATHMAX];
			snprintf(check_dir, PATHMAX, "%s", real_path);
			strcat(check_dir, "/");
			strcat(check_dir, file_list[i]->d_name);

			if(stat(check_dir, &file_stat) < 0) { // stat 함수 예외 처리
				fprintf(stderr, "stat error\n");
				exit(1);
			}

			if(S_ISDIR(file_stat.st_mode)) { // 디렉토리인 경우 무시
				continue;
			}
			tmp_node = head->next;

			Recover_file *recover_node = (Recover_file *)malloc(sizeof(Recover_file));

			while(tmp_node != NULL) { // 파일인 경우 링크드 리스트로 관리 저장
				if(!strcmp(tmp_node->source_path, check_dir)) { // 원본 링크드 리스트와 비교로 정보 획득
					strcpy(recover_node->backup_directory, tmp_node->backup_directory);
					strcpy(recover_node->name, tmp_node->backup_fname);
					strcpy(recover_node->dest_path, tmp_node->dest_path);
					strcpy(recover_node->source_path, tmp_node->source_path);	

					if(recover_head->next == NULL) {
						recover_head->next = recover_node;
						recover_node->next = NULL;
						tmp_node = tmp_node->next;
						continue;
					}
					recover_tmp = recover_head->next;
					
					while(recover_tmp != NULL) {
						if(recover_tmp->next == NULL) {
							recover_tmp->next = recover_node;
							recover_node->next = NULL;
							break;
						}
						recover_tmp = recover_tmp->next;
					}
				}
				tmp_node = tmp_node->next;
			}

			recover_tmp = recover_head->next;
			while(recover_tmp != NULL) {
				recover_file(recover_tmp->source_path); // 링크드 리스트에 저장한 파일들 recover_file로 처리
				recover_tmp = recover_tmp->next;
			}
		}
	}
}

void recover_opt_r(char *recover_path) { // recover option -r 인 경우 함수
	char real_path[PATHMAX];
	struct stat file_stat;
	struct dirent **file_list;

	if(access(recover_path, F_OK) == 0) { // 접근 가능한 경우
		strcpy(real_path, realpath(recover_path, NULL));
		
		if(real_path == NULL) { // 결대 경로 예외 처리
			if(errno == ENOENT) {
				fprintf(stderr, "Error: File or directory not found.\n");
			} 
			else if(errno == EACCES) {
				fprintf(stderr, "Error: Permission denied.\n");
			} 
			else {
				perror("realpath");
			}
		}

		if(stat(real_path, &file_stat) < 0) { // stat 함수 예외 처리
			fprintf(stderr, "stat error\n");
			exit(1);
		}

		if(!S_ISDIR(file_stat.st_mode)) { // 파일인 경우 recover_file 로 처리
			recover_file(real_path);
			return;
		}
	}
	else {
		char cwd[PATHMAX];
    	char absolute_path[PATHMAX];

    	if(getcwd(cwd, sizeof(cwd)) == NULL) { // 현재 위치 획득, 예외 처리
        	perror("getcwd");
        	exit(EXIT_FAILURE);
    	}

    	if(realpath(recover_path, absolute_path) == NULL) { // 절대 경로 획득, 예외 처리
			if(open(absolute_path, O_RDWR | O_CREAT | O_TRUNC, 0644) < 0) { // 파일 생성
				perror("open\n");
				exit(1);
			}
    	}
		recover_opt_r(absolute_path); // 다시 recover opt r 함수로 재귀
		return;
	}

	if(strlen(real_path) >= PATHMAX) { // 예외 처리
		fprintf(stderr, "Too Long!\n");
		exit(1);
	}

	Recover_file *recover_tmp = (Recover_file *)malloc(sizeof(Recover_file));
	Backup_linkedlist *tmp_node = (Backup_linkedlist *)malloc(sizeof(Backup_linkedlist));

	int n;
	if((n = scandir(real_path, &file_list, NULL, NULL)) < 0) { // 디렉토리 안에 디렉토리, 파일 개수
		perror("scandir\n");
		exit(1);
	}

	for(int i = 0; i < n; i++) {
		if(strcmp(file_list[i]->d_name, ".") != 0 && strcmp(file_list[i]->d_name, "..") != 0) { // 디렉토리 안 확인
			char check_dir[PATHMAX];
			snprintf(check_dir, PATHMAX, "%s", real_path);
			strcat(check_dir, "/");
			strcat(check_dir, file_list[i]->d_name);

			if(stat(check_dir, &file_stat) < 0) { // stat 함수 예외 처리
				fprintf(stderr, "stat error\n");
				exit(1);
			}

			if(S_ISDIR(file_stat.st_mode)) { // 디렉토리인 경우 다시 재귀
				recover_opt_r(check_dir);
				continue;
			}

			tmp_node = head->next;

			while(tmp_node != NULL) { // 파일인 경우 링크드 리스트로 관리
				if(!strcmp(tmp_node->source_path, check_dir)) { // 원본 링크드 리스트와 비교로 정보 획득
					Recover_file *recover_node = (Recover_file *)malloc(sizeof(Recover_file));
					strcpy(recover_node->backup_directory, tmp_node->backup_directory);
					strcpy(recover_node->name, tmp_node->backup_fname);
					strcpy(recover_node->dest_path, tmp_node->dest_path);
					strcpy(recover_node->source_path, tmp_node->source_path);	

					if(recover_head->next == NULL) {
						recover_head->next = recover_node;
						recover_node->next = NULL;
						tmp_node = tmp_node->next;
						continue;
					}
					recover_tmp = recover_head->next;
						
					while(recover_tmp != NULL) {
						if(!strcmp(recover_tmp->source_path, recover_node->source_path)) {
							break;
						}
						if(recover_tmp->next == NULL) {
							recover_tmp->next = recover_node;
							recover_node->next = NULL;
							break;
						}
						recover_tmp = recover_tmp->next;

					}
				}
				tmp_node = tmp_node->next;
			}
		
			recover_tmp = recover_head->next;
			while(recover_tmp != NULL) {
				if(access(recover_tmp->dest_path, F_OK) == 0) {
					recover_file(recover_tmp->source_path);	// 링크드 리스트로 저장한 파일들 recover_file 함수로 처리
				}
				recover_tmp = recover_tmp->next;
			}
		}
	}
}

void recover_opt_l(char *recover_path) { // recover opt -l 인 함수
	char real_path[PATHMAX];

	if(access(recover_path, F_OK) == 0) { // 접근 가능한 경ㅇ 
		strcpy(real_path, realpath(recover_path, NULL));
		
		if(real_path == NULL) { // 결대 경로 예외 처리
			if(errno == ENOENT) {
				fprintf(stderr, "Error: File or directory not found.\n");
			} 
			else if(errno == EACCES) {
				fprintf(stderr, "Error: Permission denied.\n");
			} 
			else {
				perror("realpath");
			}
		}

		if((open(real_path, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) { // 파일 생성
			perror("open\n");
			exit(1);
		}
	}
	else {
		char cwd[PATHMAX];
    	char absolute_path[PATHMAX];

		if(getcwd(cwd, sizeof(cwd)) == NULL) { // 현재 위치 획득
        	perror("getcwd");
        	exit(EXIT_FAILURE);
    	}

    	if(realpath(recover_path, absolute_path) == NULL) { // 결대 경로 없으면 생성
			if(open(absolute_path, O_RDWR | O_CREAT | O_TRUNC, 0644) < 0) {
				perror("open\n");
				exit(1);
			}
    	}
		recover_file(absolute_path); // recover file 함수로 처리
		return;
	}

	if(strlen(real_path) >= PATHMAX) { // 예외 처리
		fprintf(stderr, "Too Long!\n");
		exit(1);
	}

	Recover_file *recover_head = (Recover_file *)malloc(sizeof(Recover_file));
	Backup_linkedlist *tmp_node = head->next;
	while(tmp_node != NULL) { // 원본 링크드 리스트와 비교를 통해 정보 획득
		if(!strcmp(tmp_node->source_path, real_path)) {
			Recover_file *recover_tmp = (Recover_file *)malloc(sizeof(Recover_file));
			Recover_file *recover_node = (Recover_file *)malloc(sizeof(Recover_file));
			strcpy(recover_node->backup_directory, tmp_node->backup_directory);
			strcpy(recover_node->name, tmp_node->backup_fname);
			strcpy(recover_node->dest_path, tmp_node->dest_path);
			strcpy(recover_node->source_path, tmp_node->source_path);

			if(recover_head->next == NULL) { // 링크드 리스트로 관리
				recover_head->next = recover_node;
				recover_node->next = NULL;
				tmp_node = tmp_node->next;
				continue;
			}

			recover_tmp = recover_head->next;

			long recover_directory = strtol(recover_node->backup_directory, NULL, 10);
			if(strtol(recover_tmp->backup_directory, NULL, 10) >= recover_directory) { // 오름차순으로 정렬
				recover_node->next = recover_tmp;
				recover_head->next = recover_node;
				tmp_node = tmp_node->next;
				continue;
			}

			while(recover_tmp != NULL) {	
				if(recover_tmp->next == NULL) {
					recover_tmp->next = recover_node;
					recover_node->next = NULL;
					break;
				}

				long recover_tmp_directory = strtol(recover_tmp->next->backup_directory, NULL, 10);
				if(recover_tmp_directory >= recover_directory) {
					recover_node->next = recover_tmp->next;
					recover_tmp->next = recover_node;
					break;
				}
				recover_tmp = recover_tmp->next;
			}	
			
		}
		tmp_node = tmp_node->next;
	}

	Recover_file *recover_tmp = (Recover_file *)malloc(sizeof(Recover_file));
	recover_tmp = recover_head->next;

	while(recover_tmp != NULL) {
		if(recover_tmp->next == NULL) {
			break;
		}
		recover_tmp = recover_tmp->next;
	}
	// 해시 값 비교를 통한 중복 처리
	char tmp_hash[DIGESTMAX];
	char new_hash[DIGESTMAX];

	get_hash_value(recover_tmp->source_path, tmp_hash);
	get_hash_value(recover_tmp->dest_path, new_hash);

	if(!strcmp(tmp_hash, new_hash)) {
		printf("\"%s\" not changed with \"%s\"\n", recover_tmp->dest_path, recover_tmp->source_path); // 같은 경우 recover 하지 않고 출력
		return;
	}

	ssize_t bytes_read, bytes_write;
	char buf[BUFFER_SIZE];
	struct stat file_stat;
	struct dirent* entry;
	struct dirent** file_list; 
	int fd;
	if((fd = open(recover_tmp->source_path, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) { // 백업할 파일 오픈, 예외 처리
		fprintf(stderr, "open error for %s\n", recover_tmp->name);
		exit(1);
	}

	int backup_fd;
	if((backup_fd = open(recover_tmp->dest_path, O_RDONLY)) < 0) {	// 백업 파일 오픈, 예외 처리
		perror("open\n");
		exit(1);
	}

	while((bytes_read = read(backup_fd, buf, BUFFER_SIZE)) > 0) { // 읽은 내용 백업 파일에 작성, 예외 처리
		bytes_write = write(fd, buf, bytes_read);
		if(bytes_write != bytes_read) {
			fprintf(stderr, "write error\n");
			exit(1);
		}
	}	
	char *home_backup = "/home/backup";
	char recover_dir[DATAMAX];
	strcpy(recover_dir, recover_tmp->dest_path);
	
	while(strcmp(recover_dir, home_backup)) { // 경로가 /home/backup 가 될 때까지 실행
		DIR *dir = opendir(recover_dir);
		char *last_slash = strrchr(recover_dir, '/');
		if(last_slash != NULL) { // 경로의 마지막 부분 추출
			*last_slash = '\0';
		}
		int n;
		if((n = scandir(recover_dir, &file_list, NULL, NULL)) < 0) { // 디렉토리 안에 디렉토리, 파일 개수
			perror("scandir\n");
			exit(1);
		}

		if(n > 2) break;
		else {
			if(rmdir(recover_dir) != 0) { // 디렉토리 삭제, 예외 처리
				fprintf(stderr, "rmdir error!\n");
				exit(1);
			}
		}
	}
	recover_log(recover_tmp->dest_path, recover_tmp->source_path, recover_tmp->backup_directory); // 로그 파일에 내용 작성
}

void recover_file_n_file(char *recover_path, char *storage_path) { // recover option -n 인 경우
	struct stat file_stat;
		
	char *real_path = realpath(recover_path, NULL); // 절대 경로 획득
		
	if(real_path == NULL) { // 절대 경로 예외 처리
		if(errno == ENOENT) {
			fprintf(stderr, "Error: File or directory not found.\n");
		} 
		else if(errno == EACCES) {
			fprintf(stderr, "Error: Permission denied.\n");
		} 
		else {
			perror("realpath");
		}
	}

	if((open(real_path, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) { // 백업 내용 저장할 새로운 디렉토리 생성
		perror("open\n");
		exit(1);
	}
	
	if(strlen(real_path) >= PATHMAX) { // 예외 처리
		fprintf(stderr, "Too Long!\n");
		exit(1);
	}

	Recover_file *recover_head = (Recover_file *)malloc(sizeof(Recover_file));
	Backup_linkedlist *tmp_node = head->next;
	while(tmp_node != NULL) { // 원본 링크드 리스트와 비교를 통해 정보 획득
		if(!strcmp(tmp_node->source_path, real_path)) { // 링크드 리스트를 통해 저장, 관리
			Recover_file *recover_tmp = (Recover_file *)malloc(sizeof(Recover_file));
			Recover_file *recover_node = (Recover_file *)malloc(sizeof(Recover_file));
			strcpy(recover_node->backup_directory, tmp_node->backup_directory);
			strcpy(recover_node->name, tmp_node->backup_fname);
			strcpy(recover_node->dest_path, tmp_node->dest_path);
			strcpy(recover_node->source_path, tmp_node->source_path);

			if(recover_head->next == NULL) {
				recover_head->next = recover_node;
				recover_node->next = NULL;
				tmp_node = tmp_node->next;
				continue;
			}

			recover_tmp = recover_head->next;

			long recover_directory = strtol(recover_node->backup_directory, NULL, 10);
			if(strtol(recover_tmp->backup_directory, NULL, 10) >= recover_directory) { // 오름차순으로 정렬
				recover_node->next = recover_tmp;
				recover_head->next = recover_node;
				tmp_node = tmp_node->next;
				continue;
			}

			while(recover_tmp != NULL) {	
				if(recover_tmp->next == NULL) {
					recover_tmp->next = recover_node;
					recover_node->next = NULL;
					break;
				}

				long recover_tmp_directory = strtol(recover_tmp->next->backup_directory, NULL, 10);
				if(recover_tmp_directory >= recover_directory) {
					recover_node->next = recover_tmp->next;
					recover_tmp->next = recover_node;
					break;
				}
				recover_tmp = recover_tmp->next;
			}	
			
		}
		tmp_node = tmp_node->next;
	}
	
	Recover_file *recover_tmp = (Recover_file *)malloc(sizeof(Recover_file));
	Recover_file *recover_node = (Recover_file *)malloc(sizeof(Recover_file));
	
	recover_tmp = recover_head->next;
	int c = 0;
	while(recover_tmp != NULL) { // 백업 파일이 몇 개 있는지 얻는 코드
		c++;
		recover_tmp = recover_tmp->next;
	}
	recover_tmp = recover_head->next;
	if(c == 1) { // 백업 파일이 하나인 경우
		ssize_t bytes_read, bytes_write;
		char buf[BUFFER_SIZE];
		struct stat file_stat;
		struct dirent* entry;
		struct dirent** file_list;
		int fd;
		if((fd = open(storage_path, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) { // 백업 내용 저장할 파일 오픈, 예외 처리
			fprintf(stderr, "open error for %s\n", recover_tmp->name);
			exit(1);
		}

		int backup_fd;
		if((backup_fd = open(recover_tmp->dest_path, O_RDONLY)) < 0) { // 백업 내용 가져올 파일 오픈, 예외 처리
			perror("open\n");
			exit(1);
		}			
		while((bytes_read = read(backup_fd, buf, BUFFER_SIZE)) > 0) { // 백업할 내용 가져와서 저장, 예외 처리
			bytes_write = write(fd, buf, bytes_read);
			if(bytes_write != bytes_read) {
				fprintf(stderr, "write error\n");
				exit(1);	
			}
		}
		
		tmp_node = head->next;
		if(!strcmp(tmp_node->dest_path, recover_tmp->dest_path) && !strcmp(tmp_node->source_path, recover_tmp->dest_path)) {
			head->next = tmp_node->next;	
		}
		else {
			while(tmp_node->next != NULL) {
				if(!strcmp(tmp_node->next->dest_path, recover_tmp->dest_path) && !strcmp(tmp_node->next->source_path, recover_tmp->dest_path)) {
					tmp_node->next = tmp_node->next->next;
					break;
				}
				tmp_node = tmp_node->next;
			}
		}
		
		if(remove(recover_tmp->dest_path) != 0) { // 파일 삭제, 예외 처리
			perror("remove\n");
			exit(1);
		}
		
		char *home_backup = "/home/backup";
		char recover_dir[DATAMAX];
		strcpy(recover_dir, recover_tmp->dest_path);
	
		while(strcmp(recover_dir, home_backup)) { // 경로가 /home/backup 이 될 때까지 반복
			DIR *dir = opendir(recover_dir); // 경로의 상위 추출
			char *last_slash = strrchr(recover_dir, '/');
			if(last_slash != NULL) {
				*last_slash = '\0';
			}
			int n;
			if((n = scandir(recover_dir, &file_list, NULL, NULL)) < 0) { // 디렉토리 안에 디렉토리, 파일 개수
				perror("scandir\n");
				exit(1);
			}

			if(n > 2) break;
			else {
				if(rmdir(recover_dir) != 0) { // 디렉토리 삭제, 예외 처리
					fprintf(stderr, "rmdir error!\n");
					exit(1);
				}
			}
		}
		rmdir(recover_tmp->dest_path); // 디렉토리 삭제

		recover_log(recover_tmp->dest_path, recover_tmp->source_path, recover_tmp->backup_directory); // recover 로그 작성
	}
	else if(c < 1) { // 백업 파일이 없는 경우
		printf("No backup file\n");
		exit(0);
	}
	else {
		recover_tmp = recover_head->next;
		int cnt = 0;
		printf("backup files of %s\n", recover_tmp->source_path);
		printf("%d. exit\n", cnt);
		while(recover_tmp != NULL) {
			cnt++;
			if(stat(recover_tmp->source_path, &file_stat) < 0) { // stat 함수 예외 처리
				fprintf(stderr, "stat error!\n");
				exit(1);
			}
			printf("%d. %s        %ldbytes\n", cnt, recover_tmp->backup_directory, file_stat.st_size); // 예시 처럼 출력
			recover_tmp = recover_tmp->next;
		} 
		printf(">> ");
		int num = 0;
		scanf("%d", &num); // 사용자에게 숫자 입력
		if(num == 0) exit(0);
		else if(num > cnt) exit(0);
		else {
			struct dirent **file_list;
			int index = 0;
			recover_tmp = recover_head->next;
			while(recover_tmp != NULL) {
				index++;
				if(index == num) {
					ssize_t bytes_read, bytes_write;
					char buf[BUFFER_SIZE];
					struct stat file_stat;
					struct dirent* entry;
					char storage_file[PATHMAX];
					snprintf(storage_file, PATHMAX, "%s/%s", storage_path, recover_tmp->name);

					int fd;
					if((fd = open(storage_file, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) { // 저장할 파일 open, 예외처리
						fprintf(stderr, "open error for %s\n", storage_path);
						exit(1);
					}

					int backup_fd;
					if((backup_fd = open(recover_tmp->dest_path, O_RDONLY)) < 0) { // 백업 내용 오픈, 예외 처리
						perror("open");
						exit(1);
					}
					
					while((bytes_read = read(backup_fd, buf, BUFFER_SIZE)) > 0) { // 백업할 내용 write 예외 처리
						bytes_write = write(fd, buf, bytes_read);
						if(bytes_write != bytes_read) {
							fprintf(stderr, "write error\n");
							exit(1);
						}
					}
		
					if(remove(recover_tmp->dest_path) != 0) { // 파일 삭제, 예외 처리
						perror("remove\n");
						exit(1);
					}
		
					char *home_backup = "/home/backup";
					char recover_dir[DATAMAX];
					strcpy(recover_dir, recover_tmp->dest_path);
					
					while(strcmp(recover_dir, home_backup)) { // 경로가 /home/backup 가 될 때까지 반복
						DIR *dir = opendir(recover_dir); // 경로의 상위 추출
						char *last_slash = strrchr(recover_dir, '/');
						if(last_slash != NULL) {
							*last_slash = '\0';
						}
						int n;
						if((n = scandir(recover_dir, &file_list, NULL, NULL)) < 0) { // 디렉토리 안에 디렉토리, 파일 개수, 예외 처리
							perror("scandir\n");
							exit(1);
						}

						if(n > 2) break;
						else {
							if(rmdir(recover_dir) != 0) { // 디렉토리 삭제
								//fprintf(stderr, "rmdir error!\n");
								//exit(1);
							}
						}
					}
					
					recover_log(recover_tmp->dest_path, storage_file, recover_tmp->backup_directory); // 로그 파일에 작성
					break;			
				}
				recover_tmp = recover_tmp->next;
			}
		}
	}
}

void recover_opt_n(char *recover_path, char *new_path, char *path) { // recover option n 인 경우
	if(access(new_path, F_OK) == 0) { // 접근 가능한 경우
		struct stat file_stat;
		struct dirent** file_list; 
		char recover_storage[DATAMAX];
		Recover_file *recover_tmp = (Recover_file *)malloc(sizeof(Recover_file));
		Backup_linkedlist *tmp_node = (Backup_linkedlist *)malloc(sizeof(Backup_linkedlist));
		
		if(stat(path, &file_stat) < 0) { // stat 함수 예외 처리
			fprintf(stderr, "stat error\n");
			exit(1);
		}
		char *cur = path;
		strcpy(recover_storage, recover_path);

		if(!S_ISDIR(file_stat.st_mode)) { // 디렉토리가 아닌 경우 recover_file_n_file 함수로 처리
			recover_file_n_file(path, recover_path);
			return;
		}
		else {
			if(!strcmp(realpath(new_path, NULL), recover_path)) { // 파일인 경우 경로의 상위 부분 추출
				char *storage_dir = strrchr(path, '/');
				if(storage_dir != NULL) {
					strcat(recover_storage, storage_dir);
				}	
			}
		}

		int n;
		if((n = scandir(path, &file_list, NULL, NULL)) < 0) { // 디렉토리 안에 디렉토리, 파일 개수
			perror("scandir\n");
			exit(1);
		}

		for(int i = 0; i < n; i++) {
			if(strcmp(file_list[i]->d_name, ".") != 0 && strcmp(file_list[i]->d_name, "..") != 0) { // 디렉토리 안 확인
				char check_dir[PATHMAX];
				snprintf(check_dir, PATHMAX, "%s", cur);
				strcat(check_dir, "/");
				strcat(check_dir, file_list[i]->d_name);

				if(stat(check_dir, &file_stat) < 0) { // stat 함수를 통한 예외 처리
					fprintf(stderr, "stat error\n");
					exit(1);
				}
				
				char storage_path[PATHMAX];
				snprintf(storage_path, PATHMAX, "%s/%s", recover_storage, file_list[i]->d_name);
				printf("%s\n", storage_path);

				if(S_ISDIR(file_stat.st_mode)) { // 디렉토리인 경우 디렉토리 생성, 예외 처리
					if(mkdir(storage_path, 0777) != 0) {
						perror("mkdir\n");
						exit(1);
					}
					recover_opt_n(storage_path, check_dir, cur); // recover option n 함수를 통해 처리
				}
				else {
					recover_file_n_file(check_dir, recover_storage); // recover file n file 함수를 통해 처리
				}
			}
		}	
	}
	else {	
		if(realpath(new_path, recover_path) == NULL) { // 절대 경로 획득
			if(mkdir(recover_path, 0777) != 0) { // 디렉토리 생성
				perror("mkdir\n");
				exit(1);
			}	
		} 
		recover_opt_n(recover_path, new_path, path); // recover option n 함수를 통해 처리
		return;
	}
}

void Init() {
	getcwd(exePATH, PATHMAX);
	char *backup = "/home/backup";
	char *backup_log = "home/backup/ssubak.log";

	if(access(backup, F_OK) != 0) { // backup 파일이 없는 경우 생성
		mkdir(backup, 0777);
	}
	
	if(access(backup_log, F_OK) != 0) { // 로그 파일이 없는 경우 생성
		open(backup_log, O_RDONLY | O_WRONLY | O_CREAT | O_TRUNC, 0777);
	}
}

void linked_set(char *backup_route, char *backup_directory) { // 프로그램 시작하면 백업 파일을 통해 링크드 리스트 생성
	char *backup_path = "/home/backup";
	char cur[PATHMAX];
	struct dirent **file_list;
	int n;
	struct stat file_stat;
	if(backup_route == NULL) { // NULL 인 경우
		snprintf(cur, PATHMAX, "%s", backup_path);
	}
	else {
		snprintf(cur, PATHMAX, "%s", backup_route);
	}

	if((n = scandir(cur, &file_list, NULL, NULL)) < 0) { // 경로 안 디렉토리, 파일 개수
		perror("scandir\n");
		exit(1);
	}

	for(int i = 0; i < n; i++) {
		if(strcmp(file_list[i]->d_name, ".") != 0 && strcmp(file_list[i]->d_name, "..") != 0 && strcmp(file_list[i]->d_name, "ssubak.log")) {
			char check_dir[PATHMAX];
			snprintf(check_dir, PATHMAX, "%s", cur);
			strcat(check_dir, "/");
			strcat(check_dir, file_list[i]->d_name);

			if(stat(check_dir, &file_stat) < 0) { // stat 함수 예외 처리
				fprintf(stderr, "stat error\n");
				exit(1);
			}

			if(S_ISDIR(file_stat.st_mode)) { // 디렉토리 인 경우
				if(backup_directory == NULL) { // 백업 디렉토리가 NULL 인 경우 함수의 인자로 넣어서 재귀
					linked_set(check_dir, file_list[i]->d_name);
				}
				else {
					linked_set(check_dir, backup_directory); // 재귀
				}
			}
			else { // 파일인 경우
				Backup_linkedlist *node = (Backup_linkedlist *)malloc(sizeof(Backup_linkedlist));	
				if(node == NULL) {
					fprintf(stderr, "Node empty\n");
					exit(1);
				} // 연결 리스트를 통해 관리, 저장
				if(head->next == NULL) {
					head->next = node;
					tail->next = node;
				}
				else {
					Backup_linkedlist *cur = tail->next;
					cur->next = node;
					tail->next = node;
				}				
				strcpy(node->backup_directory, backup_directory); // 정보 저장
				strcpy(node->backup_fname, file_list[i]->d_name);
				strcpy(node->dest_path, check_dir);

				FILE *log_file = fopen("/home/backup/ssubak.log", "r"); // 로그 파일 오픈
				if(log_file == NULL) { // 예외 처리
					fprintf(stderr, "Log File error!\n");
					exit(1);
				}
				char line[LINEMAX];
				char find_directory[LINEMAX];
				char find_source_path[LINEMAX];
				char find_dest_path[LINEMAX];

				while(fgets(line, LINEMAX, log_file) != NULL) { // 로그 파일 열어서 한 줄씩 반복
					if (sscanf(line, "%s : \"%[^\"]\" backuped to \"%[^\"]\"", find_directory, find_source_path, find_dest_path) == 3) {
						char *file_name = strrchr(find_source_path, '/'); // 문장을 통해 정보 획득
						if(file_name != NULL) {
							if(!strcmp(node->backup_fname, file_name+1) && !strcmp(node->dest_path, find_dest_path)) {
								strcpy(node->source_path, find_source_path);
								break;
							}
						}
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[]) {
	Init(); // 프로그램 시작하면 현재위치, backup file, log file 있는지 확인하고 없으면 생성
	linked_set(NULL, NULL); // backup 디렉토리 내에 모든 백업 파일에 대해 로그 파일 내용과 대조하여 파일들에 대한 백업 상태를 링크드 리스트 생성
	
	if(argc < 2) { // 인자의 개수가 1개 이하면 에러 처리
		fprintf(stderr, "Usage : %s\n", argv[0]);
		exit(1);
	}

	if(!strcmp(argv[0], "command")) {

	}
	else if(!strcmp(argv[1], "help")) { // help 명령어
		help();
	}
	else if(!strcmp(argv[1], "list")) { // list 명령어
		
	}
	else if(!strcmp(argv[1], "backup") || !strcmp(argv[1], "remove") || !strcmp(argv[1], "recover")){
		if(argc < 2) { // 인자 개수 1개 이하면 에러 처리
			fprintf(stderr, "Usage : %s\n", argv[1]);
			exit(1);
		}

		if(!strcmp(argv[1], "backup")) {
			if(argc < 3) { // 인자 개수 2개 이하면 에러 처리
				fprintf(stderr, "Usage : %s\n", argv[1]);
				exit(1);
			}
			char *backup_path = argv[2];
			char *real_path = realpath(backup_path, NULL);
			struct stat file_stat; 
			if(strlen(real_path) > PATHMAX) { // 입력받은 절대 경로가 4096 bytes를 넘기는 경우 예외처리
				fprintf(stderr, "Too Long!\n");
				exit(1);
			}
			
			if (!starts_with_home(real_path)) { // 인자로 입력받은 경로가 사용자의 홈 디렉토리를 벗어날 경우 에러 처리
       				fprintf(stderr, "error for path\n");
     				exit(1);
			}

			if(stat(real_path, &file_stat) < 0) { // real_path를 stat 함수 사용해서 상태확인
				fprintf(stderr, "stat error\n");
				exit(1);
			}

			if (!S_ISDIR(file_stat.st_mode) && !S_ISREG(file_stat.st_mode)) { // 디렉토리 혹은 파일인지 확인
				fprintf(stderr, "Is not file or dir!\n");
				exit(1);
			}

			/*
			if (access(real_path, R_OK | W_OK | X_OK) == -1) { // 입력받은 경로에 대한 접근 권한 확인
				printf("hello\n");	
				exit(1);
			}
			*/
			
			if(argc == 3) {
				if(S_ISDIR(file_stat.st_mode)) { // 경로가 디렉토리면 에러
					fprintf(stderr, "error for DIR\n");
					exit(1);
				}
				backup(backup_path, NULL);
			}
			else {
				int opt;
				bool d = false;
				bool r = false;
				bool y = false;
				while((opt = getopt(argc, argv, "dry")) > 3) {
					switch(opt) {
						case 'd':
							d = true;
							break;
						case 'r':
							r = true;
							break;
						case 'y':
							y = true;
							break;
						default :
							fprintf(stderr, "option error!\n"); // 입력받은 옵션 인자가 올바르지 않은 경우 에러 처리
							exit(1);
							break;
					}
				}
				
				if(d) {
					if(S_ISREG(file_stat.st_mode)) { // 경로가 파일이면 에러
						if(y) {
							backup_opt_y(backup_path, NULL);
						}
						else {
							fprintf(stderr, "error for FILE\n");
							exit(1); 
						}
					}
					if(r) { 
						backup_opt_r(backup_path, NULL);
					}
					else {
						backup_opt_d(backup_path);
					}
				}
				else if(r) {
					if(S_ISREG(file_stat.st_mode)) { // 경로가 파일이면 에러
						if(y) {
							backup_opt_y(backup_path, NULL);
						}
						else {
							fprintf(stderr, "error for FILE\n");
							exit(1);
						}
					}
					else {
						backup_opt_r(backup_path, NULL);
					}
				}
				else if(y) {
					if(S_ISDIR(file_stat.st_mode)) { // 경로가 디렉토리면 에러
						fprintf(stderr, "error for DIR\n");
						exit(1);
					}
					backup_opt_y(backup_path, NULL);
				} 
			}
		}

		else if(!strcmp(argv[1], "remove")) {
			if(argc < 3) { // 인자가 2개 이하로 경로를 입력하지 않았으면 에러 처리
				fprintf(stderr, "Usage : %s\n", argv[1]);
				exit(1);
			}

			char *remove_path = argv[2];
			char *real_path = realpath(remove_path, NULL);
			struct stat file_stat; 
			if(strlen(real_path) > PATHMAX) { // 입력받은 절대 경로가 4096 bytes를 넘기는 경우 예외처리
				fprintf(stderr, "Too Long!\n");
				exit(1);
			}

			if(access(real_path, F_OK) != 0) { // 입력받은 경로와 일치하는 백업 파일이나 디렉토리가 없는 경우 에러 처리
				fprintf(stderr, "No path!\n");
				exit(1);
			}
			
			if (!starts_with_home(real_path)) { // 인자로 입력받은 경로가 사용자의 홈 디렉토리를 벗어날 경우 에러 처리
       				fprintf(stderr, "error for path\n");
     				exit(1);
			}

			if(stat(real_path, &file_stat) < 0) { // real_path를 stat 함수 사용해서 상태확인
				fprintf(stderr, "stat error\n");
				exit(1);
			}

			if (!S_ISDIR(file_stat.st_mode) && !S_ISREG(file_stat.st_mode)) { // 디렉토리 혹은 파일인지 확인
				fprintf(stderr, "Is not file or dir!\n");
				exit(1);
			}

			if(argc == 3) {
				if(S_ISDIR(file_stat.st_mode)) { // 경로가 디렉토리면 에러
					fprintf(stderr, "error for DIR\n");
					exit(1);
				}
				remove_backup_file(remove_path);
			}
			else {
				int opt;
				bool d = false;
				bool r = false;
				bool a = false;
				while((opt = getopt(argc, argv, "dra")) > 3) {
					switch(opt) {
						case 'd':
							d = true;
							break;
						case 'r':
							r = true;
							break;
						case 'a':
							a = true;
							break;
						default :
							fprintf(stderr, "option error!\n"); // 입력받은 옵션이 올바르지 않은 경우 에러 처리
							exit(1);
							break;
					}
				}
				
				if(d) {
					if(S_ISREG(file_stat.st_mode)) { // 경로가 파일이면 에러
						if(a) {
							remove_opt_a(remove_path);
						}
						else {
							fprintf(stderr, "error for FILE\n");
							exit(1);
						}
					}
					else {
						if(r) {
							remove_opt_r(remove_path);
						}
						else {
							remove_opt_d(remove_path);
						}
					}
				}
				else if(r) {
					if(S_ISREG(file_stat.st_mode)) { // 경로가 파일이면 에러
						if(a) {
							remove_opt_a(remove_path);
						}
						else {
							fprintf(stderr, "error for FILE\n");
							exit(1);
						}
					}
					else {
						remove_opt_r(remove_path); 
					}
				}
				else if(a) {
					if(S_ISDIR(file_stat.st_mode)) { // 경로가 디렉토리면 에러
						fprintf(stderr, "error for DIR\n");
						exit(1);
					}
					remove_opt_a(remove_path);		
				}
			}
		}
		else if(!strcmp(argv[1], "recover")) {
			if(argc < 3) { // 인자가 3개 이하면 에러 처리
				fprintf(stderr, "Usage : %s\n", argv[1]);
				exit(1);
			}
			char *recover_path = argv[2];
			char *real_path = realpath(recover_path, NULL);
			struct stat file_stat; 
			if(strlen(real_path) > PATHMAX) { // 입력받은 절대 경로가 4096 bytes를 넘기는 경우 예외처리
				fprintf(stderr, "Too Long!\n");
				exit(1);
			}

			if(access(real_path, F_OK) != 0) { // 입력받은 경로와 일치하는 백업 파일이나 디렉토리가 없는 경우 에러 처리
				fprintf(stderr, "No path!\n");
				exit(1);
			}
			
			if (!starts_with_home(real_path)) { // 인자로 입력받은 경로가 사용자의 홈 디렉토리를 벗어날 경우 에러 처리
       				fprintf(stderr, "error for path\n");
     				exit(1);
			}

			if(stat(real_path, &file_stat) < 0) { // real_path를 stat 함수 사용해서 상태확인
				fprintf(stderr, "stat error\n");
				exit(1);
			}

			if (!S_ISDIR(file_stat.st_mode) && !S_ISREG(file_stat.st_mode)) { // 디렉토리 혹은 파일인지 확인
				fprintf(stderr, "Is not file or dir!\n");
				exit(1);
			}


			if(argc == 3) {
				if(S_ISDIR(file_stat.st_mode)) { // 경로가 디렉토리면 에러
					fprintf(stderr, "error for DIR\n");
					exit(1);
				}
				recover_file(recover_path);
			}
			else {
				int opt;
				bool d = false;
				bool r = false;
				bool l = false;
				bool n = false;
				char *n_path;
				while((opt = getopt(argc, argv, "drln:")) > 4) {
					switch(opt) {
						case 'd':
							d = true;
							break;
						case 'r':
							r = true;
							break;
						case 'l':
							l = true;
							break;
						case 'n':
							n = true;
							n_path = optarg;
							break;
						default :
							fprintf(stderr, "option error!\n"); // 인자로 입력받은 옵션이 올바르지 않은 경우 에러 처리
							exit(1);
							break;
					}
				}
				if(n) {
					char cwd[PATHMAX];
			    		char absolute_path[PATHMAX];
					
					if(n_path == NULL) { // -n 뒤에 경로를 입력하지 않은 경우 에러 처리
						fprintf(stderr, "-n path error\n");
						exit(1);
					} 
 			   		if(getcwd(cwd, sizeof(cwd)) == NULL) {
        					perror("getcwd\n");
   				     		exit(1);
    					}
			    		
					if(realpath(n_path, absolute_path) == NULL) {
						printf("%s\n", absolute_path);
						if(mkdir(absolute_path, 0777) != 0) {
							perror("mkdir\n");
							exit(1);
						}
					}
					char *path = realpath(recover_path, NULL);

					if(d || r) {
						if(S_ISREG(file_stat.st_mode)) { // 경로가 파일이면 에러
							fprintf(stderr, "error for REG\n");
							exit(1);
						}
						recover_opt_n(absolute_path, n_path, path);
					}
					else if(l) {
						if(S_ISDIR(file_stat.st_mode)) { // 경로가 디렉토리면 에러
							fprintf(stderr, "error for DIR\n");
							exit(1);
						}
						recover_opt_n(absolute_path, n_path, path);	
					}
					else {
						recover_opt_n(absolute_path, n_path, path);
					}
				}
				else {
					if(d) {
						if(S_ISREG(file_stat.st_mode)) { // 경로가 파일이면 에러
							if(l) {
								recover_opt_l(recover_path);	
							}
							else {
								fprintf(stderr, "error for REG\n");
								exit(1);
							}
						}
						else {
							if(r) {
								recover_opt_r(recover_path);
							}
							else {
								recover_opt_d(recover_path);
							}
						}
					}
					if(r) {
						if(S_ISREG(file_stat.st_mode)) { // 경로가 파일이면 에러
							if(l) {
								recover_opt_l(recover_path);	
							}
							else {
								fprintf(stderr, "error for REG\n");
								exit(1);
							}
						}
						else {
							recover_opt_r(recover_path);
						}
					}
					if(l) {
						if(S_ISDIR(file_stat.st_mode)) { // 경로가 디렉토리면 에러
							fprintf(stderr, "error for DIR\n");
							exit(1);
						}
						recover_opt_l(recover_path);
					}
				}
			}
		}
	}
	else {
		printf("Input command error!\n"); // 잘못된 내장 명령어 입력 시 에러 처리 후 프로그램 종료
		exit(0);
	}
}
