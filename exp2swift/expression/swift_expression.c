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

#include "exppp.h"
#include "pp.h"
#include "builtin.h"

#include "pretty_expr.h"

#include "swift_expression.h"
#include "swift_symbol.h"
#include "swift_func.h"
#include "swift_entity.h"
#include "swift_type.h"

//*TY2020/07/11
#define YES_PAD	true
#define NO_PAD	false
#define YES_WRAP	true
#define NO_WRAP	false
#define OPCODE_FROM_EXPRESS	( char * )0


#define EXPRop2_swift(	SELF1,SELF2,oe,string,paren,pad) \
				EXPRop2__swift(	SELF1,SELF2,oe,string,paren,pad,		OP_UNKNOWN, YES_WRAP)

#define EXPRop_swift(		SELF,oe,paren) \
				EXPRop__swift(	SELF,oe,paren,		OP_UNKNOWN, YES_WRAP)

static void EXPR__swift( Scope SELF, Expression e, 
												bool paren, unsigned int previous_op, bool can_wrap );


static void EXPRop__swift( Scope SELF, struct Op_Subexpression * oe, 
													bool paren, unsigned int previous_op, bool can_wrap );

static void EXPRop2__swift( Scope SELF1, Scope SELF2, struct Op_Subexpression * eo, char * opcode, 
													 bool paren, bool pad, unsigned int previous_op, bool can_wrap );

static void EXPRop1_swift( Scope SELF, struct Op_Subexpression * eo, char * opcode, 
													bool paren, bool can_wrap );


void EXPR_swift( Scope SELF, Expression e, bool paren) {
	EXPR__swift( SELF,e, paren, YES_WRAP, OP_UNKNOWN );
}



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


void EXPRbounds_swift(Scope SELF, TypeBody tb, bool in_comment ) {
    if( !tb->upper ) {
        return;
    }

	wrap( "%s[", in_comment ? "" : "/*" );
    EXPR_swift(SELF, tb->lower, YES_PAREN );
    raw( ":" );
    EXPR_swift(SELF, tb->upper, YES_PAREN );
	raw( "]%s ", in_comment ? "" : "*/" );
}

static void EXPRopGroup_swift( Scope SELF, struct Op_Subexpression * eo, bool can_wrap ) {
	EXPR__swift( SELF, eo->op1, YES_PAREN, eo->op_code, YES_WRAP );
	wrap_if(YES_WRAP, ".%s",superEntity_swiftPrefix);
	EXPR__swift( SELF, eo->op2, YES_PAREN, eo->op_code, NO_WRAP );
}

static void EXPRopIn_swift( Scope SELF, struct Op_Subexpression * eo, bool can_wrap ) {
	if( TYPEis(eo->op1->return_type)==string_ && TYPEis(eo->op2->type)==funcall_ && eo->op2->u.funcall.function==func_typeof ) {
		Expression arg = LISTget_first(eo->op2->u.funcall.list);
		wrap_if(YES_WRAP, "SDAI.TYPEOF(");
		EXPR_swift( SELF, arg, NO_PAREN );
		raw(", ");
		wrap_if(YES_WRAP, "IS: ");
		EXPR__swift( SELF, eo->op1, NO_PAREN, eo->op_code, YES_WRAP );
		raw(")");
	}
	else {
		EXPR__swift( SELF, eo->op2, YES_PAREN, eo->op_code, YES_WRAP );
		positively_wrap();
		wrap_if(YES_WRAP, ".contains(");
		EXPR__swift( SELF, eo->op1, NO_PAREN, eo->op_code, YES_WRAP );
		raw(")");		
	}
	
	
	
}

static void EXPRopLike_swift( Scope SELF, struct Op_Subexpression * eo, bool can_wrap ) {
	EXPR__swift( SELF, eo->op1, YES_PAREN, eo->op_code, YES_WRAP );
	wrap_if(YES_WRAP, ".isLike( pattern: ");
	EXPR__swift( SELF, eo->op2, NO_PAREN, eo->op_code, YES_WRAP );
	raw(" )");
}

static void EXPRopMod_swift( Scope SELF, struct Op_Subexpression* eo, bool can_wrap) {
	wrap_if(can_wrap, "Int(" );
	EXPR__swift( SELF, eo->op1, NO_PAREN, eo->op_code, can_wrap );
	wrap_if(can_wrap, ") % ");
	wrap_if(can_wrap, "Int(" );
	EXPR__swift( SELF, eo->op2, NO_PAREN, eo->op_code, can_wrap );
	wrap_if(can_wrap, ")");
}

static void EXPRopDIV_swift( Scope SELF, struct Op_Subexpression* eo, bool can_wrap) {
	wrap_if(can_wrap, "Int(" );
	EXPR__swift( SELF, eo->op1, NO_PAREN, eo->op_code, can_wrap );
	wrap_if(can_wrap, ") / ");
	wrap_if(can_wrap, "Int(" );
	EXPR__swift( SELF, eo->op2, NO_PAREN, eo->op_code, can_wrap );
	wrap_if(can_wrap, ")");
}

static void EXPRrepeat_swift( Scope SELF, Expression val, Expression count) {
//	assert(val != NULL);
	assert(count != NULL);
	
	wrap_if(YES_WRAP,"SDAI.AIE(");
	if(val) {
		EXPR__swift( SELF, val, NO_PAREN, OP_UNKNOWN, YES_WRAP );
	}
	else {
		raw("###NULL###");
	}
	raw(", ");
	wrap_if(YES_WRAP,"repeat:Int(");
	EXPR__swift( SELF, count, NO_PAREN, OP_UNKNOWN, YES_WRAP );
	raw("))");
}

//MARK: - main entry point
/**
 * if paren == 1, parens are usually added to prevent possible rebind by
 *    higher-level context.  If op is similar to previous op (and
 *    precedence/associativity is not a problem) parens may be omitted.
 * if paren == 0, then parens may be omitted without consequence
 */
void EXPR__swift( Scope SELF, Expression e, bool paren, unsigned int previous_op, bool can_wrap  ) {
	int i;  /* trusty temporary */
		
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
			wrap_if(can_wrap, "%%%s", e->u.binary ); /* put "%" back */
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
					EXPR__swift(SELF, v->initializer, NO_PAREN, OP_UNKNOWN, can_wrap);
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
			EXPR__swift( SELF, e->u.query->aggregate, YES_PAREN, OP_UNKNOWN, can_wrap );
		{
			char buf[BUFSIZ];
			aggressively_wrap();
			raw( ".QUERY{ %s in ", variable_swiftName(e->u.query->local,buf));
		}
			EXPR__swift( SELF, e->u.query->expression, NO_PAREN, OP_UNKNOWN, can_wrap );
			raw( " }" );
			break;
			
			//MARK: self_
		case self_:
			wrap_if(can_wrap, "SELF" );
			break;
			
			//MARK: funcall_
		case funcall_:
		{
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
				
				aggressively_wrap();
				wrap_if(YES_WRAP, "%s%s(", qual, func );
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
						EXPR_swift( SELF, arg, NO_PAREN );
					}
					else {
						EXPRassignment_rhs_swift(SELF, arg, formal_param->type);
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
			wrap_if(can_wrap, "[" );
			char* sep = "";
			Expression prev = NULL;
			
			LISTdo( e->u.list, arg, Expression ) {
				raw(sep);
				positively_wrap();
				
				bool repeat = arg->type->u.type->body->flags.repeat;
				/* if repeat is true, the previous Expression repeats and this one is the count */
				
				if( repeat ) {
					EXPRrepeat_swift(SELF, prev, arg);
					sep = ", ";
					prev = NULL;
				}
				else {
					if(prev != NULL) {
						wrap_if(can_wrap, "SDAI.AIE(");
						EXPR__swift( SELF, prev, NO_PAREN, OP_UNKNOWN, YES_WRAP );
						raw(")");
						sep = ", ";
					}
					prev = arg;
				}
			} LISTod
			
			if(prev != NULL) {
				raw(sep);
				positively_wrap();
				EXPR__swift( SELF, prev, NO_PAREN, OP_UNKNOWN, YES_WRAP );
			}
			raw( "]" );
		}
			break;
			
			//MARK: oneof_
		case oneof_: {
			int old_indent = indent2;
			wrap_if(can_wrap, "ONEOF ( " );
			
			if( exppp_linelength == indent2 ) {
				exppp_linelength += exppp_continuation_indent;
			}
			indent2 += exppp_continuation_indent;
			
			i = 0;
			LISTdo( e->u.list, arg, Expression )
			i++;
			if( i != 1 ) {
				raw( ", " );
			}
			EXPR__swift( SELF, arg, YES_PAREN, OP_UNKNOWN, can_wrap );
			LISTod
			
			if( exppp_linelength == indent2 ) {
				exppp_linelength = old_indent;
			}
			indent2 = old_indent;
			
			raw( " )" );
			break;
		}
		default:
			fprintf( stderr, "%s:%d: ERROR - unknown expression, type %d", e->symbol.filename, e->symbol.line, TYPEis( e->type ) );
			abort();
	}
}

/** print expression that has op and operands */
void EXPRop__swift( Scope SELF, struct Op_Subexpression * oe, 
									 bool paren, unsigned int previous_op, bool can_wrap) {
    switch( oe->op_code ) {
				//MARK:OP_AND
        case OP_AND:
					EXPRop2__swift( SELF,SELF, oe, "&&", paren, YES_PAD, previous_op, YES_WRAP );
					break;
				
				//MARK:OP_ANDOR
        case OP_ANDOR:
					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, previous_op, YES_WRAP );
					break;
				
				//MARK:OP_OR
        case OP_OR:
					EXPRop2__swift( SELF,SELF, oe, "||", paren, YES_PAD, previous_op, YES_WRAP );
					break;
				
				//MARK:OP_CONCAT
        case OP_CONCAT:
					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, previous_op, YES_WRAP );
					break;
				
				//MARK:OP_EQUAL
        case OP_EQUAL:
					EXPRop2__swift( SELF,SELF, oe, "==", paren, YES_PAD, previous_op, YES_WRAP );
					break;
				
				//MARK:OP_PLUS
        case OP_PLUS:
					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, previous_op, YES_WRAP );
					break;
				
				//MARK:OP_TIMES
        case OP_TIMES:
					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, previous_op, YES_WRAP );
					break;
				
				//MARK:OP_XOR
        case OP_XOR:
            EXPRop2__swift( SELF,SELF, oe, "^^", paren, YES_PAD, previous_op, YES_WRAP );
            break;
				
				//MARK:OP_EXP
        case OP_EXP:
					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
					break;
				
				//MARK:OP_GREATER_EQUAL
        case OP_GREATER_EQUAL:
					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
					break;
				
				//MARK:OP_GREATER_THAN
        case OP_GREATER_THAN:
					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
					break;
				
				//MARK:OP_IN
        case OP_IN:
					EXPRopIn_swift( SELF, oe, YES_WRAP );
					break;
				
				//MARK:OP_INST_EQUAL
        case OP_INST_EQUAL:
					EXPRop2__swift( SELF,SELF, oe, "===", paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
					break;
				
				//MARK:OP_INST_NOT_EQUAL
        case OP_INST_NOT_EQUAL:
					EXPRop2__swift( SELF,SELF, oe, "!==", paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
					break;
				
				//MARK:OP_LESS_EQUAL
        case OP_LESS_EQUAL:
					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
					break;
				
				//MARK:OP_LESS_THAN
        case OP_LESS_THAN:
					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
					break;
				
				//MARK:OP_LIKE
        case OP_LIKE:
					EXPRopLike_swift(SELF, oe, YES_WRAP);
					break;
				
				//MARK:OP_MOD
        case OP_MOD:
//					EXPRop2__swift( SELF,SELF, oe, "%", paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
					EXPRopMod_swift(SELF, oe, YES_WRAP);
				break;
				
				//MARK:OP_NOT_EQUAL
        case OP_NOT_EQUAL:
            EXPRop2__swift( SELF,SELF, oe, "!=", paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
            break;
				
				//MARK:OP_NOT
        case OP_NOT:
            EXPRop1_swift( SELF, oe, " !", paren, NO_WRAP );
            break;
				
				//MARK:OP_REAL_DIV
        case OP_REAL_DIV:
            EXPRop2__swift( SELF,SELF, oe, "/", paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
            break;
				
				//MARK:OP_DIV
        case OP_DIV:
//            EXPRop2__swift( SELF,SELF, oe, "DIV", paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
				EXPRopDIV_swift(SELF, oe, YES_WRAP);
            break;
				
				//MARK:OP_MINUS
        case OP_MINUS:
            EXPRop2__swift( SELF,SELF, oe, "-", paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
            break;
				
				//MARK:OP_DOT
        case OP_DOT:
            EXPRop2__swift( SELF,NULL, oe, ".", paren, NO_PAD, OP_UNKNOWN, can_wrap );
            break;
				
				//MARK:OP_GROUP
        case OP_GROUP:
						EXPRopGroup_swift( SELF, oe, can_wrap );
            break;
				
				//MARK:OP_NEGATE
        case OP_NEGATE:
            EXPRop1_swift( SELF, oe, " -", paren, NO_WRAP );
            break;
				
				//MARK:OP_ARRAY_ELEMENT
        case OP_ARRAY_ELEMENT:
						EXPR__swift( SELF, oe->op1, YES_PAREN, OP_UNKNOWN, can_wrap );
//            wrap_if(can_wrap, "[SDAI.INDEX(" );
						raw("[Int(");
            EXPR__swift( SELF, oe->op2, NO_PAREN, OP_UNKNOWN, YES_WRAP );
            raw( ")]" );
            break;
				
				//MARK:OP_SUBCOMPONENT
        case OP_SUBCOMPONENT:
            EXPR__swift( SELF, oe->op1, YES_PAREN, OP_UNKNOWN, can_wrap );
            wrap_if(can_wrap, "[Int(" );
            EXPR__swift( SELF, oe->op2, NO_PAREN, OP_UNKNOWN, YES_WRAP );
            raw(") ... Int(" );
            EXPR__swift( SELF, oe->op3, NO_PAREN, OP_UNKNOWN, YES_WRAP );
            raw( ")]" );
            break;
				
        default:
            wrap_if(can_wrap, "(* unknown op-expression *)" );
    }
}

void EXPRop2__swift( Scope SELF1, Scope SELF2, struct Op_Subexpression * eo, char * opcode, 
										bool paren, bool pad, unsigned int previous_op, bool can_wrap ) {
	bool can_wrap2 = can_wrap;
	if( eo->op_code == OP_DOT ) {
		can_wrap = YES_WRAP;
		can_wrap2 = NO_WRAP;
	}
	
	if( pad && paren && ( eo->op_code != previous_op ) ) {
		wrap_if(can_wrap, "( " );
	}
	
	//OPERAND1
	EXPR__swift( SELF1, eo->op1, YES_PAREN, eo->op_code, can_wrap );
	if( pad ) {
		raw( " " );
	}

	if( eo->op_code == OP_DOT ) positively_wrap();
	
	//OP CODE
	wrap_if(can_wrap, "%s", ( opcode ? opcode : EXPop_table[eo->op_code].token ) );
	if( pad ) {
		wrap_if(can_wrap, " " );
	}

	switch (eo->op_code) {
		case OP_AND:
		case OP_OR:
		case OP_CONCAT:
			positively_wrap();	
			break;
			
		default:
			break;
	}
	
	//OPERAND2
	EXPR__swift( SELF2, eo->op2, YES_PAREN, eo->op_code, can_wrap2 );
	if( pad && paren && ( eo->op_code != previous_op ) ) {
		raw( " )" );
	}
}

/** Print out a one-operand operation.  If there were more than two of these
 * I'd generalize it to do padding, but it's not worth it.
 */
static void EXPRop1_swift( Scope SELF, struct Op_Subexpression * eo, char * opcode, 
													bool paren, bool can_wrap ) {
    if( paren ) {
        wrap_if(can_wrap, "( " );
    }
    wrap_if(can_wrap, "%s", opcode );
    EXPR__swift( SELF, eo->op1, YES_PAREN, OP_UNKNOWN, can_wrap );
    if( paren ) {
        raw( " )" );
    }
}

static void aggregate_init( Scope SELF, Expression rhs, Type lhsType ) {
	TypeBody typebody = TYPEget_body( lhsType );
	if( typebody->upper ){
		wrap("bound1: ");
		EXPR_swift(SELF, typebody->lower, NO_PAREN);
		raw(", ");
		wrap("bound2: ");
		EXPR_swift(SELF, typebody->upper, NO_PAREN);
		raw(", ");
	}
	EXPR_swift(SELF, rhs, NO_PAREN);
}

void EXPRassignment_rhs_swift( Scope SELF, Expression rhs, Type lhsType) {
	if( TYPEis(lhsType)==generic_ || TYPEis(lhsType)==aggregate_ ) {
		EXPR_swift(SELF, rhs, NO_PAREN);
	}
	else if( TYPEs_are_equal(rhs->return_type, lhsType) ) {
		EXPR_swift(SELF, rhs, NO_PAREN);
	}
	else {
		TYPE_head_swift(SELF, lhsType, NOT_IN_COMMENT);
		raw("(");
		if( TYPEis_aggregate(lhsType) ) {
			aggregate_init(SELF, rhs, lhsType);
		}
		else {
			EXPR_swift(SELF, rhs, NO_PAREN);
		}
		raw(")");
	}
}
