//
//  swift_statement.c
//  exp2swift
//
//  Created by Yoshida on 2020/07/28.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

#include <exppp/exppp.h>
#include "pp.h"

#include "swift_statement.h"
#include "swift_files.h"
#include "swift_symbol.h"
#include "swift_expression.h"
#include "swift_proc.h"
#include "decompose_expression.h"
#include "swift_func.h"

//const char * alias_swiftName(Statement s) {
//	return s->symbol.name;
//}

static void CASE_swift( Scope algo, struct Case_Statement_ * case_stmt, int* tempvar_id, int level ) {
	Statement otherwise = NULL;
	int level2 = level+nestingIndent_swift;

	Linked_List tempvars;
	Expression simplified = EXPR_decompose(case_stmt->selector, case_stmt->selector->return_type, tempvar_id, &tempvars);
	if( EXPR_tempvars_swift(algo, tempvars, level) > 0 ) indent_swift(level);

	raw("if let selector = ");
	switch ( EXPRresult_is_optional(simplified, CHECK_DEEP) ) {
		case yes_optional:
			EXPR_swift(algo, simplified, case_stmt->selector->return_type, NO_PAREN);
			break;
		case no_optional:				
		case unknown:
			raw("SDAI.FORCE_OPTIONAL(");
			EXPR_swift(algo, simplified, case_stmt->selector->return_type, NO_PAREN);
			raw(")");
			break;
	}
	EXPR_delete_tempvar_definitions(tempvars);

	raw(" {\n");
	{	
		int level3 = level2+nestingIndent_swift;
		
		indent_swift(level2);
		raw("switch selector {\n");
				
		LISTdo( case_stmt->cases, case_item, Case_Item ) {
			if( case_item->labels ) {
				char* sep = "";
				
				// case label
				indent_swift(level2);
				raw("case ");
				
				LISTdo_n( case_item->labels, label, Expression, b ) {
					raw("%s",sep);
					EXPR_swift(algo, label, NULL, NO_PAREN);
					sep = ", ";
				} LISTod
				
				raw(":\n");
				STMT_swift(algo, case_item->action, tempvar_id, level3);
				raw("\n");
			} 
			else {
				/* OTHERWISE */
				indent_swift(level2);
				raw("default:\n");
				otherwise = case_item->action;
				STMT_swift(algo, otherwise, tempvar_id, level3);
			}
		} LISTod
		
		if( otherwise == NULL ){
			indent_swift(level2);
			raw("default: break\n");
		}
		
		indent_swift(level2);
		raw("} //end switch\n");
	}
	indent_swift(level);
	raw("}\n");

	if( otherwise != NULL ){
		indent_swift(level);
		raw("else {\n");
		STMT_swift(algo, otherwise, tempvar_id, level2);
		indent_swift(level);
		raw("}\n");
	}
}

static void LOOPwithIncrementControl_swift( Scope algo, struct Loop_ *loop, int* tempvar_id, int level ) {
	DictionaryEntry de;
	DICTdo_init( loop->scope->symbol_table, &de );
	Variable v = ( Variable )DICTdo( &de );

	wrap("if let incrementControl");
	raw("/*");TYPE_head_swift(loop->scope, VARget_type(v), YES_IN_COMMENT, LEAF_OWNED); raw("*/"); // DEBUG
	
	wrap(" = SDAI.FROM(");
	raw("/*");TYPE_head_swift(algo, loop->scope->u.incr->init->return_type, YES_IN_COMMENT, LEAF_OWNED); raw("*/"); // DEBUG
	EXPR_swift(algo, loop->scope->u.incr->init,VARget_type(v), NO_PAREN); raw(", ");

	wrap("TO:");
	raw("/*");TYPE_head_swift(algo, loop->scope->u.incr->end->return_type, YES_IN_COMMENT, LEAF_OWNED); raw("*/"); // DEBUG
	EXPR_swift(algo, loop->scope->u.incr->end,VARget_type(v), NO_PAREN);

	if( loop->scope->u.incr->increment && 
		 !( TYPEis_integer(loop->scope->u.incr->increment->type) && (loop->scope->u.incr->increment->u.integer == 1)) ) {
		raw(", ");
		wrap("BY:");
		raw("/*");TYPE_head_swift(algo, loop->scope->u.incr->increment->return_type, YES_IN_COMMENT, LEAF_OWNED); raw("*/"); // DEBUG
		EXPR_swift(algo, loop->scope->u.incr->increment,VARget_type(v), NO_PAREN);		
	}

	raw(") {\n");
	{	int level2 = level+nestingIndent_swift;

		{
			char buf[BUFSIZ];
			indent_swift(level2);
			wrap("for %s in incrementControl {\n",variable_swiftName(v, buf));
		}
		{	int level3 = level2+nestingIndent_swift;
			
			if( loop->while_expr ) {
				Linked_List tempvars;
				Expression simplified = EXPR_decompose(loop->while_expr, Type_Logical, tempvar_id, &tempvars);
				EXPR_tempvars_swift(algo, tempvars, level2);

				indent_swift(level3);
				raw("if ");
				raw("!SDAI.IS_TRUE(");
				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, algo, simplified, Type_Logical, YES_PAREN, OP_UNKNOWN, YES_WRAP);
				raw(") { break }\n");

				EXPR_delete_tempvar_definitions(tempvars);
			}
			
			STMTlist_swift(algo, loop->statements, tempvar_id, level3);
			
			if( loop->until_expr ) {
				Linked_List tempvars;
				Expression simplified = EXPR_decompose(loop->until_expr, Type_Logical, tempvar_id, &tempvars);
				EXPR_tempvars_swift(algo, tempvars, level2);

				indent_swift(level3);
				raw("if ");
				raw("SDAI.IS_TRUE(");
				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, algo, simplified, Type_Logical, YES_PAREN, OP_UNKNOWN, YES_WRAP);
				raw(") { break }\n");

				EXPR_delete_tempvar_definitions(tempvars);
			}
		}
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw("}\n");	
}

static void LOOPwhile_swift( Scope algo, struct Loop_ *loop, int* tempvar_id, int level ) {
	raw("while ");
//	EXPR_swift(algo, loop->while_expr, Type_Logical, YES_PAREN);
	wrap("!SDAI.IS_TRUE(");
	EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, algo, loop->while_expr, Type_Logical, YES_PAREN, OP_UNKNOWN, YES_WRAP);
	wrap(") {\n");
	
	{	int level2 = level+nestingIndent_swift;
		
		STMTlist_swift(algo, loop->statements, tempvar_id, level2);
		
		if( loop->until_expr ) {
			indent_swift(level2);
			raw("if ");
			raw("SDAI.IS_TRUE(");
			EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, algo, loop->until_expr, Type_Logical, YES_PAREN, OP_UNKNOWN, YES_WRAP);
			raw(") { break }\n");
		}
	}
	
	indent_swift(level);
	raw("}\n");
}

static void LOOPuntil_swift( Scope algo, struct Loop_ *loop, int* tempvar_id, int level ) {
	raw("repeat {\n");
	
	{	int level2 = level+nestingIndent_swift;
		
		STMTlist_swift(algo, loop->statements, tempvar_id, level2);
	}
	
	indent_swift(level);
	raw("} while ");
	raw("SDAI.IS_TRUE(");
	EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, algo, loop->until_expr, Type_Logical, YES_PAREN, OP_UNKNOWN, YES_WRAP);
	raw(")\n");
}

static void LOOPwoRepeatControl_swift( Scope algo, struct Loop_ *loop, int* tempvar_id, int level ) {
	raw("while true {\n");
	
	{	int level2 = level+nestingIndent_swift;
		
		STMTlist_swift(algo, loop->statements, tempvar_id, level2);
	}
	
	indent_swift(level);
	raw("}\n");
}

static void LOOP_swift( Scope algo, struct Loop_ *loop, int* tempvar_id, int level ) {
	if( loop->scope ) {
		LOOPwithIncrementControl_swift(algo, loop, tempvar_id, level);
	}
	else if( loop->while_expr ) {
		LOOPwhile_swift(algo, loop, tempvar_id, level);
	}
	else if( loop->until_expr ) {
		LOOPuntil_swift(algo, loop, tempvar_id, level);
	}
	else {
		LOOPwoRepeatControl_swift(algo, loop, tempvar_id, level);
	}
}

void STMT_swift( Scope algo, Statement stmt, int* tempvar_id, int level ) {
	indent_swift(level);

	if( !stmt ) {  /* null statement */
		raw( "/*null statement*/;\n" );
		return;
	}
		
	switch( stmt->type ) {
			//MARK: STMT_ASSIGN
		case STMT_ASSIGN:
		{
			Expression lhs = stmt->u.assign->lhs;
			
			Linked_List tempvars;
			Expression rhs_simplified = EXPR_decompose(stmt->u.assign->rhs, lhs->return_type, tempvar_id, &tempvars);
			Expression lhs_simplified = EXPR_decompose_lhs(lhs, lhs->return_type, tempvar_id, tempvars);
			if( EXPR_tempvars_swift(algo, tempvars, level) > 0 )indent_swift(level);

			EXPR_swift(algo, lhs_simplified, lhs->return_type, NO_PAREN);
			raw(" = ");			
			aggressively_wrap();
			if( EXPRresult_is_optional(lhs, CHECK_SHALLOW) == yes_optional ){
				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, algo, rhs_simplified, lhs->return_type, NO_PAREN,OP_UNKNOWN,YES_WRAP);				
			}
			else {
				wrap("SDAI.UNWRAP(");
				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, algo, rhs_simplified, lhs->return_type, NO_PAREN,OP_UNKNOWN,YES_WRAP);		
				raw(")");
			}
			raw("\n");

			EXPR_delete_tempvar_definitions(tempvars);
		}
			break;
			
			//MARK: STMT_CASE
		case STMT_CASE:
			CASE_swift( algo, stmt->u.Case, tempvar_id, level );
			break;
			
			//MARK: STMT_COMPOUND
		case STMT_COMPOUND:
		{
			raw("//BEGIN\n");
			STMTlist_swift(algo, stmt->u.compound->statements, tempvar_id, level);
			indent_swift(level);
			raw("//END\n");
		}
			break;
			
			//MARK: STMT_COND
		case STMT_COND:
		{
			Linked_List tempvars;
			Expression simplified = EXPR_decompose(stmt->u.cond->test, Type_Logical, tempvar_id, &tempvars);
			if( EXPR_tempvars_swift(algo, tempvars, level) > 0 ) indent_swift(level);
			
			raw("if SDAI.IS_TRUE( ");
			EXPR_swift(algo, simplified, Type_Logical, YES_PAREN);
			wrap(" ) {\n");
			
			STMTlist_swift(algo, stmt->u.cond->code, tempvar_id, level+nestingIndent_swift);

			if( stmt->u.cond->otherwise ) {
				indent_swift(level);
				raw("}\n");
				indent_swift(level);
				raw("else {\n");
				
				STMTlist_swift(algo, stmt->u.cond->otherwise, tempvar_id, level+nestingIndent_swift);
			}

			indent_swift(level);
			raw("}\n");
			
			EXPR_delete_tempvar_definitions(tempvars);
		}
			break;
			
			//MARK: STMT_LOOP
		case STMT_LOOP:
			LOOP_swift( algo, stmt->u.loop, tempvar_id, level );
			break;
			
			//MARK: STMT_PCALL
		case STMT_PCALL:
		{
			if( stmt->u.proc->procedure->u.proc->builtin ) raw("SDAI.");
		{
			char buf[BUFSIZ];
			wrap("%s(", PROCcall_swiftName(stmt,buf));
		}
			
			char* sep = " ";
			Linked_List formals = stmt->u.proc->procedure->u.proc->parameters;
			Link formals_iter = LISTLINKfirst(formals);
			
			assert(LISTget_length(formals) == LISTget_length(stmt->u.proc->parameters));
			
			LISTdo(stmt->u.proc->parameters, actual_param, Expression) {
				raw("%s",sep);
				positively_wrap();
				
				assert(formals_iter != NULL);
				Variable formal_param = formals_iter->data;
				{
					char buf[BUFSIZ];
					wrap("%s: ", variable_swiftName(formal_param,buf));
				}
				if(VARis_inout(formal_param)) {
					raw("&");
					EXPR_swift(algo, actual_param, actual_param->return_type, NO_PAREN );
				}
				else {
					EXPRassignment_rhs_swift(YES_RESOLVING_GENERIC, algo, actual_param, formal_param->type, NO_PAREN,OP_UNKNOWN,YES_WRAP);
				}
				
				sep = ", ";
				formals_iter = LISTLINKnext(formals, formals_iter);
			}LISTod;
			
			raw(" )\n");
		}
			break;
			
			//MARK: STMT_RETURN
		case STMT_RETURN:
			if( stmt->u.ret->value == NULL ){
				raw("return\n");
			}
			else {
				assert(SCOPEis_function(algo));
				bool nested = !SCOPEis_schema(algo->superscope);
				
				
				Linked_List tempvars;
				Expression simplified = EXPR_decompose(stmt->u.ret->value, algo->u.func->return_type, tempvar_id, &tempvars);
				if( EXPR_tempvars_swift(algo, tempvars, level) > 0 ) indent_swift(level);
				
				raw("return ");
				assert(algo->u_tag==scope_is_func);
				char* closing = ")))";closing+=3;
				if( !nested ){
					char buf[BUFSIZ];
					raw("%s.updateCache(params: _params, value: ", FUNC_cache_swiftName(algo, buf));
					--closing;
				}
				if( !FUNCreturn_indeterminate(algo) ){
					raw("SDAI.UNWRAP(");
					--closing;
				}
				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, algo, simplified, algo->u.func->return_type, NO_PAREN,OP_UNKNOWN,YES_WRAP);
				raw(closing);
				raw("\n");
				
				EXPR_delete_tempvar_definitions(tempvars);
			}
			break;
			
			//MARK: STMT_ALIAS
		case STMT_ALIAS:
		{
			char buf[BUFSIZ];
			raw("do {\t/* ALIAS (%s)", variable_swiftName(stmt->u.alias->variable,buf) );
		}
			wrap(" FOR (");
			EXPR_swift(algo, stmt->u.alias->variable->initializer, stmt->u.alias->variable->initializer->return_type, YES_PAREN);
			raw(") */\n");
			
			STMTlist_swift(algo, stmt->u.alias->statements, tempvar_id, level+nestingIndent_swift);

			indent_swift(level);
		{
			char buf[BUFSIZ];			
			raw("} //END ALIAS (%s)\n",variable_swiftName(stmt->u.alias->variable,buf));
		}
			break;
			
			//MARK: STMT_SKIP
		case STMT_SKIP:
			raw("continue\n");
			break;
			
			//MARK: STMT_ESCAPE
		case STMT_ESCAPE:
			raw("break\n");
			break;
	}
}

void STMTlist_swift( Scope algo, Linked_List stmts, int* tempvar_id, int level ) {
	LISTdo(stmts, stmt, Statement) {
		STMT_swift(algo, stmt, tempvar_id, level);
	}LISTod;
}
