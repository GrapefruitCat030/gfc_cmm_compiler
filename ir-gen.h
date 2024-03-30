#pragma once

#include "tree.h"

/*  
    Operand:    kind + init value(str or sequence numb)
    ----Kind as follows:
    <VALUE_VAR: simple var value> <ADDRESS_VAR: array base addr with &>
    <EVAL_VAR: addr value with *> <CONSTANT_VAR: simple constant value> 
    <CALLPLACE: func name var> <RELOPPLACE: relop operator>
    <TEMPPLACE: temp var> <LABELPLACE: label var>
    ----Union value meanings:
    <var_name:  for VALUE_VAR, ADDRESS_VAR, EVAL_VAR variable name str>
    <con_value: for CONSTANT_VAR, save the value>
    <call_name: for CALLPLACE, the function name str>
    <relop_name:for RELOPPLACE, the relop str>
    <temp_no:   record the cuurent seq number of TEMPPPLACE>
    <label_no:  record the cuurent seq number of LABELPLACE>
    ----Actual output str:
    <output_str: get initialization in ir-gen.c:StrOperand, and
    output when PrintInterCode is called.>
*/
struct Operand {
    enum {  VALUE_VAR, ADDRESS_VAR, EVAL_VAR,
            CONSTANT_VAR, 
            CALLPLACE, RELOPPLACE,
            TEMPPLACE, LABELPLACE } kind;
    union {
        char   *var_name;
        int     con_value;
        char   *call_name;
        char   *relop_name;
        int     temp_no;
        int     lable_no;
    } u;
    char *output_str;
};


/*  
    InterCode:    operator kind + Operand(s)
    ----Operator kind as follows:
    <OP_ADD> <OP_SUB> <OP_MUL> <OP_DIV> : basic, binary operator
    <OP_AND> <OP_OR> <OP_NOT> <OP_RELOP> : boolean, binary operator
    <OP_READ> : read function
    <OP_WRITE> : write function
    <OP_ASSIGN> : assign statement, includes simple assignment, address assignment and eval-addr assignment.
    <DEC_XXX> : declaration statement, includes FUNC, PARAM, ARG, LABEL, GOTO, IFSTMT, RETURN, ARRAY_SIZE.
    ----Union value meanings:
    <dex_xxx> : the operand of DEC_XXX statement.
    <readop>  : the operand required by read function
    <writeop> : the operand required by write function
    <assignop> : binary operand of assign stmt.
    <binop>   : binary operand of calculate stmt and boolean stmt.
*/
struct InterCode {
    enum OperatorKind {  
            OP_ADD,   OP_SUB,   OP_MUL,     OP_DIV,
            OP_AND,   OP_OR ,   OP_NOT,     OP_RELOP,
            OP_READ,  OP_WRITE, 
            OP_ASSIGN,
            DEC_FUNC, DEC_PARAM,  DEC_ARG,  DEC_LABEL, 
            DEC_GOTO, DEC_IFSTMT, DEC_RETURN, DEC_SIZE
        } kind;
    union {
        struct Operand *dec_func;
        struct Operand *dec_param;
        struct Operand *dec_arg;
        struct Operand *dec_label;
        struct Operand *dec_goto;
        struct {struct Operand *left, *relop, *right, *label_true; } dec_ifstmt;
        struct Operand *dec_return;
        struct {struct Operand *varop; int size; } dec_size;
        struct Operand *readop;
        struct Operand *writeop;
        struct {struct Operand *right,  *left; } assign;
        struct {struct Operand *result, *op1, *op2; } binop;
    } u;
};

/*  Table saved intercode.  */

int optemp_num;     // used by Operand.u.temp_no
int oplabel_num;    // used by Operand.u.lable_no

struct InterCodeNode { 
    struct InterCode        *code; 
    struct InterCodeNode    *prev, *next; 
};

struct InterCodeTable {
    int ic_num;
    struct InterCodeNode *head, *tail;
} intercode_table;


/*  Initialization   */

void           init_irgen();
void           boot_irgen(struct node *root);
void           init_ir_ic_table();
// sym-table.c:init_ir_symbol_table()

/* Output the Three Address Code. */
void PrintInterCode(struct InterCode *ic);
/* Output all the InterCode saved in InterCode Table.*/
void PrintInterCodeTable();


/* Intermedia Code Generation by the semantic tree. */

void           translate_Program        (struct node *root); 
void           translate_ExtDefList     (struct node *root); 

void           translate_ExtDef         (struct node *root);
void           translate_ExtDecList     (struct node *root);

void           translate_Specifier      (struct node *root);
void           translate_StructSpecifier(struct node *root);
void           translate_OptTag         (struct node *root);

struct Operand * translate_VarDec       (struct node *root, int is_param, int is_dec);
void           translate_FunDec         (struct node *root);
void           translate_VarList        (struct node *root);
void           translate_ParamDec       (struct node *root);

void           translate_CompSt         (struct node *root);
void           translate_StmtList       (struct node *root);
void           translate_Stmt           (struct node *root);

void           translate_DefList        (struct node *root);
void           translate_Def            (struct node *root);
void           translate_DecList        (struct node *root);
void           translate_Dec            (struct node *root);

void           translate_Exp            (struct node *root, struct Operand *place);
void           translate_Cond           (struct node *root, struct Operand *label_true, struct Operand *label_false);

void           translate_Args           (struct node *root);
