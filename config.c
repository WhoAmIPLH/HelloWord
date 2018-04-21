/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2017-2018 ALPHA SOFTWARE 
 *		Author: Jennifer Pan <Jennifer_Pan@alphanetworks.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <bits/types.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"

#define LF     (u_char) '\n'
#define CR     (u_char) '\r'
#define CRLF   "\r\n"

#define CONF_BLOCK_START	1
#define CONF_BLOCK_DONE		2
#define CONF_FILE_DONE		3
#define CONF_BUFFER			4096

typedef struct buf_s {
	u_char *pos;
	u_char *last;
	off_t file_pos;
	off_t file_last;
	u_char *start;
	u_char *end;
}buf_t;

typedef struct file_s {
	int fd;
	char *name;
	struct stat info;
	off_t offset;
	off_t sys_offset;
	unsigned long int line;
	buf_t *buffer;
	word_t *pool;
}file_t;

static int push_wordList(word_t *head, word_t *new){
	if(head == NULL || new == NULL){
		printf("the second parameter cannot be empty!\n");
		return ERROR;
	}
	word_t *temp = head;
	while(temp->next != NULL){
		temp = temp->next;
	}
	temp->next = new;
	return SUCCESS;
}

static int clear_wordList(word_t *head){
	if(head == NULL){
		return ERROR;
	}
	word_t *p, *q;
	p = head->next;
	while(p != NULL){
		q = p->next;
		free(p);
		p = q;
	}
	head->next = NULL;
	return SUCCESS;
}

static int destory_wordList(word_t *head){
	if(head == NULL){
		return ERROR;
	}
	word_t *p;
	while(head){
		p = head->next;
		free(head);
		head = p;
	}
	return SUCCESS;
}

static int print_wordList(word_t *head){
	word_t *p = head;
	while(p != NULL){
		LOG(LOG_INFO, "\t[%s]\n", p->value);
		p = p->next;
	}
}
static ssize_t read_file(file_t *f, u_char *buf, size_t size, off_t offset) {
	ssize_t n;
	if(f->sys_offset != offset) {
		if(lseek(f->fd, offset, SEEK_SET) == -1) {
			printf("errno=%d when lseek!\n", errno);
			return ERROR;
		}
		f->sys_offset = offset;
	}
	n = read(f->fd, buf, size);
	if(n == -1) {
		printf("errno=%d when read file!\n", errno);
		return ERROR;
	}
	f->sys_offset += n;
	f->offset += n;
	return n;
}

static int conf_read_token(file_t *f){
	if(f == NULL){
		printf("parameters cannot be empty!\n");
		return ERROR;
	}
	u_char      *start, ch, *src, *dst;
	off_t        file_size;
	size_t       len;
	ssize_t      n, size;
	unsigned long int   found, need_space, last_space, sharp_comment, variable;
	unsigned long int   quoted, s_quoted, d_quoted, start_line;
	word_t *word;
	buf_t *b;
	
	found = 0;
	need_space = 0;
	last_space = 1;
	sharp_comment = 0;
	variable = 0;
	quoted = 0;
	s_quoted = 0;
	d_quoted = 0;

	b = f->buffer;
	start = b->pos;
	start_line = f->line;
	
	file_size = (&f->info)->st_size;

	int i = 0;
	for(;;){
		if(b->pos >= b->last){
			if(f->offset >= file_size){
				return CONF_FILE_DONE;
			}
			len = b->pos - start;
			if(len == CONF_BUFFER){
				f->line = start_line;
				if (d_quoted){
					ch = '"';
				}else if(s_quoted){
					ch = '\'';
				}else{
					printf("unexpexted char \"%c\"", ch);
					return ERROR;
				}
			}
			if(len){
				memmove(b->start, start, len);
			}
			size = (ssize_t)(file_size - f->offset);
			if (size > b->end - (b->start + len)) {
				size = b->end - (b->start + len);
			}
			n = read_file(f, b->start+len, size, f->offset);
			if(n == ERROR){
				printf("read file error!\n");
				return ERROR;
			}

			if(n != size){
				printf("returned only %z bytes instead of %z\n", n, size);
				return ERROR;
			}
			b->pos = b->start + len;
			b->last = b->pos + n;
			start = b->start;
		}
		ch = *b->pos++;
		if (ch == LF) {
			f->line++;
			if (sharp_comment){
				sharp_comment = 0;
			}
		}
		if(sharp_comment){
			continue;
		}
		if(quoted){
			quoted = 0;
			continue;
		}
		if(need_space){
			if (ch == ' ' || ch == '\t' || ch == CR || ch == LF){
				last_space = 1;
				need_space = 0;
				continue;
			}
			if (ch == ';'){
				return SUCCESS;
			}
			if (ch == '{'){
				return CONF_BLOCK_START;
			}
			if (ch == ')'){
				last_space = 1;
				need_space = 0;
			}else{
				printf("unexpected %c\n", ch);
				return ERROR;
			}
		}
		if(last_space){
			if (ch == ' ' || ch == '\t' || ch == CR || ch == LF) {
				continue;
			}
			start = b->pos - 1;
			start_line = f->line;
			switch(ch){
				case ';':
				case '{':
					if(ch == '{'){
						return CONF_BLOCK_START;
					}
					return SUCCESS;
				case '}':
					return CONF_BLOCK_DONE;
				case '#':
					sharp_comment = 1;
					continue;
				case '\\':
					quoted = 1;
					last_space = 0;
					continue;
				case '"':
					start++;
					d_quoted = 1;
					last_space = 0;
					continue;
				case '\'':
					start++;
					s_quoted = 1;
					last_space = 0;
					continue;
				default:
					last_space = 0;
			}
		}else{
			if (ch == '{' && variable){
				continue;
			}
			variable = 0;
			if (ch == '\\') {
				quoted = 1;
				continue;
			}
			if (ch == '$'){
				variable = 1;
				continue;
			}
			if (d_quoted){
				if (ch == '"'){
					d_quoted = 0;
					need_space = 1;
					found = 1;
				}
			}else if(s_quoted){
				if (ch == '\''){
					s_quoted = 0;
					need_space = 1;
					found = 1;
				}
			}else if(ch == ' ' || ch == '\t' || ch == CR || ch == LF || ch == ';' || ch == '{'){
				last_space = 1;
				found = 1;
			}
			if(found){
				word = calloc(1, sizeof(word_t));
				if(word == NULL){
					printf("errno=%d,when calloc\n", errno);
					return ERROR;
				}
				for(dst = word->value, src=start, len=0; src < b->pos - 1; len++){
					if(*src == '\\'){
						switch(src[1]){
							case '"':
							case '\'':
							case '\\':
								src++;
								break;
							case 't':
								*dst++ = '\t';
								src += 2;
								continue;
							case 'r':
								*dst++ = '\r';
								src += 2;
								continue;
							case 'n':
								*dst++ = '\n';
								src += 2;
								continue;
						}
					}
					*dst++ = *src++;
				}
				*dst = '\0';
				if(push_wordList(f->pool, word) == ERROR){
					return ERROR;
				}
				if(ch == ';'){
					return SUCCESS;
				}
				if(ch == '{'){
					return CONF_BLOCK_START;
				}
				found = 0;
			}
		}
	}
	return SUCCESS;
}

static int g_index = 0;

static int conf_handler(word_t *head, conf_info_t *conf_info){
	if(head == NULL || head->next == NULL || conf_info == NULL){
		printf("parameters or first node cannot be empty!\n");
		return ERROR;
	}
	word_t *first = head->next;
	if(!strcmp(first->value, "workdir")){
		sprintf(conf_info->workdir, "%s", first->next->value);
		clear_wordList(head);
		return SUCCESS;
	}else if(!strcmp(first->value, "logfile")){
		sprintf(conf_info->logfile, "%s", first->next->value);
		clear_wordList(head);
		return SUCCESS;
	}else if(!strcmp(first->value, "project")){
		conf_info->project[g_index] = first->next;
		free(first);
		first = NULL;
		head->next = NULL;
		g_index += 1;
		return SUCCESS;
	}else{
		return ERROR;
	}
	printf("unexpected!\n");
	return SUCCESS;
}

conf_info_t *calloc_config(void){
	conf_info_t *conf_info = (conf_info_t *)calloc(1, sizeof(conf_info_t));
	if(conf_info == NULL){
		printf("errno=%d when calloc\n", errno);
		return NULL;
	}
	return conf_info;
}

int destory_config(conf_info_t *conf_info){
	if(conf_info == NULL){
		return ERROR;
	}

	int i = 0;
	for(i=0; i<g_index; i++){
		if(destory_wordList(conf_info->project[i]) == SUCCESS){
			conf_info->project[i] = NULL;
		}else{
			LOG(LOG_ERROR, "failed when destory checkcode!\n");
		}
	}

	free(conf_info);
	conf_info = NULL;

	return SUCCESS;
}

int load_config(u_char *file, conf_info_t *conf_info){
	if(file == NULL || conf_info == NULL){
		printf("parameters cannot be empty!\n");
		return ERROR;
	}

	g_index = 0;

	int fd = 0;
	file_t f;
	buf_t buf;
	if((fd = open((const char *)file, O_RDONLY, 0 )) == -1){
		printf("errno=%d when open file %s.\n", errno, file);
		goto failed;
	}
	
	f.pool = calloc(1, sizeof(word_t));
	if(f.pool == NULL){
		printf("errno=%d when calloc!\n", errno);
		goto failed;
	}
	f.buffer = &buf;
	buf.start = malloc(CONF_BUFFER);
	if(buf.start == NULL){
		printf("errno=%d when calloc!\n", errno);
		goto failed;
	}
	buf.pos = buf.start;
	buf.last = buf.start;
	buf.end = buf.last + CONF_BUFFER;
	f.fd = fd;
	f.name = file;
	f.offset = 0;
	f.line = 1;
	if(fstat(fd, &f.info) == -1){
		printf("errno = %d when get file info!\n", errno);
		goto failed;
	}
	long int rc = 0;
	int block_start = -1;
	int block_done = -1;
	for(;;){
		rc = conf_read_token(&f);
		if(rc == ERROR){
			goto failed;
		}else if(rc == CONF_BLOCK_DONE){
			if(block_start!=1){
				printf("abundant \"}\"\n");
				goto failed;
			}
			block_start = 0;
			block_done = 1;
			if(conf_handler(f.pool,conf_info) != SUCCESS){
				printf("something wrong!\n");
				goto failed;
			}
		}else if(rc == CONF_FILE_DONE){
			goto done;
		}else if(rc == CONF_BLOCK_START){
			if(block_done == 0){
				printf("abundant \"{\"\n");
				goto failed;
			}
			block_start = 1;
			block_done = 0;
		}else if(rc == SUCCESS){
			if(block_start == 1 && block_done == 0){
			}else{
				if(conf_handler(f.pool,conf_info) != SUCCESS){
					printf("something wrong!\n");
					goto failed;
				}
			}
		}else{
			printf("unexpected rc=%d\n", rc);
			goto failed;
		}
	}

failed:
	rc = ERROR;
done:
	if(fd <= 0){
		close(fd);
		fd = 0;
	}
	destory_wordList(f.pool);
	if(buf.start != NULL){
		free(buf.start);
		buf.start = NULL;
	}

	if(rc == ERROR){
		return ERROR;
	}else{
		return SUCCESS;
	}
}

void print_config(conf_info_t *conf_info){
	if(conf_info == NULL){
		LOG(LOG_ERROR, "cannot be null!\n");
		return ;
	}
	LOG(LOG_INFO, "workdir = %s\n", conf_info->workdir);
	LOG(LOG_INFO, "logfile = %s\n", conf_info->logfile);

	int i = 0;
	for(i=0; i<g_index; i++){
		LOG(LOG_INFO, "\t%d :\n", i);
	    print_wordList(conf_info->project[i]);
	}
	return ;
}
