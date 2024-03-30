#include"tree.h"
#include"syntax.tab.h"
#include"util.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdarg.h>
#include<assert.h>


struct node * create_node(const char* ndname, const int lno, int ext_val_num, ...) {
    struct node *tmp = (struct node *)malloc(sizeof(struct node));      if(tmp == NULL) assert(0);

    // The name of node, and the line number the unit appearing.
    tmp->str = (char *)malloc(sizeof(char) * (strlen(ndname) + 1));     if(tmp->str == NULL) assert(0);
    strcpy(tmp->str, ndname);
    tmp->lineno = lno;

    tmp->child_arr = NULL;
    tmp->arr_len   = 0;
    tmp->term_kind = -1; 
    tmp->val.ptr   = NULL;

    /* Set the value for NO-terminal and terminal. */
    
    if(ext_val_num == 0) {
        // 1. Now the function is like "create_node(const char* ndname, const int lno, int ext_val_num = 0)".
    
    } else if(ext_val_num == 1) {
        // 2. Now the function is like "create_node(const char* ndname, const int lno, int ext_val_num = 1, enum kind)".
        va_list vl;
        va_start(vl, ext_val_num);
        tmp->term_kind = va_arg(vl, int);          // Set the enum value: the kind of the terminal token.
        va_end(vl);
    } else {
        // 3. Now the function is like "create_node(const char* ndname, const int lno, int ext_val_num = 2, enum kind, char *yytext)".
        assert(ext_val_num == 2);
        
        va_list vl;
        va_start(vl, ext_val_num);
        
        tmp->term_kind   = va_arg(vl, int);          // Set the enum value: the kind of the terminal token.
        const char *text = va_arg(vl, const char *);
        switch (tmp->term_kind) {
            
            case INT:
                tmp->term_kind     = INT;
                tmp->val.int_val   = atoi(text);
                break;
            case FLOAT:
                tmp->term_kind     = FLOAT;
                tmp->val.float_val = atof(text);
                break;
            case ID:
                tmp->term_kind     = ID;
                tmp->val.id_val    = (char *)malloc(sizeof(char) * (strlen(text) + 1)); if(tmp->val.id_val == NULL) assert(0);
                strcpy(tmp->val.id_val, text);
                break;
            case TYPE:
                tmp->term_kind     = TYPE;
                tmp->val.type_val  = (char *)malloc(sizeof(char) * (strlen(text) + 1)); if(tmp->val.type_val == NULL) assert(0);
                strcpy(tmp->val.type_val, text);
                break;
            case RELOP:
                tmp->term_kind     = RELOP;
                tmp->val.relop_val = (char *)malloc(sizeof(char) * (strlen(text) + 1)); if(tmp->val.relop_val == NULL) assert(0);
                strcpy(tmp->val.relop_val, text);
                break;
            default:
                break;
        
        }
        va_end(vl);
    }

    // printf("create a node : %s\n", tmp->str);
    return tmp;
}

void free_node(struct node *parent) {
    if(parent == NULL) return;
    struct node** check_list = parent->child_arr;
    size_t  list_len = parent->arr_len;  
    for(int i = 0; i < list_len; i++) {
        if(check_list[i] != NULL) 
            free_node(check_list[i]);
    }
    free(check_list);
    check_list = NULL;

    free(parent->str); parent->str = NULL;
   
    if(parent->term_kind == ID || parent->term_kind == TYPE || parent->term_kind == RELOP) {
       free(parent->val.ptr);
       parent->val.ptr = NULL; 
    } 

    free(parent);      parent = NULL;
}

void insert_node(struct node *parent, size_t n, ...){
    if(parent == NULL) return;
    assert(parent != NULL);
    parent->child_arr = (struct node **)malloc(sizeof(struct node *) * n);
    parent->arr_len   = n;
    va_list vl;
    va_start(vl, n);
    for(int i = 0; i < n; i++) {
        struct node *tmp = va_arg(vl, struct node *);
        // assert(tmp != NULL);

        parent->child_arr[i] = tmp;
    } 
    va_end(vl);
}

void print_tree(struct node *parent, int level) {
    // assert(parent != NULL);
    if(parent == NULL) return;
    
    // 1. Print the spaces according to the level.
    for(int i = 0; i < level * 2; i++) {
        putchar(' ');
    }

    // 2. If the node is terminal, print it with value; else print the NO-termial node with line numb. 
    if(parent->child_arr == NULL) {
        assert(parent->arr_len == 0);
        switch (parent->term_kind) {
            case INT:
                printf("%s: %u\n", parent->str, parent->val.int_val); 
                break;
            case FLOAT:
                printf("%s: %f\n", parent->str, parent->val.float_val);
                break;
            case ID:
                printf("%s: %s\n", parent->str, parent->val.id_val);
                break;
            case TYPE:
                printf("%s: %s\n", parent->str, parent->val.type_val);
                break;
            default:
                printf("%s\n", parent->str);
                break;
        }
    }
    else
        printf("%s (%d)\n", parent->str, parent->lineno);
    
    // 3. Rescure traverse.
    for(int i = 0; i < parent->arr_len; i++) {
        if(parent->child_arr[i] == NULL) continue; 
        print_tree(parent->child_arr[i], level + 1);
    }
}

struct node * create_syntax_node(const char *msg, struct node *first_nd) {
    if(first_nd == NULL) return NULL;

    int lno = first_nd->lineno;
    struct node *tmp = create_node(msg, lno, 0);
    return tmp;
}


