#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

/* Use operator string to see which operation to perform */
long eval_op(long x, char* op, long y) {

    if (strcmp(op, "+") == 0) { return x + y; }
    if (strcmp(op, "-") == 0) { return x - y; }
    if (strcmp(op, "*") == 0) { return x * y; }
    if (strcmp(op, "/") == 0) { return x / y; }
    if (strcmp(op, "%") == 0) { return x % y; }
    if (strcmp(op, "^") == 0) { return pow(x, y); }
    if (strcmp(op, "min") == 0) { return x < y ? x : y; }
    if (strcmp(op, "max") == 0) { return x > y ? x : y; }
    return 0;
}

long eval(mpc_ast_t* t) {

    /* if tag as number */
    if (strstr(t->tag, "number")) {
        return atoi(t->contents);
    }

    /* operator is always second child*/
    char* op = t->children[1]->contents;

    long x = eval(t->children[2]);

    int i =3;
    while(strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

int main(int argc, char **argv) {
    /* Parsers for math */
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Lispty = mpc_new("lispty");

    /* Define language */
    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                      \
      number   : /-?[0-9]+/ ;                              \
      operator : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\";   \
      expr     : <number> | '(' <operator> <expr>+ ')' ;   \
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
            long result = eval(r.output);
            printf("%li\n", result);
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
