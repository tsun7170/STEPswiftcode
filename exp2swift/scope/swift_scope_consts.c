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
		if( TYPEis_logical(VARget_type(v)) ){
			raw( " = SDAI.LOGICAL(" );
		}
		else{
			raw( " = SDAI.UNWRAP(" );
		}
		aggressively_wrap();
		int oldwrap = captureWrapIndent();
//		EXPR_swift( NULL, v->initializer, NO_PAREN );
		EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, current, v->initializer, v->type, NO_PAREN,OP_UNKNOWN,YES_WRAP);
		restoreWrapIndent(oldwrap);
	}
	
	raw( ")\n\n" );
}

/** output all consts in this scope */
void SCOPEconstList_swift( bool nested, Scope s, Linked_List orderedConsts, int level ) {
	Variable var;
	DictionaryEntry de;
//	Linked_List orderedConsts = NULL;
	int num_consts = 0;
	
	DICTdo_type_init( s->symbol_table, &de, OBJ_VARIABLE );
	while( 0 != (var = DICTdo(&de)) ) {
		if( !VARis_constant(var) ) continue;
		++num_consts;
		
//		if( !orderedConsts ) {
//			orderedConsts = LISTcreate();
//			LISTadd_first( orderedConsts, var );
//		} 
//		else {
			/* sort by v->offset */
			SCOPElocals_order( orderedConsts, var );
//		}
	}

	if( num_consts == 0 )return;
	
	raw("\n");
	indent_swift(level);
	raw( "//CONSTANT\n" );
	
	char* access = (nested ? "" : "public static ");
	
	LISTdo(orderedConsts, var, Variable) {
		assert(var->name->type->superscope != NULL);
		assert(var->type->superscope != NULL);
		SCOPEconst_swift( s, access, var, level );
	}LISTod;
	
	if(num_consts > 1) {
		indent_swift(level);
		raw( "//END_CONSTANT\n\n" );
	}
}

