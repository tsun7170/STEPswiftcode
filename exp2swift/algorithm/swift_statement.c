//
//  swift_statement.c
//  exp2swift
//
//  Created by Yoshida on 2020/07/28.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#include <exppp/exppp.h>
#include "pp.h"

#include "swift_statement.h"
#include "swift_files.h"
#include "swift_symbol.h"
#include "swift_expression.h"
#include "swift_proc.h"

//const char * alias_swiftName(Statement s) {
//	return s->symbol.name;
//}

static void CASE_swift( Scope algo, struct Case_Statement_ * case_stmt, int level ) {
	Statement otherwise = NULL;
	int level2 = level+nestingIndent_swift;
		
	raw("if let selector = ");
	switch ( EXPRresult_is_optional(case_stmt->selector, CHECK_DEEP) ) {
		case yes_optional:
			EXPR_swift(algo, case_stmt->selector, case_stmt->selector->return_type, NO_PAREN);
			break;
		case no_optional:				
		case unknown:
			raw("SDAI.FORCE_OPTIONAL(");
			EXPR_swift(algo, case_stmt->selector, case_stmt->selector->return_type, NO_PAREN);
			raw(")");
			break;
	}

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
				STMT_swift(algo, case_item->action, level3);
				raw("\n");
			} 
			else {
				/* OTHERWISE */
				indent_swift(level2);
				raw("default:\n");
				otherwise = case_item->action;
				STMT_swift(algo, otherwise, level3);
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
		STMT_swift(algo, otherwise, level2);
		indent_swift(level);
		raw("}\n");
	}
}

static void LOOPwithIncrementControl_swift( Scope algo, struct Loop_ *loop, int level ) {
	DictionaryEntry de;
	DICTdo_init( loop->scope->symbol_table, &de );
	Variable v = ( Variable )DICTdo( &de );

	wrap("if let incrementControl = SDAI.FROM(");
	EXPR_swift(algo, loop->scope->u.incr->init,VARget_type(v), NO_PAREN); raw(", ");
	wrap("TO:");
	EXPR_swift(algo, loop->scope->u.incr->end,VARget_type(v), NO_PAREN);
	if( loop->scope->u.incr->increment && 
		 !( TYPEis_integer(loop->scope->u.incr->increment->type) && (loop->scope->u.incr->increment->u.integer == 1)) ) {
		raw(", ");
		wrap("BY:");
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
				indent_swift(level3);
				wrap("if ");
//				EXPR_swift(algo, loop->while_expr, Type_Logical, YES_PAREN);
				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, algo, loop->while_expr, Type_Logical, YES_PAREN, OP_UNKNOWN, YES_WRAP);
				wrap(".isnotTRUE { break }\n");
			}
			
			STMTlist_swift(algo, loop->statements, level3);
			
			if( loop->until_expr ) {
				indent_swift(level3);
				wrap("if ");
				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, algo, loop->until_expr, Type_Logical, YES_PAREN, OP_UNKNOWN, YES_WRAP);
				wrap(".isTRUE { break }\n");
			}
		}
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw("}\n");	
}

static void LOOPwhile_swift( Scope algo, struct Loop_ *loop, int level ) {
	raw("while ");
//	EXPR_swift(algo, loop->while_expr, Type_Logical, YES_PAREN);
	EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, algo, loop->while_expr, Type_Logical, YES_PAREN, OP_UNKNOWN, YES_WRAP);
	wrap(".isnotTRUE {\n");
	
	{	int level2 = level+nestingIndent_swift;
		
		STMTlist_swift(algo, loop->statements, level2);
		
		if( loop->until_expr ) {
			indent_swift(level2);
			wrap("if ");
			EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, algo, loop->until_expr, Type_Logical, YES_PAREN, OP_UNKNOWN, YES_WRAP);
			wrap(".isTRUE { break }\n");
		}
	}
	
	indent_swift(level);
	raw("}\n");
}

static void LOOPuntil_swift( Scope algo, struct Loop_ *loop, int level ) {
	raw("repeat {\n");
	
	{	int level2 = level+nestingIndent_swift;
		
		STMTlist_swift(algo, loop->statements, level2);
	}
	
	indent_swift(level);
	raw("} while ");
//	EXPR_swift(algo, loop->until_expr, Type_Logical, YES_PAREN);
	EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, algo, loop->until_expr, Type_Logical, YES_PAREN, OP_UNKNOWN, YES_WRAP);
	wrap(".isTRUE\n");
}

static void LOOPwoRepeatControl_swift( Scope algo, struct Loop_ *loop, int level ) {
	raw("while true {\n");
	
	{	int level2 = level+nestingIndent_swift;
		
		STMTlist_swift(algo, loop->statements, level2);
	}
	
	indent_swift(level);
	raw("}\n");
}

static void LOOP_swift( Scope algo, struct Loop_ *loop, int level ) {
	if( loop->scope ) {
		LOOPwithIncrementControl_swift(algo, loop, level);
	}
	else if( loop->while_expr ) {
		LOOPwhile_swift(algo, loop, level);
	}
	else if( loop->until_expr ) {
		LOOPuntil_swift(algo, loop, level);
	}
	else {
		LOOPwoRepeatControl_swift(algo, loop, level);
	}
}

void STMT_swift( Scope algo, Statement stmt, int level ) {
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
			EXPR_swift(algo, lhs, lhs->return_type, NO_PAREN);
			raw(" = ");			
			//		EXPR_swift(NULL, stmt->u.assign->rhs, NO_PAREN);
			aggressively_wrap();
			EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, algo, stmt->u.assign->rhs, lhs->return_type, NO_PAREN,OP_UNKNOWN,YES_WRAP);
			raw("\n");
		}
			break;
			
			//MARK: STMT_CASE
		case STMT_CASE:
			CASE_swift( algo, stmt->u.Case, level );
			break;
			
			//MARK: STMT_COMPOUND
		case STMT_COMPOUND:
			raw("//BEGIN\n");
			STMTlist_swift(algo, stmt->u.compound->statements, level);
			indent_swift(level);
			raw("//END\n");
			break;
			
			//MARK: STMT_COND
		case STMT_COND:
			raw("if SDAI.IS_TRUE( ");
			EXPR_swift(algo, stmt->u.cond->test, Type_Logical, YES_PAREN);
			wrap(" ) {\n");
			
			STMTlist_swift(algo, stmt->u.cond->code, level+nestingIndent_swift);

			if( stmt->u.cond->otherwise ) {
				indent_swift(level);
				raw("}\n");
				indent_swift(level);
				raw("else {\n");
				
				STMTlist_swift(algo, stmt->u.cond->otherwise, level+nestingIndent_swift);
			}

			indent_swift(level);
			raw("}\n");
			break;
			
			//MARK: STMT_LOOP
		case STMT_LOOP:
			LOOP_swift( algo, stmt->u.loop, level );
			break;
			
			//MARK: STMT_PCALL
		case STMT_PCALL:
			if( stmt->u.proc->procedure->u.proc->builtin ) raw("SDAI.");
		{
			char buf[BUFSIZ];
			wrap("%s(", PROCcall_swiftName(stmt,buf));
		}
			
			char* sep = " ";
			Linked_List formals = stmt->u.proc->procedure->u.proc->parameters;
			Link formals_iter = LISTLINKfirst(formals);
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
			break;
			
			//MARK: STMT_RETURN
		case STMT_RETURN:
			raw("return ");
			if( stmt->u.ret->value ) {
				assert(algo->u_tag==scope_is_func);
				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, algo, stmt->u.ret->value, algo->u.func->return_type, NO_PAREN,OP_UNKNOWN,YES_WRAP);
//				EXPR_swift(NULL, stmt->u.ret->value, NO_PAREN);
			}
			raw("\n");
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
			
			STMTlist_swift(algo, stmt->u.alias->statements, level+nestingIndent_swift);

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

void STMTlist_swift( Scope algo, Linked_List stmts, int level ) {
	LISTdo(stmts, stmt, Statement) {
		STMT_swift(algo, stmt, level);
	}LISTod;
}
