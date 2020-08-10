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


static void SCOPEconst_swift( char* access, Variable v, int level ) {
	
	/* print attribute name */
	indent_swift(level);
	raw( "%slet %s: ", access, variable_swiftName(v) );
	
	/* print attribute type */
	variableType_swift(v, false, NOLEVEL);
	
	
	if( v->initializer ) {
		raw( " = " );
		positively_wrap();
		
		EXPR_swift( NULL, v->initializer, NO_PAREN );
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
		
		SCOPEconst_swift( access, v, level );
	}
	
	if(iconst > 1) {
		indent_swift(level);
		raw( "//END_CONSTANT\n\n" );
	}
}

