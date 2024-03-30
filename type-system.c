#include"type-system.h"

#include<assert.h>
#include<stdlib.h>
#include<string.h>

/* Util Function */

struct Field * stuff_S_func(struct Field *s, struct Type *res_t) {
    // Stuff  the value res_type and para_num in the S func node,  with known type and para list.
    assert(s != NULL);
    assert(s->type->kind == FUNC);
    assert(s->type->u.func.res_type == NULL && 
           s->type->u.func.para_num == 0);

    s->type->u.func.res_type = res_t; 


    for(struct Field *p = s->type->u.func.para_list; p != NULL; p = p->next) {
            s->type->u.func.para_num++;
    }
    return s;
}

/* Interface Definition */

struct Type * make_T_basic(const int type){
    struct Type *res = (struct Type *)malloc(sizeof(struct Type));
    res->kind = (type == 0) ? B_INT : B_FLOAT;
    return res;
}
struct Type * make_T_struct(struct Field *f) {
    // f is a SL, Tnode + SL --> T(struct)
    assert(f);
    assert(f->type->kind == STRUCTURE_DEF);
    assert(f->next == NULL);

    struct Type *res = (struct Type *)malloc(sizeof(struct Type));
    res->kind = STRUCTURE_VAR;    
    res->u.structure_sym = f;     
    return res;
}


struct Field * make_F_basic(const char *id) {
    struct Field *tmp = (struct Field *)malloc(sizeof(struct Field));
    tmp->name         = (char *)malloc(sizeof(char) * (strlen(id) + 1)); if(tmp->name == NULL) assert(0);
    strcpy(tmp->name, id);
    tmp->type = NULL;
    tmp->next = NULL;
    return tmp;
}
struct Field * make_F_array(struct Field *f, const int num) {
    struct Type *t = (struct Type *)malloc(sizeof(struct Type));
    t->kind = ARRAY;
    t->u.array.size = num;
    t->u.array.elem = NULL;

    if(f->type == NULL) 
        f->type = t;
    else {
        struct Type *p = f->type;
        assert(p->kind == ARRAY);
        for(; p->u.array.elem != NULL; p = p->u.array.elem)
            assert(p->kind == ARRAY);
        p->u.array.elem = t;
    }
    return f;
}
struct Field * make_F_struct(const char *id) {
    // T node: struct SL.
    struct Type *t = (struct Type *)malloc(sizeof(struct Type));
    t->kind = STRUCTURE_DEF;
    t->u.structure_elems = NULL;
    
    // F node: struct
    struct Field *tmp = (struct Field *)malloc(sizeof(struct Field));
    tmp->name         = (char *)malloc(sizeof(char) * (strlen(id) + 1)); if(tmp->name == NULL) assert(0);
                        strcpy(tmp->name, id);
    tmp->type         = t;
    tmp->next         = NULL;
    return tmp;
}
struct Field * make_F_func(const char *id) {
    // T node: func.
    struct Type *t = (struct Type *)malloc(sizeof(struct Type));
    t->kind = FUNC;
    t->u.func.res_type  = NULL;
    t->u.func.para_num  = 0;
    t->u.func.para_list = NULL;
    
    // F node: struct
    struct Field *tmp = (struct Field *)malloc(sizeof(struct Field));
    tmp->name         = (char *)malloc(sizeof(char) * (strlen(id) + 1)); if(tmp->name == NULL) assert(0);
                        strcpy(tmp->name, id);
    tmp->type         = t;
    tmp->next         = NULL;
    return tmp;
}


struct Field * make_S_single(struct Field *f, struct Type *t) {
    assert(f && t);
    assert(f->type == NULL);

    f->type = t;
    return f;
}
struct Field * make_S_array(struct Field *f, struct Type *t) {
    assert(f && t);
    struct Type *tmp = f->type;
    for(; tmp->u.array.elem != NULL; tmp = tmp->u.array.elem) {
        assert(tmp);
        assert(tmp->kind == ARRAY);
    }
    assert(tmp);
    assert(tmp->kind == ARRAY && tmp->u.array.elem == NULL);
    

    tmp->u.array.elem = t;
    return f;
}
struct Field * make_S_struct(struct Field *f, struct Field *sl) {
    assert(f->type->kind == STRUCTURE_DEF);
    assert(f->type->u.structure_elems == NULL);
    
    f->type->u.structure_elems = sl;
    return f;
}
struct Field * make_S_func(struct Type *t, struct Field *f, struct Field *sl) {
    assert(f != NULL && t != NULL);
    assert(f->type->kind == FUNC);
    assert(f->type->u.func.res_type   == NULL && 
           f->type->u.func.para_num   == 0    && 
           f->type->u.func.para_list  == NULL );
    
    // Warning: the current S func node is incomplete.

    f->type->u.func.para_list = sl; 

    stuff_S_func(f, t);
    return f;
}


struct Field * make_SL(struct Field *s, struct Field *sl) {
    assert(s);

    // When param sl is null, the result SL is a single S node.
    struct Field *tail;
    for(tail = s; tail->next != NULL; tail = tail->next);

    tail->next = sl;
    return s;
}

// struct Field * make_FL(struct Field *f, struct Field *fl) {
//     assert(f);
//     assert(f->type == NULL || f->type->kind == ARRAY);
//     // When param fl is null, the result FL is a single F node.
//     struct Field *tail;
//     for(tail = f; tail->next != NULL; tail = tail->next);
//     tail->next = fl;
//     return f;
// }

// struct Field * make_SL_extend(struct Field *fl, struct Type *type) {
//     for(struct Field *f = fl; f != NULL; f = f->next) {
//         if(f->type == NULL) {
//             make_S_single(f, type);
//         } else if (f->type->kind == ARRAY) {
//             make_S_array(f, type);
//         } else {assert(0);}
//     }
//     return fl;
// }