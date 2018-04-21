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
#include "common.h"

static const char * g_levelToName[] = {"[NOTSET]", "[DEBUG]", "[INFO]", "[WARN]", "[ERROR]", "[FATAL]"};
char  g_logFile[50] = "/tmp/log.txt";

int getCurrentHMS(char res[]){
	if(res == NULL){
		printf("error\n");
		return ERROR;
	}
	time_t now;
	struct tm *tm_now;
	const char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun","Jul", "Aug", "Sep", "Oct", "Nov", "Dec",};
	if ((now = time(NULL)) < 0)
		return ERROR;
	tm_now = localtime(&now);
	sprintf(res, "%3s/%02d/%04d %02d:%02d:%02d", 
			month[tm_now->tm_mon], tm_now->tm_mday,
			tm_now->tm_year + 1900,
			tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
	return SUCCESS;
}

void log_clear(void){
	FILE *fp = fopen(g_logFile, "a+");
    if(!fp){
        printf("open %s failed!\n", g_logFile);
        return;
    }
	unlink(g_logFile);
	return;
}

void log_printf(int level, char *fmt,...){
	FILE *fp = fopen(g_logFile, "a+");
	if(!fp){
		printf("open %s failed!\n", g_logFile);
		return;
	}

	char hms[200] = {0};
	if(getCurrentHMS(hms) == SUCCESS){
		fprintf(fp, "%s ", hms);
	}

    if(level<0 || level > (sizeof(g_levelToName)/sizeof(char *) -1)){
        fprintf(fp, "[OTHER]");
    }else{
        fprintf(fp, g_levelToName[level]);
    }

	char msg[512] = {0};
	va_list args;
	va_start(args, fmt);
	vsnprintf(msg, 512, fmt, args);
	va_end(args);
	fprintf(fp, " %s", msg);
//	printf("%s\n", msg);
	fclose(fp);
	fp = NULL;

	return;
}
