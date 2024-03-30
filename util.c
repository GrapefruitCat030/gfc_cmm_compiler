#include "util.h"
#include<stdio.h>
#include<assert.h>

void err_report(char err_num, int lineno, const char *err_msg, const char *ext_msg) {
    switch (err_num) {
    case 'A':
        fprintf(ERR_OUT_FD, "Error type A at Line %d: Mysterious characters \'%s\'. -------------------------------------<LEX ERROR>\n", lineno, err_msg);
        break;
    case 'B':
        fprintf(ERR_OUT_FD, "Error type B at Line %d: %s at \'%s\' -------------------------------------------------<SYNTAX ERROR>\n", lineno, err_msg, ext_msg);
        break;
    default:
        assert(0);
        break;
    }
}

void err_occur_sem(int err_type, int lineno, const char *err_msg, char *ext_msg) {
    fprintf(ERR_OUT_FD, "Error type %d at Line %d: %s \"%s\".\n", err_type, lineno, err_msg, ext_msg);
}



int hash_pjw(const char* name) {
    unsigned int val = 0, i;
    for (; *name; ++name) {
        val = (val << 2) + *name;
        if (i = val & ~HASH_TABLE_SIZE) val = (val ^ (i >> 12)) & HASH_TABLE_SIZE;
    }
    return val;
}
