#include "interpreter/kutval.h"
#include "interpreter/kutstring.h"
#include "interpreter/kuttable.h"
#include "interpreter/kutfunc.h"
#include "interpreter/kutreference.h"

#include "kutparser.h"
#include "kutast.h"
#include "kutcompiler.h"

#include <stdio.h>
#include <math.h>
#include <inttypes.h>

void test_interpreter() {
    // KutValue literals[] = {
    //     kutstring_wrap(kutstring_literal("+")),
    //     kutnumber_wrap(0),
    //     kuttable_wrap(kuttable_literal(kutnumber_wrap(0), kutnumber_wrap(1), kutnumber_wrap(2), kutnumber_wrap(3), kutnumber_wrap(4))),
    //     kutstring_wrap(kutstring_literal("sar")),
    //     kutstring_wrap(kutstring_literal("*")),
    // };

    // const KutInstruction inner_instructions[] = {
    //     kutfunc_getliteral(2, 0),
    //     kutfunc_getliteral(3, 4),
    //     kutfunc_pushvalue1(1),
    //     kutfunc_methodcall(1, 1, 3),
    //     kutfunc_pushvalue1(1),
    //     kutfunc_methodcall(0, 0, 2),
    //     kutfunc_returncall(0),
    //     kutfunc_noperation(),
    // };
    // KutFuncTemplate* inner_template = kutfunc_templateLiteral(inner_instructions, literals, 4, 0);
    // const KutFuncTemplate* more_inner_template = kutfunc_templateLiteral(inner_instructions, literals, 2, 256+0);
    
    // const KutFuncTemplate* templates[] = {
    //     inner_template,
    //     more_inner_template,
    // };

    // inner_template->function_templates = templates;

    // const KutInstruction instructions[] = {
    //     kutfunc_gettmplate(1, 0),
    //     kutfunc_getliteral(0, 1),
    //     kutfunc_getliteral(2, 2),
    //     kutfunc_getliteral(3, 3),
    //     kutfunc_pushvalue1(1),
    //     kutfunc_methodcall(4, 2, 3),
    //     kutfunc_returncall(4),
    //     kutfunc_noperation(),
    // };
    // KutFuncTemplate template = {
    //     .closure_count = 0,
    //     .instructions = instructions,
    //     .literals = literals,
    //     .register_count = 7,
    //     .function_templates = templates,
    // };

    // KutValue func = kutfunc_wrap(kutfunc_new(NULL, &template));
    // KutValue ret = kutfunc_run(&func, empty_table);
    // KutString* str = kut_tostring(&ret, 0);
    // printf("%.*s\n", kutstring_format(str));
    // kut_decref(&func);
    // kut_decref(&ret);
    // free(str);
    // str = NULL;
}

void test_parser() {
    // const char* str =
    //     "x 5 olsun\\\n"
    //     "ekran x yazsin\n"
    //     "t [(x -2 ^) 2 3] olsun\n"
    //     "\n# a\n";
    // size_t len = strlen(str);
    // const char* endptr = str+len;
    // KutToken tok = (KutToken){.type = KUTTOKEN_START};
    // while((tok = next_token(&str, endptr)).type != KUTTOKEN_INVALID) {
    //     debug_token(tok);
    // }
}

void test_compiler() {
    #include "test.c"
    const char* bort = test_kut;
    const char* endptr = test_kut+sizeof(test_kut);
    KutASTNode nod = {0};
    KutCompilerInfo root_info;
    kutcompiler_new(NULL, &root_info);
    KutASTNode* nodes = calloc(1, sizeof(nodes[0]));
    size_t node_count = 0;
    while(bort != endptr) {
        nod = kutast_newStatement(&bort, endptr);
        nodes = realloc(nodes, (node_count+1)*sizeof(nodes[0]));
        nodes[node_count] = nod;
        node_count += 1;
    }
    nod = (KutASTNode){
        .argument_count = 0,
        .arguments = NULL,
        .children = nodes,
        .children_count = node_count,
        .token = invalid_token,
        .type = KUTAST_FUNCTION,
    };
    kutast_debug(nod, "|   ");
    kutcompiler_compileFunction(nod, &root_info, false, 0);
    kutast_destroy(nod);
    for(size_t i = 0; i < root_info.templates->len; i++) {
        printf("Template %zu\n", i);
        kutfunctemplate_debug(&root_info.templates->data[i]);
        printf("Literals: [");
        for(size_t j = 0; j < root_info.templates->data[i].literals->len; j++) {
            KutValue* self = &root_info.templates->data[i].literals->data[j];
            KutString* str = NULL;
            if(self->methods) {
                str = self->methods->tostring(self, 0);
            } else {
                printf("nil");
                goto zort;
            }
            printf("%.*s", kutstring_format(str));
            KutValue strself = kutstring_wrap(str);
            kut_decref(&strself);
zort:
            if(j != root_info.templates->data[0].literals->len-1) {
                printf(", ");
            }
        }
        printf("]\n");
        printf("\n");
        free(root_info.templates->data[i].capture_infos);
        free(root_info.templates->data[i].instructions);
    }
    for(size_t i = 0; i < root_info.templates->data[0].literals->len; i++) {
        KutValue* self = &root_info.templates->data[0].literals->data[i];
        kut_decref(self);
    }
    free(root_info.templates->data[0].literals->data);
    free(root_info.templates->data[0].literals);
    free(root_info.templates->data);
    free(root_info.templates);
    kutcompiler_destroyInfo(&root_info);
}

int main() {    
    test_compiler();
}
