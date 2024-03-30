#include "ir-gen.h"

#include<stdlib.h>
#include<stdio.h>
#include<assert.h>
#include<string.h>

#include "tree.h"
#include "type-system.h"
#include "sym-table.h"
#include "syntax.tab.h"

extern FILE *irgen_fno;                             // main.c:      the IR output target (stdout or ir-file)
extern struct ExpType * sem_Exp(struct node *root); // semantic.c:  used for catch the valiable type (value or array).

#define OUTPUT_FILENO irgen_fno

void    init_irgen() {
    init_ir_sym_table();
    init_ir_ic_table();
}
void    boot_irgen(struct node *root) {
    translate_Program(root);
    PrintInterCodeTable();
}

/* IC table (NOT ir symbol table)*/

void    init_ir_ic_table() {
    optemp_num = 0;
    oplabel_num = 1; 
    intercode_table.ic_num = 0;
    intercode_table.head = NULL;
    intercode_table.tail = NULL;
}
void    InsertInterCode(struct InterCode *ic) {
    if(ic == NULL)
        return;

    intercode_table.ic_num++;

    struct InterCodeNode *nd = (struct InterCodeNode *)malloc(sizeof(struct InterCodeNode));
    nd->code = ic;
    nd->prev = NULL;
    nd->next = NULL;

    if(intercode_table.head == NULL) {
        intercode_table.head = nd;
        intercode_table.tail = nd;
        
        return;
    }

    intercode_table.tail->next = nd;
    nd->prev                   = intercode_table.tail;
    intercode_table.tail       = nd; 

}


/* Constructor */

struct Operand * MakeVarOperand     (char *name) {
    struct Operand *res = malloc(sizeof(struct Operand));
    res->kind = VALUE_VAR;
    res->u.var_name = malloc(strlen(name) + 1);
    strcpy(res->u.var_name, name); res->u.var_name[strlen(name)] = '\0';
    return res;
}
struct Operand * MakeAddrVarOperand (char *name) {
    struct Operand *res = malloc(sizeof(struct Operand));
    res->kind = ADDRESS_VAR;
    res->u.var_name = malloc(strlen(name) + 1);
    strcpy(res->u.var_name, name); res->u.var_name[strlen(name)] = '\0';
    return res;
}
struct Operand * MakeEvalVarOperand (struct Operand *op) {
    assert(op->kind == TEMPPLACE);
    struct Operand *res = malloc(sizeof(struct Operand));
    res->kind = EVAL_VAR;
    res->u.temp_no = op->u.temp_no;
    return res;
}
struct Operand * MakeRelOperand     (char *name) {
    struct Operand *res = malloc(sizeof(struct Operand));
    res->kind = RELOPPLACE;
    res->u.relop_name = malloc(strlen(name) + 1);
    strcpy(res->u.relop_name, name); res->u.relop_name[strlen(name)] = '\0';
    return res;
}
struct Operand * MakeCallOperand    (char *name) {
    struct Operand *res = malloc(sizeof(struct Operand));
    res->kind = CALLPLACE;
    res->u.call_name = malloc(strlen(name) + 1);
    strcpy(res->u.call_name, name); res->u.var_name[strlen(name)] = '\0';
    return res;
}
struct Operand * MakeConOperand     (int value) {
    struct Operand *res = malloc(sizeof(struct Operand));
    res->kind = CONSTANT_VAR;
    res->u.con_value = value;
    return res;
}
struct Operand * MakeTempOperand    () {
    struct Operand *res = malloc(sizeof(struct Operand));
    res->kind = TEMPPLACE;
    res->u.temp_no = optemp_num++;
    return res;
}
struct Operand * MakeLabelOperand   () {
    struct Operand *res = malloc(sizeof(struct Operand));
    res->kind = LABELPLACE;
    res->u.temp_no = oplabel_num++;
    return res;
}

struct InterCode * Construct_DecFuncCode    (struct Operand *func) {
    assert(func);

    struct InterCode *ic = malloc(sizeof(struct InterCode));
    ic->u.dec_func = func; 
    ic->kind       = DEC_FUNC;
    return ic;
}
struct InterCode * Construct_DecParamCode   (struct Operand *param) {
    assert(param);

    struct InterCode *ic = malloc(sizeof(struct InterCode));
    ic->u.dec_param = param; 
    ic->kind        = DEC_PARAM;
    return ic;
}
struct InterCode * Construct_DecArgCode     (struct Operand *arg) {
    assert(arg);

    struct InterCode *ic = malloc(sizeof(struct InterCode));
    ic->u.dec_arg = arg; 
    ic->kind      = DEC_ARG;
    return ic;
}
struct InterCode * Construct_DecLabelCode   (struct Operand *label) {
    assert(label->kind == LABELPLACE);
    struct InterCode *ic = malloc(sizeof(struct InterCode));
    ic->u.dec_label = label; 
    ic->kind        = DEC_LABEL;
    return ic;
}
struct InterCode * Construct_DecGotoCode    (struct Operand *label) {
    assert(label->kind == LABELPLACE);
    struct InterCode *ic = malloc(sizeof(struct InterCode));
    ic->u.dec_goto = label; 
    ic->kind         = DEC_GOTO;
    return ic;
}
struct InterCode * Construct_DecIfCode      (struct Operand *left, struct Operand *relop, struct Operand *right, struct Operand *label) {
    // assert(left->kind == TEMPPLACE);
    assert(relop->kind == RELOPPLACE);
    // assert(right->kind == TEMPPLACE);
    assert(label->kind == LABELPLACE);
    struct InterCode *ic = malloc(sizeof(struct InterCode));
    ic->u.dec_ifstmt.left       = left;
    ic->u.dec_ifstmt.right      = right;
    ic->u.dec_ifstmt.relop      = relop;
    ic->u.dec_ifstmt.label_true = label;
    ic->kind                = DEC_IFSTMT;
    return ic;
}
struct InterCode * Construct_DecReturnCode  (struct Operand *temp) {
    assert(temp->kind == TEMPPLACE);
    struct InterCode *ic = malloc(sizeof(struct InterCode));
    ic->u.dec_return = temp; 
    ic->kind         = DEC_RETURN;
    return ic;
}
struct InterCode * Construct_DecSizeCode    (struct Operand *varop, int size) {
    assert(varop->kind == VALUE_VAR);
    struct InterCode *ic = malloc(sizeof(struct InterCode));
    ic->u.dec_size.varop = varop;
    ic->u.dec_size.size  = size;
    ic->kind         = DEC_SIZE;
    return ic;
}
struct InterCode * Construct_ReadCallCode   (struct Operand *readop) {
    assert(readop);

    struct InterCode *ic = malloc(sizeof(struct InterCode));
    ic->u.readop = readop; 
    ic->kind     = OP_READ;
    return ic;
}
struct InterCode * Construct_WriteCallCode  (struct Operand *writeop) {
    assert(writeop);

    struct InterCode *ic = malloc(sizeof(struct InterCode));
    ic->u.readop = writeop; 
    ic->kind     = OP_WRITE;
    return ic;
}
struct InterCode * Construct_AssignCode     (struct Operand *left, struct Operand *right) {
    if(left == NULL) 
        return NULL;
    
    struct InterCode *ic = malloc(sizeof(struct InterCode));
    ic->u.assign.left   = left;
    ic->u.assign.right  = right;
    ic->kind            = OP_ASSIGN;
    return ic;
}
struct InterCode * Construct_BinOpCode      (int kind, struct Operand *result, struct Operand *op1, struct Operand *op2) {
    assert(result);
    struct InterCode *ic = malloc(sizeof(struct InterCode));
    ic->u.binop.result  = result;
    ic->u.binop.op1     = op1;
    ic->u.binop.op2     = op2;
    ic->kind            = (enum OperatorKind)kind;
    return ic;
}

/* Constructor End  */



/* Utility Function */

/* Transform the Operand origin value to the output str value. */
char* StrOperand(struct Operand *op) {
    /*  If the Operand output str has been initialized before, just return it.  */ 
    if(op->output_str) 
        return op->output_str;

    /*  Otherwise, a str malloc is required.   */
    assert(op->output_str == NULL);
    if(op->kind == VALUE_VAR) {
        op->output_str = op->u.var_name;
        return op->output_str;
    }
    if(op->kind == ADDRESS_VAR) {
        op->output_str = malloc(1 + strlen(op->u.call_name) + 1);
        sprintf(op->output_str, "&%s", op->u.var_name); op->output_str[1 + strlen(op->u.var_name)] = '\0';
        return op->output_str;
    }
    if(op->kind == EVAL_VAR) {
        op->output_str = malloc(7);
        sprintf(op->output_str, "*t%d", op->u.temp_no); op->output_str[6] = '\0';
        return op->output_str;
    }
    if(op->kind == CONSTANT_VAR) {
        op->output_str = malloc(5);
        sprintf(op->output_str, "#%d", op->u.con_value); op->output_str[4] = '\0';
        return op->output_str;
    }
    if(op->kind == CALLPLACE) {
        op->output_str = malloc(5 + strlen(op->u.call_name) + 1);
        sprintf(op->output_str, "CALL %s", op->u.call_name); op->output_str[5 + strlen(op->u.call_name)] = '\0';
        return op->output_str;
    }
    if(op->kind == RELOPPLACE) {
        op->output_str = op->u.relop_name;
        return op->output_str;
    }
    if(op->kind == TEMPPLACE){
        op->output_str = malloc(5);
        sprintf(op->output_str, "t%d", op->u.temp_no); op->output_str[4] = '\0';
        return op->output_str;
    }
    if(op->kind == LABELPLACE){
        op->output_str = malloc(5+5);
        sprintf(op->output_str, "label%d", op->u.temp_no); op->output_str[9] = '\0';
        return op->output_str;
    }
    assert(0);
}

void PrintInterCode(struct InterCode *ic) {
    if(ic->kind == DEC_FUNC) {
        fprintf(OUTPUT_FILENO, "FUNCTION %s :\n",   
                StrOperand(ic->u.dec_func));
        return;
    }
    if(ic->kind == DEC_PARAM) {
        fprintf(OUTPUT_FILENO, "PARAM %s\n",   
                StrOperand(ic->u.dec_param));
        return;
    }
    if(ic->kind == DEC_ARG) {
        fprintf(OUTPUT_FILENO, "ARG %s\n",   
                StrOperand(ic->u.dec_arg));
        return;
    }
    if(ic->kind == DEC_LABEL) {
        fprintf(OUTPUT_FILENO, "LABEL %s :\n",   
                StrOperand(ic->u.dec_label));
        return;
    }
    if(ic->kind == DEC_GOTO) {
        fprintf(OUTPUT_FILENO, "GOTO %s\n",   
                StrOperand(ic->u.dec_goto));
        return;
    }
    if(ic->kind == DEC_IFSTMT) {
        fprintf(OUTPUT_FILENO, "IF %s %s %s GOTO %s\n",   
                StrOperand(ic->u.dec_ifstmt.left),
                StrOperand(ic->u.dec_ifstmt.relop),
                StrOperand(ic->u.dec_ifstmt.right),
                StrOperand(ic->u.dec_ifstmt.label_true)
                );
        return;
    }
    if(ic->kind == DEC_RETURN) {
        fprintf(OUTPUT_FILENO, "RETURN %s\n",   
                StrOperand(ic->u.dec_return));
        return;
    }
    if(ic->kind == DEC_SIZE) {
        fprintf(OUTPUT_FILENO, "DEC %s %d\n",   
                StrOperand(ic->u.dec_size.varop), ic->u.dec_size.size);
        return;
    }
    if(ic->kind == OP_ADD) {
        fprintf(OUTPUT_FILENO, "%s := %s + %s\n",   
                StrOperand(ic->u.binop.result),
                StrOperand(ic->u.binop.op1),
                StrOperand(ic->u.binop.op2));
        return;
    }
    if(ic->kind == OP_SUB) {
        fprintf(OUTPUT_FILENO, "%s := %s - %s\n",   
                StrOperand(ic->u.binop.result),
                StrOperand(ic->u.binop.op1),
                StrOperand(ic->u.binop.op2));
        return;
    }
    if(ic->kind == OP_MUL) {
        fprintf(OUTPUT_FILENO, "%s := %s * %s\n",   
                StrOperand(ic->u.binop.result),
                StrOperand(ic->u.binop.op1),
                StrOperand(ic->u.binop.op2));
        return;
    }
    if(ic->kind == OP_DIV) {
        fprintf(OUTPUT_FILENO, "%s := %s / %s\n",   
                StrOperand(ic->u.binop.result),
                StrOperand(ic->u.binop.op1),
                StrOperand(ic->u.binop.op2));
        return;
    }
    if(ic->kind == OP_READ) {
        fprintf(OUTPUT_FILENO, "READ %s\n",   
                StrOperand(ic->u.readop));
        return;
    }
    if(ic->kind == OP_WRITE) {
        fprintf(OUTPUT_FILENO, "WRITE %s\n",   
                StrOperand(ic->u.writeop));
        return;
    }
    if(ic->kind == OP_ASSIGN) {
        fprintf(OUTPUT_FILENO, "%s := %s\n",   
                StrOperand(ic->u.assign.left),
                StrOperand(ic->u.assign.right));
        return;
    }    
    assert(0);
}
void PrintInterCodeTable() {
    for(struct InterCodeNode *nd = intercode_table.head;
        nd != NULL;
        nd = nd->next)
    {
        PrintInterCode(nd->code);
    }
}

/*  
    Cannot use the type-system:make_F_xxx function directly,
    since it requires recursive support of the semantic:sem_VarDec function.
*/
static struct Field * Create_Ftype(struct node *root) {
    assert(root);
    /* VarDec -> ID */
    if(root->arr_len == 1) {
        struct node  *idnode    = root->child_arr[0];
        struct Field *vardec_t  = make_F_basic(idnode->val.id_val); 
        return vardec_t;
    } 

    /* VarDec -> VarDec LB INT RB  */
    else if(root->arr_len == 4) {
        struct Field *vardec1_t = Create_Ftype(root->child_arr[0]);
        struct node  *intnode   = root->child_arr[2];
        struct Field *vardec_t  = make_F_array(vardec1_t, intnode->val.int_val);
        return vardec_t;
    }
    else {assert(0);}   
}
/*
    Add the symbol to the table and return the S type factor.
    因为实验中数组作为参数时，传入的为数组基址，并且lab3中规定数组只为INT类型，
    那么做一个小的trick:使用type system里面数组元素的类型来区分数组为形参还是声明
    B_INT:      数组声明(DEC)
    B_FLOAT:    数组形参(PARAM)
*/
static struct Field * AddSymbol(struct node *root, int is_array, int is_dec, int is_param) {
    if(!is_array) {
        assert(!is_dec && !is_param);
        struct Type *spec_t     = make_T_basic(B_INT);
        struct Field *vardec_t  = Create_Ftype(root); 
        struct Field *dec_t     = make_S_single(vardec_t, spec_t); 
        assert(dec_t->type->kind == B_INT);
        add_symbol(dec_t, root->lineno); 
        return dec_t;
    }
    else {
        assert(is_array);
        if(is_dec) {
            struct Type *spec_t     = make_T_basic(B_INT);
            struct Field *vardec_t  = Create_Ftype(root); 
            struct Field *dec_t     = make_S_array(vardec_t, spec_t); 
            assert(dec_t->type->kind == ARRAY);
            add_symbol(dec_t, root->lineno); 
            return dec_t;
        }
        else if(is_param) {
            struct Type *spec_t     = make_T_basic(B_FLOAT);
            struct Field *vardec_t  = Create_Ftype(root); 
            struct Field *dec_t     = make_S_array(vardec_t, spec_t); 
            assert(dec_t->type->kind == ARRAY);
            add_symbol(dec_t, root->lineno); 
            return dec_t;
        }
        else assert(0);
    }
}
/*  
    Gernerate some Intercode for calculating the array offset
    which required to saved in "place" (a TEMPPLACE Operand),
    and use side effect to set "name". 
*/ 
static void           CalculateOffset (struct node *root, char **name, struct Operand * place) {
    int verify = 0;
    struct node *nd1 = root;
    for(;nd1->arr_len != 1; nd1 = nd1->child_arr[0], verify++);
    nd1 = nd1->child_arr[0];
    assert(nd1->term_kind == ID);
    struct symbol_node *id_sym_node = search_symbol(nd1->val.id_val);  assert(id_sym_node); 
    struct Field *f = id_sym_node->sym.sym_field;
    struct Type *arrtype = f->type;
    assert(arrtype->kind == ARRAY);


    int dimensions = 0;
    for(struct Type *t = arrtype; 
        (t->kind != B_INT) && (t->kind != B_FLOAT); 
        t = t->u.array.elem, dimensions++); 
    assert(dimensions == verify);
    
    // origin[dimensions], record[dimensions]
    int *origin = (int *)malloc(dimensions);
    struct Operand * *record = (struct Operand **)malloc(dimensions * sizeof(struct Operand *));
    
    int i = 0;
    for(struct Type *t = arrtype;(t->kind != B_INT) && (t->kind != B_FLOAT); t = t->u.array.elem) {
        assert(i < dimensions);
        origin[i++] = t->u.array.size;
    }
    
    i = dimensions - 1;
    for(struct node *nd = root; nd->arr_len != 1; nd = nd->child_arr[0]) {
        assert(i >= 0);
        struct Operand *t = MakeTempOperand();
        translate_Exp(nd->child_arr[2], t);
        record[i--] = t;
    }

    // printf("the array dimension: \n");
    // for(int j = 0; j < dimensions; j++) {
    //     printf("%d: origin %d record %s\n", j, origin[j], StrOperand(record[j]));
    // }
    struct Operand *offset = MakeTempOperand();
    InsertInterCode(Construct_AssignCode(offset, MakeConOperand(0)));
    int multiplier = 1;
    for (int i = dimensions - 1; i >= 0; i--) {
        struct Operand *t = MakeTempOperand();
        InsertInterCode(Construct_BinOpCode(OP_MUL, t, record[i], MakeConOperand(multiplier)));
        InsertInterCode(Construct_BinOpCode(OP_ADD, offset, offset, t));
        // offset += record[i] * multiplier;
        multiplier *= origin[i];
    }
    
    *name = malloc(strlen(f->name) + 1);
    strcpy(*name, f->name); 
    (*name)[strlen(f->name)] = '\0';
    
    InsertInterCode(Construct_BinOpCode(OP_MUL, offset, offset, MakeConOperand(4)));
    InsertInterCode(Construct_AssignCode(place, offset));
}

/* Utility Function End */

/*  Interface    */

/*
    注意ir的符号表做了一个trick：依靠lab3的性质——变量名只会出现一次，不用顾及作用域
    并且init_ir_gen之后，sym-table作用的对象跑到ir_table上面
    进行semantic的时候，符号通通都是加入ir_table
    但是因为semantic的特性，最终只有函数的symbol会保留下来，里面的变量定义都被删掉
    然后再轮到ir-gen时，ir-gen加入符号表的符号只会是函数内的变量（简单变量或数组）
    这样就使得函数里面定义的变量会一直保留

    VarDec return the Operand<Var>, 顺便将var加入符号表  
    如果是简单的变量，那么无论是形参还是声明，则均直接加入符号表
    如果是数组变量，如果是形参，那么该数组变量应为一个传入的地址值，无需使用DecSize分配空间
    如果是声明，那么该数组变量为一个实打实的数组，应该分配空间

    为了做到ASSIGN和EXP取值分开，
    ASSIGN中等式左侧不进入translate_Exp的递归，除非为数组域赋值。

    当靠semantic.c:sem_Exp取得类型时，
    如果为B_INT，则为简单变量、常量或者为 DEC数组 访问
    如果为B_FLOAT，那么在lab3中只可能为 PARAM数组 访问
    如果为ARRAY，那么就需要进行对类型的遍历，以获得最底层元素类型以判断是DEC或PARAM数组。

*/

void           translate_Program        (struct node *root) {
    assert(root);
    assert(root->arr_len == 1);
    translate_ExtDefList(root->child_arr[0]);
} 
void           translate_ExtDefList     (struct node *root) {
    if(root == NULL) return;
    assert(root);
    assert(root->arr_len == 2);
    translate_ExtDef(root->child_arr[0]);
    translate_ExtDefList(root->child_arr[1]);
} 

void           translate_ExtDef         (struct node *root) {
    assert(root);
    /*  ExtDef -> Specifier SEMI  
    */
    if(root->arr_len == 2) {
        translate_Specifier(root->child_arr[0]);
        return;
    }
    
    /* ExtDef -> Specifier ExtDecList SEMI 
    */
    else if(root->arr_len == 3 && root->child_arr[2]->term_kind == SEMI) {   
        translate_Specifier(root->child_arr[0]);
        return;
    }

    /* ExtDef -> Specifier FunDec CompSt    
    */
    else if(root->arr_len == 3 && root->child_arr[2]->term_kind == -1) {
        assert(strcmp(root->child_arr[1]->str, "FunDec") == 0);

        // enter_scope();

        // struct Type  *spec_t   = sem_Specifier(root->child_arr[0]); if(spec_t == NULL) return;
        // struct Field *fundec_t = sem_FunDec(root->child_arr[1], spec_t);
        translate_Specifier(root->child_arr[0]);
        translate_FunDec(root->child_arr[1]);
        translate_CompSt(root->child_arr[2]);

        // exit_scope();

        // struct Field *extdef_t = fundec_t;
        // add_symbol(extdef_t, root->lineno);
    } else { assert(0);}
}
void           translate_ExtDecList     (struct node *root) { return; }

void           translate_Specifier      (struct node *root) {  
    assert(root);
    assert(root->arr_len == 1);
    // Specifier -> TYPE 
    if(root->child_arr[0]->term_kind == TYPE) {     
        return;
    } 
    // Specifier -> StructSpecifier 
    else {                                        
        fprintf(stdout, "Cannot translate: Code contains variables or parameters of structure type.\n");
        exit(0);
    }
}
void           translate_StructSpecifier(struct node *root) { return; }
void           translate_OptTag         (struct node *root) { return; }

struct Operand * translate_VarDec       (struct node *root, int is_param, int is_dec) {
    assert(root);
    /* VarDec -> ID */
    if(root->arr_len == 1) {

        AddSymbol(root, 0, 0, 0);

        struct node   *idnode = root->child_arr[0];
        struct Operand *varop = MakeVarOperand(idnode->val.id_val); // single var
        if(is_dec) {
            // None
            return varop;
        }
        if(is_param) {
            InsertInterCode(Construct_DecParamCode(varop));
            return NULL;
        }
    } 

    /* VarDec -> VarDec LB INT RB  */
    else if(root->arr_len == 4) {
        if(is_dec) {

            struct Field *dec_t   = AddSymbol(root, 1, 1, 0);
            struct Operand *varop = MakeVarOperand(dec_t->name);
        
            int size = 1;
            struct Type  *t = dec_t->type;
            for(t; t->kind != B_INT; t = t->u.array.elem) {
                size *= t->u.array.size;
            }
            size *= 4;
            InsertInterCode(Construct_DecSizeCode(varop, size));    
            return NULL;
        }
        if(is_param) {
            // 这里是将B_FLOAT类型代表 PARAM 的 数组参数的定义（不用取地址符&）

            struct Field *dec_t   = AddSymbol(root, 1, 0, 1);
            struct Operand *varop = MakeVarOperand(dec_t->name);

            InsertInterCode(Construct_DecParamCode(varop));
            return NULL;
        }
    }
    else {assert(0);}   
}
void           translate_FunDec         (struct node *root) {
    assert(root);
    /*  FunDec -> ID LP VarList RP  */
    if(root->arr_len == 4) {
        struct node *idnode = root->child_arr[0];
        InsertInterCode(
            Construct_DecFuncCode(
                MakeVarOperand(idnode->val.id_val)
        ));
        
        translate_VarList(root->child_arr[2]);
    } 

    /*  FunDec -> ID LP RP   */
    else if(root->arr_len == 3) {
        struct node *idnode = root->child_arr[0];
        InsertInterCode(
            Construct_DecFuncCode(
                MakeVarOperand(idnode->val.id_val)
        ));
    }
    else {assert(0);}

}
void           translate_VarList        (struct node *root) {
    assert(root);
    /* VarList -> ParamDec COMMA VarList    
    */
    if(root->arr_len == 3) {
        translate_ParamDec(root->child_arr[0]); 
        translate_VarList(root->child_arr[2]);
    } 

    /* VarList -> ParamDec  */
    else if(root->arr_len == 1) {
        translate_ParamDec(root->child_arr[0]);
    }
    else {assert(0);}

}
void           translate_ParamDec       (struct node *root) {
    assert(root);
    assert(root->arr_len == 2);
    /* ParamDec -> Specifier VarDec  */
    translate_VarDec(root->child_arr[1], 1, 0); 

}

void           translate_CompSt         (struct node *root) {
    assert(root);
    /*  CompSt ->   LC          
                    DefList     
                    StmtList     
                    RC          
    */
    translate_DefList(root->child_arr[1]);
    translate_StmtList(root->child_arr[2]);
}
void           translate_StmtList       (struct node *root) {
    // assert(root);
    if(root == NULL) return;
    /* StmtList -> Stmt StmtList   
    */
    translate_Stmt(root->child_arr[0]);
    translate_StmtList(root->child_arr[1]);
}
void           translate_Stmt           (struct node *root) {
    assert(root);
    /*
        Stmt -> Exp SEMI
            | CompSt                    
            | RETURN Exp SEMI
            | IF LP Exp RP Stmt
            | IF LP Exp RP Stmt ELSE Stmt
            | WHILE LP Exp RP Stmt
    */
    if(root->arr_len == 2) {
        translate_Exp(root->child_arr[0], NULL);
    } 
    else if(root->arr_len == 1) {
        // enter_scope();
        translate_CompSt(root->child_arr[0]);
        // exit_scope();
    }
    else if(root->arr_len == 3) {
        struct Operand *t1 = MakeTempOperand();
        // Code 1
        translate_Exp(root->child_arr[1], t1);
        // Code 2
        InsertInterCode(Construct_DecReturnCode(t1));
    }
    else if(root->arr_len == 7) {
        struct Operand *label1, *label2, *label3;
        label1 = MakeLabelOperand();
        label2 = MakeLabelOperand();
        label3 = MakeLabelOperand();
        
        // Code 1
        translate_Cond(root->child_arr[2], label1, label2);
        
        InsertInterCode(Construct_DecLabelCode(label1));
        
        // Code 2
        translate_Stmt(root->child_arr[4]);
        
        InsertInterCode(Construct_DecGotoCode(label3));
        
        InsertInterCode(Construct_DecLabelCode(label2));
        
        // Code 3
        translate_Stmt(root->child_arr[6]);
        
        InsertInterCode(Construct_DecLabelCode(label3));
    }
    else if(root->arr_len == 5 && root->child_arr[0]->term_kind == IF) {
        struct Operand *label1, *label2;
        label1 = MakeLabelOperand();
        label2 = MakeLabelOperand();
        // Code 1
        translate_Cond(root->child_arr[2], label1, label2);
        
        InsertInterCode(Construct_DecLabelCode(label1));
        
        // Code 2
        translate_Stmt(root->child_arr[4]);
        
        InsertInterCode(Construct_DecLabelCode(label2));
    }
    else if(root->arr_len == 5 && root->child_arr[0]->term_kind == WHILE) {
        struct Operand *label1, *label2, *label3;
        label1 = MakeLabelOperand();
        label2 = MakeLabelOperand();
        label3 = MakeLabelOperand();
        
        InsertInterCode(Construct_DecLabelCode(label1));
        // Code 1
        translate_Cond(root->child_arr[2], label2, label3);
        
        InsertInterCode(Construct_DecLabelCode(label2));
        // Code 2
        translate_Stmt(root->child_arr[4]);
        
        InsertInterCode(Construct_DecGotoCode(label1));
        
        InsertInterCode(Construct_DecLabelCode(label3));
    }
    else {assert(0);}
}

void           translate_DefList        (struct node *root) {
    // assert(root);
    if(root == NULL) return;
    /* DefList -> Def DefList */
    translate_Def(root->child_arr[0]);     
    translate_DefList(root->child_arr[1]); 
}
void           translate_Def            (struct node *root) {
    assert(root);
    /* Def -> Specifier DecList SEMI   */
    translate_Specifier(root->child_arr[0]); 
    translate_DecList(root->child_arr[1]);
}
void           translate_DecList        (struct node *root) {
    assert(root);
    /* DecList ->  Dec 
    */
    if(root->arr_len == 1) {
        translate_Dec(root->child_arr[0]);
    }
    /*  DecList ->  Dec COMMA DecList 
    */
    else if(root->arr_len == 3) {
        translate_Dec(root->child_arr[0]);
        translate_DecList(root->child_arr[2]);
    }
}
void           translate_Dec            (struct node *root) {
    // Operand:place (the temp of Dec value) is null!

    /* Dec -> VarDec */

    if(root->arr_len == 1) {
        translate_VarDec(root->child_arr[0], 0, 1);
        return; 
    }
    
    /* Dec -> VarDec ASSIGNOP Exp  */
    else if(root->arr_len == 3) {
        struct Operand *varop = translate_VarDec(root->child_arr[0], 0, 1); assert(varop);
        assert(varop->kind == VALUE_VAR);
        struct Operand *t1 = MakeTempOperand();
        translate_Exp(root->child_arr[2], t1);
        InsertInterCode(Construct_AssignCode(varop, t1));
        return;
    }

}

void           translate_Exp            (struct node *root, struct Operand *place) {
    
    assert(root);

/*
    Exp -> ID              
        | INT
        | FLOAT
*/
    if(root->arr_len == 1) {
        struct node *term_token = root->child_arr[0];
        assert(term_token->term_kind != -1);
        assert(term_token->term_kind == ID || 
               term_token->term_kind == INT ||
               term_token->term_kind == FLOAT);
        
        if(term_token->term_kind == ID) {
            struct symbol_node *nd = search_symbol(term_token->val.id_val);
            assert(nd);
            
            // 进入此处ID判断，只可能为Var取值或Arr域赋值
            // 该产生式的调用者不可能为 Exp -> Exp [ Exp ]
            // 所以只可能为：
            // array := array, 
            
            struct ExpType *e = malloc(sizeof(struct ExpType));
            e->kind       = nd->sym.sym_field->type->kind;
            e->t_ptr      = nd->sym.sym_field->type;
            e->lrval_flag = LVAL;

            if(e->kind == B_INT) {
                struct Operand *varop = MakeVarOperand(nd->sym.sym_field->name);
                InsertInterCode(Construct_AssignCode(place, varop));
            }
            else {
                assert(e->kind == ARRAY);
                /*  遍历访问底层类型，判断是DEC还是PARAM    */
                struct Type *t = search_symbol(nd->sym.sym_field->name)->sym.sym_field->type; 
                assert(t && t->kind == ARRAY);
                for(;t->kind == ARRAY; t = t->u.array.elem);
                assert(t->kind == B_INT || t->kind == B_FLOAT);

                int is_param_arr = ((t->kind) == B_INT) ? 0 : 1; 
            
                if(is_param_arr) {
                    struct Operand *varop = MakeVarOperand(nd->sym.sym_field->name);
                    InsertInterCode(Construct_AssignCode(place, varop));
                }
                else {
                    struct Operand *varop = MakeAddrVarOperand(nd->sym.sym_field->name);
                    InsertInterCode(Construct_AssignCode(place, varop));
                }
            }
            return;
        }
        if(term_token->term_kind == INT)   { 
            struct Operand *numop = MakeConOperand(term_token->val.int_val);
            InsertInterCode(Construct_AssignCode(place, numop));
            return;
        }
        if(term_token->term_kind == FLOAT) { 
            assert(0);            
        }
    }

/*
    Exp -> Exp ASSIGNOP Exp
        | Exp AND Exp
        | Exp OR Exp
        | Exp RELOP Exp
        | Exp PLUS Exp
        | Exp MINUS Exp
        | Exp STAR Exp
        | Exp DIV Exp
        | LP Exp RP
        | ID LP RP        // Func Call
        | Exp DOT ID      // STRUCT ACCESS
*/
    if(root->arr_len == 3) {

        struct node *mid_token = root->child_arr[1];
        
        /*
            为了做到ASSIGN和EXP取值分开，
            ASSIGN中等式左侧不进入translate_Exp的递归，除非为数组域赋值。
        */
        if(mid_token->term_kind == ASSIGNOP) {
            struct node *left_node = root->child_arr[0];
            struct node *right_node = root->child_arr[2];
            
            // e1 := e2
            struct ExpType *e1 = sem_Exp(left_node); assert(e1->lrval_flag == LVAL);
            
            // Difference with Exp1, B_INT is just a single assign but ARRAY need field stuffing.
            if(e1->kind == B_INT) {
                // Exp1 -> ID
                if(left_node->arr_len == 1) {
                    struct Operand *varop = MakeVarOperand(left_node->child_arr[0]->val.id_val);
                    struct Operand *t = MakeTempOperand();
                    translate_Exp(right_node, t);
                    InsertInterCode(Construct_AssignCode(varop, t));
                    InsertInterCode(Construct_AssignCode(place, varop));
                }
                // Exp1 -> E [ E ]
                else if(left_node->arr_len == 4){
                    assert(left_node->child_arr[3]->term_kind == RB);

                    struct Operand *t_address   = MakeTempOperand();
                    
                    char *arr_name;
                    struct Operand *t_offset    = MakeTempOperand();
                    CalculateOffset(left_node, &arr_name, t_offset);
                    
                    struct Operand *arr_address = MakeAddrVarOperand(arr_name);
                    InsertInterCode(Construct_BinOpCode(OP_ADD, t_address, arr_address, t_offset));
                    struct Operand *varop = MakeEvalVarOperand(t_address);

                    struct Operand *t = MakeTempOperand();
                    translate_Exp(right_node, t);
                    InsertInterCode(Construct_AssignCode(varop, t));
                    InsertInterCode(Construct_AssignCode(place, varop));
                }
                else assert(0);
            } 
            else if(e1->kind == B_FLOAT) {
                // Exp1 -> E [ E ]
                assert(left_node->arr_len == 4);
                assert(left_node->child_arr[3]->term_kind == RB);

                struct Operand *t_address   = MakeTempOperand();
                
                char *arr_name;
                struct Operand *t_offset    = MakeTempOperand();
                CalculateOffset(left_node, &arr_name, t_offset);
                
                struct Operand *arr_address = MakeVarOperand(arr_name);
                InsertInterCode(Construct_BinOpCode(OP_ADD, t_address, arr_address, t_offset));
                struct Operand *varop = MakeEvalVarOperand(t_address);

                struct Operand *t = MakeTempOperand();
                translate_Exp(right_node, t);
                InsertInterCode(Construct_AssignCode(varop, t));
                InsertInterCode(Construct_AssignCode(place, varop));
            }
            else {
                assert(e1->kind == ARRAY);
                struct ExpType *e2 = sem_Exp(right_node); assert(e2->kind == ARRAY);
                    
                //  生成域赋值填充代码
                
                int size1 = 1, size2 = 1;
                struct Type *tp1 = e1->t_ptr,
                            *tp2 = e2->t_ptr;
                while(tp1->kind == ARRAY) {
                    assert(tp2->kind == ARRAY);
                    size1 *= tp1->u.array.size;
                    size2 *= tp2->u.array.size;
                    tp1 = tp1->u.array.elem;
                    tp2 = tp2->u.array.elem;
                }
                int stuff_size = (size1 < size2) ? size1 : size2;

                // 生成各自的赋值开始地址
                struct Operand  *left_addr = MakeTempOperand(),
                                *right_addr = MakeTempOperand();
                translate_Exp(left_node, left_addr);
                translate_Exp(right_node, right_addr);
            
                for(int i = 0; i < stuff_size; i++) {
                    struct Operand *ta1 = MakeTempOperand();
                    struct Operand *ta2 = MakeTempOperand();
                    InsertInterCode(Construct_BinOpCode(OP_ADD, ta1, left_addr, MakeConOperand(i * 4)));
                    InsertInterCode(Construct_BinOpCode(OP_ADD, ta2, right_addr, MakeConOperand(i * 4)));
                    InsertInterCode(
                        Construct_AssignCode(   MakeEvalVarOperand(ta1),
                                                MakeEvalVarOperand(ta2))
                    );
                } 
            }
        }

        if(mid_token->term_kind == AND)  {
            struct Operand *label1, *label2;
            label1 = MakeLabelOperand();
            label2 = MakeLabelOperand();
            // Code 0
            InsertInterCode(Construct_AssignCode(place, MakeConOperand(0)));
            // Code 1
            translate_Cond(root, label1, label2);
            // Code 2
            InsertInterCode(Construct_DecLabelCode(label1));
            InsertInterCode(Construct_AssignCode(place, MakeConOperand(1)));

            InsertInterCode(Construct_DecLabelCode(label2));
            return;
        }
        if(mid_token->term_kind == OR)  {
            struct Operand *label1, *label2;
            label1 = MakeLabelOperand();
            label2 = MakeLabelOperand();
            // Code 0
            InsertInterCode(Construct_AssignCode(place, MakeConOperand(0)));
            // Code 1
            translate_Cond(root, label1, label2);
            // Code 2
            InsertInterCode(Construct_DecLabelCode(label1));
            InsertInterCode(Construct_AssignCode(place, MakeConOperand(1)));

            InsertInterCode(Construct_DecLabelCode(label2));
            return;
        }
        if(mid_token->term_kind == RELOP)  {
            struct Operand *label1, *label2;
            label1 = MakeLabelOperand();
            label2 = MakeLabelOperand();
            // Code 0
            InsertInterCode(Construct_AssignCode(place, MakeConOperand(0)));
            // Code 1
            translate_Cond(root, label1, label2);
            // Code 2
            InsertInterCode(Construct_DecLabelCode(label1));
            InsertInterCode(Construct_AssignCode(place, MakeConOperand(1)));

            InsertInterCode(Construct_DecLabelCode(label2));
            return;
        }
        
        if(mid_token->term_kind == PLUS)  {
            struct Operand *t1, *t2;
            t1 = MakeTempOperand();
            t2 = MakeTempOperand();
            translate_Exp(root->child_arr[0], t1); 
            translate_Exp(root->child_arr[2], t2); 
            InsertInterCode(Construct_BinOpCode(OP_ADD, place, t1, t2));
            return;
        }
        if(mid_token->term_kind == MINUS)  {
            struct Operand *t1, *t2;
            t1 = MakeTempOperand();
            t2 = MakeTempOperand();
            translate_Exp(root->child_arr[0], t1); 
            translate_Exp(root->child_arr[2], t2); 
            InsertInterCode(Construct_BinOpCode(OP_SUB, place, t1, t2));
            return;
        }        
        if(mid_token->term_kind == STAR)  {
            struct Operand *t1, *t2;
            t1 = MakeTempOperand();
            t2 = MakeTempOperand();
            translate_Exp(root->child_arr[0], t1); 
            translate_Exp(root->child_arr[2], t2); 
            InsertInterCode(Construct_BinOpCode(OP_MUL, place, t1, t2));
            return;
        }        
        if(mid_token->term_kind == DIV)  {
            struct Operand *t1, *t2;
            t1 = MakeTempOperand();
            t2 = MakeTempOperand();
            translate_Exp(root->child_arr[0], t1); 
            translate_Exp(root->child_arr[2], t2); 
            InsertInterCode(Construct_BinOpCode(OP_DIV, place, t1, t2));
            return;
        }
        
        if(mid_token->term_kind == -1) {
            translate_Exp(root->child_arr[1], place);
            return;
        }
        
        if(mid_token->term_kind == LP)  { // func 
            struct Operand *func = MakeCallOperand(root->child_arr[0]->val.id_val);
            if(strcmp("read", root->child_arr[0]->val.id_val) == 0) {
                InsertInterCode(Construct_ReadCallCode(place));
                return;
            }

            assert(strcmp("read", root->child_arr[0]->val.id_val) != 0);
            InsertInterCode(Construct_AssignCode(place, func));
            return;
        }
        if(mid_token->term_kind == DOT) { // struct
            assert(0);
        }

    }

/*  Exp -> MINUS Exp
        | NOT Exp
*/
    if(root->arr_len == 2) {
        struct node *token = root->child_arr[0];

        if(token->term_kind == MINUS) {
            struct Operand *t1 = MakeTempOperand();
            // Code 1
            translate_Exp(root->child_arr[1], t1);
            // Code 2
            InsertInterCode(Construct_BinOpCode(OP_SUB, place, MakeConOperand(0), t1));
            return;
        }
        if(token->term_kind == NOT) {
            struct Operand *label1, *label2;
            label1 = MakeLabelOperand();
            label2 = MakeLabelOperand();
            // Code 0
            InsertInterCode(Construct_AssignCode(place, MakeConOperand(0)));
            // Code 1
            translate_Cond(root, label1, label2);
            // Code 2
            InsertInterCode(Construct_DecLabelCode(label1));
            InsertInterCode(Construct_AssignCode(place, MakeConOperand(1)));

            InsertInterCode(Construct_DecLabelCode(label2));
            return;
        }

    }

/*  Exp -> ID LP Args RP   // Func Call
        | Exp LB Exp RB   // ARRAY ACCESS
*/
    if(root->arr_len == 4) {
        struct node *token = root->child_arr[3];

        if(token->term_kind == RP) { // func
            // assert(strcmp("write", root->child_arr[0]->val.id_val) != 0);
            if(strcmp("write", root->child_arr[0]->val.id_val) == 0) 
            {
                assert(root->child_arr[2]->arr_len == 1);
                
                struct Operand *t1 = MakeTempOperand();
                translate_Exp(root->child_arr[2]->child_arr[0], t1);
                InsertInterCode(Construct_WriteCallCode(t1));
                InsertInterCode(Construct_AssignCode(place, MakeConOperand(0)));
                return;
            }


            struct Operand *func = MakeCallOperand(root->child_arr[0]->val.id_val);

            // Code 1
            translate_Args(root->child_arr[2]);
            
            if(place == NULL) {
                struct Operand *res = MakeTempOperand();
                InsertInterCode(Construct_AssignCode(res, func));
            }
            else
                InsertInterCode(Construct_AssignCode(place, func));
            return;
        }
        if(token->term_kind == RB) { // array

            struct ExpType *e = sem_Exp(root);
            // 出现在此处的数组Exp一律在赋值操作的右边
            if(e->kind == B_INT) {
                char *arr_name;
                struct Operand *t_address   = MakeTempOperand();
                struct Operand *t_offset    = MakeTempOperand();
                CalculateOffset(root, &arr_name, t_offset);
                struct Operand *arr_address = MakeAddrVarOperand(arr_name);
                InsertInterCode(Construct_BinOpCode(OP_ADD, t_address, arr_address, t_offset));
                
                InsertInterCode(Construct_AssignCode(place, MakeEvalVarOperand(t_address)));
            }
            // 这里是将B_FLOAT类型代表 PARAM 的 数组参数的使用（不用取地址符&）
            else if (e->kind == B_FLOAT) {
                char *arr_name;
                struct Operand *t_address   = MakeTempOperand();
                struct Operand *t_offset    = MakeTempOperand();
                CalculateOffset(root, &arr_name, t_offset);
                struct Operand *arr_address = MakeVarOperand(arr_name);
                InsertInterCode(Construct_BinOpCode(OP_ADD, t_address, arr_address, t_offset));
                
                InsertInterCode(Construct_AssignCode(place, MakeEvalVarOperand(t_address)));

            }
            // 这里便是 ARR 域赋值时，取得ARR address的地方
            else {
                assert(e->kind == ARRAY);

                struct Operand *t_address   = MakeTempOperand();
                
                char *arr_name;
                struct Operand *t_offset    = MakeTempOperand();
                CalculateOffset(root, &arr_name, t_offset);
                
                /*  遍历访问底层类型，判断是DEC还是PARAM    */
                struct Type *t = search_symbol(arr_name)->sym.sym_field->type; 
                assert(t && t->kind == ARRAY);
                for(;t->kind == ARRAY; t = t->u.array.elem);
                assert(t->kind == B_INT || t->kind == B_FLOAT);

                int is_param_arr = ((t->kind) == B_INT) ? 0 : 1; 

                struct Operand *arr_address;
                if(is_param_arr)
                    arr_address = MakeVarOperand(arr_name);
                else
                    arr_address = MakeAddrVarOperand(arr_name);

                InsertInterCode(Construct_BinOpCode(OP_ADD, t_address, arr_address, t_offset));

                InsertInterCode(Construct_AssignCode(place, MakeEvalVarOperand(t_address)));
            }

        }
    }

}
void           translate_Cond           (struct node *root, struct Operand *label_true, struct Operand *label_false) {
    if(root->arr_len == 2) {
        assert(root->child_arr[1]->term_kind == NOT);
        translate_Cond(root->child_arr[1], label_false, label_true);
        return;
    }
    else if(root->arr_len == 3) {
        struct node *mid_token = root->child_arr[1];
        
        if(mid_token->term_kind == AND)  {
            struct Operand *label1 = MakeLabelOperand();
            // Code 1
            translate_Cond(root->child_arr[0], label1, label_false);
            
            InsertInterCode(Construct_DecLabelCode(label1));
            // Code 2
            translate_Cond(root->child_arr[2], label_true, label_false);
            return;
        }
        if(mid_token->term_kind == OR)  {
            struct Operand *label1 = MakeLabelOperand();
            // Code 1
            translate_Cond(root->child_arr[0], label_true, label1);
            
            InsertInterCode(Construct_DecLabelCode(label1));
            // Code 2
            translate_Cond(root->child_arr[2], label_true, label_false);
            return;
        }
        if(mid_token->term_kind == RELOP)  {
            struct Operand *t1, *t2;
            t1 = MakeTempOperand();
            t2 = MakeTempOperand();
            // Code 1
            translate_Exp(root->child_arr[0], t1);
            // Code 2
            translate_Exp(root->child_arr[2], t2);
            // Code 3 
            struct Operand *relop = MakeRelOperand(root->child_arr[1]->val.relop_val);
            InsertInterCode(Construct_DecIfCode(t1, relop, t2, label_true));

            InsertInterCode(Construct_DecGotoCode(label_false));
            return;
        }
        return;
    }
    else {
        struct Operand *t = MakeTempOperand();
        translate_Exp(root, t);
        InsertInterCode(Construct_DecIfCode(t, MakeRelOperand("!="), MakeConOperand(0), label_true));
        InsertInterCode(Construct_DecGotoCode(label_false));
    }
    
}

void           translate_Args           (struct node *root) {
    assert(root);
    /*  Args -> Exp  */
    if(root->arr_len == 1) {
        
        struct ExpType *e = sem_Exp(root->child_arr[0]);
        if(e->kind == B_INT) {
            struct Operand *t = MakeTempOperand();
            translate_Exp(root->child_arr[0], t);
            InsertInterCode(Construct_DecArgCode(t));
        }
        else if(e->kind == ARRAY) {

            // 一维数组作参数，参数出现的两种情况：
            // 1. 如 int a[3]; access_array_elem(a) 此时需要 ARG &a
            // 2. 参数为调用者的形参，如 func(int a[3]) {access_array_elem(a)}
            //    此时需要 ARG a
            assert(root->child_arr[0]->arr_len == 1);

            char *arr_name = root->child_arr[0]->child_arr[0]->val.id_val;
            struct Type *t = search_symbol(arr_name)->sym.sym_field->type; 
            assert(t && t->kind == ARRAY);
            for(;t->kind == ARRAY; t = t->u.array.elem);
            assert(t->kind == B_INT || t->kind == B_FLOAT);

            int is_param_arr = ((t->kind) == B_INT) ? 0 : 1; 
        
            struct Operand *arr_address;
            if(is_param_arr)
                arr_address = MakeVarOperand(arr_name);
            else
                arr_address = MakeAddrVarOperand(arr_name);


            InsertInterCode(Construct_DecArgCode(arr_address));

        }
        else assert(0);

    }
    /* Args -> Exp COMMA Args */    
    else if(root->arr_len == 3) {
        translate_Args(root->child_arr[2]);

        struct ExpType *e = sem_Exp(root->child_arr[0]);
        if(e->kind == B_INT) {
            struct Operand *t = MakeTempOperand();
            translate_Exp(root->child_arr[0], t);
            InsertInterCode(Construct_DecArgCode(t));
        }
        else if(e->kind == ARRAY) {
            // 一维数组作参数，参数出现的两种情况：
            // 1. 如 int a[3]; access_array_elem(a) 此时需要 ARG &a
            // 2. 参数为调用者的形参，如 func(int a[3]) {access_array_elem(a)}
            //    此时需要 ARG a
            assert(root->child_arr[0]->arr_len == 1);

            char *arr_name = root->child_arr[0]->child_arr[0]->val.id_val;
            struct Type *t = search_symbol(arr_name)->sym.sym_field->type; 
            assert(t && t->kind == ARRAY);
            for(;t->kind == ARRAY; t = t->u.array.elem);
            assert(t->kind == B_INT || t->kind == B_FLOAT);

            int is_param_arr = ((t->kind) == B_INT) ? 0 : 1; 
        
            struct Operand *arr_address;
            if(is_param_arr)
                arr_address = MakeVarOperand(arr_name);
            else
                arr_address = MakeAddrVarOperand(arr_name);


            InsertInterCode(Construct_DecArgCode(arr_address));
        }
        else assert(0);

    }
    else assert(0);
}
