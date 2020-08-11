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

#include "pretty_expr.h"

#include "swift_expression.h"
#include "swift_symbol.h"
#include "swift_func.h"
#include "swift_entity.h"



extern bool printedSpaceLast;
int nextBreakpoint( const char * pos, const char * end );
bool shouldBreak( int len );

/** Insert newline if it makes sense. */
void maybeBreak_swift( int len, bool first ) {
    if( shouldBreak( len ) ) {
        if( first ) {
            raw( "\n%*s\"", indent2, "" );
        } else {
            raw( "\"\n%*s+ \"", indent2, "" );
        }
    } else if( first ) {
        /* staying on same line */
        raw( "%s", ( printedSpaceLast ? "\"": " \"" ) );
    }
}


void breakLongStr_swift( const char * instring ) {
	const char * iptr = instring, * end;
	int inlen = (int)strlen( instring );
	bool first = true;
	/* used to ensure that we don't overrun the input buffer */
	end = instring + inlen;
	
	if( ( inlen == 0 ) || ( ( ( int ) inlen + curpos ) < exppp_linelength ) ) {
		/* short enough to fit on current line */
		wrap("\"%s\"", instring );
		return;
	}
	
	/* insert newlines at dots as necessary */
	while( ( iptr < end ) && ( *iptr ) ) {
		int i = nextBreakpoint( iptr, end );
		maybeBreak_swift( i, first );
		first = false;
		raw( "%.*s", i, iptr );
		iptr += i;
	}
	raw( "\" ");
}

void EXPRbounds_swift(Scope SELF, TypeBody tb ) {
    if( !tb->upper ) {
        return;
    }

    wrap( "/*[" );
    EXPR_swift(SELF, tb->lower, NO_PAREN );
    raw( ":" );
    EXPR_swift(SELF, tb->upper, NO_PAREN );
    raw( "]*/ " );
}


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
			if( e == LITERAL_INFINITY ) {
				wrap_if(can_wrap, "nil" );
			} else {
				wrap_if(can_wrap, "%d", e->u.integer );
			}
			break;
			
			//MARK: real_
		case real_:
			if( e == LITERAL_PI ) {
				wrap_if(can_wrap, "SDAI.PI" );
			} else if( e == LITERAL_E ) {
				wrap_if(can_wrap, "SDAI.E" );
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
				wrap_if(can_wrap, "\"%s\"", e->symbol.name );
			} else {
				breakLongStr_swift( e->symbol.name );
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
					raw("(/*%s*/",e->symbol.name);
					EXPR__swift(SELF, v->initializer, NO_PAREN, OP_UNKNOWN, can_wrap);
					raw(")");
					break;
				}
				Entity ent = v->defined_in;
				if( ENTITYis_a(SELF, ent) ) {
					wrap_if(can_wrap, "SELF.");
				}
			}
			raw( "%s", e->symbol.name );
			break;
			
			//MARK: query_
		case query_:
			EXPR__swift( SELF, e->u.query->aggregate, YES_PAREN, OP_UNKNOWN, can_wrap );
			force_wrap();raw( ".QUERY{ %s in ", variable_swiftName(e->u.query->local));
			EXPR__swift( SELF, e->u.query->expression, YES_PAREN, OP_UNKNOWN, can_wrap );
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
			
			char buf[BUFSIZ];
			char* qual = "";
			const char* func;
			Linked_List formals;
			
			if( isfunc ) {
				if( e->u.funcall.function->u.func->builtin ) qual = "SDAI.";
				func = FUNCcall_swiftName(e);
				formals = e->u.funcall.function->u.func->parameters;
			}
			else {	// entity constructor call
				func = partialEntity_swiftName(e->u.funcall.function, buf);
				formals = ENTITYget_constructor_params(e->u.funcall.function);
			}
			
			wrap_if(can_wrap, "%s%s(", qual, func );
			
			char* sep = "";
			if( isfunc && (LISTget_second(formals) == NULL) ) formals = NULL;
			
			if( formals != NULL ) {	// func call with multiple args
				Link formals_iter = LISTLINKfirst(formals);
				LISTdo( e->u.funcall.list, arg, Expression ) {
					raw("%s",sep);
					positively_wrap();
					
					assert(formals_iter != NULL);
					Variable formal_param = formals_iter->data;
					wrap("%s: ", variable_swiftName(formal_param));
					if(VARis_inout(formal_param)) {
						raw("&");
					}
					EXPR__swift( SELF, arg, NO_PAREN, OP_UNKNOWN, NO_WRAP );
					
					sep = ", ";
					formals_iter = LISTLINKnext(formals, formals_iter);
				}LISTod
			}
			else {	
				LISTdo( e->u.funcall.list, arg, Expression ) {
					raw("%s",sep);
					positively_wrap();
					
					EXPR__swift( SELF, arg, NO_PAREN, OP_UNKNOWN, YES_WRAP );
					
					sep = ", ";
				}LISTod
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
			wrap_if(can_wrap, "[" );
			i = 0;
			LISTdo( e->u.list, arg, Expression ) {
				bool repeat = arg->type->u.type->body->flags.repeat;
				/* if repeat is true, the previous Expression repeats and this one is the count */
				i++;
				if( i != 1 ) {
					if( repeat ) {
						raw( " : " );
					} else {
						raw( ", " );
						positively_wrap();
					}
				}
				EXPR__swift( SELF, arg, NO_PAREN, OP_UNKNOWN, YES_WRAP );
			} LISTod
			raw( "]" );
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
void EXPRop__swift( Scope SELF, struct Op_Subexpression * oe, bool paren, unsigned int previous_op, bool can_wrap) {
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
					EXPRopIn_swift( SELF, oe, can_wrap );
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
					EXPRop2__swift( SELF,SELF, oe, "~=", paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
					break;
				
				//MARK:OP_MOD
        case OP_MOD:
					EXPRop2__swift( SELF,SELF, oe, "%", paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
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
            EXPRop2__swift( SELF,SELF, oe, "DIV", paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
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
            wrap_if(can_wrap, "[SDAI.INDEX(" );
            EXPR__swift( SELF, oe->op2, NO_PAREN, OP_UNKNOWN, YES_WRAP );
            raw( ")]" );
            break;
				
				//MARK:OP_SUBCOMPONENT
        case OP_SUBCOMPONENT:
            EXPR__swift( SELF, oe->op1, YES_PAREN, OP_UNKNOWN, can_wrap );
            wrap_if(can_wrap, "[SDAI.RANGE(" );
            EXPR__swift( SELF, oe->op2, NO_PAREN, OP_UNKNOWN, YES_WRAP );
            raw(", " );
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

void EXPRopGroup_swift( Scope SELF, struct Op_Subexpression * eo, bool can_wrap ) {
	EXPR__swift( SELF, eo->op1, YES_PAREN, eo->op_code, can_wrap );
	wrap_if(YES_WRAP, ".SUPER_");
	EXPR__swift( SELF, eo->op2, YES_PAREN, eo->op_code, NO_WRAP );
}

void EXPRopIn_swift( Scope SELF, struct Op_Subexpression * eo, bool can_wrap ) {
	EXPR__swift( SELF, eo->op2, YES_PAREN, eo->op_code, can_wrap );
	positively_wrap();
	wrap_if(YES_WRAP, ".contains( ");
	EXPR__swift( SELF, eo->op1, YES_PAREN, eo->op_code, YES_WRAP );
	raw(" )");
}

/** Print out a one-operand operation.  If there were more than two of these
 * I'd generalize it to do padding, but it's not worth it.
 */
void EXPRop1_swift( Scope SELF, struct Op_Subexpression * eo, char * opcode, bool paren, bool can_wrap ) {
    if( paren ) {
        wrap_if(can_wrap, "( " );
    }
    wrap_if(can_wrap, "%s", opcode );
    EXPR__swift( SELF, eo->op1, YES_PAREN, OP_UNKNOWN, can_wrap );
    if( paren ) {
        raw( " )" );
    }
}

