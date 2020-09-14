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
	int level2 = level+nestingIndent_swift;
	
	raw("switch ");
	EXPR_swift(NULL, case_stmt->selector, NO_PAREN);
	raw(" {\n");
	
	LISTdo( case_stmt->cases, case_item, Case_Item ) {
		if( case_item->labels ) {
			// case label mark
			indent_swift(level);
			raw("//MARK:");
			indent_with_char(level/nestingIndent_swift-3, '.');
			
			char* sep = "";
			LISTdo_n( case_item->labels, label, Expression, b ) {
				raw("%s",sep);
				EXPR_swift(NULL, label, NO_PAREN);
				sep = ", ";
			} LISTod
			raw("\n");
						
			// case label
			indent_swift(level);
			raw("case ");
			
			sep = "";
			LISTdo_n( case_item->labels, label, Expression, b ) {
				raw("%s",sep);
				EXPR_swift(NULL, label, NO_PAREN);
				sep = ", ";
			} LISTod
			
			raw(":\n");
			STMT_swift(algo, case_item->action, level2);
			raw("\n");
		} 
		else {
			/* print OTHERWISE */
			indent_swift(level);
			raw("default:\n");
			STMT_swift(algo, case_item->action, level2);
		}
	} LISTod
	
	indent_swift(level);
	raw("} //end switch\n");
}

static void LOOPwithIncrementControl_swift( Scope algo, struct Loop_ *loop, int level ) {
	DictionaryEntry de;
	DICTdo_init( loop->scope->symbol_table, &de );
	Variable v = ( Variable )DICTdo( &de );

	{
		char buf[BUFSIZ];
		wrap("for %s in ",variable_swiftName(v, buf));
	}
	
	if( loop->scope->u.incr->increment && 
		 !(TYPEis(loop->scope->u.incr->increment->type) == integer_ && (loop->scope->u.incr->increment->u.integer == 1)) ) {
		wrap("stride(from: ");
		EXPR_swift(NULL, loop->scope->u.incr->init, NO_PAREN);
		
		wrap(", through: ");
		EXPR_swift(NULL, loop->scope->u.incr->end, NO_PAREN);
		
		wrap(", by: ");
		EXPR_swift(NULL, loop->scope->u.incr->increment, NO_PAREN);
		
		raw(") {\n");
	}
	else {
		EXPR_swift(NULL, loop->scope->u.incr->init, YES_PAREN);
		wrap(" ... ");
		EXPR_swift(NULL, loop->scope->u.incr->end, YES_PAREN);
		raw(" {\n");
	}
	
	{	int level2 = level+nestingIndent_swift;
		
		if( loop->while_expr ) {
			indent_swift(level2);
			wrap("if ");
			EXPR_swift(NULL, loop->while_expr, YES_PAREN);
			wrap(".isnotTRUE { break }\n");
		}
		
		STMTlist_swift(algo, loop->statements, level2);
		
		if( loop->until_expr ) {
			indent_swift(level2);
			wrap("if ");
			EXPR_swift(NULL, loop->until_expr, YES_PAREN);
			wrap(".isTRUE { break }\n");
		}
	}
	
	indent_swift(level);
	raw("}\n");
}

static void LOOPwhile_swift( Scope algo, struct Loop_ *loop, int level ) {
	raw("while ");
	EXPR_swift(NULL, loop->while_expr, YES_PAREN);
	wrap(".isTRUE {\n");
	
	{	int level2 = level+nestingIndent_swift;
		
		STMTlist_swift(algo, loop->statements, level2);
		
		if( loop->until_expr ) {
			indent_swift(level2);
			wrap("if ");
			EXPR_swift(NULL, loop->until_expr, YES_PAREN);
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
	EXPR_swift(NULL, loop->until_expr, YES_PAREN);
	wrap(".isnotTRUE\n");
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
			EXPR_swift(NULL, lhs, NO_PAREN);
			raw(" = ");			
			//		EXPR_swift(NULL, stmt->u.assign->rhs, NO_PAREN);
			aggressively_wrap();
			EXPRassignment_rhs_swift(NULL, stmt->u.assign->rhs, lhs->return_type);
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
			raw("if ");
			EXPR_swift(NULL, stmt->u.cond->test, YES_PAREN);
			wrap(".isTRUE {\n");
			
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
					EXPR_swift(NULL, actual_param, NO_PAREN );
				}
				else {
					EXPRassignment_rhs_swift(NULL, actual_param, formal_param->type);
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
				EXPRassignment_rhs_swift(algo, stmt->u.ret->value, algo->u.func->return_type);
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
			EXPR_swift(NULL, stmt->u.alias->variable->initializer, YES_PAREN);
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
