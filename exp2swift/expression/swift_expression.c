//
//  swift_expression.c
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#include <stdio.h>
#include <sc_stdbool.h>
#include <stdlib.h>
#include <resolve.h>

#include "exppp.h"
#include "pp.h"
#include "builtin.h"

#include "pretty_expr.h"

#include "swift_expression.h"
#include "swift_symbol.h"
#include "swift_func.h"
#include "swift_entity.h"
#include "swift_type.h"

//#define YES_PAD	true
//#define NO_PAD	false
#define OPCODE_FROM_EXPRESS	( char * )0


//#define EXPRop2_swift(	SELF1,SELF2,oe,string,paren,pad) \
//				EXPRop2__swift(	SELF1,SELF2,oe,string,paren,pad,		OP_UNKNOWN, YES_WRAP)
//
//#define EXPRop_swift(		SELF,oe,paren) \
//				EXPRop__swift(	SELF,oe,paren,		OP_UNKNOWN, YES_WRAP)

static void EXPR__swift( Scope SELF, Expression e, Type target_type, 
												bool paren, unsigned int previous_op, bool can_wrap );


static void EXPRop__swift( Scope SELF, struct Op_Subexpression * oe, 
													bool paren, unsigned int previous_op, bool can_wrap );

static void EXPRop2__swift( Scope SELF1, Scope SELF2, struct Op_Subexpression * eo, char * opcode, 
													 bool paren, unsigned int previous_op, bool can_wrap );

static void EXPRop1_swift( Scope SELF, struct Op_Subexpression * eo, char * opcode, 
													bool paren, bool can_wrap );


void EXPR_swift( Scope SELF, Expression e, Type target_type, bool paren) {
	EXPR__swift( SELF,e,target_type, paren, YES_WRAP, OP_UNKNOWN );
}


//MARK: - string literal
static void simpleStringLiteral_swift(const char* instring ) {
	int inlen = (int)strlen( instring );
	wrap_provisionally(inlen);
	
	const char* chptr = instring;
	wrap("\"");
	while( *chptr ) {
		if( *chptr == '\\' || *chptr == '\"' ) {
			raw("\\"); // escape backslash
		}
		raw("%c",*chptr);
		++chptr;
	}
	raw("\"");
}

static void encodedStringLiteral_swift(const char* instring ) {
	int inlen = (int)strlen( instring );
	wrap_provisionally((inlen*3)/2);
	
	const char* chptr = instring;
	wrap("\"");
	while( *chptr ) {
		assert(chptr[1]);
		assert(chptr[2]);
		assert(chptr[3]);
		assert(chptr[4]);
		assert(chptr[5]);
		assert(chptr[6]);
		assert(chptr[7]);
		raw("\\u{%8.8s}", chptr);
		chptr += 8;
	}
	raw("\"");
}

//MARK: - aggregation bounds
void EXPRbounds_swift(Scope SELF, TypeBody tb, SwiftOutCommentOption in_comment ) {
    if( !tb->upper ) {
        return;
    }

	if( in_comment != WO_COMMENT ){
		wrap( "%s[", in_comment==YES_IN_COMMENT ? "" : "/*" );
		EXPR_swift(SELF, tb->lower, Type_Integer, YES_PAREN );
		raw( ":" );
		EXPR_swift(SELF, tb->upper, Type_Integer, YES_PAREN );
		raw( "]%s ", in_comment==YES_IN_COMMENT ? "" : "*/" );
	}
}

//MARK: - special operators
static void EXPRopGroup_swift( Scope SELF, struct Op_Subexpression * oe, bool can_wrap ) {
	EXPR__swift( SELF, oe->op2, oe->op2->return_type, NO_PAREN, oe->op_code, can_wrap );	// should be an entity reference
	raw("(");
	EXPR__swift( SELF, oe->op1, oe->op1->return_type, NO_PAREN, oe->op_code, YES_WRAP );
	raw(")");
}

static void EXPRopDot_swift( Scope SELF, struct Op_Subexpression * oe, bool can_wrap ) {
	if( !EXPRresult_is_optional(oe->op1) ) {
		EXPR__swift( SELF, oe->op1, oe->op1->return_type, YES_PAREN, oe->op_code, can_wrap );
	}
	else {
		EXPR__swift( SELF, oe->op1, oe->op1->return_type, YES_PAREN, oe->op_code, can_wrap );
		raw("?");
	}
	wrap_if(YES_WRAP, ".");
	EXPR__swift( NULL, oe->op2, oe->op2->return_type, YES_PAREN, oe->op_code, NO_WRAP );
}

//MARK: TYPEOF()
static void EXPRopIn_swift( Scope SELF, struct Op_Subexpression * oe, bool can_wrap ) {
	if( TYPEis(oe->op1->type)==string_ && TYPEis(oe->op2->type)==funcall_ && oe->op2->u.funcall.function==func_typeof ) {
		Expression arg = LISTget_first(oe->op2->u.funcall.list);
		wrap_if(can_wrap, "SDAI.TYPEOF(");
		EXPR_swift( SELF, arg, arg->return_type, NO_PAREN );
		raw(", ");
		char buf[BUFSIZ];
		assert(oe->op1->symbol.name != NULL);
		wrap_if(YES_WRAP, "IS: %s.self", canonical_swiftName(oe->op1->symbol.name, buf));
//		EXPR__swift( SELF, oe->op1, Type_String, NO_PAREN, oe->op_code, YES_WRAP );
		raw(")");
	}
	else {
		EXPR__swift( SELF, oe->op2, oe->op2->return_type, YES_PAREN, oe->op_code, YES_WRAP );
		if( EXPRresult_is_optional(oe->op2) ) raw("?");
		positively_wrap();
		wrap_if(YES_WRAP, ".CONTAINS(");
		EXPR__swift( SELF, oe->op1, TYPEget_base_type(oe->op2->return_type), NO_PAREN, oe->op_code, YES_WRAP );
		raw(")");		
	}
}

static void EXPRopLike_swift( Scope SELF, struct Op_Subexpression * oe, bool can_wrap ) {
	EXPR__swift( SELF, oe->op1, oe->op1->return_type, YES_PAREN, oe->op_code, YES_WRAP );
	if( EXPRresult_is_optional(oe->op1) ) raw("?");
	wrap_if(YES_WRAP, ".ISLIKE( PATTERN: ");
	EXPR__swift( SELF, oe->op2, Type_String, NO_PAREN, oe->op_code, YES_WRAP );
	raw(" )");
}


//MARK: - aggregate initializer
static void EXPRrepeat_swift( Scope SELF, Expression val, Expression count, Type basetype) {
//	assert(val != NULL);
	assert(count != NULL);
	
	wrap_if(YES_WRAP,"SDAI.AIE(");
	if(val) {
//		EXPR__swift( SELF, val, NO_PAREN, OP_UNKNOWN, YES_WRAP );
		EXPRassignment_rhs_swift(SELF, val, basetype, NO_PAREN, OP_UNKNOWN, YES_WRAP);
	}
	else {
		raw("###NULL###");
	}
	raw(", ");
	wrap_if(YES_WRAP,"repeat:Int(");
	EXPR__swift( SELF, count, Type_Integer, NO_PAREN, OP_UNKNOWN, YES_WRAP );
	raw("))");
}

//MARK: - expression return type check
static bool operator_returns_optional(struct Op_Subexpression * oe) {
	switch( oe->op_code ) {
			//MARK: - Arithmetic Op
		case OP_NEGATE:
		case OP_PLUS:
		case OP_MINUS:
		case OP_TIMES:
		case OP_REAL_DIV:
		case OP_EXP:
		case OP_DIV:
		case OP_MOD:
			return true;
			
			//MARK: - Relational Op
		case OP_EQUAL:
		case OP_NOT_EQUAL:
		case OP_GREATER_THAN:
		case OP_LESS_THAN:
		case OP_GREATER_EQUAL:
		case OP_LESS_EQUAL:
		case OP_INST_EQUAL:
		case OP_INST_NOT_EQUAL:
		case OP_IN:
		case OP_LIKE:
			return false;
			
			
			//MARK: - Logical Op
		case OP_NOT:
		case OP_AND:
		case OP_OR:
		case OP_XOR:
			return false;
			
			//MARK: - String Op
		case OP_SUBCOMPONENT:
			return true;
			
			//MARK: - Aggregate Op
		case OP_ARRAY_ELEMENT:
			return true;
			
			//MARK: - Reference Op
		case OP_DOT:
			return EXPRresult_is_optional(oe->op2);
			
		case OP_GROUP:
			return true;
			
			//MARK: - Complex Entity Constructor
		case OP_CONCAT:
			return true;
			
			//case OP_ANDOR:
			
		default:
			fprintf( stderr, "ERROR - unknown op-expression\n" );
			abort();
	}
}

bool EXPRresult_is_optional(Expression e) {
	switch( TYPEis( e->type ) ) {
		case integer_:
		case real_:
		case binary_:
		case logical_:
		case boolean_:
		case string_:
			return false;
			
		case attribute_:
			return VARis_optional(e->u.variable);
			
		case entity_:
			return false;
			
		case identifier_:
			if( e->u_tag == expr_is_variable ){
				return VARis_optional(e->u.variable);
			}
			return true;	// attribute of generic entity
			
		case enumeration_:
			return false;
			
		case query_:
			return false;
			
		case self_:
			return false;	
			
		case funcall_:
			if( e->u.funcall.function->u_tag != scope_is_func ) return false;
			if( TYPEis_boolean(e->u.funcall.function->u.func->return_type) ) return false;
			if( TYPEis_logical(e->u.funcall.function->u.func->return_type) ) return false;
			return true;
			
		case op_:
			return operator_returns_optional(&e->e);
			
		case aggregate_:
			return false;

		default:
			fprintf( stderr, "%s:%d: ERROR - unknown expression, type %d", e->symbol.filename, e->symbol.line, TYPEis( e->type ) );
			abort();
	}
}

// special func calls
static void emit_usedin_call( Scope SELF, Expression e, bool can_wrap ){
	wrap_if(can_wrap, "SDAI.USEDIN(" );
	Expression arg1 = LISTget_first(e->u.funcall.list);
	Expression arg2 = LISTget_second(e->u.funcall.list);
	
	wrap("T: ");
	EXPRassignment_rhs_swift(SELF, arg1, Type_Generic, NO_PAREN, OP_UNKNOWN, YES_WRAP);

	if( (arg2 != NULL) && (strlen(arg2->symbol.name) > 0) ){
		raw(", ");
		assert( TYPEis_string(arg2->type) );
		char* sep = strrchr(arg2->symbol.name, '.');
		assert( sep != NULL );
		char buf[BUFSIZ];
		int symlen = (int)(sep - arg2->symbol.name);
		assert( symlen < BUFSIZ );
		wrap("ENTITY: %s.self, ", canonical_swiftName_n(arg2->symbol.name, buf, symlen));
		wrap("ATTR: ");
		simpleStringLiteral_swift(sep+1);
	}
	
	raw(")");
}

//MARK: - main entry point
/**
 * if paren == 1, parens are usually added to prevent possible rebind by
 *    higher-level context.  If op is similar to previous op (and
 *    precedence/associativity is not a problem) parens may be omitted.
 * if paren == 0, then parens may be omitted without consequence
 */
void EXPR__swift( Scope SELF, Expression e, Type target_type, bool paren, unsigned int previous_op, bool can_wrap  ) {
//	int i;  /* trusty temporary */
		
	switch( TYPEis( e->type ) ) {
			//MARK: integer_
		case integer_:
			if( isLITERAL_INFINITY(e) ) {	//*TY2020/08/26
				wrap_if(can_wrap, "nil" );
			} else {
				wrap_if(can_wrap, "%d", e->u.integer );
			}
			break;
			
			//MARK: real_
		case real_:
			if( isLITERAL_PI(e) ) {	//*TY2020/08/26
				wrap_if(can_wrap, "SDAI.PI" );
			} else if( isLITERAL_E(e) ) {
				wrap_if(can_wrap, "SDAI.CONST_E" );
			} else {
				wrap_if(can_wrap, real2exp( e->u.real ) );
			}
			break;
			
			//MARK: binary_
		case binary_:
			wrap_if(can_wrap, "\"%%%s\"", e->u.binary ); /* put "%" back */
			break;
			
			//MARK: logical_, boolean_
		case logical_:
		case boolean_:
			switch( e->u.logical ) {
				case Ltrue:
					wrap_if(can_wrap, "SDAI.TRUE" );
					break;
				case Lfalse:
					wrap_if(can_wrap, "SDAI.FALSE" );
					break;
				default:
					wrap_if(can_wrap, "SDAI.UNKNOWN" );
					break;
			}
			break;
			
			//MARK: string_
		case string_:
			if( TYPEis_encoded( e->type ) ) {
				encodedStringLiteral_swift(e->symbol.name);
			} 
			else {
				simpleStringLiteral_swift(e->symbol.name);
			}
			break;
			
			//MARK: attribute_, entity_, identifier_, enumeration_
		case attribute_:
		case entity_:
		case identifier_:
		case enumeration_:			
			if( e->u_tag == expr_is_variable ) {
				Variable v = e->u.variable;
				if( v->flags.alias ) {
					{
						char buf[BUFSIZ];
						raw("(/*%s*/",canonical_swiftName(e->symbol.name,buf));
					}
					EXPR__swift(SELF, v->initializer, target_type, NO_PAREN, OP_UNKNOWN, can_wrap);
					raw(")");
					break;
				}
				Entity ent = v->defined_in;
				if( ENTITYis_a(SELF, ent) ) {
					wrap_if(can_wrap, "SELF.");
				}
			}
		{
			char buf[BUFSIZ];
			raw( "%s", canonical_swiftName(e->symbol.name,buf) );
		}
			break;
			
			//MARK: query_
		case query_:
			EXPR__swift( SELF, e->u.query->aggregate, e->u.query->aggregate->return_type, YES_PAREN, OP_UNKNOWN, can_wrap );
			if( EXPRresult_is_optional(e->u.query->aggregate) ) raw("?");
		{
			char buf[BUFSIZ];
			aggressively_wrap();
			raw( ".QUERY{ %s in ", variable_swiftName(e->u.query->local,buf));
		}
			
//			EXPR__swift( SELF, e->u.query->expression, NO_PAREN, OP_UNKNOWN, can_wrap );
			EXPRassignment_rhs_swift(SELF, e->u.query->expression, Type_Logical, NO_PAREN, OP_UNKNOWN, YES_WRAP);
			raw( " }" );
			break;
			
			//MARK: self_
		case self_:
			wrap_if(can_wrap, "SELF" );
			break;
			
			//MARK: funcall_
		case funcall_:
		{
			// special treatment for usedin() func.
			if( e->u.funcall.function == FUNC_USEDIN ){
				emit_usedin_call(SELF, e, can_wrap);
				break;
			}
			
			bool isfunc = (e->u.funcall.function->u_tag == scope_is_func);
			assert(isfunc || (e->u.funcall.function->u_tag == scope_is_entity));
			
			
			Linked_List formals;
			{
				char buf[BUFSIZ];
				char* qual = "";
				const char* func;
				
				if( isfunc ) {
					if( e->u.funcall.function->u.func->builtin ) qual = "SDAI.";
					func = FUNCcall_swiftName(e,buf);
					formals = e->u.funcall.function->u.func->parameters;
				}
				else {	// entity constructor call
					func = partialEntity_swiftName(e->u.funcall.function, buf);
					formals = ENTITYget_constructor_params(e->u.funcall.function);
				}
				
				if(can_wrap) aggressively_wrap();
				wrap_if(can_wrap, "%s%s(", qual, func );
			}
			
			if( !LISTis_empty(e->u.funcall.list) ) {
				positively_wrap();
				int oldwrap = captureWrapIndent();
				if( LISTget_length(e->u.funcall.list)<=1 )restoreWrapIndent(oldwrap);
				
				char* sep = "";
				bool noLabel = ( isfunc && (LISTget_second(formals) == NULL) );
				
				Link formals_iter = LISTLINKfirst(formals);
				LISTdo( e->u.funcall.list, arg, Expression ) {
					raw("%s",sep);
					positively_wrap();
					
					assert(formals_iter != NULL);
					Variable formal_param = formals_iter->data;
					if( !noLabel ) {
						char buf[BUFSIZ];
						wrap("%s: ", variable_swiftName(formal_param,buf));
					}
					if(VARis_inout(formal_param)) {
						raw("&");
						EXPR_swift( SELF, arg, arg->return_type, NO_PAREN );
					}
					else {
						EXPRassignment_rhs_swift(SELF, arg, formal_param->type, NO_PAREN, OP_UNKNOWN, YES_WRAP);
					}
					
					sep = ", ";
					formals_iter = LISTLINKnext(formals, formals_iter);
				}LISTod
				restoreWrapIndent(oldwrap);
			}
			raw( ")" );
		}
			break;
			
			//MARK: op_
		case op_:
			EXPRop__swift( SELF, &e->e, paren, previous_op, can_wrap );
			break;
			
			//MARK: aggregate_
		case aggregate_:
		{
			if(LISTis_empty(e->u.list)){
				wrap_if(can_wrap, "SDAI.EMPLY_AGGREGATE" );
				break;
			}
			
			Type basetype = TYPE_retrieve_aggregate_base(e->return_type, NULL);
			if( basetype == NULL || TYPEis_runtime(basetype) ){
				basetype = TYPE_retrieve_aggregate_base(target_type, NULL);
			}
			
			wrap_if(can_wrap, "[" );
			char* sep = "";
			Expression prev = NULL;
			
			LISTdo( e->u.list, arg, Expression ) {
				raw(sep);
				positively_wrap();
				
				bool repeat = arg->type->u.type->body->flags.repeat;
				/* if repeat is true, the previous Expression repeats and this one is the count */
				
				if( repeat ) {
					EXPRrepeat_swift(SELF, prev, arg, basetype);
					sep = ", ";
					prev = NULL;
				}
				else {
					if(prev != NULL) {
						wrap_if(can_wrap, "SDAI.AIE(");
						if( prev->return_type == basetype ){
							EXPR__swift( SELF, prev, prev->return_type, NO_PAREN, OP_UNKNOWN, YES_WRAP );
						}
						else if( TYPEis_runtime(basetype) ){
							raw("/*runtime*/");
							EXPR__swift( SELF, prev, prev->return_type, NO_PAREN, OP_UNKNOWN, YES_WRAP );
						}
						else {
							TYPE_head_swift(SELF, basetype, WO_COMMENT);
							raw("(");
							EXPR__swift( SELF, prev, prev->return_type, NO_PAREN, OP_UNKNOWN, YES_WRAP );
							raw(")");
						}
						raw(")");
						sep = ", ";
					}
					prev = arg;
				}
			} LISTod
			
			if(prev != NULL) {
				raw(sep);
				positively_wrap();
				wrap_if(can_wrap, "SDAI.AIE(");
				if( prev->return_type == basetype ){
					EXPR__swift( SELF, prev, prev->return_type, NO_PAREN, OP_UNKNOWN, YES_WRAP );
				}
				else if( TYPEis_runtime(basetype) ){
					raw("/*runtime*/");
					EXPR__swift( SELF, prev, prev->return_type, NO_PAREN, OP_UNKNOWN, YES_WRAP );
				}
				else {
					TYPE_head_swift(SELF, basetype, WO_COMMENT);
					raw("(");
					EXPR__swift( SELF, prev, prev->return_type, NO_PAREN, OP_UNKNOWN, YES_WRAP );
					raw(")");
				}
				raw(")");
			}
			raw( "]" );
		}
			break;
			
//			//MARK: oneof_
//		case oneof_: {
//			int old_indent = indent2;
//			wrap_if(can_wrap, "ONEOF ( " );
//			
//			if( exppp_linelength == indent2 ) {
//				exppp_linelength += exppp_continuation_indent;
//			}
//			indent2 += exppp_continuation_indent;
//			
//			i = 0;
//			LISTdo( e->u.list, arg, Expression )
//			i++;
//			if( i != 1 ) {
//				raw( ", " );
//			}
//			EXPR__swift( SELF, arg, YES_PAREN, OP_UNKNOWN, can_wrap );
//			LISTod
//			
//			if( exppp_linelength == indent2 ) {
//				exppp_linelength = old_indent;
//			}
//			indent2 = old_indent;
//			
//			raw( " )" );
//			break;
//		}
		default:
			fprintf( stderr, "%s:%d: ERROR - unknown expression, type %d", e->symbol.filename, e->symbol.line, TYPEis( e->type ) );
			abort();
	}
}

/** print expression that has op and operands */
void EXPRop__swift( Scope SELF, struct Op_Subexpression * oe, 
									 bool paren, unsigned int previous_op, bool can_wrap) {
    switch( oe->op_code ) {
		//MARK: - Arithmetic Op
			//MARK:OP_NEGATE
			case OP_NEGATE:
				EXPRop1_swift( SELF, oe, " -", paren, NO_WRAP );
				break;
				
			//MARK:OP_PLUS
			case OP_PLUS:
				EXPRop2__swift( SELF,SELF, oe, " + ", paren, previous_op, can_wrap );
				break;
				
				//MARK:OP_TIMES
			case OP_TIMES:
				EXPRop2__swift( SELF,SELF, oe, " * ", paren, previous_op, can_wrap );
				break;
				
			//MARK:OP_MINUS
			case OP_MINUS:
				EXPRop2__swift( SELF,SELF, oe, " - ", paren, OP_UNKNOWN, can_wrap );
				break;
				
			//MARK:OP_REAL_DIV
			case OP_REAL_DIV:
				EXPRop2__swift( SELF,SELF, oe, " / ", paren, OP_UNKNOWN, can_wrap );
				break;
				
			//MARK:OP_EXP
			case OP_EXP:
				EXPRop2__swift( SELF,SELF, oe, " ** ", paren, OP_UNKNOWN, can_wrap );
				break;
				
				//MARK:OP_DIV
			case OP_DIV:
				EXPRop2__swift( SELF,SELF, oe, " ./. ", paren, OP_UNKNOWN, can_wrap );
				break;
				
				//MARK:OP_MOD
			case OP_MOD:
				EXPRop2__swift( SELF,SELF, oe, " % ", paren, OP_UNKNOWN, can_wrap );
				break;
				
				
		//MARK: - Relational Op
			//MARK:OP_EQUAL
			case OP_EQUAL:
				EXPRop2__swift( SELF,SELF, oe, " .==. ", paren, OP_UNKNOWN, can_wrap );
				break;
				
			//MARK:OP_NOT_EQUAL
			case OP_NOT_EQUAL:
				EXPRop2__swift( SELF,SELF, oe, " .!=. ", paren, OP_UNKNOWN, can_wrap );
				break;
				
			//MARK:OP_GREATER_THAN
			case OP_GREATER_THAN:
				EXPRop2__swift( SELF,SELF, oe, " > ", paren, OP_UNKNOWN, can_wrap );
				break;
				
			//MARK:OP_LESS_THAN
			case OP_LESS_THAN:
				EXPRop2__swift( SELF,SELF, oe, " < ", paren, OP_UNKNOWN, can_wrap );
				break;
				
			//MARK:OP_GREATER_EQUAL
			case OP_GREATER_EQUAL:
				EXPRop2__swift( SELF,SELF, oe, " >= ", paren, OP_UNKNOWN, can_wrap );
				break;
				
			//MARK:OP_LESS_EQUAL
			case OP_LESS_EQUAL:
				EXPRop2__swift( SELF,SELF, oe, " <= ", paren, OP_UNKNOWN, can_wrap );
				break;
				
			//MARK:OP_INST_EQUAL
			case OP_INST_EQUAL:
				EXPRop2__swift( SELF,SELF, oe, " .===. ", paren, OP_UNKNOWN, can_wrap );
				break;
				
			//MARK:OP_INST_NOT_EQUAL
			case OP_INST_NOT_EQUAL:
				EXPRop2__swift( SELF,SELF, oe, " .!==. ", paren, OP_UNKNOWN, can_wrap );
				break;
				
			//MARK:OP_IN
			case OP_IN:
				EXPRopIn_swift( SELF, oe, can_wrap );
				break;
				
			//MARK:OP_LIKE
			case OP_LIKE:
				EXPRopLike_swift(SELF, oe, can_wrap);
				break;
				
				
		//MARK: - Logical Op
			//MARK:OP_NOT
			case OP_NOT:
				EXPRop1_swift( SELF, oe, " !", paren, NO_WRAP );
				break;
				
			//MARK:OP_AND
			case OP_AND:
				EXPRop2__swift( SELF,SELF, oe, " && ", paren, previous_op, can_wrap );
				break;
				
			//MARK:OP_OR
			case OP_OR:
				EXPRop2__swift( SELF,SELF, oe, " || ", paren, previous_op, can_wrap );
				break;
				
			//MARK:OP_XOR
			case OP_XOR:
				EXPRop2__swift( SELF,SELF, oe, " .!=. ", paren, OP_UNKNOWN, can_wrap );
				break;
				
		//MARK: - String Op
			//MARK:OP_SUBCOMPONENT
			case OP_SUBCOMPONENT:
				EXPR__swift( SELF, oe->op1, oe->op1->return_type, YES_PAREN, OP_UNKNOWN, can_wrap );
				wrap_if(can_wrap, "[" );
				EXPR__swift( SELF, oe->op2, Type_Integer, NO_PAREN, OP_UNKNOWN, YES_WRAP );
				raw(" ... " );
				EXPR__swift( SELF, oe->op3, Type_Integer, NO_PAREN, OP_UNKNOWN, YES_WRAP );
				raw( "]" );
				break;
				
				
		//MARK: - Aggregate Op
			//MARK:OP_ARRAY_ELEMENT
			case OP_ARRAY_ELEMENT:
				EXPR__swift( SELF, oe->op1,oe->op1->return_type, YES_PAREN, OP_UNKNOWN, can_wrap );
				if( EXPRresult_is_optional(oe->op1) ){
					raw("?");
				}
				raw("[");
				EXPR__swift( SELF, oe->op2, Type_Integer, NO_PAREN, OP_UNKNOWN, YES_WRAP );
				raw( "]" );
				break;
				
		//MARK: - Reference Op
			//MARK:OP_DOT
			case OP_DOT:
				EXPRopDot_swift( SELF, oe, can_wrap );
				break;
				
			//MARK:OP_GROUP
			case OP_GROUP:
				EXPRopGroup_swift( SELF, oe, can_wrap );
				break;
				
				
		//MARK: - Complex Entity Constructor
			//MARK:OP_CONCAT
			case OP_CONCAT:
				EXPRop2__swift( SELF,SELF, oe, " .||. ", paren, previous_op, can_wrap );
				break;
				
				
//				//MARK:OP_ANDOR
//        case OP_ANDOR:
//					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, previous_op, YES_WRAP );
//					break;
				
        default:
            wrap_if(can_wrap, "(* unknown op-expression *)" );
    }
}

void EXPRop2__swift( Scope SELF1, Scope SELF2, struct Op_Subexpression * oe, char * opcode, 
										bool paren, unsigned int previous_op, bool can_wrap ) {
	bool can_wrap2 = YES_WRAP;
	if( oe->op_code == OP_DOT ) {
		can_wrap = YES_WRAP;
		can_wrap2 = NO_WRAP;
	}
	
	if( paren && ( oe->op_code != previous_op ) ) {
		wrap_if(can_wrap, "( " );
	}
	
	//OPERAND1
#if 0
	raw("/*");
	TYPE_head_swift(SELF1, oe->op1->return_type, YES_IN_COMMENT);
	raw("*/");
#endif
	EXPRassignment_rhs_swift(SELF1, oe->op1, oe->op1->return_type, YES_PAREN, oe->op_code, can_wrap);

	if( oe->op_code == OP_DOT ) positively_wrap();
	
	//OP CODE
	assert(opcode != NULL);
	wrap_if(can_wrap, "%s", opcode);

	switch (oe->op_code) {
		case OP_AND:
		case OP_OR:
		case OP_CONCAT:
			positively_wrap();	
			break;
			
		default:
			break;
	}
	
	//OPERAND2
#if 0
	raw("/*");
	TYPE_head_swift(SELF2, oe->op2->return_type, YES_IN_COMMENT);
	raw("*/");
#endif
	EXPRassignment_rhs_swift(SELF2, oe->op2, oe->op2->return_type, YES_PAREN, oe->op_code, can_wrap2);
	if( paren && ( oe->op_code != previous_op ) ) {
		raw( " )" );
	}
}

/** Print out a one-operand operation.  If there were more than two of these
 * I'd generalize it to do padding, but it's not worth it.
 */
static void EXPRop1_swift( Scope SELF, struct Op_Subexpression * oe, char * opcode, 
													bool paren, bool can_wrap ) {
    if( paren ) {
        wrap_if(can_wrap, "( " );
    }
    wrap_if(can_wrap, "%s", opcode );
    EXPR__swift( SELF, oe->op1, oe->op1->return_type, YES_PAREN, OP_UNKNOWN, can_wrap );
    if( paren ) {
        raw( " )" );
    }
}

static void aggregate_init( Scope SELF, Expression rhs, Type lhsType ) {
	if( !isLITERAL_INFINITY(rhs) ){
		TYPE_head_swift(SELF, lhsType, WO_COMMENT);
		raw("(");

		TypeBody typebody = TYPEget_body( lhsType );
		if( typebody->upper ){
			wrap("bound1: ");
			EXPR_swift(SELF, typebody->lower, Type_Integer, NO_PAREN);
			raw(", ");
			wrap("bound2: ");
			EXPR_swift(SELF, typebody->upper, Type_Integer, NO_PAREN);
			raw(", ");
		}
		EXPR_swift(SELF, rhs, lhsType, NO_PAREN);

		raw(")");
	}
	else {		
		EXPR_swift(SELF, rhs, lhsType, NO_PAREN);
	}
}

void EXPRassignment_rhs_swift( Scope SELF, Expression rhs, Type lhsType, bool paren, unsigned int previous_op, bool can_wrap) {
	if( lhsType == NULL ) {
//		raw("/*unknown lhs type*/");
		EXPR__swift(SELF, rhs, rhs->return_type, paren, previous_op, can_wrap);
		return;
	}
	
	if( TYPEcontains_generic(lhsType) ) {
//		raw("/*generic lhs type*/");
		EXPR__swift(SELF, rhs, rhs->return_type, paren, previous_op, can_wrap);
		return;
	}
	
	if( TYPEs_are_equal(rhs->return_type, lhsType) ) {
//		raw("/*equal lhs type*/");
		if( TYPEis_AGGREGATE(rhs->type) ) {
			aggregate_init(SELF, rhs, lhsType);
			return;;
		}
		
		EXPR__swift(SELF, rhs, lhsType, paren, previous_op, can_wrap);
		return;
	}
	
	if( TYPEis_aggregation_data_type(lhsType) ) {
//		raw("/*%s lhs type*/",TYPEis_runtime(lhsType)? "runtime":"unknown");
		aggregate_init(SELF, rhs, lhsType);
		return;
	}
	
	if( lhsType == Type_Entity ) {
//		raw("SDAI.EntityReference(");
		EXPR__swift(SELF, rhs, lhsType, paren, previous_op, can_wrap);
		raw(")");
		return;
	}
	
	if( !isLITERAL_INFINITY(rhs) ) {
		TYPE_head_swift(SELF, lhsType, WO_COMMENT);	// wrap with explicit type cast
		raw("(");
		EXPR__swift(SELF, rhs, lhsType, paren, previous_op, can_wrap);
		raw(")");
		return;
	}

	EXPR__swift(SELF, rhs, lhsType, paren, previous_op, can_wrap);
	return;
}
