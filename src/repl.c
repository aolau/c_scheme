#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "lsp.h"

static void load_library(lsp_context *ctx) {
    printf("loading library bs.lsp...\n");
    lsp_obj *ro = lsp_read("(load \"bs.lsp\")", ctx);
    lsp_obj *eo = lsp_eval(ro, ctx);
    lsp_obj_mark(ro, UNUSED);
}

int main() {
    printf("-- Welcome to LSP! --\n\n");
    
    lsp_context *ctx = lsp_init();
    load_library(ctx);
    
    while (1) {
        char * line = readline("\nLSP> ");

        if (strcmp(line, "(quit)") == 0)
            break;

        add_history(line);
        
        lsp_obj *ro = lsp_read(line, ctx);
        lsp_obj *eo = lsp_eval(ro, ctx);

        char *res = lsp_print(eo);
        printf(": %s\n", res);
        
        lsp_obj_mark(ro, UNUSED);
        lsp_obj_mark(eo, UNUSED);
    }
    
    lsp_shutdown(ctx);
    return 1;
}
