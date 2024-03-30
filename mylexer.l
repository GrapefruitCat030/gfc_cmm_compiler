%{
    #include"syntax.tab.h"
    // The flags controlling error handling actions.
    extern int err_occur;
    extern int lex_error;
    extern int cmt_error;
    void lex_error_handle(const char *err_msg, const char *ext_msg); 
%}

%{
    #include<stdio.h>
    #include<stdlib.h>
    #include"tree.h"
    #include"util.h"
%}


%option yylineno


%{
int yycolumn = 1;
#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;
%}

/*  Definitions of manifest constants
        INT, FLOAT, ID
        SEMI, COMMA, ASSIGNOP, DOT,             ; , = . 
RELOP:  LT, LE, EQ, NE, GT, GE                  < <= == != > >= 
CALOP:  PLUS, MINUS, STAR, DIV, AND, OR, NOT    +-*\/ &&||!
BRACKET:LP, RP, LB, RB, LC, RC                  ()[]{}
        TYPE, T_INT, T_FLOAT
        STRUCT, RETURN, IF, ELSE, WHILE
*/


ws          [ \t]
line        [\n]
letter      [a-zA-Z]
hexletter   [a-fA-F]
digit       [0-9]
octdigit    [0-7]
non_zero    [1-9]

zero        0
digit_int   (({non_zero}{digit}*)|{zero}) 
digit_hex   (0[xX]({hexletter}|{digit}){0,8})
digit_oct   (0{octdigit}+)
digit_float ({digit_int}((\.{digit}+)?))
error_float ({digit_int}?)([\.])(({digit}+)?)([Ee])(({digit}+)|([+-])({digit}+))
id          ({letter}|_)(({letter}|{digit}|_){0,31})
/* errid       (({digit})({letter}|{digit}|_){0,31}) */

%x SLASH_COMMENT STAR_COMMENT
db_slash_cmt    ([/]{2})([^{line}]*)
star_cmt        ([*][^\n]*)

%%

{db_slash_cmt}          {cmt_error=1; yyerror("//"); BEGIN(SLASH_COMMENT);}
<SLASH_COMMENT>{line}   {BEGIN(INITIAL);}
<SLASH_COMMENT>.        {/* no action */}

"/*"                         {cmt_error=1; yyerror("/*"); BEGIN(STAR_COMMENT);}
<STAR_COMMENT>"*/"           {cmt_error=1; yyerror("*/"); BEGIN(INITIAL);}
<STAR_COMMENT>{line}         {/* no action */}   
<STAR_COMMENT>{star_cmt}     {cmt_error=1; yyerror("*");}
<STAR_COMMENT>.              {/* no action */}

{ws}            {/* no action */}    


";"         {yylval.nd = create_node("SEMI",     yylineno, 1, SEMI);     return (SEMI);}
","         {yylval.nd = create_node("COMMA",    yylineno, 1, COMMA);    return (COMMA);}
"="         {yylval.nd = create_node("ASSIGNOP", yylineno, 1, ASSIGNOP); return (ASSIGNOP);}

"+"         {yylval.nd = create_node("PLUS",  yylineno, 1, PLUS);  return (PLUS); /* return  (CALOP);*/}
"-"         {yylval.nd = create_node("MINUS", yylineno, 1, MINUS); return (MINUS); /* return  (CALOP);*/}
"*"         {yylval.nd = create_node("STAR",  yylineno, 1, STAR);  return (STAR); /* return  (CALOP);*/}
"/"         {yylval.nd = create_node("DIV",   yylineno, 1, DIV);   return (DIV); /* return  (CALOP);*/}
"&&"        {yylval.nd = create_node("AND",   yylineno, 1, AND);   return (AND); /* return  (CALOP);*/}
"||"        {yylval.nd = create_node("OR",    yylineno, 1, OR);    return (OR); /* return  (CALOP);*/}
"."         {yylval.nd = create_node("DOT",   yylineno, 1, DOT);   return (DOT);}
"!"         {yylval.nd = create_node("NOT",   yylineno, 1, NOT);   return (NOT); /* return  (CALOP);*/}

"<"         {yylval.nd = create_node("RELOP", yylineno, 2, RELOP, yytext); return  (RELOP); /* return LT; */}
"<="        {yylval.nd = create_node("RELOP", yylineno, 2, RELOP, yytext); return  (RELOP); /* return LE; */}
"=="        {yylval.nd = create_node("RELOP", yylineno, 2, RELOP, yytext); return  (RELOP); /* return EQ; */}
"!="        {yylval.nd = create_node("RELOP", yylineno, 2, RELOP, yytext); return  (RELOP); /* return NE; */}
">="        {yylval.nd = create_node("RELOP", yylineno, 2, RELOP, yytext); return  (RELOP); /* return GE; */}
">"         {yylval.nd = create_node("RELOP", yylineno, 2, RELOP, yytext); return  (RELOP); /* return GT; */}

"("         {yylval.nd = create_node("LP", yylineno, 1, LP); return LP; /* return  (BRACKET); */}
")"         {yylval.nd = create_node("RP", yylineno, 1, RP); return RP; /* return  (BRACKET); */}
"["         {yylval.nd = create_node("LB", yylineno, 1, LB); return LB; /* return  (BRACKET); */}
"]"         {yylval.nd = create_node("RB", yylineno, 1, RB); return RB; /* return  (BRACKET); */}
"{"         {yylval.nd = create_node("LC", yylineno, 1, LC); return LC; /* return  (BRACKET); */}
"}"         {yylval.nd = create_node("RC", yylineno, 1, RC); return RC; /* return  (BRACKET); */}

"int"       {yylval.nd = create_node("TYPE",   yylineno, 2, TYPE, yytext); return TYPE;  } 
"float"     {yylval.nd = create_node("TYPE",   yylineno, 2, TYPE, yytext); return TYPE;  } 
"struct"    {yylval.nd = create_node("STRUCT", yylineno, 1, STRUCT);       return STRUCT;}
"return"    {yylval.nd = create_node("RETURN", yylineno, 1, RETURN);       return RETURN;}
"if"        {yylval.nd = create_node("IF",     yylineno, 1, IF);           return IF;    }
"else"      {yylval.nd = create_node("ELSE",   yylineno, 1, ELSE);         return ELSE;  }
"while"     {yylval.nd = create_node("WHILE",  yylineno, 1, WHILE);        return WHILE; }



{line}                 {    yycolumn = 1; }
{digit_int}            {
                            yylval.nd = create_node("INT",   yylineno, 2, INT, yytext); 
                            return INT;   
                       }
{digit_oct}            {
                            char buf[11];
                            int tmp;
                            sscanf(yytext, "%o", &tmp);
                            sprintf(buf, "%u", tmp);
                            yylval.nd = create_node("INT",   yylineno, 2, INT, buf); 
                            return INT;   
                       }
{digit_hex}            {
                            char buf[11];
                            int tmp;
                            sscanf(yytext, "%x", &tmp);
                            sprintf(buf, "%u", tmp);
                            yylval.nd = create_node("INT",   yylineno, 2, INT, buf); 
                            return INT;   
                       }
{digit_float}          {
                            yylval.nd = create_node("FLOAT", yylineno, 2, FLOAT, yytext); 
                            return FLOAT;   
                       }
{id}                   {
                            yylval.nd = create_node("ID",    yylineno, 2, ID, yytext); 
                            return ID;   
                       }
.|{error_float}        {    lex_error_handle(NULL, NULL); }

%%

void lex_error_handle(const char *err_msg, const char *ext_msg) {
    const char *tmp_msg = (err_msg == NULL) ? yytext : err_msg; 
    err_occur = 1; 
    lex_error = 1;  // If a Class A error(lex error) occurs, a class B error(syntax error) is not reported 
    err_report('A', yylineno, tmp_msg, NULL);
}

/* {errid}                   {
                            char buf[4+32+1];
                            err_occur = 1; fprintf(stdout, "Error type A at Line %d: Error ID \'%s\'\n", yylineno, yytext);                 
                            sprintf(buf, "ID: %s", yytext);
                            yylval.nd = create_node(buf, yylineno, 0); 
                            return ID;   
} */

