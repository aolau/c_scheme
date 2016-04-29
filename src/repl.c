#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "scheme.h"

static void load_library(scheme_context *ctx) {
    printf("loading library bs.lsp...\n");
    scheme_obj *ro = scheme_read("(load \"bs.lsp\")", ctx);
    scheme_obj *eo = scheme_eval(ro, ctx);
    scheme_obj_mark(ro, UNUSED);
}

int main() {
    printf("-- Welcome to LSP! --\n\n");
    
    scheme_context *ctx = scheme_init();
    load_library(ctx);
    
    while (1) {
        char * line = readline("\nLSP> ");

        if (strcmp(line, "(quit)") == 0)
            break;

        add_history(line);
        
        scheme_obj *ro = scheme_read(line, ctx);
        scheme_obj *eo = scheme_eval(ro, ctx);

        char *res = scheme_print(eo);
        printf(": %s\n", res);
        
        scheme_obj_mark(ro, UNUSED);
        scheme_obj_mark(eo, UNUSED);
    }
    
    scheme_shutdown(ctx);
    return 1;
}
