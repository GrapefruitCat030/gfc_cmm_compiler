%{
    void yyerror(const char* msg);

    // #define YYSTYPE struct node *
    #include "tree.h"
    #include "sdt.h"
    #include "lex.yy.c"    

    // struct symbol symbol_table[100];
%}


%{
    // 在本地定义的东西
    
    #include<stdarg.h>
    #include<assert.h>
    int err_occur = 0;
    int lex_error = 0;
    int cmt_error = 0;
%}

%locations  // 启用 %location 功能: 添加位置信息

%union {
    struct node * nd;
}

%token <nd> INT
%token <nd> FLOAT
%token <nd> ID

%token <nd> SEMI COMMA ASSIGNOP DOT
%token <nd> OR AND
%token <nd> RELOP
%token <nd> PLUS MINUS STAR DIV 
%token <nd> NOT
%token <nd> LP RP LB RB LC RC  
%token <nd> TYPE
%token <nd> STRUCT RETURN IF ELSE WHILE

%right ASSIGNOP
%left OR
%left AND 
%left RELOP 
%left PLUS MINUS 
%left STAR DIV
%right UNARY
%right NOT
%left LP RP LB RB DOT

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%type <nd> Program
%type <nd> ExtDefList
%type <nd> ExtDef
%type <nd> ExtDecList
%type <nd> Specifier
%type <nd> StructSpecifier
%type <nd> OptTag
%type <nd> Tag
%type <nd> VarDec
%type <nd> FunDec
%type <nd> VarList
%type <nd> ParamDec
%type <nd> CompSt
%type <nd> StmtList
%type <nd> Stmt
%type <nd> DefList
%type <nd> Def
%type <nd> DecList
%type <nd> Dec
%type <nd> Exp
%type <nd> Args


%start Program

/* %destructor {
                if($$ != NULL) { 
                    free_node($$);
                    $$ = NULL;
                }
} INT FLOAT ID SEMI COMMA ASSIGNOP DOT OR AND RELOP PLUS MINUS STAR DIV NOT LP RP LB RB LC RC TYPE STRUCT RETURN IF ELSE WHILE */

%destructor { free_node($$); } <nd>

%%

/* High-level Definitions */
Program : ExtDefList                    {$$ = create_syntax_node("Program", $1); insert_node($$, 1, $1);
                                            // if(!err_occur) 
                                            //     print_tree($$, 0); 
                                            // assert(!err_occur);
                                            if(!err_occur) {
                                                init_sdt();
                                                boot_sdt($$); 
                                            }
                                        }
    ;
ExtDefList : ExtDef ExtDefList          {$$ = create_syntax_node("ExtDefList", $1);      insert_node($$, 2, $1, $2);}
    | error ExtDef ExtDefList           {$$ = create_syntax_node("Error-ExtDefList", $2);insert_node($$, 2, $2, $3); 
                                            /* Error Recovery */
                                        }
    | %empty /* empty */                {$$ = NULL;}
    ;
ExtDef : Specifier ExtDecList SEMI      {$$ = create_syntax_node("ExtDef", $1);         insert_node($$, 3, $1, $2, $3); }
    | Specifier SEMI                    {$$ = create_syntax_node("ExtDef", $1);         insert_node($$, 2, $1, $2); }
    | Specifier FunDec CompSt           {$$ = create_syntax_node("ExtDef", $1);         insert_node($$, 3, $1, $2, $3); }
    | Specifier error SEMI              {$$ = create_syntax_node("Error-Specifier", $1);insert_node($$, 2, $1, $3); 
                                            /* Error Recovery */
                                        }
    ;
ExtDecList : VarDec                     {$$ = create_syntax_node("ExtDecList", $1);     insert_node($$, 1, $1); }
    | VarDec COMMA ExtDecList           {$$ = create_syntax_node("ExtDecList", $1);     insert_node($$, 3, $1, $2, $3); }
    ;


/* Specifiers */
Specifier : TYPE                                {$$ = create_syntax_node("Specifier", $1); insert_node($$, 1, $1);}
    | StructSpecifier                           {$$ = create_syntax_node("Specifier", $1); insert_node($$, 1, $1);}
    ;
StructSpecifier : STRUCT OptTag LC DefList RC   {$$ = create_syntax_node("StructSpecifier", $1);       insert_node($$, 5, $1, $2, $3, $4, $5); }
    | STRUCT OptTag LC error RC                 {$$ = create_syntax_node("Error-StructSpecifier", $1); insert_node($$, 4, $1, $2, $3, $5); 
                                                                       /* Error Recovery */
                                                }
    | STRUCT Tag                                {$$ = create_syntax_node("StructSpecifier", $1);       insert_node($$, 2, $1, $2); }
    ;
OptTag : ID                                     {$$ = create_syntax_node("OptTag", $1);                insert_node($$, 1, $1); }
    | %empty /* empty */                        {$$ = NULL;}
    ;
Tag : ID                                        {$$ = create_syntax_node("Tag", $1);                   insert_node($$, 1, $1); }
    ;


/* Declarators */
VarDec : ID                             {$$ = create_syntax_node("VarDec", $1);       insert_node($$, 1, $1); }
    | VarDec LB INT RB                  {$$ = create_syntax_node("VarDec", $1);       insert_node($$, 4, $1, $2, $3, $4); }
    | VarDec LB error RB                {$$ = create_syntax_node("Error-VarDec", $1); insert_node($$, 3, $1, $2, $4); 
                                            /* Error Recovery */ 
                                        }
    ;
FunDec : ID LP VarList RP               {$$ = create_syntax_node("FunDec", $1);       insert_node($$, 4, $1, $2, $3, $4); }
    | ID LP RP                          {$$ = create_syntax_node("FunDec", $1);       insert_node($$, 3, $1, $2, $3); }
    | ID LP error RP                    {$$ = create_syntax_node("Error-FunDec", $1); insert_node($$, 3, $1, $2, $4); 
                                            /* Error Recovery */
                                        }
    ;
VarList : ParamDec COMMA VarList        {$$ = create_syntax_node("VarList", $1);      insert_node($$, 3, $1, $2, $3); }
    | ParamDec                          {$$ = create_syntax_node("VarList", $1);      insert_node($$, 1, $1); }
    ;
ParamDec : Specifier VarDec             {$$ = create_syntax_node("ParamDec", $1);     insert_node($$, 2, $1, $2); }
    ;


/* Statements */
CompSt : LC DefList StmtList RC                     {$$ = create_syntax_node("CompSt", $1);     insert_node($$, 4, $1, $2, $3, $4); }
StmtList : Stmt StmtList                            {$$ = create_syntax_node("StmtList", $1);   insert_node($$, 2, $1, $2); }
    | %empty /* empty */                            {$$ = NULL;}
    ;
Stmt : Exp SEMI                                     {$$ = create_syntax_node("Stmt", $1);       insert_node($$, 2, $1, $2); }
    | CompSt                                        {$$ = create_syntax_node("Stmt", $1);       insert_node($$, 1, $1); }
    | RETURN Exp SEMI                               {$$ = create_syntax_node("Stmt", $1);       insert_node($$, 3, $1, $2, $3); }
    | RETURN error SEMI                             {$$ = create_syntax_node("Error-Stmt", $1); insert_node($$, 2, $1, $3); 
                                                        /* Error Recovery */ 
                                                    }
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE       {$$ = create_syntax_node("Stmt", $1);       insert_node($$, 5, $1, $2, $3, $4, $5); }
    | IF LP error RP Stmt %prec LOWER_THAN_ELSE     {$$ = create_syntax_node("Error-Stmt", $1); insert_node($$, 4, $1, $2, $4, $5); 
                                                        /* Error Recovery */ 
                                                    }
    | IF LP Exp RP Stmt ELSE Stmt                   {$$ = create_syntax_node("Stmt", $1);       insert_node($$, 7, $1, $2, $3, $4, $5, $6, $7); }
    | IF LP error RP Stmt ELSE Stmt                 {$$ = create_syntax_node("Error-Stmt", $1); insert_node($$, 6, $1, $2, $4, $5, $6, $7); 
                                                        /* Error Recovery */ 
                                                    }
    | WHILE LP Exp RP Stmt                          {$$ = create_syntax_node("Stmt", $1);       insert_node($$, 5, $1, $2, $3, $4, $5); }
    | WHILE LP Exp error RP Stmt                    {$$ = create_syntax_node("Error-Stmt", $1); insert_node($$, 5, $1, $2, $3, $5, $6);  
                                                        /* Error Recovery */ 
                                                    }
    | WHILE LP error RP Stmt                        {$$ = create_syntax_node("Error-Stmt", $1); insert_node($$, 4, $1, $2, $4, $5); 
                                                    }
                                                        /* Error Recovery */ 
    | error SEMI                                    {$$ = create_syntax_node("Error-Stmt", $2); insert_node($$, 1, $2); 
                                                        /* Error Recovery */ 
                                                    }
    ;


/* Local Definitions */
DefList : Def DefList               {$$ = create_syntax_node("DefList", $1);   insert_node($$, 2, $1, $2); }
    | %empty /* empty */            {$$ = NULL;}
    ;
Def : Specifier DecList SEMI        {$$ = create_syntax_node("Def", $1);       insert_node($$, 3, $1, $2, $3); }
    | Specifier error SEMI          {$$ = create_syntax_node("Error-Def", $1); insert_node($$, 2, $1, $3); 
                                       /* Error Recovery */ 
                                    }
    ;
DecList : Dec                       {$$ = create_syntax_node("DecList", $1);   insert_node($$, 1, $1); }
    | Dec COMMA DecList             {$$ = create_syntax_node("DecList", $1);   insert_node($$, 3, $1, $2, $3); }
    ;
Dec : VarDec                        {$$ = create_syntax_node("Dec", $1);       insert_node($$, 1, $1); }
    | VarDec ASSIGNOP Exp           {$$ = create_syntax_node("Dec", $1);       insert_node($$, 3, $1, $2, $3); }
    ;


/* Expressions */
Exp : Exp ASSIGNOP Exp              {$$ = create_syntax_node("Exp", $1); insert_node($$, 3, $1, $2, $3); }
    | Exp OR Exp                    {$$ = create_syntax_node("Exp", $1); insert_node($$, 3, $1, $2, $3); }
    | Exp AND Exp                   {$$ = create_syntax_node("Exp", $1); insert_node($$, 3, $1, $2, $3); }
    | Exp RELOP Exp                 {$$ = create_syntax_node("Exp", $1); insert_node($$, 3, $1, $2, $3); }
    | Exp PLUS Exp                  {$$ = create_syntax_node("Exp", $1); insert_node($$, 3, $1, $2, $3); }
    | Exp MINUS Exp                 {$$ = create_syntax_node("Exp", $1); insert_node($$, 3, $1, $2, $3); }
    | Exp STAR Exp                  {$$ = create_syntax_node("Exp", $1); insert_node($$, 3, $1, $2, $3); }
    | Exp DIV Exp                   {$$ = create_syntax_node("Exp", $1); insert_node($$, 3, $1, $2, $3); }
    | LP Exp RP                     {$$ = create_syntax_node("Exp", $1); insert_node($$, 3, $1, $2, $3); }
    | MINUS Exp %prec UNARY         {$$ = create_syntax_node("Exp", $1); insert_node($$, 2, $1, $2); }
    | NOT Exp                       {$$ = create_syntax_node("Exp", $1); insert_node($$, 2, $1, $2); }
    | ID LP Args RP                 {$$ = create_syntax_node("Exp", $1); insert_node($$, 4, $1, $2, $3, $4); }
    | ID LP Args error RP           {$$ = create_syntax_node("Error-Exp", $1); insert_node($$, 4, $1, $2, $3, $5); 
                                       /* Error Recovery */ 
                                    }
    | ID LP RP                      {$$ = create_syntax_node("Exp", $1);       insert_node($$, 3, $1, $2, $3); }
    | ID LP error RP                {$$ = create_syntax_node("Error-Exp", $1); insert_node($$, 3, $1, $2, $4); 
                                       /* Error Recovery */ 
                                    }
    | Exp LB Exp RB                 {$$ = create_syntax_node("Exp", $1);       insert_node($$, 4, $1, $2, $3, $4); }
    | Exp LB Exp error RB           {$$ = create_syntax_node("Error-Exp", $1); insert_node($$, 4, $1, $2, $3, $5); 
                                       /* Error Recovery */ 
                                    }
    | Exp DOT ID                    {$$ = create_syntax_node("Exp", $1);  insert_node($$, 3, $1, $2, $3); }
    | ID                            {$$ = create_syntax_node("Exp", $1);  insert_node($$, 1, $1);}
    | INT                           {$$ = create_syntax_node("Exp", $1);  insert_node($$, 1, $1);}
    | FLOAT                         {$$ = create_syntax_node("Exp", $1);  insert_node($$, 1, $1);} 
    ;
Args : Exp                          {$$ = create_syntax_node("Args", $1); insert_node($$, 1, $1); }
    | Exp COMMA Args                {$$ = create_syntax_node("Args", $1); insert_node($$, 3, $1, $2, $3); }
    ;


%%

void yyerror(const char *msg) {
    err_occur = 1;
    char *location_msg = yytext;
    int  lineno = yylineno;
    if(cmt_error == 1) {
        location_msg = "COMMENT PART";
        cmt_error = 0;
    }
    // If a Class A error(lex error) occurs, a class B error(syntax error) is not reported. 
    if(!lex_error)
        err_report('B', lineno, msg, location_msg);
}
