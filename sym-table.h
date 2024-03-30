#pragma once


#define SCOPE_STK_DEPTH 16

// 符号
struct symbol {                 
    struct Field *sym_field; //  S  in the type system.
};

// 符号表节点
struct symbol_node {
    int hval;                       // 节点哈希值
    int sval;                       // 节点所处作用域
    int lineno;                     // 符号出现行数
    struct symbol_node *slot_next;  // 指向同一个slot的下一个节点
    struct symbol_node *scope_next; // 指向同一个scope的下一个节点
    struct symbol sym;
};

// 符号表/作用域栈 的数组元素，代表slot头或scope头
struct header {
    union {
        struct symbol_node *slot_ptr;   
        struct symbol_node *scope_ptr;
    };
};

struct SymbolTable {
    int stk_top;                    // scope深度初始值，栈顶，the initial value should be -1.
    int sym_num;                    // 符号数量
    struct header *symbol_table;    // 符号表头部数组(动态)，每一个头部指向一个节点组成的链表slot，slot用来规避哈希冲突（即链地址法,）
    struct header *scope_stack;     // 作用域头部栈(动态)，不同头部指向不同的"作用域符号节点链表"
} sem_symbol_table, ir_symbol_table;

/*  
    The IR boot flag, 
    used to distinguish between SEMANTIC and IR-GEN.
    0:   sem_symbol_table
    1:   ir_symbol_table
*/
static int is_ir_gen = 0;



/*  Interface   */


void                 init_sem_sym_table();
void                 init_ir_sym_table();

void                 enter_scope();
void                 exit_scope();

void                 add_symbol(struct Field *f, int lineno);
struct symbol_node * search_symbol(const char *name);
void                 free_symbol(struct symbol_node *node);

void print_sym_table();