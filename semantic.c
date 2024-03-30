#include"semantic.h"

#include "tree.h"
#include "type-system.h"
#include "sym-table.h"
#include "syntax.tab.h"
#include "util.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>

/* 处理struct内部定义使用 */

static int instruct_flag = -1;
static int anony_num = 0;
static int dup_table[6][HASH_TABLE_SIZE];
static void cleanup_dup() {
    memset(dup_table[instruct_flag], 0, sizeof(dup_table[instruct_flag]));
}

/*  Semantic Interface   */

void init_semantic() {
    init_sem_sym_table();
    memset(dup_table, 0, sizeof(dup_table));
}
void boot_semantic(struct node *root) {
    sem_Program(root);
}


void           sem_Program       (struct node *root) {
    assert(root);
    assert(root->arr_len == 1);
    sem_ExtDefList(root->child_arr[0]);
} 
void           sem_ExtDefList    (struct node *root) {
    if(root == NULL) return;
    assert(root);
    assert(root->arr_len == 2);
    sem_ExtDef(root->child_arr[0]);
    sem_ExtDefList(root->child_arr[1]);
}


void           sem_ExtDef         (struct node *root) {
    assert(root);
    /*  ExtDef -> Specifier SEMI  
    */
    if(root->arr_len == 2) {
        sem_Specifier(root->child_arr[0]);
    }
    
    /* ExtDef -> Specifier {ExtDecList.b = Spec.t}
                ExtDecList SEMI {ExtDef.l = ExtDecList.t; ADD_SYM(ExtDef.l)}
    */
    else if(root->arr_len == 3 && root->child_arr[2]->term_kind == SEMI) {   
        
        struct Field *extdef_l, *extdeclist_t;
        struct Type  *spec_t,   *extdeclist_b;

        spec_t       = sem_Specifier(root->child_arr[0]); if(spec_t == NULL) return;
        extdeclist_b = spec_t;

        extdeclist_t = sem_ExtDecList(root->child_arr[1], extdeclist_b);

        extdef_l     = extdeclist_t;

        // Add the S node as symbol val to the table, for the SL.
        for(struct Field *old,*extdef_t = extdef_l; extdef_t != NULL; ) {
            add_symbol(extdef_t, root->lineno);
            old = extdef_t;
            extdef_t = extdef_t->next;
            old->next = NULL;
        }

    }
   
    // 注意Func的参数同时在作用域内外
    /* ExtDef ->    Specifier           {FunDec.b = Spec.t}
                    FunDec              {ExtDef.t = FunDec.t;ADD_SYM(ExtDef.t)}
                                        {enter scope}
                                        {add symbol for the param in varlist}
                                        {CompSt.b = Spec.t}
                    CompSt               
                                        {exit scope}
    */
    else if(root->arr_len == 3 && root->child_arr[2]->term_kind == -1) {
        // TODO: 注意函数体内的变量定义在退出作用域后应不应该删除？如果删除了，后面Exp调用怎么办？
        assert(strcmp(root->child_arr[1]->str, "FunDec") == 0);


        struct Type  *spec_t   = sem_Specifier(root->child_arr[0]); if(spec_t == NULL) return;
        struct Field *fundec_t = sem_FunDec(root->child_arr[1], spec_t);
        struct Field *extdef_t = fundec_t;
        add_symbol(extdef_t, root->lineno);

        enter_scope();

        // add symbol for the varlist.
        struct Field *param = fundec_t->type->u.func.para_list;
        while(param != NULL) {
            add_symbol(param, root->lineno);
            param = param->next;
        }

        sem_CompSt(root->child_arr[2], spec_t);
        exit_scope();


    } else { assert(0);}
}
struct Field * sem_ExtDecList     (struct node *root, struct Type *inh_val) {
    assert(root);
    /* ExtDecList -> VarDec {ExtDecList.t = make_S(ExtDecList.b, VarDec.t)}
    */
    if(root->arr_len == 1) {
        struct Field *vardec_t = sem_VarDec(root->child_arr[0]);
        struct Field *extdeclist_t;
        if(vardec_t->type == NULL)
            extdeclist_t = make_S_single(vardec_t, inh_val);
        else  {
            assert(vardec_t->type->kind == ARRAY);
            extdeclist_t = make_S_array(vardec_t, inh_val);
        }

        return extdeclist_t;
    }

    /* ExtDecList -> VarDec           {ExtDecList.t = make_S(ExtDecList.b, VarDec.t)}
                     COMMA ExtDecList {ExtDecList.t = make_SL(ExtDecList.t, ExtDecList1.t)}
    */
    else if (root->arr_len == 3) {
        struct Field *vardec_t = sem_VarDec(root->child_arr[0]);
        struct Field *extdeclist_t;
        if(vardec_t->type == NULL)
            extdeclist_t = make_S_single(vardec_t, inh_val);
        else  {
            assert(vardec_t->type->kind == ARRAY);
            extdeclist_t = make_S_array(vardec_t, inh_val);
        }

        struct Field *extdeclist1_t = sem_ExtDecList(root->child_arr[2], inh_val);
        
        extdeclist_t = make_SL(extdeclist_t, extdeclist1_t);
        return extdeclist_t;
    }
   else {assert(0);}
}

struct Type *  sem_Specifier      (struct node *root) {
    assert(root);
    assert(root->arr_len == 1);

    struct Type * spec_t;  

    // Specifier -> TYPE {Spec.t = make_T(TYPE)} 
    if(root->child_arr[0]->term_kind == TYPE) {     
        int t = B_INT;
        if(!strcmp(root->child_arr[0]->val.type_val, "float")) t = B_FLOAT;
        spec_t = make_T_basic(t);
    } 
    // Specifier -> StructSpecifier {Spec.t = make_T(StructSpecifier.t)}
    else {                                        
        struct Field *strctspec_t;
        strctspec_t = sem_StructSpecifier(root->child_arr[0]); if(strctspec_t == NULL) return NULL;
        spec_t      = make_T_struct(strctspec_t);
    }

    return spec_t;
}
struct Field * sem_StructSpecifier(struct node *root) {
    assert(root);
    /* StructSpecifier -> STRUCT OptTag {enter struct scope;}
                          LC DefList RC {exit struct scope; StructSpecifier.t = make_S(OptTag.t, DefList.t)}
                                        {ADD_SYM(StructSpecifier.t);} 
    */
    if(root->arr_len == 5) {
        
        struct Field *opttag_t   = sem_OptTag(root->child_arr[1]);
       
        instruct_flag += 1; cleanup_dup();
        struct Field *deflist_t  = sem_DefList(root->child_arr[3]);
        struct Field *stctspec_t = make_S_struct(opttag_t, deflist_t);
        instruct_flag -= 1;
        
        add_symbol(stctspec_t, root->lineno);
        
        return stctspec_t; 
    } 

    /* StructSpecifier -> STRUCT Tag {StructSpecifier.t = check_S(Tag.t) return table(Tag) or ERROR}
    */
    else if(root->arr_len == 2) {
        struct node *tag = root->child_arr[1];
        struct symbol_node *sym_node = search_symbol(tag->child_arr[0]->val.id_val); 

        // Error 17: struct has not be defined.
        if(sym_node == NULL) {
            err_occur_sem(17, root->lineno, "Undefined structure", tag->child_arr[0]->val.id_val);
            return NULL;
        } assert(sym_node != NULL);
        // Error 17: end 

        struct Field *stctspec_t = sym_node->sym.sym_field;
        assert(stctspec_t != NULL);
        return stctspec_t;
    }
    else {assert(0);}
}
struct Field * sem_OptTag         (struct node *root) {

    char *fname;
    struct Field *opttag_t;

    /* OptTag -> null    {create a anonymous struct name.}
    */ 
    if(root == NULL) {
        fname = (char *)malloc(10);
        sprintf(fname, "anony_%d", anony_num++);
    }   
    /* OptTag -> ID    {OptTag.t = make_F(ID)}
    */ 
    else {
        assert(root->arr_len == 1);
        fname = root->child_arr[0]->val.id_val;
    }
    opttag_t = make_F_struct(fname);
    return opttag_t;
}

struct Field * sem_VarDec         (struct node *root) {
    assert(root);
    /* VarDec -> ID {V.t = make_F(ID)}
    */
    if(root->arr_len == 1) {
        struct node  *idnode   = root->child_arr[0];
        struct Field *vardec_t = make_F_basic(idnode->val.id_val);
        return vardec_t;
    } 

    /* VarDec -> VarDec LB INT RB {V.t = make_F(V1.t, INT)}
    */
    else if(root->arr_len == 4) {
        struct Field *vardec1_t = sem_VarDec(root->child_arr[0]);
        struct node *intnode    = root->child_arr[2];
        struct Field *vardec_t  = make_F_array(vardec1_t, intnode->val.int_val);
        return vardec_t;
    }
    else {assert(0);}
}
struct Field * sem_FunDec         (struct node *root, struct Type *inh_val) {
    assert(root);
    /*  FunDec -> ID LP VarList RP           { FunDec.t = make_S(FunDec.b, make_F(ID), VarList.t)) }
    */
    if(root->arr_len == 4) {
        struct node *idnode = root->child_arr[0];
        struct Field *varlist_t = sem_VarList(root->child_arr[2]);
        struct Field *fundec_t  = make_S_func(inh_val, 
                                              make_F_func(idnode->val.id_val),
                                              varlist_t);
        return fundec_t;
    } 

    /* FunDec -> ID LP RP { FunDec.t = make_S(FunDec.b, make_F(ID), null))}
    */
    else if(root->arr_len == 3) {
        struct node *idnode = root->child_arr[0];
        struct Field *fundec_t  = make_S_func(inh_val, 
                                              make_F_func(idnode->val.id_val),
                                              NULL);
        return fundec_t;
    }
    else {assert(0);}
}
struct Field * sem_VarList        (struct node *root) {
    assert(root);
    /* VarList -> ParamDec COMMA VarList    {VarList.t = make_SL(ParamDec.t, VarList1.t)}
    */
    if(root->arr_len == 3) {
        struct Field *paramdec_t = sem_ParamDec(root->child_arr[0]); if(paramdec_t == NULL) return NULL;
        struct Field *varlist1_t = sem_VarList(root->child_arr[2]);
        struct Field *varlist_t  = make_SL(paramdec_t, varlist1_t);
        return varlist_t;
    } 

    /* VarList -> ParamDec  {VarList.t = ParamDec.t}
    */
    else if(root->arr_len == 1) {
        struct Field *paramdec_t = sem_ParamDec(root->child_arr[0]); 
        struct Field *varlist_t  = paramdec_t;
        return varlist_t;
    }
    else {assert(0);}
}
struct Field * sem_ParamDec       (struct node *root) {
    assert(root);
    assert(root->arr_len == 2);
    /* ParamDec -> Specifier VarDec  {ParamDec.t = make_S(Spec.t, VarDec.t); ADD_SYM(ParamDec.t)}
    */
    struct Type  *spec_t     = sem_Specifier(root->child_arr[0]); if(spec_t == NULL) return NULL;
    struct Field *vardec_t   = sem_VarDec(root->child_arr[1]);

    struct Field *paramdec_t;
    if(vardec_t->type == NULL)
        paramdec_t = make_S_single(vardec_t, spec_t);
    else  {
        assert(vardec_t->type->kind == ARRAY);
        paramdec_t = make_S_array(vardec_t, spec_t);
    }

    // Add the param to the func scope
    // add_symbol(paramdec_t, root->lineno);

    return paramdec_t;
}

void           sem_CompSt         (struct node *root, struct Type *inh_val) {
    assert(root);
    /*  CompSt ->   LC          
                    DefList     {StmtList.b = CompSt.b}
                    StmtList     
                    RC          
    */
    sem_DefList(root->child_arr[1]);
    sem_StmtList(root->child_arr[2], inh_val);

}
void           sem_StmtList       (struct node *root, struct Type *inh_val) {
    if(root == NULL) return;
    /* StmtList -> Stmt StmtList   {Stmt.b = StmtList.b; StmtList1.b = StmtList.b}
    */
    sem_Stmt(root->child_arr[0], inh_val);
    sem_StmtList(root->child_arr[1], inh_val);
}
void           sem_Stmt           (struct node *root, struct Type *inh_val) {
    assert(root);
    /*
        Stmt -> Exp SEMI
            | {enter scope; CompSt.b = Stmt.b}
               CompSt                    
              {exit scope;} 
            | RETURN Exp SEMI
            | IF LP Exp RP Stmt
            | IF LP Exp RP Stmt ELSE Stmt
            | WHILE LP Exp RP Stmt
    */
    if(root->arr_len == 2) {
        sem_Exp(root->child_arr[0]);
    } 
    else if(root->arr_len == 1) {
        enter_scope();
        sem_CompSt(root->child_arr[0], inh_val);
        exit_scope();
    }
    else if(root->arr_len == 3) {
        struct ExpType *e = sem_Exp(root->child_arr[1]);
        
        // Error 8: return value type not match
        if(e == NULL || e->kind != inh_val->kind) {
            err_occur_sem(8, root->lineno, "Type mismatched for return", "^^");
            // free(e); e = NULL;
            return;
        }
        assert(e->kind == inh_val->kind);
        if(e->kind == STRUCTURE_VAR) { // inh_val is a T_struct: T-(F-T-SL)
            assert(e->t_ptr->kind == STRUCTURE_DEF);
            if(e->t_ptr != inh_val->u.structure_sym->type) {
                err_occur_sem(8, root->lineno, "Type mismatched for return", "^^");
                free(e); e = NULL;
                return;
            }
            assert(e->t_ptr == inh_val->u.structure_sym->type);
        }
        // Error 8: end.
    }
    else if(root->arr_len == 7) {
        sem_Exp(root->child_arr[2]);
        sem_Stmt(root->child_arr[4], inh_val);
        sem_Stmt(root->child_arr[6], inh_val);
    }
    else if(root->arr_len == 5 && root->child_arr[0]->term_kind == IF) {
        sem_Exp(root->child_arr[2]);
        sem_Stmt(root->child_arr[4], inh_val);
    }
    else if(root->arr_len == 5 && root->child_arr[0]->term_kind == WHILE) {
        sem_Exp(root->child_arr[2]);
        sem_Stmt(root->child_arr[4], inh_val);
    }
    else {assert(0);}
}

struct Field * sem_DefList        (struct node *root) {
    
    if(root == NULL) return NULL;

    assert(root);
    /* DefList -> Def DefList {DefList.t = make_SL(Def.t, DefList1.t)}
    */
    struct Field *def_t      = sem_Def(root->child_arr[0]);     
    struct Field *deflist1_t = sem_DefList(root->child_arr[1]); if(def_t == NULL) return deflist1_t;
    struct Field *deflist_t  = make_SL(def_t, deflist1_t);
    return deflist_t;
}
struct Field * sem_Def            (struct node *root) {
    assert(root);
    /* Def -> Specifier {DecList.b = Spec.t} DecList SEMI    {Def.t = DecList.t}
    */
    struct Type *spec_t     = sem_Specifier(root->child_arr[0]); if(spec_t == NULL) return NULL;
    struct Field *declist_t = sem_DecList(root->child_arr[1], spec_t);
    struct Field *def_t     = declist_t;
    // struct Field *def_t = make_SL_extend(declist_t, spec_t);

    return def_t;
}
struct Field * sem_DecList        (struct node *root, struct Type *inh_val) {
    assert(root);
    /* DecList -> {Dec.b = DecList.b} Dec {DecList.t = Dec.t}
    */
    if(root->arr_len == 1) {
        struct Field *dec_t     = sem_Dec(root->child_arr[0], inh_val);
        struct Field *declist_t = dec_t;
        return declist_t;
    }
    /*  DecList -> {Dec.b = DecList.b} Dec COMMA {DecList1.b = DecList.b} DecList {DecList.t = make_SL(Dec.t, DecList1.t)}
    */
    else if(root->arr_len == 3) {
        struct Field *dec_t      = sem_Dec(root->child_arr[0], inh_val);
        struct Field *declist1_t = sem_DecList(root->child_arr[2], inh_val); if(dec_t == NULL) return declist1_t;
        struct Field *declist_t  = make_SL(dec_t, declist1_t);
        return declist_t;
    }
}
struct Field * sem_Dec            (struct node *root, struct Type *inh_val) {

    /* Dec -> VarDec                   {Dec.t = make_S(V.t, Dec.b);ADD_SYM(Dec.t, null)} 
    */
    struct Field *dec_t;
    if(root->arr_len == 1) {
        struct Field *vardec_t = sem_VarDec(root->child_arr[0]);
        

        if      (vardec_t->type == NULL)        { dec_t = make_S_single(vardec_t, inh_val);} 
        else if (vardec_t->type->kind == ARRAY) { dec_t = make_S_array(vardec_t, inh_val); }
        
        if(instruct_flag == -1) {
            add_symbol(dec_t, root->lineno); 
        }    
        else {
            int hval = hash_pjw(dec_t->name);
            
            // Error 15: check if there are redefined struct symbol.
            if(dup_table[instruct_flag][hval]) {
                err_occur_sem(15, root->lineno, "Redefined struct field", dec_t->name);
                return NULL;
            } assert(dup_table[instruct_flag][hval] == 0);
            // Error 15: end.
            
            dup_table[instruct_flag][hval] = 1;
        } 
        return dec_t;
    }
    
    /* Dec -> VarDec ASSIGNOP Exp       {Dec.t = make_S(V.t, Dec.b);check the Exp.t with Dec.b; ADD_SYM(Dec.t, Exp) }
    */
    else if(root->arr_len == 3) {
        struct Field *vardec_t = sem_VarDec(root->child_arr[0]);
       
        if      (vardec_t->type == NULL)        { dec_t = make_S_single(vardec_t, inh_val);} 
        else if (vardec_t->type->kind == ARRAY) { dec_t = make_S_array(vardec_t, inh_val);}
        
        // Error 15: Assignment definition in the body of structure
        if(instruct_flag >= 0) {
            err_occur_sem(15, root->lineno, "Prohibit assignment in structure", vardec_t->name);
            return dec_t;
        } assert(instruct_flag == -1);
        // Error 15: end.

        struct ExpType *e = sem_Exp(root->child_arr[2]);

        if(e == NULL) {
            add_symbol(dec_t, root->lineno); 
            return dec_t;
        } assert(e); 

        // Error 5: unmatched expression type.
        if(e->kind != inh_val->kind) {
            err_occur_sem(5, root->lineno, "Type mismatched for assignment", dec_t->name);
            return NULL;
        }
        assert(e->kind == inh_val->kind);
        if(e->kind == STRUCTURE_VAR) { // inh_val is a T_struct: T-(F-T-SL)
            assert(e->t_ptr->kind == STRUCTURE_DEF);
            if(e->t_ptr != inh_val->u.structure_sym->type) {
                err_occur_sem(5, root->lineno, "Type mismatched for assignment", dec_t->name);
                return NULL;
            }
            assert(e->t_ptr == inh_val->u.structure_sym->type);
        }
        // Error 5: end.

        add_symbol(dec_t, root->lineno); 

        return dec_t;
    }
}


struct ExpType * sem_Exp(struct node *root) {
    
    assert(root);
    struct ExpType *res = (struct ExpType *)malloc(sizeof(struct ExpType));

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
            
            // Error 1: Var has not be defined.
            if(nd == NULL) {
                err_occur_sem(1, root->lineno, "Undefined variable", term_token->val.id_val); 
                free(res); res = NULL; 
                return NULL;
            } assert(nd);
            // Error 1: end.

            // Init the ExpType node.
            if(nd->sym.sym_field->type->kind == STRUCTURE_VAR) {    // nd->sym.sym_field: F-T-(F-T-SL)
                res->kind       = STRUCTURE_VAR;
                res->t_ptr      = nd->sym.sym_field->type->u.structure_sym->type; assert(res->t_ptr->kind == STRUCTURE_DEF);
                res->lrval_flag = LVAL;
            } else { // B_INT B_FLOAT ARRAY
                res->kind       = nd->sym.sym_field->type->kind;
                res->t_ptr      = nd->sym.sym_field->type;
                res->lrval_flag = LVAL;
            }

        }
        if(term_token->term_kind == INT)   { res->t_ptr = NULL; res->kind = B_INT;   res->lrval_flag = RVAL;}
        if(term_token->term_kind == FLOAT) { res->t_ptr = NULL; res->kind = B_FLOAT; res->lrval_flag = RVAL;}
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
        
        if(mid_token->term_kind == ASSIGNOP) {
            struct ExpType *e1, *e2;
            e1 = sem_Exp(root->child_arr[0]);
            e2 = sem_Exp(root->child_arr[2]);


            // Error 5: unmatched expression type.
            if(e1 == NULL || e2 == NULL || compare_exptype(e1, e2) != 0) {
                err_occur_sem(5, root->lineno, "Type mismatched for assignment", "^^");
                free(res); res = NULL;
                return NULL;
            }
            // Error 5: end.

            // Error 6: right value should not be in left side.
            if(e1->lrval_flag == RVAL) {
                err_occur_sem(6, root->lineno, "The left-hand side of an assignment must be a variable", "^^");
                free(res); res = NULL;
                return NULL;
            } assert(e1->lrval_flag == LVAL); 
            // Error 6: end.
        }
        
        if(mid_token->term_kind == AND)  {
            struct ExpType *e1, *e2; 
            e1 = sem_Exp(root->child_arr[0]); 
            e2 = sem_Exp(root->child_arr[2]); 

            if(e1 == NULL || e2 == NULL) {
                free(res); res = NULL;
                return NULL;
            }

            // Error 7: unmatched expression type for operands.
            assert(e1->kind == B_INT || e1->kind == B_FLOAT); 
            if(compare_exptype(e1, e2) != 0) {
                err_occur_sem(7, root->lineno, "Type mismatched for operands", "^^");
                free(res); res = NULL;
                return NULL;
            }
            // Error 7: end.
            
            res = e1;
            res->lrval_flag = RVAL;
        }
        if(mid_token->term_kind == OR)  {
            struct ExpType *e1, *e2; 
            e1 = sem_Exp(root->child_arr[0]); 
            e2 = sem_Exp(root->child_arr[2]); 

            if(e1 == NULL || e2 == NULL) {
                free(res); res = NULL;
                return NULL;
            }

            // Error 7: unmatched expression type for operands.
            assert(e1->kind == B_INT || e1->kind == B_FLOAT); 
            if(compare_exptype(e1, e2) != 0) {
                err_occur_sem(7, root->lineno, "Type mismatched for operands", "^^");
                free(res); res = NULL;
                return NULL;
            }
            // Error 7: end.
            
            res = e1;
            res->lrval_flag = RVAL;
        }
        if(mid_token->term_kind == RELOP)  {
            struct ExpType *e1, *e2; 
            e1 = sem_Exp(root->child_arr[0]); 
            e2 = sem_Exp(root->child_arr[2]); 
            
            if(e1 == NULL || e2 == NULL) {
                free(res); res = NULL;
                return NULL;
            }

            // Error 7: unmatched expression type for operands.
            assert(e1->kind == B_INT || e1->kind == B_FLOAT); 
            if(compare_exptype(e1, e2) != 0) {
                err_occur_sem(7, root->lineno, "Type mismatched for operands", "^^");
                free(res); res = NULL;
                return NULL;
            }
            // Error 7: end.
            
            res = e1;
            res->lrval_flag = RVAL;
        }
        
        if(mid_token->term_kind == PLUS)  {
            struct ExpType *e1, *e2; 
            e1 = sem_Exp(root->child_arr[0]); 
            e2 = sem_Exp(root->child_arr[2]); 

            if(e1 == NULL || e2 == NULL) {
                free(res); res = NULL;
                return NULL;
            }

            // Error 7: unmatched expression type for operands.
            if(compare_exptype(e1, e2) != 0) {
                err_occur_sem(7, root->lineno, "Type mismatched for operands", "^^");
                free(res); res = NULL;
                return NULL;
            } assert(e1->kind == B_INT || e1->kind == B_FLOAT); 
            // Error 7: end.
            
            res = e1;
            res->lrval_flag = RVAL;

        }
        if(mid_token->term_kind == MINUS)  {
            struct ExpType *e1, *e2; 
            e1 = sem_Exp(root->child_arr[0]); 
            e2 = sem_Exp(root->child_arr[2]); 

            if(e1 == NULL || e2 == NULL) {
                free(res); res = NULL;
                return NULL;
            }

            // Error 7: unmatched expression type for operands.
            assert(e1->kind == B_INT || e1->kind == B_FLOAT); 
            if(compare_exptype(e1, e2) != 0) {
                err_occur_sem(7, root->lineno, "Type mismatched for operands", "^^");
                free(res); res = NULL;
                return NULL;
            }
            // Error 7: end.
            
            res = e1;
            res->lrval_flag = RVAL;

        }        
        if(mid_token->term_kind == STAR)  {
            struct ExpType *e1, *e2; 
            e1 = sem_Exp(root->child_arr[0]); 
            e2 = sem_Exp(root->child_arr[2]); 

            if(e1 == NULL || e2 == NULL) {
                free(res); res = NULL;
                return NULL;
            }

           // Error 7: unmatched expression type for operands.
            assert(e1->kind == B_INT || e1->kind == B_FLOAT); 
            if(compare_exptype(e1, e2) != 0) {
                err_occur_sem(7, root->lineno, "Type mismatched for operands", "^^");
                free(res); res = NULL;
                return NULL;
            }
            // Error 7: end.
            
            res = e1;
            res->lrval_flag = RVAL;

        }        
        if(mid_token->term_kind == DIV)  {
            struct ExpType *e1, *e2; 
            e1 = sem_Exp(root->child_arr[0]); 
            e2 = sem_Exp(root->child_arr[2]); 

            if(e1 == NULL || e2 == NULL) {
                free(res); res = NULL;
                return NULL;
            }

            // Error 7: unmatched expression type for operands.
            assert(e1->kind == B_INT || e1->kind == B_FLOAT); 
            if(compare_exptype(e1, e2) != 0) {
                err_occur_sem(7, root->lineno, "Type mismatched for operands", "^^");
                free(res); res = NULL;
                return NULL;
            }
            // Error 7: end.
            
            res = e1;
            res->lrval_flag = RVAL;
        }
        
        if(mid_token->term_kind == -1) {
            struct ExpType *e = sem_Exp(root->child_arr[1]); 
            if(!e){
                free(res); res = NULL; 
                return NULL;
            } 
            else res = e;
        }

        if(mid_token->term_kind == LP)  { // func 
            struct symbol_node *nd = search_symbol(root->child_arr[0]->val.id_val);
            // Error 2: Function has not be defined.
            if(nd == NULL) {
                err_occur_sem(2, root->lineno, "Undefined function", root->child_arr[0]->val.id_val);
                free(res); res = NULL; 
                return NULL;
            } assert(nd);
            // Error 2: end.
            
            // Error 11: must be function
            if(nd->sym.sym_field->type->kind != FUNC) {
                err_occur_sem(11, root->lineno, "Not a function", nd->sym.sym_field->name);
                free(res); res = NULL;
                return NULL;
            } assert(nd->sym.sym_field->type->kind == FUNC); 
            // Error 11: end.

            // Init the ExpType node.
            struct Type *func_type = nd->sym.sym_field->type->u.func.res_type;
            if(func_type->kind == STRUCTURE_VAR) {    // struct type res_t: T-(F-T-SL)
                res->kind       = STRUCTURE_VAR;
                res->t_ptr      = func_type->u.structure_sym->type; assert(res->t_ptr->kind == STRUCTURE_DEF);
                res->lrval_flag = RVAL;
            } else { // B_INT B_FLOAT ARRAY
                res->kind       = func_type->kind;
                res->t_ptr      = func_type;
                res->lrval_flag = RVAL;
            }
        }
        if(mid_token->term_kind == DOT) { // struct
            
            struct ExpType *e = sem_Exp(root->child_arr[0]); 

            if(e == NULL) {
                free(res); res = NULL;
                return NULL;
            }
            assert(e);

            // Error 13: must be struct defined valiable, like: struct S var.(var)
            if(e->kind != STRUCTURE_VAR) {
                err_occur_sem(13, root->lineno, "Illegal use of", ".");
                free(e);     e = NULL;
                free(res); res = NULL;
                return NULL;
            } assert(e->kind == STRUCTURE_VAR);
            // Error 13: end.
            
            // Error 14: the elem of struct should be defined.
            char *id        = root->child_arr[2]->val.id_val;       assert(e->t_ptr->kind == STRUCTURE_DEF);
            struct Field *p = e->t_ptr->u.structure_elems;
            for(;p!=NULL;p=p->next) 
                if(!strcmp(p->name, id)) break; // find the elem.
            
            if(p == NULL) {
                err_occur_sem(14, root->lineno, "Non-existent struct field", id);
                free(e);     e = NULL;
                free(res); res = NULL;
                return NULL;
            } assert(p);
            // Error 14: end.


            // Init the ExpType node.
            if(p->type->kind == STRUCTURE_VAR) {    // nd->sym.sym_field: F-T-(F-T-SL)
                res->kind       = STRUCTURE_VAR;
                res->t_ptr      = p->type->u.structure_sym->type; assert(res->t_ptr->kind == STRUCTURE_DEF);
                res->lrval_flag = LVAL;
            } else { // B_INT B_FLOAT ARRAY
                res->kind       = p->type->kind;
                res->t_ptr      = p->type;
                res->lrval_flag = LVAL;
            }
        }

    }

/*  Exp -> MINUS Exp
        | NOT Exp
*/
    if(root->arr_len == 2) {
        struct node *token = root->child_arr[0];

        if(token->term_kind == MINUS) {
            struct ExpType *e1 = sem_Exp(root->child_arr[1]); 
            assert(e1->kind == B_INT || e1->kind == B_FLOAT); 
            res = e1; 
            res->lrval_flag = RVAL;
        }
        if(token->term_kind == NOT) {
            res = sem_Exp(root->child_arr[1]); 
            res->lrval_flag = RVAL;
        }

    }

/*  Exp -> ID LP Args RP   // Func Call
        | Exp LB Exp RB   // ARRAY ACCESS
*/
    if(root->arr_len == 4) {
        struct node *token = root->child_arr[3];

        if(token->term_kind == RP) { // func
            struct symbol_node *nd = search_symbol(root->child_arr[0]->val.id_val);
            
            // Error 2: Function has not be defined.
            if(nd == NULL) {
                err_occur_sem(2, root->lineno, "Undefined function", root->child_arr[0]->val.id_val);
                free(res); res = NULL; 
                return NULL;
            } assert(nd);
            // Error 2: end.

            // Error 11: must be function
            if(nd->sym.sym_field->type->kind != FUNC) {
                err_occur_sem(11, root->lineno, "Not a function", nd->sym.sym_field->name);
                free(res); res = NULL;
                return NULL;
            } assert(nd->sym.sym_field->type->kind == FUNC); 
            // Error 11: end.
            
            sem_Args(root->child_arr[2],
                     nd->sym.sym_field->type->u.func.para_list); 
            
            // Init the ExpType node.
            struct Type *func_type = nd->sym.sym_field->type->u.func.res_type;
            if(func_type->kind == STRUCTURE_VAR) {    // struct type res_t: T-(F-T-SL)
                res->kind       = STRUCTURE_VAR;
                res->t_ptr      = func_type->u.structure_sym->type; assert(res->t_ptr->kind == STRUCTURE_DEF);
                res->lrval_flag = RVAL;
            } else {                                  // B_INT B_FLOAT ARRAY
                res->kind       = func_type->kind;
                res->t_ptr      = func_type;
                res->lrval_flag = RVAL;
            }
        }
        if(token->term_kind == RB) { // array
            struct ExpType *e1, *e2;
            e1 = sem_Exp(root->child_arr[0]);
            
            // Error 10: Exp should be array type.
            if(e1->kind != ARRAY) {
                err_occur_sem(10, root->lineno, "Not an array", "^^");
                free(e1); e1 = NULL;
                return NULL;
            } assert(e1->kind == ARRAY);
            // Error 10: end.

            e2 = sem_Exp(root->child_arr[2]);
            
            // Error 12: array access number should be INT.
            if(e2->kind != B_INT) {
                err_occur_sem(12, root->lineno, "A non-integer constant appears in the array accessor", "^^");
                free(e1); e1 = NULL;
                free(e2); e2 = NULL;
                return NULL;
            } assert(e2->kind == B_INT);
            // Error 12: end.
            
            if(e1 == NULL || e2 == NULL) {
                free(res); res = NULL;
                return NULL;
            }


            // Init the ExpType node.
            // [] make the res be the type of e1.type.type.
            struct Type *arr_type = e1->t_ptr->u.array.elem;
            if(arr_type->kind == STRUCTURE_VAR) {    // struct type res_t: T-(F-T-SL)
                res->kind       = STRUCTURE_VAR;
                res->t_ptr      = arr_type->u.structure_sym->type; assert(res->t_ptr->kind == STRUCTURE_DEF);
                res->lrval_flag = LVAL;
            } else {                                // B_INT B_FLOAT ARRAY
                res->kind       = arr_type->kind;
                res->t_ptr      = arr_type;
                res->lrval_flag = LVAL;
            }
        }
    }

    return res;
}

void sem_Args(struct node *root, struct Field *para_list) {
    // para_list: the func symbol para list.
    assert(root);
/* Args -> Exp COMMA Args
        | Exp
*/    
    if(root->arr_len == 1) {
        struct ExpType *e = sem_Exp(root->child_arr[0]);

        // Error 9: Function is not applicable for arguments 
        if(para_list == NULL || para_list->next != NULL) {
            err_occur_sem(9, root->lineno, "Function is not applicable for arguments", "^^");
            // free(e); e = NULL;
            return;
        }
        assert(para_list);
        assert(para_list->next == NULL);

        struct ExpType tmp;
        if(para_list->type->kind == STRUCTURE_VAR) {    // struct type para_list: F-T-(F-T-SL)ype
            tmp.kind       = STRUCTURE_VAR;
            tmp.t_ptr      = para_list->type->u.structure_sym->type; assert(tmp.t_ptr->kind == STRUCTURE_DEF);
            tmp.lrval_flag = LVAL;
        } else {                                        // B_INT B_FLOAT ARRAY
            tmp.kind       = para_list->type->kind;
            tmp.t_ptr      = para_list->type;
            tmp.lrval_flag = LVAL;
        }        
        if(compare_exptype(e, &tmp) != 0) {
            err_occur_sem(9, root->lineno, "Function is not applicable for arguments", "^^");
            return;
        }
        // Error 9: end.

    }
    else if(root->arr_len == 3) {
        struct ExpType *e = sem_Exp(root->child_arr[0]);
        
        // Error 9: Function is not applicable for arguments 
        if(para_list == NULL || para_list->next == NULL) {
            err_occur_sem(9, root->lineno, "Function is not applicable for arguments", "^^");
            // free(e); e = NULL;
            return;
        }
        assert(para_list);
        assert(para_list->next != NULL);

        struct ExpType tmp;
        if(para_list->type->kind == STRUCTURE_VAR) {    // struct type para_list: F-T-(F-T-SL)ype
            tmp.kind       = STRUCTURE_VAR;
            tmp.t_ptr      = para_list->type->u.structure_sym->type; assert(tmp.t_ptr->kind == STRUCTURE_DEF);
            tmp.lrval_flag = LVAL;
        } else {                                        // B_INT B_FLOAT ARRAY
            tmp.kind       = para_list->type->kind;
            tmp.t_ptr      = para_list->type;
            tmp.lrval_flag = LVAL;
        }        
        if(compare_exptype(e, &tmp) != 0) {
            err_occur_sem(9, root->lineno, "Function is not applicable for arguments", "^^");
            return;
        }
        // Error 9: end.

        sem_Args(root->child_arr[2], para_list->next);
    }
    else assert(0);
}

// Return 0 if success, else -1.
static int compare_exptype(struct ExpType *e1, struct ExpType *e2) {
    assert(e1 && e2);
    if(e1->kind != e2->kind) return -1;
    else {
        if(e1->kind == STRUCTURE_VAR) {
            assert(e1->t_ptr->kind == STRUCTURE_DEF);
            assert(e2->t_ptr->kind == STRUCTURE_DEF);
            if(e1->t_ptr != e2->t_ptr) return -1;
        }
        return 0;
    }
}
