#pragma once

#include<stdlib.h>

/* Parsering Tree */

struct node {
    char*         str;
    int           lineno; 
    struct node* *child_arr;
    size_t        arr_len;
    int           term_kind;          // set with the flex enum value. 
    union {
        /* 4 BYTES data */
        
        /* terminal token */ 
        int     int_val;
        float   float_val;
        char*   id_val;
        char*   type_val;
        char*   relop_val;
        
        /* NO-terminal token */ 
        struct Field *F;
        struct Type  *T;
        struct Field *S;
        
        void*   ptr;        // used to memory free.
    } val;
};

struct node * create_node(const char* ndname, const int lno, int ext_val_num, ...);

void free_node(struct node *parent);

void insert_node(struct node *parent, size_t n, ...);

void print_tree(struct node *parent, int level); 

struct node * create_syntax_node(const char *msg, struct node *first_nd);

/* Error Report */

void err_report(char err_num, int lineno, const char *err_msg, const char *ext_msg);
