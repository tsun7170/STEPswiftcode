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
#include "swift_files.h"
#include "swift_symbol.h"
#include "swift_expression.h"
#include "swift_type.h"


void SCOPElocalList_swift( Scope s, int level ) {
	Variable var;
	DictionaryEntry de;
	
	int i_local = 0;
	
	DICTdo_type_init( s->symbol_table, &de, OBJ_VARIABLE );
	while( 0 != ( var = ( Variable )DICTdo( &de ) ) ) {
		if( VARis_constant(var) ) continue;
		if( VARis_parameter(var) ) continue;
		
		if( ++i_local==1 ) {
			raw("\n");
			indent_swift(level);
			raw("//LOCAL\n");
		}
		
		indent_swift(level);
		wrap("var %s: ", variable_swiftName(var));
		
		variableType_swift(var, true, level);
		
		if( var->initializer ) {
			wrap( " = " );
			EXPR_swift( NULL, var->initializer, NO_PAREN );
		}
		
		raw( "\n" );
	}
		
	if(i_local > 1) {
		indent_swift(level);
		raw("//END_LOCAL\n");
	}
	if(i_local > 0 ) {
		raw("\n");
	}
}

