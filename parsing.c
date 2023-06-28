#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <string.h>

static char buffer[2048]

    char *
    readline(char *prompt)
{

    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char *cpy = malloc(strlen(buffer) + 1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy) - 1] = '\0';
    return cpy
}

void add_history(char *unused) {}

#else
#include <editline/readline.h>
#endif

int main(int argc, char **argv)
{
    /* Parsers for math */
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Oparator = mpc_new("oparator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Lispty = mpc_new("lispty");

    while (1)
    {

        char *input = readline("lipsty> ");
        add_history(input);
        printf("No you're a %s\n", input);
        free(input);
    }
    return 0;
}
