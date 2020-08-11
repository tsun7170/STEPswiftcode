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
		else if( TYPEhas_bounds(var->type) ) {
			const char* aggr;
			switch (TYPEget_type(var->type)) {
				case aggregate_:
					aggr = TYPE_swiftName(TYPEget_body(var->type)->tag);
					break;
				case array_:
					aggr = "SDAI.ARRAY";
					break;
					
				case bag_:
					aggr = "SDAI.BAG" ;
					break;
					
				case set_:
					aggr = "SDAI.SET" ;
					break;
					
				case list_:
					aggr = "SDAI.LIST" ;
					break;
					
				default:
					aggr = "#UNKNOWN_TYPE#";
					break;
			}
			wrap(" = %s(bound1:",aggr);
			EXPR_swift(NULL,TYPEget_body(var->type)->lower, NO_PAREN);
			raw(", bound2:");
			EXPR_swift(NULL,TYPEget_body(var->type)->upper, NO_PAREN);
			raw(")");
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

