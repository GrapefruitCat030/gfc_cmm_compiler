#pragma once

/* 类型构造 */
enum Tenum { B_INT, B_FLOAT, ARRAY, STRUCTURE_VAR, STRUCTURE_DEF, FUNC };

struct Field {
    char*         name; // 域的名字
    struct Type  *type; // 域的类型
    struct Field *next; // 下一个域
};

struct Type {
    enum Tenum kind;
    union {
        
        struct {                        // F_array 和 S_array 使用
            struct Type *elem; 
            int size; 
        } array;                 

        struct Field *structure_sym;    // T_struct_var 使用

        struct Field *structure_elems;  // F_struct_def 和 S_struct_def 使用
    
        struct {                        // F_func 和 S_func 使用
            struct Type * res_type;
            int para_num;
            struct Field *para_list;
        } func;
    } u;
};

struct ExpType {
    enum Tenum kind;                // 表达式类型
    enum {LVAL, RVAL} lrval_flag;   // 左右值类型
    struct Type *t_ptr;             // Type 指针，当表达式为特定类型(struct、func)时启用
};


/* Interface */

struct Type * make_T_basic(const int type);
struct Type * make_T_struct(struct Field *f);

struct Field * make_F_basic(const char *id);
struct Field * make_F_array(struct Field *f, const int num);
struct Field * make_F_struct(const char *id); 
struct Field * make_F_func(const char *id);

struct Field * make_S_single(struct Field *f, struct Type *t);
struct Field * make_S_array(struct Field *f, struct Type *t);
struct Field * make_S_struct(struct Field *f, struct Field *sl);
struct Field * make_S_func(struct Type *t, struct Field *f, struct Field *sl);

struct Field * make_SL(struct Field *s, struct Field *sl);
// struct Field * make_FL(struct Field *f, struct Field *fl);
// struct Field * make_SL_extend(struct Field *fl, struct Type *type);

void free_field(struct Field *f);
