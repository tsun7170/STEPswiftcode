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
#include <resolve.h>

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

static Expression create_temp_expression(int*/*inout*/ tempvar_id, Type target_type, Expression definition, Linked_List tempvars){
//	if( TYPEis_partial_entity(target_type) ){
//		target_type = TYPEcopy(target_type);
//		TYPEget_body(target_type)->flags.partial = false;
//	}
	Expression simplified = EXP_new();
	assign_tempvar_symbol(simplified, tempvar_id);
	simplified->type = Type_Identifier;
	simplified->return_type = target_type;
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

static Expression EXPRop_decompose( Expression e, Type target_type, int*/*inout*/ tempvar_id, Linked_List tempvars );
static Expression EXPRop2_decompose( Expression e, Type op1target_type, Type op2target_type, int*/*inout*/ tempvar_id, Linked_List tempvars );
static Expression EXPRop1_decompose( Expression e, Type op1target_type, int*/*inout*/ tempvar_id, Linked_List tempvars );
static Expression EXPRop3_decompose( Expression e, Type op1target_type, Type op2target_type, Type op3target_type, int*/*inout*/ tempvar_id, Linked_List tempvars );
static Expression EXPRop2_decompose_subexpr( Expression e, Type op1target_type, Type op2target_type, int*/*inout*/ tempvar_id, Linked_List tempvars );

typedef enum {
	preserve_op1,
	preserve_op2	
}target_preservation;

static Expression EXPR_lhsop_decompose( Expression e, Type target_type, int*/*inout*/ tempvar_id, Linked_List tempvars );
static Expression EXPR_lhsop2_decompose(target_preservation preserve, Expression e, Type op1target_type, Type op2target_type, int*/*inout*/ tempvar_id, Linked_List tempvars );
static Expression EXPR_lhsop3_decompose( Expression e, Type op1target_type, Type op2target_type, Type op3target_type, int*/*inout*/ tempvar_id, Linked_List tempvars );


static Expression EXPRexpr_decompose( Expression e, Type target_type, int*/*inout*/ tempvar_id, Linked_List tempvars ) {
		
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
					Expression alias_source = EXPRexpr_decompose(v->initializer, target_type, tempvar_id, tempvars);
					Expression simplified = create_temp_expression(tempvar_id, target_type, alias_source, tempvars);
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
			
			Expression query_aggregate_source = EXPRexpr_decompose(e->u.query->aggregate, e->u.query->aggregate->return_type, tempvar_id, tempvars);
			query_expr->u.query->aggregate = query_aggregate_source;
			Expression simplified = create_temp_expression(tempvar_id, query_expr->return_type, query_expr, tempvars);
			return simplified;
		}
				
			//MARK: self_
		case self_:
			return e;
			
			//MARK: funcall_
		case funcall_:
		{
			Expression funcall_expr = copy_expression(e);
			Type target_type = funcall_expr->return_type;
			//			e->u.funcall.list
			
			bool isfunc = (e->u.funcall.function->u_tag == scope_is_func);
			assert(isfunc || (e->u.funcall.function->u_tag == scope_is_entity));
			Linked_List formals;
			if( isfunc ) {
				formals = e->u.funcall.function->u.func->parameters;
			}
			else {	// entity constructor call
				formals = ENTITYget_constructor_params(e->u.funcall.function);
				funcall_expr->return_type = TYPEcopy(target_type);
				TYPEget_body(funcall_expr->return_type)->flags.partial = false;
			}

			assert(LISTget_length(formals) == LISTget_length(e->u.funcall.list));
			
			Linked_List funcall_list = LISTcreate();
			
			Link formals_iter = LISTLINKfirst(formals);
			LISTdo( e->u.funcall.list, arg, Expression ) {
				assert(formals_iter != NULL);
				Variable formal_param = formals_iter->data;

				Expression arg_source = EXPRexpr_decompose(arg, formal_param->type, tempvar_id, tempvars);
				LISTadd_last(funcall_list, arg_source);
				formals_iter = LISTLINKnext(formals, formals_iter);
			}LISTod;
			
			funcall_expr->u.funcall.list = funcall_list;
			Expression simplified = create_temp_expression(tempvar_id, target_type, funcall_expr, tempvars);
			return simplified;
		}
			
			//MARK: op_
		case op_:
		{
			return EXPRop_decompose(e, target_type, tempvar_id, tempvars);
		}
			
			//MARK: aggregate_
		case aggregate_:
		{
			Expression aggrinit_expr = copy_expression(e);
//			e->u.list
			
			Type basetype = TYPE_retrieve_aggregate_base(target_type, NULL);
//			if( TYPEis_runtime(basetype) ){
//				bool debug = true;
//			}//DEBUG

			Linked_List aggrinit_list = LISTcreate();
			LISTdo( e->u.list, elem, Expression ){
				Expression elem_source = EXPRexpr_decompose(elem, basetype, tempvar_id, tempvars);
				LISTadd_last(aggrinit_list, elem_source);
			}LISTod;
			
			aggrinit_expr->u.list = aggrinit_list;
//			Type temptype = TYPEcreate_aggregate(aggregate_, basetype, NULL, NULL, false, false);
			Expression simplified = create_temp_expression(tempvar_id, target_type, aggrinit_expr, tempvars);
			return simplified;
			
		}
			
//			//MARK: oneof_
//		case oneof_: 

		default:
			fprintf( stderr, "%s:%d: ERROR - unknown expression, type %d\n", e->symbol.filename, e->symbol.line, TYPEis( e->type ) );
			return e;
	}
}

Expression EXPR_decompose_lhs( Expression e, Type target_type, int*/*inout*/ tempvar_id, Linked_List tempvars ) {
		
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
					Expression alias_source = EXPR_decompose_lhs(v->initializer, target_type, tempvar_id, tempvars);
					return alias_source;
				}
			}
			return e;

		case enumeration_:			
		case entity_:
			return e;

				
			//MARK: query_
		case query_:
		{
//			Expression query_expr = copy_expression(e);
//			//					reuse:					
//			//					e->u.query->local
//			//					e->u.query->expression
//			
//			Expression query_aggregate_source = EXPRexpr_decompose(e->u.query->aggregate, e->u.query->aggregate->return_type, tempvar_id, tempvars);
//			query_expr->u.query->aggregate = query_aggregate_source;
//			Expression simplified = create_temp_expression(tempvar_id, query_expr->return_type, query_expr, tempvars);
//			return simplified;
			return e;
		}
				
			//MARK: self_
		case self_:
			return e;
			
			//MARK: funcall_
		case funcall_:
		{
//			Expression funcall_expr = copy_expression(e);
//			//			e->u.funcall.list
//			
//			bool isfunc = (e->u.funcall.function->u_tag == scope_is_func);
//			assert(isfunc || (e->u.funcall.function->u_tag == scope_is_entity));
//			Linked_List formals;
//			if( isfunc ) {
//				formals = e->u.funcall.function->u.func->parameters;
//			}
//			else {	// entity constructor call
//				formals = ENTITYget_constructor_params(e->u.funcall.function);
//			}
//
//			Linked_List funcall_list = LISTcreate();
//			
//			Link formals_iter = LISTLINKfirst(formals);
//			LISTdo( e->u.funcall.list, arg, Expression ) {
//				assert(formals_iter != NULL);
//				Variable formal_param = formals_iter->data;
//
//				Expression arg_source = EXPRexpr_decompose(arg, formal_param->type, tempvar_id, tempvars);
//				LISTadd_last(funcall_list, arg_source);
//				formals_iter = LISTLINKnext(formals, formals_iter);
//			}LISTod;
//			
//			funcall_expr->u.funcall.list = funcall_list;
//			Expression simplified = create_temp_expression(tempvar_id, funcall_expr->return_type, funcall_expr, tempvars);
//			return simplified;
			return e;
		}
			
			//MARK: op_
		case op_:
		{
			return EXPR_lhsop_decompose(e, target_type, tempvar_id, tempvars);
		}
			
			//MARK: aggregate_
		case aggregate_:
		{
//			Expression aggrinit_expr = copy_expression(e);
////			e->u.list
//			
//			Type basetype = TYPE_retrieve_aggregate_base(target_type, NULL);
//
//			Linked_List aggrinit_list = LISTcreate();
//			LISTdo( e->u.list, elem, Expression ){
//				Expression elem_source = EXPRexpr_decompose(elem, basetype, tempvar_id, tempvars);
//				LISTadd_last(aggrinit_list, elem_source);
//			}LISTod;
//			
//			aggrinit_expr->u.list = aggrinit_list;
//			Type temptype = TYPEcreate_aggregate(aggregate_, basetype, NULL, NULL, false, false);
//			Expression simplified = create_temp_expression(tempvar_id, temptype, aggrinit_expr, tempvars);
//			return simplified;
			return e;
		}
			
//			//MARK: oneof_
//		case oneof_: 

		default:
			fprintf( stderr, "%s:%d: ERROR - unknown expression, type %d\n", e->symbol.filename, e->symbol.line, TYPEis( e->type ) );
			return e;
	}
}

//MARK: CONTAINS(), TYPEOF()
static Expression EXPRopIn_decompose( Expression e, Type target_type, int*/*inout*/ tempvar_id, Linked_List tempvars ) {
	if( TYPEis(e->e.op1->type)==string_ && TYPEis(e->e.op2->type)==funcall_ && e->e.op2->u.funcall.function==func_typeof ) {
		// TYPEOF( IS:)
		Expression simplified = create_temp_expression(tempvar_id, e->return_type, e, tempvars);
		return simplified;
	}
	else {
		// .CONTAINS()
		Type op1type = e->e.op1->return_type;	// lhs: basetype of rhs
		Type op2type = e->e.op2->return_type;	// rhs: aggregation type
		Type op2basetype = TYPE_retrieve_aggregate_base(op2type, NULL);
		if( op2basetype == NULL || TYPEis_runtime(op2basetype) ){
			op2type = TYPEcreate_aggregate(TYPEis(op2type), op1type, TYPEget_body(op2type)->lower, TYPEget_body(op2type)->upper, TYPEget_body(op2type)->flags.unique, TYPEget_body(op2type)->flags.optional);
		}
		else if( op1type == NULL || TYPEis_runtime(op1type) ){
			op1type = op2basetype;
		}
		
		return EXPRop2_decompose(e, op1type, op2type, tempvar_id, tempvars);
	}
}

static Expression EXPRop_decompose( Expression e, Type target_type, int*/*inout*/ tempvar_id, Linked_List tempvars ) {
		switch( e->e.op_code ) {
		//MARK: - Arithmetic Op
			//MARK:OP_NEGATE
			case OP_NEGATE:
				return EXPRop1_decompose(e, e->e.op1->return_type, tempvar_id, tempvars);
				
			//MARK:OP_PLUS
			case OP_PLUS:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
				//MARK:OP_TIMES
			case OP_TIMES:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
			//MARK:OP_MINUS
			case OP_MINUS:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
			//MARK:OP_REAL_DIV
			case OP_REAL_DIV:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
			//MARK:OP_EXP
			case OP_EXP:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
				//MARK:OP_DIV
			case OP_DIV:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
				//MARK:OP_MOD
			case OP_MOD:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
				
		//MARK: - Relational Op
			//MARK:OP_EQUAL
			case OP_EQUAL:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
			//MARK:OP_NOT_EQUAL
			case OP_NOT_EQUAL:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
			//MARK:OP_GREATER_THAN
			case OP_GREATER_THAN:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
			//MARK:OP_LESS_THAN
			case OP_LESS_THAN:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
			//MARK:OP_GREATER_EQUAL
			case OP_GREATER_EQUAL:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
			//MARK:OP_LESS_EQUAL
			case OP_LESS_EQUAL:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
			//MARK:OP_INST_EQUAL
			case OP_INST_EQUAL:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
			//MARK:OP_INST_NOT_EQUAL
			case OP_INST_NOT_EQUAL:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
			//MARK:OP_IN
			case OP_IN:
				return EXPRopIn_decompose(e, target_type, tempvar_id, tempvars);
				
			//MARK:OP_LIKE
			case OP_LIKE:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
				
		//MARK: - Logical Op
			//MARK:OP_NOT
			case OP_NOT:
				return EXPRop1_decompose(e, e->e.op1->return_type, tempvar_id, tempvars);
				
			//MARK:OP_AND
			case OP_AND:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
			//MARK:OP_OR
			case OP_OR:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
			//MARK:OP_XOR
			case OP_XOR:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
		//MARK: - String Op
			//MARK:OP_SUBCOMPONENT
			case OP_SUBCOMPONENT:
				return EXPRop3_decompose(e, e->e.op1->return_type, e->e.op2->return_type, e->e.op3->return_type, tempvar_id, tempvars);
				
				
		//MARK: - Aggregate Op
			//MARK:OP_ARRAY_ELEMENT
			case OP_ARRAY_ELEMENT:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
		//MARK: - Reference Op
			//MARK:OP_DOT
			case OP_DOT:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
			//MARK:OP_GROUP
			case OP_GROUP:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
				
		//MARK: - Complex Entity Constructor
			//MARK:OP_CONCAT
			case OP_CONCAT:
				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
				
//				//MARK:OP_ANDOR
//        case OP_ANDOR:
				
				//MARK: - REPEAT
			case OP_REPEAT:
//				return EXPRop2_decompose(e, tempvar_id, tempvars);
//				return e;
				return EXPRop2_decompose_subexpr(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);

				
				default:
				fprintf( stderr, "%s:%d: ERROR - unknown expression opcode, opcode %u\n", e->symbol.filename, e->symbol.line, e->e.op_code );
				return e;
		}
}

static Expression EXPR_lhsop_decompose( Expression e, Type target_type, int*/*inout*/ tempvar_id, Linked_List tempvars ) {
		switch( e->e.op_code ) {
		//MARK: - Arithmetic Op
			//MARK:OP_NEGATE
			case OP_NEGATE:
//				return EXPRop1_decompose(e, e->e.op1->return_type, tempvar_id, tempvars);
				return e;
				
			//MARK:OP_PLUS
			case OP_PLUS:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
				//MARK:OP_TIMES
			case OP_TIMES:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
			//MARK:OP_MINUS
			case OP_MINUS:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
			//MARK:OP_REAL_DIV
			case OP_REAL_DIV:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
			//MARK:OP_EXP
			case OP_EXP:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
				//MARK:OP_DIV
			case OP_DIV:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
				//MARK:OP_MOD
			case OP_MOD:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
				
		//MARK: - Relational Op
			//MARK:OP_EQUAL
			case OP_EQUAL:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
			//MARK:OP_NOT_EQUAL
			case OP_NOT_EQUAL:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
			//MARK:OP_GREATER_THAN
			case OP_GREATER_THAN:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
			//MARK:OP_LESS_THAN
			case OP_LESS_THAN:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
			//MARK:OP_GREATER_EQUAL
			case OP_GREATER_EQUAL:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
			//MARK:OP_LESS_EQUAL
			case OP_LESS_EQUAL:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
			//MARK:OP_INST_EQUAL
			case OP_INST_EQUAL:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
			//MARK:OP_INST_NOT_EQUAL
			case OP_INST_NOT_EQUAL:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
			//MARK:OP_IN
			case OP_IN:
//				return EXPRopIn_decompose(e, target_type, tempvar_id, tempvars);
				return e;
				
			//MARK:OP_LIKE
			case OP_LIKE:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
				
		//MARK: - Logical Op
			//MARK:OP_NOT
			case OP_NOT:
//				return EXPRop1_decompose(e, e->e.op1->return_type, tempvar_id, tempvars);
				return e;
				
			//MARK:OP_AND
			case OP_AND:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
			//MARK:OP_OR
			case OP_OR:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
			//MARK:OP_XOR
			case OP_XOR:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
		//MARK: - String Op
			//MARK:OP_SUBCOMPONENT
			case OP_SUBCOMPONENT:
				return EXPR_lhsop3_decompose(e, e->e.op1->return_type, e->e.op2->return_type, e->e.op3->return_type, tempvar_id, tempvars);
				
				
		//MARK: - Aggregate Op
			//MARK:OP_ARRAY_ELEMENT
			case OP_ARRAY_ELEMENT:
				return EXPR_lhsop2_decompose(preserve_op1, e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
		//MARK: - Reference Op
			//MARK:OP_DOT
			case OP_DOT:
				return EXPR_lhsop2_decompose(preserve_op2, e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
			//MARK:OP_GROUP
			case OP_GROUP:
				return EXPR_lhsop2_decompose(preserve_op2, e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				
				
		//MARK: - Complex Entity Constructor
			//MARK:OP_CONCAT
			case OP_CONCAT:
//				return EXPRop2_decompose(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;
				
				
//				//MARK:OP_ANDOR
//        case OP_ANDOR:
				
				//MARK: - REPEAT
			case OP_REPEAT:
//				return EXPRop2_decompose_subexpr(e, e->e.op1->return_type, e->e.op2->return_type, tempvar_id, tempvars);
				return e;

				
				default:
				fprintf( stderr, "%s:%d: ERROR - unknown expression opcode, opcode %u\n", e->symbol.filename, e->symbol.line, e->e.op_code );
				return e;
		}
}

static Expression EXPRop2_decompose_subexpr( Expression e, Type op1target_type, Type op2target_type, int*/*inout*/ tempvar_id, Linked_List tempvars ) {
	Expression op_expr = copy_expression(e);
//	e->e.op1;
//	e->e.op2;
	
	Expression op1_source = EXPRexpr_decompose(e->e.op1, op1target_type, tempvar_id, tempvars);
	Expression op2_source = EXPRexpr_decompose(e->e.op2, op2target_type, tempvar_id, tempvars);
	
	op_expr->e.op1 = op1_source;
	op_expr->e.op2 = op2_source;
	return op_expr;
}

static Expression EXPRop2_decompose( Expression e, Type op1target_type, Type op2target_type, int*/*inout*/ tempvar_id, Linked_List tempvars ) {
//	Expression op_expr = copy_expression(e);
////	e->e.op1;
////	e->e.op2;

	Expression op_expr = EXPRop2_decompose_subexpr(e, op1target_type, op2target_type, tempvar_id, tempvars);
	Expression simplified = create_temp_expression(tempvar_id, op_expr->return_type, op_expr, tempvars);
	return simplified;
}

static Expression EXPRop1_decompose( Expression e, Type op1target_type, int*/*inout*/ tempvar_id, Linked_List tempvars ) {
	if( EXP_is_literal(e->e.op1) ) return e;
	if( TYPEis(e->e.op1->type) == identifier_ ) return e;
	if( TYPEis(e->e.op1->type) == attribute_ ) return e;
	
	Expression op_expr = copy_expression(e);
//	e->e.op1;
	
	Expression op1_source = EXPRexpr_decompose(e->e.op1, op1target_type, tempvar_id, tempvars);
	
	op_expr->e.op1 = op1_source;
	Expression simplified = create_temp_expression(tempvar_id, op_expr->return_type, op_expr, tempvars);
	return simplified;
}

static Expression EXPRop3_decompose( Expression e, Type op1target_type, Type op2target_type, Type op3target_type, int*/*inout*/ tempvar_id, Linked_List tempvars ){
	Expression op_expr = copy_expression(e);
//	e->e.op1;
//	e->e.op2;
//	e->e.op3;
	
	Expression op1_source = EXPRexpr_decompose(e->e.op1, op1target_type, tempvar_id, tempvars);
	Expression op2_source = EXPRexpr_decompose(e->e.op2, op2target_type, tempvar_id, tempvars);
	Expression op3_source = EXPRexpr_decompose(e->e.op3, op3target_type, tempvar_id, tempvars);
	
	op_expr->e.op1 = op1_source;
	op_expr->e.op2 = op2_source;
	op_expr->e.op3 = op3_source;
	Expression simplified = create_temp_expression(tempvar_id, op_expr->return_type, op_expr, tempvars);
	return simplified;
}




static Expression EXPR_lhsop2_decompose(target_preservation preserve, Expression e, Type op1target_type, Type op2target_type, int*/*inout*/ tempvar_id, Linked_List tempvars ) {
	Expression op_expr = copy_expression(e);
//	e->e.op1;
//	e->e.op2;
	
	switch (preserve) {
		case preserve_op1:
		{
			Expression op1_source = EXPR_decompose_lhs(e->e.op1, op1target_type, tempvar_id, tempvars);
			Expression op2_source = EXPRexpr_decompose(e->e.op2, op2target_type, tempvar_id, tempvars);
			op_expr->e.op1 = op1_source;
			op_expr->e.op2 = op2_source;
		}
			break;
			
		case preserve_op2:
		{
			Expression op1_source = EXPRexpr_decompose(e->e.op1, op1target_type, tempvar_id, tempvars);
			Expression op2_source = EXPR_decompose_lhs(e->e.op2, op2target_type, tempvar_id, tempvars);
			op_expr->e.op1 = op1_source;
			op_expr->e.op2 = op2_source;
		}
			break;
	}
	return op_expr;
}

static Expression EXPR_lhsop3_decompose( Expression e, Type op1target_type, Type op2target_type, Type op3target_type, int*/*inout*/ tempvar_id, Linked_List tempvars ){
	Expression op_expr = copy_expression(e);
//	e->e.op1;
//	e->e.op2;
//	e->e.op3;
	
	Expression op1_source = EXPR_decompose_lhs(e->e.op1, op1target_type, tempvar_id, tempvars);
	Expression op2_source = EXPRexpr_decompose(e->e.op2, op2target_type, tempvar_id, tempvars);
	Expression op3_source = EXPRexpr_decompose(e->e.op3, op3target_type, tempvar_id, tempvars);
	
	op_expr->e.op1 = op1_source;
	op_expr->e.op2 = op2_source;
	op_expr->e.op3 = op3_source;
//	Expression simplified = create_temp_expression(tempvar_id, op_expr->return_type, op_expr, tempvars);
	return op_expr;
}

Expression EXPR_decompose( Expression original, Type target_type, int*/*inout*/ tempvar_id, Linked_List*/*out*/ tempvar_definitions ){
	Linked_List tempvars = (*tempvar_definitions) = LISTcreate();
	
	Expression decomposed = EXPRexpr_decompose(original, target_type, tempvar_id, tempvars);
	
	return decomposed;
}

int EXPR_tempvars_swift( Scope s, Linked_List tempvar_definitions, int level ){
	int num_tempvars = LISTget_length(tempvar_definitions);
	if( num_tempvars == 0 )return num_tempvars;
	
	raw("\n");
//	indent_swift(level);
//	raw("//DECOMPOSED EXPRESSIONS\n");

	LISTdo(tempvar_definitions, expr, Expression){
		Expression tempvar = LIST_aux;
		
		char buf[BUFSIZ];
		indent_swift(level);
		raw("let %s ", asVariable_swiftName_n(tempvar->symbol.name, buf, BUFSIZ));
//		raw("/*");TYPE_head_swift(s, tempvar->return_type, YES_IN_COMMENT);raw("*/");	// DEBUG
		raw("= ");
		EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, s, expr, tempvar->return_type, NO_PAREN, OP_UNKNOWN, YES_WRAP);
//		EXPR_swift(s, expr, expr->return_type, NO_PAREN);
		raw("\n");
	}LISTod;
	
//	raw("\n");
	return num_tempvars;
}

