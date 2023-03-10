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
    #include "test.kut.c"
        // "liste degiskeni [1 2 3 4] olsun\n"
        // "a degiskeni 5 olsun\n"
        // "b degiskeni 8 olsun\n"
        // "liste {x | liste {y | a y +\n"
        // "} zort\n"
        // "x 3 +\n}:ile esle\n"
        // "ekran (3 2 +) yazsin\n\n\n\n";
    const char* bort = test_kut;
    const char* endptr = test_kut+sizeof(test_kut);
    KutASTNode nod = {0};
    KutCompilerInfo root_info;
    KutTemplateArray arr;
    KutValueArray literals = {0};
    kutcompiler_new(NULL, &root_info);
    while(bort != endptr) {
        // printf("\"%s\"\n", bort);
        nod = kutast_newStatement(&bort, endptr);
        kutcompiler_compileStatement(nod, &root_info);
        kutast_destroy(nod);
    }
    KutValue* funcs = calloc(root_info.templates->len, sizeof(funcs[0]));
    for(size_t i = 0; i < root_info.templates->len; i++) {
        funcs[i] = kutfunc_wrap(kutfunc_new(i == 0 ? NULL : kutfunc_cast(funcs[i-1]), &root_info.templates->data[i]));
        KutString* d = kutstring_cast(kutfunc_debug(&funcs[i]));
        printf("Template %zu:\n%.*s\n", i, kutstring_format(d));
        free(d);
    }
    for(size_t i = 0; i < root_info.templates->len; i++) {
        kut_decref(&funcs[i]);
    }
    free(funcs);
    for(size_t i = 0; i < root_info.templates->len; i++) {
        free(root_info.templates->data[i].capture_infos);
        free(root_info.templates->data[i].instructions);
    }
    printf("Literals: [");
    for(size_t i = 0; i < root_info.templates->data[0].literals->len; i++) {
        KutValue* self = &root_info.templates->data[0].literals->data[i];
        KutString* str = self->methods->tostring(self, 0);
        printf("%.*s", kutstring_format(str));
        KutValue strself = kutstring_wrap(str);
        kut_decref(&strself);
        kut_decref(self);
        if(i != root_info.templates->data[0].literals->len-1) {
            printf(", ");
        }
    }
    printf("]\n");
    free(root_info.templates->data[0].literals->data);
    free(root_info.templates->data[0].literals);
    free(root_info.templates->data);
    free(root_info.templates);
    kutcompiler_destroyInfo(&root_info);
}

int main() {    
    test_compiler();
}
