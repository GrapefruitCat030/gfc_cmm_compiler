#include"sym-table.h"

#include"type-system.h"
#include"util.h"

#include<assert.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>

// #define PRINT_TABLE

void init_sem_sym_table() {
    sem_symbol_table.stk_top      = 0;
    sem_symbol_table.sym_num      = 0;
    sem_symbol_table.symbol_table = (struct header *)malloc(HASH_TABLE_SIZE * sizeof(struct header));   
    sem_symbol_table.scope_stack  = (struct header *)malloc(SCOPE_STK_DEPTH * sizeof(struct header));  
    memset(sem_symbol_table.symbol_table, 0, HASH_TABLE_SIZE * sizeof(struct header));  
    memset(sem_symbol_table.scope_stack,  0, SCOPE_STK_DEPTH * sizeof(struct header)); 

    is_ir_gen = 0;
}
void init_ir_sym_table() {
    ir_symbol_table.stk_top      = 0;
    ir_symbol_table.sym_num      = 0;
    ir_symbol_table.symbol_table = (struct header *)malloc(HASH_TABLE_SIZE * sizeof(struct header));   
    ir_symbol_table.scope_stack  = (struct header *)malloc(SCOPE_STK_DEPTH * sizeof(struct header));  
    memset(ir_symbol_table.symbol_table, 0, HASH_TABLE_SIZE * sizeof(struct header));  
    memset(ir_symbol_table.scope_stack,  0, SCOPE_STK_DEPTH * sizeof(struct header)); 

    is_ir_gen = 1;

    /* Add the read function symbol to the table. */
    struct Field *rdf  = make_F_func("read");
    struct Type  *rdt  = make_T_basic(B_INT);
    struct Field *rdsl = NULL;
    rdf = make_S_func(rdt, rdf, rdsl);
    add_symbol(rdf, -1);

    /* Add the write function symbol to the table. */
    struct Field *wrf  = make_F_func("write");
    struct Type  *wrt  = make_T_basic(B_INT);
    struct Field *wrsl = make_S_single(make_F_basic("para_1"), make_T_basic(B_INT));
    wrf = make_S_func(wrt, wrf, wrsl);
    add_symbol(wrf, -1);

}

void add_symbol(struct Field *f, int lineno) {
    /*  Choose the table and stack  */
    struct SymbolTable *table;
    if(is_ir_gen)   table = &ir_symbol_table;
    else            table = &sem_symbol_table;
    
    struct symbol_node *nd = search_symbol(f->name);

    // Error 3 && 4 && 16: repeated definition.
    if(nd != NULL && nd->sval == table->stk_top) {
        if(f->type->kind == FUNC)
            err_occur_sem(4, lineno, "Redefined function", nd->sym.sym_field->name);
        else if(f->type->kind == STRUCTURE_DEF)
            err_occur_sem(16, lineno, "Duplicated name", nd->sym.sym_field->name);
        else 
            err_occur_sem(3, lineno, "Redefined variable", nd->sym.sym_field->name);
        return;
    }


    assert(nd == NULL || nd->sval != table->stk_top);

    unsigned int hval = hash_pjw(f->name);
    
    struct symbol_node *new_node = (struct symbol_node *)malloc(sizeof(struct symbol_node));


    /* TODO: Initialize the node. */
    new_node->sym.sym_field = f;
    // new_node->sym.sym_place   = place;
    new_node->hval = hval;
    new_node->sval = table->stk_top;
    new_node->lineno = lineno;

    // Add symbol to the hash slot.
    struct symbol_node *slot_firstnd = table->symbol_table[hval].slot_ptr;      assert(slot_firstnd == NULL || new_node->hval == slot_firstnd->hval);
    new_node->slot_next              = slot_firstnd; 
    table->symbol_table[hval].slot_ptr      = new_node;

    // Add symbol to the scope list.
    struct symbol_node *scope_firstnd = table->scope_stack[table->stk_top].scope_ptr;
    new_node->scope_next              = scope_firstnd;
    table->scope_stack[table->stk_top].scope_ptr    = new_node;

    table->sym_num++;

#ifdef PRINT_TABLE
    puts("Add symbol !\n");
    print_sym_table();
#endif
}

struct symbol_node * search_symbol(const char *name) {

    struct SymbolTable *table;
    if(is_ir_gen)   table = &ir_symbol_table;
    else            table = &sem_symbol_table;



    unsigned int hval = hash_pjw(name);
    struct symbol_node *tmp_node = table->symbol_table[hval].slot_ptr; 
    if(tmp_node == NULL)
        return NULL;


    assert(tmp_node != NULL);
    while (strcmp(name, tmp_node->sym.sym_field->name) != 0){
        tmp_node = tmp_node->slot_next;
        if(tmp_node == NULL) break;
    }
    
    return tmp_node;
}

void free_symbol(struct symbol_node *node) {
    /*  Choose the table and stack  */
    struct SymbolTable *table;
    if(is_ir_gen)   table = &ir_symbol_table;
    else            table = &sem_symbol_table;

    table->sym_num--;
    node->slot_next = NULL;
    node->scope_next= NULL;
    // free_field(node->sym.sym_field); // TODO: 不删是因为交织有点严重
    free(node);
    node = NULL;
}

void enter_scope() {
    /*  Choose the table and stack  */
    struct SymbolTable *table;
    if(is_ir_gen)   table = &ir_symbol_table;
    else            table = &sem_symbol_table;

    table->stk_top++;
    assert(table->stk_top <= SCOPE_STK_DEPTH);

#ifdef PRINT_TABLE
    printf("\n----\tEnter scope: %d\t----\n", table->stk_top);
    print_sym_table();
#endif
}

void exit_scope() {
    /*  Choose the table and stack  */
    struct SymbolTable *table;
    if(is_ir_gen)   table = &ir_symbol_table;
    else            table = &sem_symbol_table;

    assert(table->stk_top >= 0);
    struct symbol_node *tmp_node  = table->scope_stack[table->stk_top].scope_ptr;
    struct symbol_node *next_node = NULL;

    while (tmp_node != NULL) {
        assert(table->stk_top == tmp_node->sval);
        // Fix the node list in hash table slot.        
        int tmp_hval                    = tmp_node->hval;        assert(tmp_node == table->symbol_table[tmp_hval].slot_ptr); // 确定作用域内节点均在表头（最前面）
        table->symbol_table[tmp_hval].slot_ptr = tmp_node->slot_next;   assert(table->symbol_table[tmp_hval].slot_ptr == NULL || tmp_hval == table->symbol_table[tmp_hval].slot_ptr->hval);
        tmp_node->slot_next = NULL;

        next_node = tmp_node->scope_next;
        free_symbol(tmp_node);
        tmp_node = next_node;    // 当前free完成，切换到作用域内下一个节点继续 
    }
    assert(tmp_node == NULL);
    table->scope_stack[table->stk_top].scope_ptr = NULL;
    table->stk_top--;

#ifdef PRINT_TABLE
    printf("\n----\tExit scope\t----\n");
    print_sym_table();
#endif
}

void print_sym_table() {
    
    /*  Choose the table and stack  */
    struct SymbolTable *table;
    if(is_ir_gen)   table = &ir_symbol_table;
    else            table = &sem_symbol_table;

    int tmp_top = 0;
    printf("\tsem:%d ir:%d the sym number is %d\t\n", is_ir_gen, is_ir_gen, table->sym_num);
    for(tmp_top; tmp_top <= table->stk_top; tmp_top++) {
        struct symbol_node *nd = table->scope_stack[tmp_top].scope_ptr;
        for(nd; nd != NULL; nd = nd->scope_next) {
            char *name  = nd->sym.sym_field->name;
            int type    = nd->sym.sym_field->type->kind;
            // char *place = nd->sym.sym_place;
            int line    = nd->lineno;
            printf("<%s>\tscope:%d kind: %d val:<null> line:%d\n", name, tmp_top, type, line);
        }
        putchar('\n');
    }
}