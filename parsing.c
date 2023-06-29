#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };

typedef struct lval{
    int type;
    long num;
    char* err;
    char* sym;
    int count;
    struct lval** cell;
} lval;

lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval* lval_err(char* m) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
}

lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_read_num(mpc_ast_t* t) {

    errno = 0;
    long x = strtol(t->contents,NULL,10);
    return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}

lval* lval_read(mpc_ast_t* t) {

    if(strstr(t->tag, "number")) {return lval_read_num(t);}
    if(strstr(t->tag, "symbol")) {return lval_sym(t->contents);}

    lval* x = NULL;
    if(strstr(t->tag, ">")) {x = lval_sexpr();}
    if(strstr(t->tag, "sexpr")) {x = lval_sexpr();}

    for(int i=0; i < t->children_num;i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

void lval_del(lval* v) {

    switch (v->type) {

        case LVAL_NUM : break;
        case LVAL_SYM : free(v->sym); break;
        case LVAL_ERR : free(v->err); break;
        case LVAL_SEXPR:
            for (int i=0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            free(v->cell);
            break;
    }       
    free(v);
}

void lval_print(lval* v);

void lval_expr_print(lval* v, char open, char close) {

    putchar(open);
    for (int i = 0; i < v->count; i++) {

        lval_print(v->cell[i]);

        if (i != (v->count-1)) {
        putchar(' ');
        }
    }
    putchar(close);
}


void lval_print(lval* v) {

    switch (v->type)
    {
    case LVAL_NUM: printf ("%li" , v->num); break;
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    break;
    
    }
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval* lval_eval_sexpr(lval* v);

lval* lval_eval(lval* v) {

    if(v->type == LVAL_SEXPR) {return lval_eval_sexpr(v);}
    return v;
}

lval* lval_eval_sexpr(lval* v) {

    /* evaluate childrens */
    for (int i=0; i<v->count;i++) {
        v->cell[i] = lval_eval(v->cell[i]);
    }

    /* error cheching */
    for (int i=0; i<v->count;i++) {
        if(v->cell[i] == LVAL_ERR) {return lval_take(v, i);}
    }

    if (v->count == 0) {return v;}

    if (v->count ==1 ) {return lval_take(v,0);}

    lval* f = lval_pop(v,0);
    if(f->type !=  LVAL_SYM) {
        lval_del(f); lval_del(v);
        return lval_err("S-expression Does not start with symbol!");
    }

    lval* result  = builtin_op(v, f->sym);
    lval_del(f);
    return result;
}

lval* lval_take(lval* v, int i) {

    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}

lval* lval_pop(lval* v, int i) {

    lval* x = v->cell[i];

    memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));

    v->count--;

    v->cell =  realloc(v->cell, sizeof(lval*) * v->count);

    return x;
}

/* Use operator string to see which operation to perform */
// lval* eval_op(lval x, char* op, lval y) {

//     if (x.type == LVAL_ERR) { return x; }
//     if (y.type == LVAL_ERR) { return y; }

//     if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
//     if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
//     if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
//     if (strcmp(op, "/") == 0) { 

//         return y.num == 0 ? lval_err(LERR_DIV_ZERO):lval_num(x.num / y.num);
//     }
//     if (strcmp(op, "%") == 0) { 

//         return y.num == 0 ? lval_err(LERR_DIV_ZERO):lval_num(x.num % y.num);
//     }
//     if (strcmp(op, "^") == 0) { return lval_num(pow(x.num, y.num)); }
//     if (strcmp(op, "min") == 0) { return x.num < y.num ? lval_num(x.num) : lval_num(y.num); }
//     if (strcmp(op, "max") == 0) { return x.num > y.num ? lval_num(x.num) : lval_num(y.num); }
//     return lval_err(LERR_BAD_OP);
// }

// lval eval(mpc_ast_t* t) {

//     /* if tag as number */
//     if (strstr(t->tag, "number")) {

//         errno = 0;
//         long x = strtol(t->contents, NULL, 10);
//         return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
//     }

//     /* operator is always second child*/
//     char* op = t->children[1]->contents;

//     lval x = eval(t->children[2]);

//     int i =3;
//     while(strstr(t->children[i]->tag, "expr")) {
//         x = eval_op(x, op, eval(t->children[i]));
//         i++;
//     }

//     return x;
// }

int main(int argc, char **argv) {
    /* Parsers for math */
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Lispty = mpc_new("lispty");

    /* Define language */
    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                      \
      number   : /-?[0-9]+/ ;                              \
      symbol : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\";   \
      sexpr     : '(' <expr>* ')' ;   \
      expr     : <number> | <symbol> | <sexpr> ;   \
      lispty    : /^/ <expr>* /$/ ;             \
    ",
    Number, Symbol, Sexpr, Expr, Lispty);

    while (1) {

        char* input = readline("lispy> ");
        add_history(input);

        /* parse the user input */
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispty, &r))
        {
            // lval result = eval(r.output);
            lval* x = lval_read(r.output); 
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        free(input);
    }

    /* undefine and delete  parsers */
    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispty);
    return 0;
}
