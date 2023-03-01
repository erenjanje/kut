#include "kutval.h"
#include "kutstring.h"
#include "kuttable.h"
#include "kutfunc.h"
#include "kutreference.h"

#include <stdio.h>
#include <math.h>
#include <inttypes.h>


int main() {    
    KutValue literals[] = {
        kutstring_wrap(kutstring_literal("+")),
        kutnumber_wrap(0),
        kuttable_wrap(kuttable_literal(kutnumber_wrap(0), kutnumber_wrap(1), kutnumber_wrap(2), kutnumber_wrap(26))),
        kutstring_wrap(kutstring_literal("eşle")),
    };

    const KutInstruction inner_instructions[] = {
        kutfunc_getliteral(1, 0),
        kutfunc_getclosure(2, 0),
        kutfunc_pushvalue1(2),
        kutfunc_methodcall(2, 0, 1),
        kutfunc_setclosure(2, 0),
        kutfunc_returncall(2),
        kutfunc_noperation(),
    };
    KutFuncTemplate* inner_template = kutfunc_templateLiteral(inner_instructions, literals, 4, 0);
    const KutFuncTemplate* more_inner_template = kutfunc_templateLiteral(inner_instructions, literals, 2, 256+0);
    
    const KutFuncTemplate* templates[] = {
        inner_template,
        more_inner_template,
    };

    inner_template->function_templates = templates;

    const KutInstruction instructions[] = {
        kutfunc_gettmplate(1, 0),
        kutfunc_getliteral(0, 1),
        kutfunc_getliteral(2, 2),
        kutfunc_getliteral(3, 3),
        kutfunc_pushvalue1(1),
        kutfunc_methodcall(4, 2, 3),
        kutfunc_returncall(1),
        kutfunc_noperation(),
    };
    KutFuncTemplate template = {
        .capture_count = 0,
        .instructions = instructions,
        .literals = literals,
        .register_count = 7,
        .function_templates = templates,
    };

    KutValue func = kutfunc_wrap(kutfunc_new(NULL, &template));
    KutValue ret = kutfunc_run(&func, empty_table);
    KutString* str = kut_tostring(&func, 0);
    // printf("%.*s\n", kutstring_format(str));
    kut_decref(&func);
    kut_decref(&ret);
    free(str);
    str = NULL;
    return 0;
}
