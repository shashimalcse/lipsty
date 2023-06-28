#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

int main(int argc, char **argv) {
    /* Parsers for math */
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Lispty = mpc_new("lispty");

    /* Define language */
    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     \
      number   : /-?[0-9]+/ ;                             \
      operator : '+' | '-' | '*' | '/' ;                  \
      expr     : <number> | '(' <operator> <expr>+ ')' ;  \
      lispty    : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Number, Operator, Expr, Lispty);

    while (1) {

        char* input = readline("lispy> ");
        add_history(input);

        /* parse the user input */
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispty, &r))
        {
            mpc_ast_print(r.output);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        free(input);
    }

    /* undefine and delete  parsers */
    mpc_cleanup(4, Number, Operator, Expr, Lispty);
    return 0;
}
