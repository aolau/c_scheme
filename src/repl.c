#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "scheme.h"

int main() {
    printf("-- Welcome to lsp! --\n\n");
    
    scheme_context *ctx = scheme_init();
    
    while (1) {
        char * line = readline("\nlsp> ");

        if (strcmp(line, "quit") == 0)
            break;

        add_history(line);
        
        scheme_obj *o = scheme_read(line, ctx);
        scheme_obj *eo = scheme_eval(o, ctx);

        char *res = scheme_print(eo);
        printf(": %s\n", res);
        
        scheme_obj_mark(o, UNUSED);
        scheme_obj_mark(eo, UNUSED);
    }
    
    scheme_shutdown(ctx);
    return 1;
}
