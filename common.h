/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Copyright (C) 2017-2018 ALPHA SOFTWARE 
 *      Author: Jennifer Pan <Jennifer_Pan@alphanetworks.com>
 */
#ifndef __COMMON_H_
#define __COMMON_H_

#include <stdlib.h>
#include <stdio.h>

#define LOG(level, format, ...) do{ log_printf(level, "[%s@%s,%d] " format ,__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__); }while(0)
#define LOG_CRITICAL 5
#define LOG_FATAL LOG_CRITICAL
#define LOG_ERROR 4
#define LOG_WARNING 3
#define LOG_WARN LOG_WARNING
#define LOG_INFO 2
#define LOG_DEBUG 1
#define LOG_NOTSET 0

#define ERROR -1
#define SUCCESS 0
#define TRUE 1
#define FALSE 0

//#define DEBUG 

void log_clear(void);
void log_printf(int level, char *fmt,...);

typedef struct word_s{
	char value[256];
	struct word_s *next;
}word_t;

typedef struct{
	char workdir[512];
	char logfile[512];
	word_t *project[15];
}conf_info_t;

int load_config(u_char *configfile, conf_info_t *conf_info);
void print_config(conf_info_t *conf_info);
conf_info_t *calloc_config(void);
int destory_config(conf_info_t *conf_info);

int getCurrentHMS(char res[]);

#endif
