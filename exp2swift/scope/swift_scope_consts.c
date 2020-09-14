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
#include "swift_algorithm.h"


static void SCOPEconst_swift(Scope current, char* access, Variable v, int level ) {
	
	/* print attribute name */
	indent_swift(level);
	{
		char buf[BUFSIZ];
		raw( "%slet %s: ", access, variable_swiftName(v,buf) );
	}
	
	/* print attribute type */
	variableType_swift(current, v, NO_FORCE_OPTIONAL, NOT_IN_COMMENT);
	
	
	if( v->initializer ) {
		raw( " = " );
		aggressively_wrap();
		int oldwrap = captureWrapIndent();
//		EXPR_swift( NULL, v->initializer, NO_PAREN );
		EXPRassignment_rhs_swift(current, v->initializer, v->type);
		restoreWrapIndent(oldwrap);
	}
	
	raw( "\n\n" );
}

/** output all consts in this scope */
void SCOPEconstList_swift( bool nested, Scope s, int level ) {
	Variable v;
	DictionaryEntry de;
	
	char* access = (nested ? "" : "public static ");
	
	int iconst = 0;
	DICTdo_type_init( s->symbol_table, &de, OBJ_VARIABLE );
	while( 0 != ( v = ( Variable )DICTdo( &de ) ) ) {
		if( !VARis_constant(v) ) continue;
		
		if(++iconst==1) {
			raw("\n");
			indent_swift(level);
			raw( "//CONSTANT\n" );
		}
		
		assert(v->name->type->superscope != NULL);
		assert(v->type->superscope != NULL);
		SCOPEconst_swift( s, access, v, level );
	}
	
	if(iconst > 1) {
		indent_swift(level);
		raw( "//END_CONSTANT\n\n" );
	}
}

