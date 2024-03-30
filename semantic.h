#pragma once

#include "tree.h"
#include "type-system.h"

void           init_semantic();
void           boot_semantic      (struct node *root);

void           sem_Program        (struct node *root); 
void           sem_ExtDefList     (struct node *root); 

void           sem_ExtDef         (struct node *root);
struct Field * sem_ExtDecList     (struct node *root, struct Type *inh_val);

struct Type *  sem_Specifier      (struct node *root);
struct Field * sem_StructSpecifier(struct node *root);
struct Field * sem_OptTag         (struct node *root);
// void sem_Tag();

struct Field * sem_VarDec         (struct node *root);
struct Field * sem_FunDec         (struct node *root, struct Type *inh_val);
struct Field * sem_VarList        (struct node *root);
struct Field * sem_ParamDec       (struct node *root);

void           sem_CompSt         (struct node *root, struct Type *inh_val);
void           sem_StmtList       (struct node *root, struct Type *inh_val);
void           sem_Stmt           (struct node *root, struct Type *inh_val);

struct Field * sem_DefList        (struct node *root);
struct Field * sem_Def            (struct node *root);
struct Field * sem_DecList        (struct node *root, struct Type *inh_val);
struct Field * sem_Dec            (struct node *root, struct Type *inh_val);

struct ExpType * sem_Exp(struct node *root);
void             sem_Args(struct node *root, struct Field *para_list);


static int compare_exptype(struct ExpType *e1, struct ExpType *e2);
