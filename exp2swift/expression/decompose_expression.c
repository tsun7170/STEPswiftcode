//
//  decompose_expression.c
//  exp2swift
//
//  Created by Yoshida on 2021/01/16.
//  Copyright © 2021 Minokamo, Japan. All rights reserved.
//

#include "decompose_expression.h"
#include "sc_memmgr.h"
#include "builtin.h"

#include "pp.h"
#include "exppp.h"

#include "swift_files.h"
#include "swift_symbol.h"
#include "swift_expression.h"


static const char* TEMPVAR_MARK = "_TEMPVAR_";
#define COPIED_EXPR_MARK	(-1)

static bool is_temp_expr(Expression e){
	return e->symbol.filename == TEMPVAR_MARK && e->symbol.line > 0;	
}
static bool is_copied_expr(Expression e){
	return e->symbol.filename == TEMPVAR_MARK && e->symbol.line == COPIED_EXPR_MARK;
}

static void assign_tempvar_symbol(Expression e, int*/*inout*/ tempvar_id){
	assert(*tempvar_id > 0);
	
	char buf[BUFSIZ];
	sprintf(buf, "_temp%d",*tempvar_id);
	assert(strlen(buf) < BUFSIZ);
	
	char* symbol_name = sc_malloc(strlen(buf)+1);
	strncpy(symbol_name, buf, strlen(buf)+1);
	
	e->symbol.name = symbol_name;
	e->symbol.filename = TEMPVAR_MARK;
	e->symbol.line = (*tempvar_id);
	
	++(*tempvar_id);
}

static Expression create_temp_expression(int*/*inout*/ tempvar_id, Expression definition, Linked_List tempvars){
	Expression simplified = EXP_new();
	assign_tempvar_symbol(simplified, tempvar_id);
	simplified->type = Type_Identifier;
	simplified->return_type = definition->return_type;
	simplified->u.user_defined = (Generic)EXPRresult_is_optional(definition, CHECK_DEEP);
	simplified->u_tag = expr_is_user_defined;
	
	LISTadd_last_marking_aux(tempvars, definition, simplified);
	return simplified;
}

static Expression copy_expression( Expression original ){
	Expression copied = EXP_new();
	(*copied) = (*original);
	if( original->u_tag == expr_is_query ){
		struct Query_* copied_query = QUERY_new();
		(*copied_query) = (*(original->u.query));
		copied->u.query = copied_query;
	}
	
	copied->symbol.filename = TEMPVAR_MARK;
	copied->symbol.line = COPIED_EXPR_MARK;
	return copied;
}

static void delete_temp_expression(Expression e){
	assert(is_temp_expr(e));
	sc_free(e->symbol.name);
	e->symbol.filename = NULL;
	EXP_destroy(e);
}

static void delete_copied_expression(Expression e){
	assert(is_copied_expr(e));
	
	if( e->u_tag == expr_is_query ){
		QUERY_destroy(e->u.query);
	}	
	
	e->symbol.filename = NULL;
	EXP_destroy(e);
}


void EXPR_delete_tempvar_definitions( Linked_List tempvar_definitions ){
	
	while( !LISTis_empty(tempvar_definitions) ){
		Link node = LISTLINKlast(tempvar_definitions);
		Expression definition = node->data;
		Expression simplified = node->aux;
		
		if( is_temp_expr(simplified) ){
			delete_temp_expression(simplified);
		}

		if( is_copied_expr(definition) ){
			delete_copied_expression(definition);
		}
		
		LINKremove(node);
	}
	
	LISTfree(tempvar_definitions);
}

static Expression EXPRop_decompose( Expression e, int*/*inout*/ tempvar_id, Linked_List tempvars );
static Expression EXPRop2_decompose( Expression e, int*/*inout*/ tempvar_id, Linked_List tempvars );
static Expression EXPRop1_decompose( Expression e, int*/*inout*/ tempvar_id, Linked_List tempvars );
static Expression EXPRop3_decompose( Expression e, int*/*inout*/ tempvar_id, Linked_List tempvars );


static Expression EXPRexpr_decompose( Expression e, int*/*inout*/ tempvar_id, Linked_List tempvars ) {
		
	switch( TYPEis( e->type ) ) {
			//MARK: indeterminate_
		case indeterminate_:
			return e;
			
			//MARK: integer_
		case integer_:
			return e;
			
			//MARK: real_
		case real_:
			return e;
			
			//MARK: binary_
		case binary_:
			return e;
			
			//MARK: logical_, boolean_
		case logical_:
		case boolean_:
			return e;
			
			//MARK: string_
		case string_:
			return e;
			
			//MARK: attribute_, entity_, identifier_, enumeration_
		case attribute_:
		case identifier_:
			if( e->u_tag == expr_is_variable ) {
				Variable v = e->u.variable;
				if( v->flags.alias ) {
					Expression alias_source = EXPRexpr_decompose(v->initializer, tempvar_id, tempvars);
					Expression simplified = create_temp_expression(tempvar_id, alias_source, tempvars);
					return simplified;
				}
			}
			return e;

		case enumeration_:			
		case entity_:
			return e;

				
			//MARK: query_
		case query_:
		{
			Expression query_expr = copy_expression(e);
			//					reuse:					
			//					e->u.query->local
			//					e->u.query->expression
			
			Expression query_aggregate_source = EXPRexpr_decompose(e->u.query->aggregate, tempvar_id, tempvars);
//			Expression query_aggregate = create_temp_expression(tempvar_id, query_aggregate_source, tempvars);
			query_expr->u.query->aggregate = query_aggregate_source;
			Expression simplified = create_temp_expression(tempvar_id, query_expr, tempvars);
			return simplified;
		}
				
			//MARK: self_
		case self_:
			return e;
			
			//MARK: funcall_
		case funcall_:
		{
			Expression funcall_expr = copy_expression(e);
			//			e->u.funcall.list
			
			Linked_List funcall_list = LISTcreate();
			LISTdo( e->u.funcall.list, arg, Expression ) {
				Expression arg_source = EXPRexpr_decompose(arg, tempvar_id, tempvars);
				LISTadd_last(funcall_list, arg_source);
			}LISTod;
			
			funcall_expr->u.funcall.list = funcall_list;
			Expression simplified = create_temp_expression(tempvar_id, funcall_expr, tempvars);
			return simplified;
		}
			
			//MARK: op_
		case op_:
		{
			return EXPRop_decompose(e, tempvar_id, tempvars);
		}
			
			//MARK: aggregate_
		case aggregate_:
		{
			Expression aggrinit_expr = copy_expression(e);
//			e->u.list
			
			Linked_List aggrinit_list = LISTcreate();
			LISTdo( e->u.list, elem, Expression ){
				Expression elem_source = EXPRexpr_decompose(elem, tempvar_id, tempvars);
				LISTadd_last(aggrinit_list, elem_source);
			}LISTod;
			
			aggrinit_expr->u.list = aggrinit_list;
			Expression simplified = create_temp_expression(tempvar_id, aggrinit_expr, tempvars);
			return simplified;
			
		}
			
//			//MARK: oneof_
//		case oneof_: 

		default:
			fprintf( stderr, "%s:%d: ERROR - unknown expression, type %d", e->symbol.filename, e->symbol.line, TYPEis( e->type ) );
			return e;
	}
}

//MARK: CONTAINS(), TYPEOF()
static Expression EXPRopIn_decompose( Expression e, int*/*inout*/ tempvar_id, Linked_List tempvars ) {
	if( TYPEis(e->e.op1->type)==string_ && TYPEis(e->e.op2->type)==funcall_ && e->e.op2->u.funcall.function==func_typeof ) {
		// TYPEOF( IS:)
		return e;
	}
	else {
		// .CONTAINS()
		return EXPRop2_decompose(e, tempvar_id, tempvars);
	}
}

static Expression EXPRop_decompose( Expression e, int*/*inout*/ tempvar_id, Linked_List tempvars ) {
		switch( e->e.op_code ) {
		//MARK: - Arithmetic Op
			//MARK:OP_NEGATE
			case OP_NEGATE:
				return EXPRop1_decompose(e, tempvar_id, tempvars);
				
			//MARK:OP_PLUS
			case OP_PLUS:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
				//MARK:OP_TIMES
			case OP_TIMES:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
			//MARK:OP_MINUS
			case OP_MINUS:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
			//MARK:OP_REAL_DIV
			case OP_REAL_DIV:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
			//MARK:OP_EXP
			case OP_EXP:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
				//MARK:OP_DIV
			case OP_DIV:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
				//MARK:OP_MOD
			case OP_MOD:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
				
		//MARK: - Relational Op
			//MARK:OP_EQUAL
			case OP_EQUAL:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
			//MARK:OP_NOT_EQUAL
			case OP_NOT_EQUAL:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
			//MARK:OP_GREATER_THAN
			case OP_GREATER_THAN:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
			//MARK:OP_LESS_THAN
			case OP_LESS_THAN:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
			//MARK:OP_GREATER_EQUAL
			case OP_GREATER_EQUAL:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
			//MARK:OP_LESS_EQUAL
			case OP_LESS_EQUAL:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
			//MARK:OP_INST_EQUAL
			case OP_INST_EQUAL:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
			//MARK:OP_INST_NOT_EQUAL
			case OP_INST_NOT_EQUAL:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
			//MARK:OP_IN
			case OP_IN:
				return EXPRopIn_decompose(e, tempvar_id, tempvars);
				
			//MARK:OP_LIKE
			case OP_LIKE:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
				
		//MARK: - Logical Op
			//MARK:OP_NOT
			case OP_NOT:
				return EXPRop1_decompose(e, tempvar_id, tempvars);
				
			//MARK:OP_AND
			case OP_AND:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
			//MARK:OP_OR
			case OP_OR:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
			//MARK:OP_XOR
			case OP_XOR:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
		//MARK: - String Op
			//MARK:OP_SUBCOMPONENT
			case OP_SUBCOMPONENT:
				return EXPRop3_decompose(e, tempvar_id, tempvars);
				
				
		//MARK: - Aggregate Op
			//MARK:OP_ARRAY_ELEMENT
			case OP_ARRAY_ELEMENT:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
		//MARK: - Reference Op
			//MARK:OP_DOT
			case OP_DOT:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
			//MARK:OP_GROUP
			case OP_GROUP:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
				
		//MARK: - Complex Entity Constructor
			//MARK:OP_CONCAT
			case OP_CONCAT:
				return EXPRop2_decompose(e, tempvar_id, tempvars);
				
				
//				//MARK:OP_ANDOR
//        case OP_ANDOR:
				
				default:
				fprintf( stderr, "%s:%d: ERROR - unknown expression opcode, opcode %u", e->symbol.filename, e->symbol.line, e->e.op_code );
				return e;
		}
}

static Expression EXPRop2_decompose( Expression e, int*/*inout*/ tempvar_id, Linked_List tempvars ) {
	Expression op_expr = copy_expression(e);
//	e->e.op1;
//	e->e.op2;
	
	Expression op1_source = EXPRexpr_decompose(e->e.op1, tempvar_id, tempvars);
	Expression op2_source = EXPRexpr_decompose(e->e.op2, tempvar_id, tempvars);
	
	op_expr->e.op1 = op1_source;
	op_expr->e.op2 = op2_source;
	Expression simplified = create_temp_expression(tempvar_id, op_expr, tempvars);
	return simplified;
}

static Expression EXPRop1_decompose( Expression e, int*/*inout*/ tempvar_id, Linked_List tempvars ) {
//	return e;
	
	Expression op_expr = copy_expression(e);
//	e->e.op1;
	
	Expression op1_source = EXPRexpr_decompose(e->e.op1, tempvar_id, tempvars);
	
	op_expr->e.op1 = op1_source;
	Expression simplified = create_temp_expression(tempvar_id, op_expr, tempvars);
	return simplified;
}

static Expression EXPRop3_decompose( Expression e, int*/*inout*/ tempvar_id, Linked_List tempvars ){
	Expression op_expr = copy_expression(e);
//	e->e.op1;
//	e->e.op2;
//	e->e.op3;
	
	Expression op1_source = EXPRexpr_decompose(e->e.op1, tempvar_id, tempvars);
	Expression op2_source = EXPRexpr_decompose(e->e.op2, tempvar_id, tempvars);
	Expression op3_source = EXPRexpr_decompose(e->e.op3, tempvar_id, tempvars);
	
	op_expr->e.op1 = op1_source;
	op_expr->e.op2 = op2_source;
	op_expr->e.op3 = op3_source;
	Expression simplified = create_temp_expression(tempvar_id, op_expr, tempvars);
	return simplified;
}



Expression EXPR_decompose( Expression original, int*/*inout*/ tempvar_id, Linked_List*/*out*/ tempvar_definitions ){
	Linked_List tempvars = (*tempvar_definitions) = LISTcreate();
	
	Expression decomposed = EXPRexpr_decompose(original, tempvar_id, tempvars);
	
	return decomposed;
}

void EXPR_tempvars_swift( Scope s, Linked_List tempvar_definitions, int level ){
	int num_tempvars = LISTget_length(tempvar_definitions);
	if( num_tempvars == 0 )return;
	
	indent_swift(level);
	raw("//DECOMPOSED EXPRESSIONS\n");

	LISTdo(tempvar_definitions, expr, Expression){
		Expression tempvar = LIST_aux;
		
		char buf[BUFSIZ];
		indent_swift(level);
		raw("let %s = ", asVariable_swiftName_n(tempvar->symbol.name, buf, BUFSIZ));
		EXPR_swift(s, expr, expr->return_type, NO_PAREN);
		raw("\n");
	}LISTod;
	
	raw("\n");
}

