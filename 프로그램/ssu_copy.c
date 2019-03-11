#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <utime.h>
#include <sys/wait.h>

#define DIRECTORY_SIZE MAXNAMLEN

void file_cp(char*, char* ,int, int); //기본적인 파일 복사 함수
int get_filetype(char *); //파일의 형식을 파악하는 함수
void flag_on(char); //옵션사용시 flag값을 바꿔주는 함수
void dir_cp(char *, char*, int, int, int, int, int); //디렉토리 복사 함수
void print_info(char*, struct stat); //-p옵션을 위한 파일 정보 출력
char* to_abs_path(char *); // 상대경로 파일을 절대경로로 바꿔주는 함수
char* add_path(char*,char*); // path와 filename이 주어질시 path/filename로 바꿔주는 함수
char* get_filename(char*);// path/filename이 주어질시 filename을 추출하느 함수
void paste_info(struct stat,char*); //-l타입을 위한 복사파일의 파일 정보를 수정해주는 함수
int flag_i=0,flag_n=0,flag_r=0,flag_l=0,flag_p=0,flag_s=0,flag_d=0; //각 옵션들에대한 flag
void error_meg(); //사용법 출력 함수
int issame(char*,char*);
int d_num,status;
char source[PATH_MAX];
char target[PATH_MAX];
int main(int argc, char * argv[])
{
	
	struct stat stat_buf;
	int option;
	while ((option = getopt(argc,argv,"sinlprd:SINLPRD:")) != -1) { 
		switch(option){
			case 'S':
			case 's':
				flag_on('s');
				break;
			case 'I':
			case 'i':
				flag_on('i');
				break;
			case 'N':
			case 'n': 
				flag_on('n');
				break;
			case 'L':
			case 'l':
				flag_on('l');
				break;
			case 'P':
			case 'p':
				flag_on('p');
				break;
			case 'R':
			case 'r':
				flag_on('r');
				break;
			case 'D':
			case 'd':
				flag_on('d');
				d_num = atoi(optarg);
				break;
			case '?':
				exit(1);
				break;
		}
	}
	if(optind +2 > argc){ //소스나 타겟이 없는 경우
		fprintf(stderr,"no source or target\n");
		error_meg();
		exit(1);
	}
	else if(optind +2 < argc){ // 인자가 3개이상 오는 경우
		fprintf(stderr,"too many argument\n");
		error_meg();
		exit(1);
	}
	for (int index = optind ; index < argc ; index++) {
		if (argv[index][0] == '-') {
			break;
		}
		else{
			strcpy(source,argv[index]); 
			strcpy(target,argv[index+1]);
			index++;
		}
	}
	char * abs_source = to_abs_path(source);// 소스를 절대 경로화 시킴
	char * abs_target = to_abs_path(target); // 타겟을 절대 경로화 시킴
	if(access(abs_source,F_OK)<0){ //소스파일이 존재유무 검사
		fprintf(stderr,"source file not exist\n");
		error_meg();
		exit(1);
	}
	if(issame(abs_source,abs_target)==1){
		fprintf(stderr,"source and target are same\n");
		error_meg();
		exit(1);
	}
	lstat(abs_source,&stat_buf); //소스의 파일 정보를 구조체에 저장
	if(flag_s == 1){
		if(flag_r == 1 || flag_d == 1 || flag_i ==1 || flag_n == 1 || flag_l == 1 || flag_p == 1){ //s옵션 사용시 다른 옵션이 같이 오는 경우
			fprintf(stderr,"other options can't come with s option\n");
			error_meg();
			exit(1);
		}
		if(get_filetype(abs_source) == 1||get_filetype(abs_source) == 3){ //일반파일 or 링크파일인경우
			if(symlink(get_filename(abs_source),get_filename(abs_target)) < 0){ //symbolic link 파일 생성
				if(access(abs_target,F_OK)==0){ //이미 같은 이름의 파일이 존재하는 경우 삭제 후 symbolic link 파일 생성
					remove(abs_target);
					symlink(get_filename(abs_source),get_filename(abs_target));
				}
			}
		}
	}
	
	else{ //s 옵션 이외의 옵션
		if(get_filetype(abs_source) == 1 || get_filetype(abs_source) == 3){//소스 파일이 일반파일 or 링크파일인 경우
			if(flag_r == 1 || flag_d == 1){ 
				fprintf(stderr,"file didn't come with d or r option\n");
				error_meg();
				exit(1);
			}
			if(flag_i == 1 && flag_n == 1){
				fprintf(stderr,"i option and n option can't come together\n");
				error_meg();
				exit(1);
			}
			file_cp(abs_source,abs_target,flag_i,flag_n); //일반 파일 복사
		}
		else if(get_filetype(abs_source) == 2){ //소스가 디렉토리인경우
			if(flag_i == 1 && flag_n == 1){
				fprintf(stderr,"i option and n option can't come together\n");
				error_meg();
				exit(1);
			}
			if(flag_r == 1 && flag_d == 1){
				fprintf(stderr,"r option and d option can't come together\n\n");
				error_meg();
				exit(1);
			}
			if(flag_r == 0 && flag_d == 0){
				fprintf(stderr,"directory copy option didn't exist\n");
				error_meg();
				exit(1);
			}
			dir_cp(abs_source,abs_target,flag_i,flag_n,flag_r,flag_d,d_num); //디렉토리 복사
		
		}
		else{ // 일반파일도 아니고 디렉토리도 아닌 경우
			fprintf(stderr,"it's not file format\n");
			error_meg();
			exit(1);
		}
	}
	wait(&status);
	printf("target : %s\n",target);
	printf("src : %s\n",source);
	if(flag_l == 1){ 
		paste_info(stat_buf,abs_target); //l옵션이 온 경우 파일의 정보를 바꿔줌
	}
	if(flag_p == 1){ 
		print_info(abs_source,stat_buf); //p옵션이 온 경우 파일 정보 출력
	}
}

void file_cp(char* abs_source, char* abs_target,int iflag,int nflag){
	char buf[BUFSIZ];
	int count;
	int fd_t,fd_s;
	struct stat s_stat;
	char c;
	fd_s = open(abs_source,O_RDONLY);
	if(get_filetype(abs_target) == 2){ //타겟으로 주어진 파일명을 가진 디렉토리가 존재하는 경우 그 디렉토리 안에 파일 복사
		char* new_target = add_path(abs_target,target);
		file_cp(abs_source,new_target,iflag,nflag);
	}
	else{ //그 외에 경우
		if(iflag == 1){ //i option이 온 경우
			if(access(abs_target,F_OK)==0){ //타겟파일이 이미 존재하는 경우 overwrite 여부를 물어봄
				printf("overwrite %s (y/n)? ",target);
				scanf("%c",&c);
				if(c=='y'||c=='Y'){
					fd_t=open(abs_target,O_WRONLY|O_TRUNC);
					stat(abs_source,&s_stat);
					fchmod(fd_t,s_stat.st_mode); //기존 파일의 실행권한으로 새 파일 생성
					while((count = read(fd_s,buf,BUFSIZ)) > 0){
						write(fd_t,buf,count);
					}
				}
			}
			else{ // 타겟 파일이 존재하지 않는 경우
				fd_t=open(abs_target,O_WRONLY|O_CREAT,0644);
				stat(abs_source,&s_stat);
				fchmod(fd_t,s_stat.st_mode);
				while((count = read(fd_s,buf,BUFSIZ)) > 0){
					write(fd_t,buf,count);
				}
			}
		}
		else if(nflag == 1){ //n option이 온 경우
			if(access(abs_target,F_OK)!=0){ //파일이 존재하지 않는 경우 overwrite 복사
				fd_t=open(abs_target,O_WRONLY|O_CREAT,0644);
				stat(abs_source,&s_stat);
				fchmod(fd_t,s_stat.st_mode);
				while((count = read(fd_s,buf,BUFSIZ)) > 0){
					write(fd_t,buf,count);
				}
			}
		}
		else{ //아무옵션이 오지 않는경우 overwrite
			fd_t=open(abs_target,O_WRONLY|O_CREAT|O_TRUNC,0644);
			stat(abs_source,&s_stat);
			fchmod(fd_t,s_stat.st_mode);
			while((count = read(fd_s,buf,BUFSIZ)) > 0){
				write(fd_t,buf,count);
			}
		}		
	}

}

void dir_cp(char* abs_source, char*abs_target,int iflag,int nflag,int rflag,int dflag,int d_num){
	DIR * dirp = opendir(abs_source);
	struct dirent *dentry;
	char filename[PATH_MAX+1];
	char * file_path_s;
	char * file_path_t;
	char c;
	if(iflag == 1){ //i 옵션이 올시 overwrite 여부 확인
		if(access(abs_target,F_OK)==0){
			printf("overwrite %s (y/n)? ",target);
			scanf("%c",&c);
			if(c!='y'&&c!='Y'){
				return;
			}
		}
	}
	else if(nflag == 1){ //n옵션이 올시 타겟이 존재하는 경우 복사 실행 x
		if(access(abs_target,F_OK)==0){
			return;
		}
	}
	if(mkdir(abs_target,0775) < 0){ //폴더 생성이 실패하는 경우
		if(access(abs_target,F_OK)!=0){ //경로가 존재하지 않는 경우
			fprintf(stderr,"path didn't exist\n");
			error_meg();
			exit(1);
		}
		else if(get_filetype(abs_target)==1 || get_filetype(abs_target)==3){ //동일한 이름을 가진 파일이 존재하는 경우
			fprintf(stderr,"same name file exist\n");
			error_meg();
			exit(1);			
		}
	}
	if(rflag == 1){ //r옵션이 오는 경우 
		while((dentry = readdir(dirp)) != NULL){ //폴더내에 파일, 디렉토리 명을 순차적으로 받아옴
			if(dentry->d_ino == 0){
				continue;
			}
			if(strcmp(dentry->d_name,".")==0 || strcmp(dentry->d_name,"..")==0){
				continue;
			}
			memcpy(filename, dentry->d_name, PATH_MAX); 
			file_path_s = add_path(abs_source,filename); //파일을 절대 경로화 시킴
			file_path_t = add_path(abs_target,filename); //파일을 절대 경로화 시킴
			if(get_filetype(file_path_s) == 1 || get_filetype(file_path_s) == 3){ //일반 파일이거나 링크 파일일 경우 파일 복사
				file_cp(file_path_s,file_path_t,0,0);
			}
			else if(get_filetype(file_path_s) == 2){ //디렉토리일경우 제귀 호출
				dir_cp(file_path_s,file_path_t,0,0,1,0,0);
			}
			memset(filename,0,PATH_MAX);
		}
	}
	else if(dflag == 1){ //d옵션이 오는 경우
		struct dirent **namelist;
		int count;
		int i=0;
		int status;
		int temp;
		if(d_num < 1 || d_num > 10){
			fprintf(stderr,"N can be 1~10\n");
			error_meg();
			exit(1);
		}
		int dir_list[10];
		pid_t pids[d_num];
		int run_process = 0;
		if((count = scandir(abs_source, &namelist, NULL, alphasort)) == -1){ //폴더 내에 파일 목록을 구조체에 저장시킴
			fprintf(stderr,"scandir error\n");
			error_meg();
			exit(1);
		}
		for(int j = 0 ; j < 10 ; j++){
			dir_list[j] = 0;
		}
		for(int j = 0 ; j < count ; j++){ //파일 목록에서 디렉토리 인경우 dir_list에 맵핑함
			if(strcmp(namelist[j]->d_name,".")==0 || strcmp(namelist[j]->d_name,"..")==0){
				continue;
			}
			if(get_filetype(add_path(abs_source,namelist[j]->d_name))==2){
				dir_list[i]=j;
				i++;
			}
		}
		i=-1;
		while(run_process < d_num){
			i++;
			pids[run_process] = fork(); //자식 프로세스 생성
			if(pids[run_process] < 0){
				exit(1);
			}
			else if(pids[run_process] == 0){ //자식은 dir_list를 통해서 폴더를 복사함
				if(dir_list[i] != 0){
					dir_cp(add_path(abs_source,namelist[dir_list[i]]->d_name),add_path(abs_target,namelist[dir_list[i]]->d_name),0,0,1,0,0);
					printf("directory name : %s pid : %ld\n",namelist[dir_list[i]]->d_name,(long)getpid());
				}
				exit(1);
			}
			run_process++;
		}
		if(dir_list[d_num-1] == 0){ //자식이 최종적으로 복사한 위치 파악
			for(temp = 0 ; temp < d_num ; temp++){
				if(dir_list[temp] == 0)
					break;
			}
			temp = dir_list[temp - 1];
		}
		else{
			temp = dir_list[d_num-1];
		}
		wait(&status);
		for(i = 0 ; i < count ; i++){ //파일의 경우 부모프로세스에서 복사 디렉토리인 경우 자식이 하지 않은 디렉토리만 복사
			if(strcmp(namelist[i]->d_name,".")==0 || strcmp(namelist[i]->d_name,"..")==0){
				continue;
			}
			if(get_filetype(add_path(abs_source,namelist[i]->d_name)) == 1 || get_filetype(add_path(abs_source,namelist[i]->d_name)) == 3){
				file_cp(add_path(abs_source,namelist[i]->d_name),add_path(abs_target,namelist[i]->d_name),0,0);
			}
			else if(get_filetype(add_path(abs_source,namelist[i]->d_name)) == 2){
				if(i>temp){
					dir_cp(add_path(abs_source,namelist[i]->d_name),add_path(abs_target,namelist[i]->d_name),0,0,1,0,0);
					printf("directory name : %s pid : %ld\n",namelist[i]->d_name,(long)getpid());
				}
			}
		}
	}
	
}

// 1:기본파일 2:디렉토리 3:심볼릭 링크
int get_filetype(char * file){ //파일타입 파악 함수
	struct stat stat_buf;
	if(!access(file,F_OK)){
		lstat(file,&stat_buf);
		if(S_ISREG(stat_buf.st_mode)||S_ISCHR(stat_buf.st_mode)||S_ISBLK(stat_buf.st_mode)||S_ISFIFO(stat_buf.st_mode)||S_ISSOCK(stat_buf.st_mode)){
			return 1;
		}
		else if(S_ISDIR(stat_buf.st_mode)){
			return 2;
		}
		else if(S_ISLNK(stat_buf.st_mode)){
			return 3;
		}	
	}
	return -1; //파일이 존재하지 않는 경우 -1
}

void flag_on(char flag){ //옵션 flag on시켜주는 함수
	switch(flag){
			case 's': 
				if(flag_s == 1){
					fprintf(stderr,"s option is already on\n");
					error_meg();
					exit(1);
				}
				else{
					printf("s option is on\n");
					flag_s=1;
				}
				break;
			case 'i':
				if(flag_i == 1){
					fprintf(stderr,"i option is already on\n");
					error_meg();
					exit(1);
				}
				else{
					printf("i option is on\n");
					flag_i=1;
				}
				break;
			case 'n': 
				if(flag_n == 1){
					fprintf(stderr,"n option is already on\n");
					error_meg();
					exit(1);
				}
				else{
					printf("n option is on\n");
					flag_n=1;
				}
				break;
			case 'l':
				if(flag_l == 1){
					fprintf(stderr,"l option is already on\n");
					error_meg();
					exit(1);
				}
				else{
					printf("l option is on\n");
					flag_l=1;
				}
				break;
			case 'p':
				if(flag_p == 1){
					fprintf(stderr,"p option is already on\n");
					error_meg();
					exit(1);
				}
				else{
					printf("p option is on\n");
					flag_p=1;
				}
				break;
			case 'r':
				if(flag_r == 1){
					fprintf(stderr,"r option is already on\n");
					error_meg();
					exit(1);
				}
				else{
					printf("r option is on\n");
					flag_r=1;
				}
				break;
			case 'd':
				if(flag_d == 1){
					fprintf(stderr,"d option is already on\n");
					error_meg();
					exit(1);
				}
				else{
					printf("d option is on\n");
					flag_d=1;
				}
				break;
		}
}

void print_info(char* abs_source,struct stat stat_buf){ //소스 파일의 정보를 출력해주는 함수
	struct group *group_info;
	struct passwd *user_info;
	struct tm *tm_p;
	group_info = getgrgid(stat_buf.st_gid);
	user_info = getpwuid(stat_buf.st_uid);
	printf("file name : %s\n",get_filename(abs_source));
	tm_p = localtime(&stat_buf.st_atime);
	printf("last access time : %d.%d.%d %d:%d:%d\n", tm_p -> tm_year+1900,tm_p -> tm_mon+1, tm_p -> tm_mday, tm_p -> tm_hour, tm_p -> tm_min, tm_p -> tm_sec);
	tm_p = localtime(&stat_buf.st_mtime);
	printf("last modify time : %d.%d.%d %d:%d:%d\n", tm_p -> tm_year+1900,tm_p -> tm_mon+1, tm_p -> tm_mday, tm_p -> tm_hour, tm_p -> tm_min, tm_p -> tm_sec);
	tm_p = localtime(&stat_buf.st_ctime);
	printf("last change time : %d.%d.%d %d:%d:%d\n", tm_p -> tm_year+1900,tm_p -> tm_mon+1, tm_p -> tm_mday, tm_p -> tm_hour, tm_p -> tm_min, tm_p -> tm_sec);	
	printf("GROUP : %s\n",group_info -> gr_name);
	printf("OWNER : %s\n",user_info -> pw_name);
	printf("file size : %ld\n",stat_buf.st_size);
}
void paste_info(struct stat stat_buf, char* abs_target){ //타겟 파일의 정보를 변경해주는 함수
	if(S_ISREG(stat_buf.st_mode) && get_filetype(abs_target) == 2){
		add_path(abs_target,get_filename(abs_target));
	}
	struct utimbuf time_buf;
	chmod(abs_target,stat_buf.st_mode); 
	chown(abs_target,stat_buf.st_uid,stat_buf.st_gid);
	time_buf.actime = stat_buf.st_atime;
	time_buf.modtime = stat_buf.st_mtime;
	utime(abs_target,&time_buf);
}

char* to_abs_path(char * path){ //상대경로를 절대경로로 바꿔주는 함수
	char *abs_path = malloc(PATH_MAX);
	if(path[0] == '/'){
		return path;
	}
	else{
		getcwd(abs_path,PATH_MAX);
		abs_path[strlen(abs_path)] ='/';
		abs_path[strlen(abs_path)+1] ='\0';
		strcat(abs_path,path);
		return abs_path;
	}
}
char* add_path(char* path,char*filename){ //경로에 파일명을 더해주는 함수
	char* result=malloc(PATH_MAX);
	strcpy(result,path);
	if(result[strlen(result)-1] =='/'){
		strcat(result,filename);
		return result;
	}
	else{
		result[strlen(result)] ='/';
		result[strlen(result)+1] ='\0';
		strcat(result,filename);
		return result;
	}
}
char* get_filename(char* abs_path){ //경로에서 파일명을 추출하는 함수
	char* result = malloc(PATH_MAX);
	char * temp = malloc(PATH_MAX);
	strcpy(temp,abs_path);
	
	if(temp[strlen(temp)-1] == '/'){
		temp[strlen(temp)-1] = '\0';
	}
	int temp_num=0;
	for(int i = 0 ; i < strlen(temp) ; i++){
		if(temp[i] == '/'){
			temp_num = i;
		}
	}
	strcpy(result,&temp[temp_num+1]);
	return result;
}

void error_meg(){ //사용법 출력 함수
	printf("ssu_cp error\n");
	printf("cp [-i/n][-l][-p]    [source][target]\nor cp [-s][source][target]\nin case of directory cp [-i/n][-l][-p][-r][-d][N]\n");
}

int issame(char * abs_source,char* abs_target){
	struct stat stat_s,stat_t;
	if(access(abs_target,F_OK)==0){
		lstat(abs_target,&stat_t);
		lstat(abs_source,&stat_s);
		if(stat_s.st_ino == stat_t.st_ino){
			return 1;
		}
		else{
			return -1;
		}
	}
	return -1;
}
