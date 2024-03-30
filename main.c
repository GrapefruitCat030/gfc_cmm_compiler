#include<stdio.h>

#ifdef D_MOD
extern int yydebug;
#endif

FILE *irgen_fno;

int main(int argc, char* argv[]) {
    if(argc <= 1) return 1;
    FILE *fno = fopen(argv[1], "r");
    if(!fno){
        perror(argv[1]);
        return 1;
    }
    
    /* Create a fstream for Intermediate Representation generating. */
    if(argc >= 3) {
        irgen_fno = fopen(argv[2], "w+");
        if(!irgen_fno) {
            perror(argv[2]);
            return 1;
        }
    }


    yyrestart(fno);
#ifdef D_MOD
    yydebug = 1;
#endif
    yyparse();

    return 0;

}