//
//  swift_scope_locals.c
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
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
#include "swift_algorithm.h"


void SCOPElocalList_swift( Scope s, int level ) {
	Variable var;
	DictionaryEntry de;
	Linked_List orderedLocals = NULL; /**< this list is used to order the vars the same way they were in the file */
	int num_locals = 0;
	
	DICTdo_type_init( s->symbol_table, &de, OBJ_VARIABLE );
	while( 0 != (var = DICTdo(&de)) ) {
		if( var->flags.constant ) {
			continue;
		}
		if( var->flags.parameter ) {
			continue;
		}
		++num_locals;
		
		if( !orderedLocals ) {
			orderedLocals = LISTcreate();
			LISTadd_first( orderedLocals, (Generic) var );
		} 
		else {
			/* sort by v->offset */
			SCOPElocals_order( orderedLocals, var );
		}
	}

	if( num_locals == 0 )return;
	
	indent_swift(level);
	raw("//LOCAL\n");

	LISTdo( orderedLocals, var, Variable ) {
		char var_name[BUFSIZ];
		indent_swift(level);
		raw("var %s: ", variable_swiftName(var,var_name));
		
		variableType_swift(s, var, NO_FORCE_OPTIONAL, NOT_IN_COMMENT);
		
		if( var->initializer ) {
			raw( " = " );
			if( TYPEis_aggregation_data_type(var->type) ){
//			force_wrap();
//				aggressively_wrap();
				positively_wrap();
			}
			else {
				positively_wrap();
//			aggressively_wrap();
			}
			
			{	int oldwrap = captureWrapIndent();
				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, s, var->initializer, var->type, NO_PAREN,OP_UNKNOWN,YES_WRAP);
				restoreWrapIndent(oldwrap);
			}
			raw( "; SDAI.TOUCH(var: &%s)", var_name );
		}
		
		else if( TYPEhas_bounds(var->type) && !VARis_optional_by_large(var) ) {
//			char buf[BUFSIZ];
//			const char* aggr;
//			switch (TYPEget_type(var->type)) {
//				case aggregate_:
//					aggr = TYPE_swiftName(TYPEget_body(var->type)->tag,NO_QUALIFICATION,buf);
//					break;
//				case array_:
//					aggr = "SDAI.ARRAY";
//					break;
//					
//				case bag_:
//					aggr = "SDAI.BAG" ;
//					break;
//					
//				case set_:
//					aggr = "SDAI.SET" ;
//					break;
//					
//				case list_:
//					aggr = "SDAI.LIST" ;
//					break;
//					
//				default:
//					aggr = "#UNKNOWN_TYPE#";
//					break;
//			}
			raw( " = " );
			force_wrap();
//		aggressively_wrap();
//		positively_wrap();
			{	int oldwrap = captureWrapIndent();
				TYPE_head_swift(s, var->type, WO_COMMENT, LEAF_OWNED);
				wrap("(bound1: ");
				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, s, TYPEget_body(var->type)->lower, Type_Integer, NO_PAREN,OP_UNKNOWN,YES_WRAP);
				raw(", bound2: ");
				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, s, TYPEget_body(var->type)->upper, Type_Integer, NO_PAREN,OP_UNKNOWN,YES_WRAP);
				raw(")");
				restoreWrapIndent(oldwrap);
			}
//			raw( "; SDAI.TOUCH(var: &%s)", var_name );
		}
		raw( "\n" );
	}LISTod;
			
	if(num_locals > 1) {
		indent_swift(level);
		raw("//END_LOCAL\n");
	}
	raw("\n");
}

