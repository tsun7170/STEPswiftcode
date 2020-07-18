//
//  swift_scope_algs.c
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

#include "swift_scope_algs.h"

void SCOPEalgList_swift( Scope s, int level ) {
    /* Supplementary Directivies 2.1.1 requires rules to be separated */
    /* might as well separate funcs and procs, too */
    SCOPEruleList_swift( s, level );
    SCOPEfuncList_swift( s, level );
    SCOPEprocList_swift( s, level );
}


/** print the rules in a scope */
void SCOPEruleList_swift( Scope s, int level ) {
    Rule r;
    DictionaryEntry de;

    if( exppp_alphabetize == false ) {
        DICTdo_type_init( s->symbol_table, &de, OBJ_RULE );
        while( 0 != ( r = ( Rule )DICTdo( &de ) ) ) {
            RULE_out( r, level );
        }
    } else {
        Linked_List alpha = LISTcreate();

        DICTdo_type_init( s->symbol_table, &de, OBJ_RULE );
        while( 0 != ( r = ( Rule )DICTdo( &de ) ) ) {
            SCOPEadd_inorder( alpha, r );
        }

        LISTdo( alpha, ru, Rule ) {
            RULE_out( ru, level );
        } LISTod

        LISTfree( alpha );
    }

}

/** print the functions in a scope */
void SCOPEfuncList_swift( Scope s, int level ) {
    Function f;
    DictionaryEntry de;

    if( exppp_alphabetize == false ) {
        DICTdo_type_init( s->symbol_table, &de, OBJ_FUNCTION );
        while( 0 != ( f = ( Function )DICTdo( &de ) ) ) {
            FUNC_out( f, level );
        }
    } else {
        Linked_List alpha = LISTcreate();

        DICTdo_type_init( s->symbol_table, &de, OBJ_FUNCTION );
        while( 0 != ( f = ( Function )DICTdo( &de ) ) ) {
            SCOPEadd_inorder( alpha, f );
        }

        LISTdo( alpha, fun, Function ) {
            FUNC_out( fun, level );
        } LISTod

        LISTfree( alpha );
    }

}

/* print the procs in a scope */
void SCOPEprocList_swift( Scope s, int level ) {
    Procedure p;
    DictionaryEntry de;

    if( exppp_alphabetize == false ) {
        DICTdo_type_init( s->symbol_table, &de, OBJ_PROCEDURE );
        while( 0 != ( p = ( Procedure )DICTdo( &de ) ) ) {
            PROC_out( p, level );
        }
    } else {
        Linked_List alpha = LISTcreate();

        DICTdo_type_init( s->symbol_table, &de, OBJ_PROCEDURE );
        while( 0 != ( p = ( Procedure )DICTdo( &de ) ) ) {
            SCOPEadd_inorder( alpha, p );
        }

        LISTdo( alpha, pr, Procedure ) {
            PROC_out( pr, level );
        } LISTod

        LISTfree( alpha );
    }

}
