#include "mpc.h"
#include <editline/readline.h>
#include <stdio.h>
#include <stdlib.h>

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

/* Value that represent number, symbol, expr, Sexpr */
typedef struct lval {
  int type;
  long num;
  char *err;
  char *sym;
  int count;
  struct lval **cell;
} lval;

/* Create a pointer to new Number lval */
lval *lval_num(long x) {

  lval *v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

/* Create a pointer to new Error lval */
lval *lval_err(char *m) {

  lval *v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
}

/* Create a pointer to new Symbol lval */
lval *lval_sym(char *s) {

  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

/* Create a pointer to new Sexpr lval */
lval *lval_sexpr(void) {

  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

/* Create a pointer to new Qexpr lval */
lval *lval_qexpr(void) {

  lval *v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval *lval_read_num(mpc_ast_t *t) {

  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_num(x) : lval_err("Invalid number");
}

/* Add lval to another lval */
lval *lval_add(lval *v, lval *x) {

  v->count++;
  v->cell = realloc(v->cell, sizeof(lval *) * v->count);
  v->cell[v->count - 1] = x;
  return v;
}

/* Create lval from parser output */
lval *lval_read(mpc_ast_t *t) {

  if (strstr(t->tag, "number")) {
    return lval_read_num(t);
  }
  if (strstr(t->tag, "symbol")) {
    return lval_sym(t->contents);
  }

  lval *x = NULL;
  if (strstr(t->tag, ">")) {
    x = lval_sexpr();
  }
  if (strstr(t->tag, "sexpr")) {
    x = lval_sexpr();
  }
  if (strstr(t->tag, "qexpr")) {
    x = lval_qexpr();
  }

  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) {
      continue;
    }
    if (strcmp(t->children[i]->contents, ")") == 0) {
      continue;
    }
    if (strcmp(t->children[i]->contents, "{") == 0) {
      continue;
    }
    if (strcmp(t->children[i]->contents, "}") == 0) {
      continue;
    }
    if (strcmp(t->children[i]->tag, "regex") == 0) {
      continue;
    }
    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

/* Delete lval */
void lval_del(lval *v) {

  switch (v->type) {

    case LVAL_NUM:
      break;
    case LVAL_SYM:
      free(v->sym);
      break;
    case LVAL_ERR:
      free(v->err);
      break;
    /* Delete all elements*/  
    case LVAL_QEXPR:
    case LVAL_SEXPR:
      for (int i = 0; i < v->count; i++) {
        lval_del(v->cell[i]);
      }
      free(v->cell);
      break;
  }
  free(v);
}

/* Get lval without deleting lval children */
lval *lval_pop(lval *v, int i) {

  lval *x = v->cell[i];
  memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval *) * (v->count - i - 1));
  v->count--;
  v->cell = realloc(v->cell, sizeof(lval *) * v->count);
  return x;
}

/* Get lval (delet children) */
lval *lval_take(lval *v, int i) {

  lval *x = lval_pop(v, i);
  lval_del(v);
  return x;
}


/* Join lval */
lval *lval_join(lval *x ,lval *y) {

  while(y->count) {
    x = lval_add(x, lval_pop(y,0));
  }
  lval_del(y);
  return x;
}

void lval_print(lval *v);

/* Print lval expr */
void lval_expr_print(lval *v, char open, char close) {

  putchar(open);
  for (int i = 0; i < v->count; i++) {

    lval_print(v->cell[i]);

    if (i != (v->count - 1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

/* Print lval */
void lval_print(lval *v) {

  switch (v->type) {
  case LVAL_NUM:
    printf("%li", v->num);
    break;
  case LVAL_ERR:
    free(v->err);
    break;
  case LVAL_SYM:
    printf("%s", v->sym);
    break;
  case LVAL_SEXPR:
    lval_expr_print(v, '(', ')');
    break;
    break;
  case LVAL_QEXPR:
    lval_expr_print(v, '{', '}');
    break;
    break;    
  }
}

void lval_println(lval *v) {

  lval_print(v);
  putchar('\n');
}

lval *lval_eval_sexpr(lval *v);

/* Evaluate lval */
lval *lval_eval(lval *v) {

  if (v->type == LVAL_SEXPR) {
    return lval_eval_sexpr(v);
  }
  return v;
}

/* Get Qexpr with only the first element */
lval *builtin_head(lval *a) {

  if (a->count != 1) {
    lval_del(a);
    return lval_err("Function 'head' passed too many arguments!");
  }
  if (a->cell[0]->type != LVAL_QEXPR) {
    lval_del(a);
    return lval_err("Function 'head' passed incorrect types!");
  }
  if (a->cell[0]->count == 0) {
    lval_del(a);
    return lval_err("Function 'head' passed {}!");
  }
  lval* v = lval_take(a, 0);
  while (v->count > 1) { lval_del(lval_pop(v, 1)); }
  return v;
}

/* Get Qexpr without the first element */
lval* builtin_tail(lval* a) {

  if (a->count != 1) {
    lval_del(a);
    return lval_err("Function 'tail' passed too many arguments!");
  }
  if (a->cell[0]->type != LVAL_QEXPR) {
    lval_del(a);
    return lval_err("Function 'tail' passed incorrect types!");
  }
  if (a->cell[0]->count == 0) {
    lval_del(a);
    return lval_err("Function 'tail' passed {}!");
  }
  lval* v = lval_take(a, 0);
  lval_del(lval_pop(v, 0));
  return v;
}

/* Get Qexpr from Sexpr */
lval* builtin_list(lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* builtin_join(lval* a) {

  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_QEXPR) {
      lval_del(a);
      return lval_err("Function 'tail' passed incorrect types!");
    }
  }
  lval* x = lval_pop(a, 0);
  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }
  lval_del(a);
  return x;
}

/* Handle operations */
lval *builtin_op(lval *a, char *op) {

  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on non-number!");
    }
  }
  lval *x = lval_pop(a, 0);

  /* unary negation */
  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->num = -x->num;
  }

  while (a->count > 0) {

    lval *y = lval_pop(a, 0);

    if (strcmp(op, "+") == 0) {
      x->num += y->num;
    }
    if (strcmp(op, "-") == 0) {
      x->num -= y->num;
    }
    if (strcmp(op, "*") == 0) {
      x->num *= y->num;
    }
    if (strcmp(op, "/") == 0) {

      if (y->num == 0) {
        lval_del(x);
        lval_del(y);
        x = lval_err("Division By Zero!");
        break;
      }
      x->num /= y->num;
    }
    if (strcmp(op, "%") == 0) {

      if (y->num == 0) {
        lval_del(x);
        lval_del(y);
        x = lval_err("Division By Zero!");
        break;
      }
      x->num %= y->num;
    }
    if (strcmp(op, "^") == 0) {
      x->num = pow(x->num, y->num);
    }
    if (strcmp(op, "min") == 0) {
      if (x->num > y->num) {
        x->num = y->num;
      }
    }
    if (strcmp(op, "max") == 0) {
      if (x->num < y->num) {
        x->num = y->num;
      }
    }
    lval_del(y);
  }

  lval_del(a);
  return x;
}

lval *builtin_eval(lval *a) {

  if (a->count != 1) {
    lval_del(a);
    return lval_err("Function 'eval' passed too many arguments!");
  }
  if (a->cell[0]->type != LVAL_QEXPR) {
    lval_del(a);
    return lval_err("Function 'eval' passed incorrect types!");
  }
  lval *x = lval_take(a,0);
  x->type = LVAL_SEXPR;
  return lval_eval(x);
}

lval* builtin(lval* a, char* func) {
  
  if (strcmp("list", func) == 0) { return builtin_list(a); }
  if (strcmp("head", func) == 0) { return builtin_head(a); }
  if (strcmp("tail", func) == 0) { return builtin_tail(a); }
  if (strcmp("join", func) == 0) { return builtin_join(a); }
  if (strcmp("eval", func) == 0) { return builtin_eval(a); }
  if (strcmp("min", func)) { return builtin_op(a, func); }
  if (strcmp("max", func)) { return builtin_op(a, func); }
  if (strstr("+-/*%", func)) { return builtin_op(a, func); }
  lval_del(a);
  return lval_err("Unknown Function!");
}

/* Evaluate lval Sexpr */
lval *lval_eval_sexpr(lval *v) {

  /* evaluate childrens */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }

  /* error cheching */
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) {
      return lval_take(v, i);
    }
  }

  if (v->count == 0) {
    return v;
  }
  if (v->count == 1) {
    return lval_take(v, 0);
  }

  lval *f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f);
    lval_del(v);
    return lval_err("S-expression Does not start with symbol!");
  }

  lval *result = builtin(v, f->sym);
  lval_del(f);
  return result;
}

int main(int argc, char **argv) {

  /* Parsers for the lipsty */
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Symbol = mpc_new("symbol");
  mpc_parser_t *Sexpr = mpc_new("sexpr");
  mpc_parser_t *Qexpr = mpc_new("qexpr");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Lispty = mpc_new("lispty");

  /* Define language grammar */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                                   \
      number  : /-?[0-9]+/ ;                                            \
      symbol  : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\"   \
              | \"list\" | \"head\" | \"tail\" | \"join\" | \"eval\" ;  \
      sexpr   : '(' <expr>* ')' ;                                       \
      qexpr   :   '{' <expr>* '}' ;                                     \
      expr    : <number> | <symbol> | <sexpr>  | <qexpr> ;              \
      lispty  : /^/ <expr>* /$/ ;                                       \
    ",
    Number, Symbol, Sexpr, Qexpr, Expr, Lispty);

  while (1) {

    char *input = readline("lispty> ");
    add_history(input);

    /* Parse the user input */
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispty, &r)) {
      lval *x = lval_eval(lval_read(r.output));
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
  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispty);
  return 0;
}
