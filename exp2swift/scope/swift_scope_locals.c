//
//  swift_scope_locals.c
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
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

#include "swift_scope_locals.h"

void SCOPElocalList_swift( Scope s, int level ) {
    Variable v;
    DictionaryEntry de;
    Linked_List orderedLocals = 0; /**< this list is used to order the vars the same way they were in the file */
    size_t max_indent = 0;
    Dictionary d = s->symbol_table;

    DICTdo_type_init( d, &de, OBJ_VARIABLE );
    while( 0 != ( v = ( Variable )DICTdo( &de ) ) ) {
        if( v->flags.constant ) {
            continue;
        }
        if( v->flags.parameter ) {
            continue;
        }
        if( strlen( v->name->symbol.name ) > max_indent ) {
            max_indent = strlen( v->name->symbol.name );
        }
    }

    if( !max_indent ) {
        return;
    }

    first_newline();

    raw( "%*sLOCAL\n", level, "" );
    indent2 = level + max_indent + strlen( ": " ) + exppp_continuation_indent;

    DICTdo_type_init( d, &de, OBJ_VARIABLE );
    while( 0 != ( v = ( Variable )DICTdo( &de ) ) ) {
        if( v->flags.constant ) {
            continue;
        }
        if( v->flags.parameter ) {
            continue;
        }
        if( !orderedLocals ) {
            orderedLocals = LISTcreate();
            LISTadd_first( orderedLocals, (Generic) v );
        } else {
            /* sort by v->offset */
            SCOPElocals_order( orderedLocals, v );
        }
    }
    LISTdo( orderedLocals, var, Variable ) {
        /* print attribute name */
        raw( "%*s%-*s :", level + exppp_nesting_indent, "",
             max_indent, var->name->symbol.name );

        /* print attribute type */
        if( VARget_optional( var ) ) {
            wrap( " OPTIONAL" );
        }
        TYPE_head_out( var->type, NOLEVEL );

        if( var->initializer ) {
            wrap( " := " );
            EXPR_out( var->initializer, 0 );
        }

        raw( ";\n" );
    } LISTod
    LISTfree( orderedLocals );

    raw( "%*sEND_LOCAL;\n", level, "" );
}
