//
//  swift_scope_const.c
//  exp2swift
//
//  Created by Yoshida on 2020/06/13.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#include "pp.h"
#include "exppp.h"
#include "pretty_expr.h"
#include "pretty_rule.h"
#include "pretty_type.h"
#include "pretty_func.h"
#include "pretty_entity.h"
#include "pretty_proc.h"
#include "pretty_scope.h"

#include "swift_scope_consts.h"
#include "swift_files.h"
#include "swift_symbol.h"
#include "swift_type.h"
#include "swift_expression.h"


/** output one const - used in SCOPEconsts_out, below */
static void SCOPEconst_swift( Variable v, int level, int max_indent ) {
//    int old_indent2;

    /* print attribute name */
	indent_swift(level);
	raw( "public static let %s : ", variable_swiftName(v) );

    /* print attribute type */
	TYPE_head_swift( v->type, NOLEVEL );
    if( VARget_optional( v ) ) {
        raw( "?" );
    }


    if( v->initializer ) {
        int old_ll = exppp_linelength; /* so exppp_linelength can be restored */
        raw( " = " );

        /* let '[' on first line of initializer stick out so strings are aligned */
        raw( "\n%*s", indent2 - 2, "" );

        if( exppp_aggressively_wrap_consts ) {
            /* causes wrap() to always begin new line */
            exppp_linelength = indent2;
        }
        EXPR_swift( NULL, v->initializer, 0 );
        exppp_linelength = old_ll;
    }

    raw( "\n" );
}

/** output all consts in this scope */
void SCOPEconstList_swift( Scope s, int level ) {
    Variable v;
    DictionaryEntry de;
    int max_indent = 0;
    Dictionary d = s->symbol_table;

    /* checks length of constant names */
    DICTdo_type_init( d, &de, OBJ_VARIABLE );
    while( 0 != ( v = ( Variable )DICTdo( &de ) ) ) {
        if( !v->flags.constant ) {
            continue;
        }
        if( strlen( v->name->symbol.name ) > max_indent ) {
            max_indent = (int)strlen( v->name->symbol.name );
        }
    }

    if( !max_indent ) {
        return;
    }

    first_newline();

	indent_swift(level);
    raw( "/* CONSTANT */\n" );

    /* if max_indent is too big, wrap() won't insert any newlines
     * fiddled with this until it looked ok on 242 arm
     */
    if( ( max_indent + 20 ) > exppp_linelength / 2 ) {
        max_indent = exppp_linelength / 3;
    }
    indent2 = level + max_indent + strlen( ": ab" ) + exppp_continuation_indent;

    if( !exppp_alphabetize ) {
        DICTdo_type_init( d, &de, OBJ_VARIABLE );
        while( 0 != ( v = ( Variable )DICTdo( &de ) ) ) {
            if( !v->flags.constant ) {
                continue;
            }
            SCOPEconst_swift( v, level, max_indent );
        }
    } else {
        Linked_List alpha = LISTcreate();

        DICTdo_type_init( d, &de, OBJ_VARIABLE );
        while( 0 != ( v = ( Variable )DICTdo( &de ) ) ) {
            if( !v->flags.constant ) {
                continue;
            }
            SCOPEaddvars_inorder( alpha, v );
        }
        LISTdo( alpha, cnst, Variable ) {
            SCOPEconst_swift( cnst, level, max_indent );
        } LISTod
        LISTfree( alpha );
    }
	indent_swift(level);
    raw( "/* END_CONSTANT */\n" );
}

