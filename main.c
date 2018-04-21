@@ -0,0 +1,281 @@
/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Copyright (C) 2017-2018 ALPHA SOFTWARE 
 *      Author: Jennifer Pan <Jennifer_Pan@alphanetworks.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "common.h"

extern char g_logFile[50];
static char g_workpath[128] = "/home/jennifer/temp/";

int create_daemon(void){
	pid_t pid;
	pid = fork();
	if(pid < 0){
		LOG(LOG_FATAL, "fork error when create daemon!\n"); 
		return ERROR;
	}
	if(pid > 0){
		LOG(LOG_INFO, "parent process exit!\n");
		exit(EXIT_SUCCESS);
	}
	if(setsid() == -1){
		LOG(LOG_ERROR, "setsid error!\n");
	}
	if(chdir(g_workpath) == -1){
		LOG(LOG_ERROR, "change path %s error!\n", g_workpath);
	}else{
		LOG(LOG_INFO, "work in path : %s\n", g_workpath);
	}
	int i = 0;
	for(i=0; i<3; ++i){
		close(i);
		open("/dev/null", O_RDWR);
		dup(0);
		dup(0);
	}
	umask(0);
	return SUCCESS;
}


int run_cmd(const char *string){
	LOG(LOG_INFO, "system(%s)\n", string);
	int re = system(string);
	if(re == -1){
		LOG(LOG_ERROR, "system error!\n");
		return ERROR;
	}else{
		if(WIFEXITED(re)){
			if(0 == WEXITSTATUS(re)){
				return SUCCESS;
			}else{
				LOG(LOG_ERROR, "run shell script fail, script exit code %d\n", WEXITSTATUS(re));
				return ERROR;
			}
		}else{
			LOG(LOG_INFO, "exit status = %d\n", WEXITSTATUS(re));
			return SUCCESS;
		}
	}
	LOG(LOG_ERROR, "unexpected!\n");
	return ERROR;
}

void run_sleep(int seconds){
	if(seconds < 0){
		LOG(LOG_ERROR, "seconds %d should be a effective number!\n", seconds);
		return;
	}
	LOG(LOG_DEBUG, "enter sleep! %d\n", seconds);
	sleep(seconds);
	LOG(LOG_DEBUG, "end sleep!\n");
}

static int checktime(char timestr[], int ret[]){
	int h = 0;	//0-23
	int m = 0;	//0-59
	if(sscanf(timestr, "%d:%d", &h, &m) < 2){
		printf("-T time formar error!\n");
		return ERROR;
	}
	if((h>=0) && (h<=23)){
		if((m>=0) && (m<=59)){
			ret[0] = h;
			ret[1] = m;
			return SUCCESS;
		}else{
			printf("-T time minutes range error!\n");
			return ERROR;
		}
	}else{
		printf("-T time hours range error!\n");
		return ERROR;
	}
}

static int g_show_help = 0;
static u_char g_config_file[512] = {0};
static int g_work_time[3] = {0};	//[0]:Hour [1]:Minute [2]:Flag
static int get_options(int argc, char *argv[]){
	u_char *p;
	int i = 0;
	for(i=1; i<argc; i++){
		p = (u_char *)argv[i];
		if (*p++ != '-') {
			printf("invalid option: %s\n", argv[i]);
			return ERROR;
		}
		while(*p){
			switch(*p++){
				case '?':
				case 'h':
					g_show_help = 1;
					break;
				case 'c':
					if(*p){
						sprintf(g_config_file, "%s", p);
						goto next;
					}
					if(argv[++i]){
						sprintf(g_config_file, "%s", (u_char*)argv[i]);
						goto next;
					}
					printf("option \"-c\" requires file name\n");
					return ERROR;
				case 'T':
					if(*p){
						if(checktime((char*)p, g_work_time) == ERROR){
							return ERROR;
						}else{
							g_work_time[2] = 1;
							goto next;
						}
					}
					if(argv[++i]){
						if(checktime(argv[i], g_work_time) == ERROR){
                            return ERROR;
                        }else{
							g_work_time[2] = 1;
                            goto next;
                        }
					}
					printf("option \"-c\" requires parameters, like hh:mm.\n");
					return ERROR;
				default:
					printf("invalid option:\"%c\"\n", *(p-1));
					return ERROR;
			}
		}
next:
		continue;
	}
	if(g_show_help == 1){
		return SUCCESS;
	}else{
		if(strlen(g_config_file) <=0 ){
			printf("option \"-c\" needed for load configure file.\n");
			return ERROR;
		}else{
			return SUCCESS;
		}
	}
}

#define LINEFEED   "\x0a"
static void show_help(void){
	if(g_show_help){
		printf("Options:"LINEFEED
				"  -?,-h         : Print help" LINEFEED
				"  -c file       : -Must     Use an alternative configuration file. " LINEFEED
				"  -T hh:mm      : -Optional Define activity time. like 09:21. " LINEFEED);
	}
	return ;
}

conf_info_t *g_conf_info = NULL;

static int init(int argc, char *argv[]){
    if(get_options(argc, argv) != SUCCESS){
        return ERROR;
    }
    if(g_show_help){
        show_help();
        return SUCCESS;
    }

    g_conf_info = calloc_config();
    if(load_config(g_config_file, g_conf_info) != SUCCESS){
        return ERROR;
    }
    sprintf(g_workpath, "%s", g_conf_info->workdir);
    sprintf(g_logFile, "%s", g_conf_info->logfile);
	log_clear();
    return SUCCESS;
}

int run_project(word_t *project){
	word_t *p = project;
	char tmp[512] = {0};

	if(chdir(g_workpath) == -1){
		LOG(LOG_ERROR, "chdir(%s) failed!\n", g_workpath);
		return ERROR;
	}else{
		LOG(LOG_INFO, "chdir(%s)!\n", g_workpath);
	}
	
	int ret = SUCCESS;
	while(p != NULL){
		ret = SUCCESS;
		if(strstr(p->value, "cd ")){
			sprintf(tmp, "%s%s", g_workpath, p->value+3);
			if(chdir(tmp) == -1){
				LOG(LOG_ERROR, "chdir(%s) failed!\n", tmp);
				ret = ERROR;
			}else{
				LOG(LOG_INFO, "chdir(%s)!\n", tmp);
			}
		}else{
			ret = run_cmd(p->value);
		}
		p = p->next;
	}
	return ret;
}

int main(int argc, char *argv[])
{	
	if(init(argc, argv) != SUCCESS){
		LOG(LOG_ERROR, "failed when init!\n");
		return ERROR;
	}
#ifdef DEBUG
    printf("H=%d; M=%d; Flag=%d\n", g_work_time[0], g_work_time[1], g_work_time[2]);
#endif
#ifndef DEBUG
	if(create_daemon() != SUCCESS){
		LOG(LOG_ERROR, "failed when create daemon!\n");
		return ERROR;
	}
#endif

	if(g_work_time[2]){
		printf("delay to %02d:%02d to excute!\n", g_work_time[0], g_work_time[1]);
		char tmp_str[100] = {0};
		getCurrentHMS(tmp_str);
		printf("%s\n", tmp_str);
	}

	int i = 0;
	for(;;){
#ifdef DEBUG
		printf("should excute, but we stop it!\n");
		break;
#endif
		if(g_conf_info->project[i] == NULL){
			break;
		}
		if(run_project(g_conf_info->project[i]) == SUCCESS){
			LOG(LOG_INFO, "run %d success!\n\n", i);
		}else{
			LOG(LOG_ERROR, "run %d failed!\n\n", i);
		}
		i+=1;
	//	break;
	}
	return SUCCESS;
}
