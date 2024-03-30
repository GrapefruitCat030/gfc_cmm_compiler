#pragma once


#define ERR_OUT_FD stdout

#define HASH_TABLE_SIZE 0X3FF


void err_occur_sem(int err_type, int lineno, const char *err_msg, char *ext_msg);

void err_report(char err_num, int lineno, const char *err_msg, const char *ext_msg); 

int hash_pjw(const char* name);
