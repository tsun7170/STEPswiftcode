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


void breakLongStr_swift( const char * in ) {
    const char * iptr = in, * end;
    int inlen = (int)strlen( in );
    bool first = true;
    /* used to ensure that we don't overrun the input buffer */
    end = in + inlen;

    if( ( inlen == 0 ) || ( ( ( int ) inlen + curpos ) < exppp_linelength ) ) {
        /* short enough to fit on current line */
        raw( "%s\"%s\"", ( printedSpaceLast ? "": " " ), in );
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
    wrap( ":" );
    EXPR_swift(SELF, tb->upper, NO_PAREN );
    raw( "]*/" );
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
		case integer_:
			if( e == LITERAL_INFINITY ) {
				wrap_if(can_wrap, "nil" );
			} else {
				wrap_if(can_wrap, "%d", e->u.integer );
			}
			break;
		case real_:
			if( e == LITERAL_PI ) {
				wrap_if(can_wrap, "SDAI.PI" );
			} else if( e == LITERAL_E ) {
				wrap_if(can_wrap, "SDAI.E" );
			} else {
				wrap_if(can_wrap, real2exp( e->u.real ) );
			}
			break;
		case binary_:
			wrap_if(can_wrap, "%%%s", e->u.binary ); /* put "%" back */
			break;
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
		case string_:
			if( TYPEis_encoded( e->type ) ) {
				wrap_if(can_wrap, "\"%s\"", e->symbol.name );
			} else {
				breakLongStr_swift( e->symbol.name );
			}
			break;
		case attribute_:
		case entity_:
		case identifier_:
		case enumeration_:			
			if( e->u_tag == expr_is_variable ) {
				Variable v = e->u.variable;
				Entity ent = v->defined_in;
				if( ENTITYis_a(SELF, ent) ) {
					wrap_if(can_wrap, "SELF.");
				}
			}
			raw( "%s", e->symbol.name );
			break;
		case query_:
			EXPR__swift( SELF, e->u.query->aggregate, YES_PAREN, OP_UNKNOWN, can_wrap );
			force_wrap();raw( ".QUERY{ %s in ",e->u.query->local->name->symbol.name);
			EXPR__swift( SELF, e->u.query->expression, YES_PAREN, OP_UNKNOWN, can_wrap );
			raw( " }" );
			break;
		case self_:
			wrap_if(can_wrap, "SELF" );
			break;
		case funcall_:
			wrap_if(can_wrap, "%s( ", e->symbol.name );
			i = 0;
			LISTdo( e->u.funcall.list, arg, Expression )
			i++;
			if( i != 1 ) {
				raw( ", " );
			}
			EXPR__swift( SELF, arg, NO_PAREN, OP_UNKNOWN, YES_WRAP );
			LISTod
			raw( " )" );
			break;
		case op_:
			EXPRop__swift( SELF, &e->e, paren, previous_op, can_wrap );
			break;
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
					}
				}
				EXPR__swift( SELF, arg, NO_PAREN, OP_UNKNOWN, YES_WRAP );
			} LISTod
			raw( "]" );
			break;
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
        case OP_AND:
					EXPRop2__swift( SELF,SELF, oe, "&&", paren, YES_PAD, previous_op, YES_WRAP );
					break;
        case OP_ANDOR:
					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, previous_op, YES_WRAP );
					break;
        case OP_OR:
					EXPRop2__swift( SELF,SELF, oe, "||", paren, YES_PAD, previous_op, YES_WRAP );
					break;
        case OP_CONCAT:
					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, previous_op, YES_WRAP );
					break;
        case OP_EQUAL:
					EXPRop2__swift( SELF,SELF, oe, "==", paren, YES_PAD, previous_op, YES_WRAP );
					break;
        case OP_PLUS:
					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, previous_op, YES_WRAP );
					break;
        case OP_TIMES:
					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, previous_op, YES_WRAP );
					break;
        case OP_XOR:
            EXPRop2__swift( SELF,SELF, oe, "^^", paren, YES_PAD, previous_op, YES_WRAP );
            break;
				
        case OP_EXP:
					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
					break;
        case OP_GREATER_EQUAL:
					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
					break;
        case OP_GREATER_THAN:
					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
					break;
        case OP_IN:
					EXPRopIn_swift( SELF, oe, can_wrap );
					break;
        case OP_INST_EQUAL:
					EXPRop2__swift( SELF,SELF, oe, "===", paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
					break;
        case OP_INST_NOT_EQUAL:
					EXPRop2__swift( SELF,SELF, oe, "!==", paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
					break;
        case OP_LESS_EQUAL:
					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
					break;
        case OP_LESS_THAN:
					EXPRop2__swift( SELF,SELF, oe, OPCODE_FROM_EXPRESS, paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
					break;
        case OP_LIKE:
					EXPRop2__swift( SELF,SELF, oe, "~=", paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
					break;
        case OP_MOD:
					EXPRop2__swift( SELF,SELF, oe, "%", paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
					break;
        case OP_NOT_EQUAL:
            EXPRop2__swift( SELF,SELF, oe, "!=", paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
            break;
				
        case OP_NOT:
            EXPRop1_swift( SELF, oe, " !", paren, NO_WRAP );
            break;
        case OP_REAL_DIV:
            EXPRop2__swift( SELF,SELF, oe, "/", paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
            break;
        case OP_DIV:
            EXPRop2__swift( SELF,SELF, oe, "DIV", paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
            break;
        case OP_MINUS:
            EXPRop2__swift( SELF,SELF, oe, "-", paren, YES_PAD, OP_UNKNOWN, YES_WRAP );
            break;
        case OP_DOT:
            EXPRop2__swift( SELF,NULL, oe, ".", paren, NO_PAD, OP_UNKNOWN, can_wrap );
            break;
        case OP_GROUP:
						EXPRopGroup_swift( SELF, oe, can_wrap );
            break;
        case OP_NEGATE:
            EXPRop1_swift( SELF, oe, " -", paren, NO_WRAP );
            break;
        case OP_ARRAY_ELEMENT:
				EXPR__swift( SELF, oe->op1, YES_PAREN, OP_UNKNOWN, can_wrap );
            wrap_if(can_wrap, "[" );
            EXPR__swift( SELF, oe->op2, NO_PAREN, OP_UNKNOWN, YES_WRAP );
            raw( "]" );
            break;
        case OP_SUBCOMPONENT:
            EXPR__swift( SELF, oe->op1, YES_PAREN, OP_UNKNOWN, can_wrap );
            wrap_if(can_wrap, "[" );
            EXPR__swift( SELF, oe->op2, NO_PAREN, OP_UNKNOWN, YES_WRAP );
            wrap_if(can_wrap, " : " );
            EXPR__swift( SELF, oe->op3, NO_PAREN, OP_UNKNOWN, YES_WRAP );
            raw( "]" );
            break;
        default:
            wrap_if(can_wrap, "(* unknown op-expression *)" );
    }
}

void EXPRop2__swift( Scope SELF1, Scope SELF2, struct Op_Subexpression * eo, char * opcode, 
										bool paren, bool pad, unsigned int previous_op, bool can_wrap ) {
	if( pad && paren && ( eo->op_code != previous_op ) ) {
		wrap_if(can_wrap, "( " );
	}
	EXPR__swift( SELF1, eo->op1, YES_PAREN, eo->op_code, can_wrap );
	if( pad ) {
		raw( " " );
	}
	wrap_if(can_wrap, "%s", ( opcode ? opcode : EXPop_table[eo->op_code].token ) );
	if( pad ) {
		wrap_if(can_wrap, " " );
	}
	bool can_wrap2 = can_wrap;
	if( eo->op_code == OP_DOT ) {
		can_wrap2 = NO_WRAP;
	}
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
	force_wrap();
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

//int EXPRop_length( struct Op_Subexpression * oe ) {
//    switch( oe->op_code ) {
//        case OP_DOT:
//        case OP_GROUP:
//            return( 1 + EXPRlength( oe->op1 )
//                    + EXPRlength( oe->op2 ) );
//        default:
//            fprintf( stdout, "EXPRop_length: unknown op-expression" );
//    }
//    return 0;
//}

//
///** returns printable representation of expression rather than printing it
// * originally only used for general references, now being expanded to handle
// * any kind of expression
// * contains fragment of string, adds to it
// */
//void EXPRstring_swift( Scope SELF, char * buffer, Expression e ) {
//    int i;
//
//    switch( TYPEis( e->type ) ) {
//        case integer_:
//            if( e == LITERAL_INFINITY ) {
//                strcpy( buffer, "?" );
//            } else {
//                sprintf( buffer, "%d", e->u.integer );
//            }
//            break;
//        case real_:
//            if( e == LITERAL_PI ) {
//                strcpy( buffer, "SDAI.PI" );
//            } else if( e == LITERAL_E ) {
//                strcpy( buffer, "SDAI.E" );
//            } else {
//                sprintf( buffer, "%s", real2exp( e->u.real ) );
//            }
//            break;
//        case binary_:
//            sprintf( buffer, "%%%s", e->u.binary ); /* put "%" back */
//            break;
//        case logical_:
//        case boolean_:
//            switch( e->u.logical ) {
//                case Ltrue:
//                    strcpy( buffer, "SDAI.TRUE" );
//                    break;
//                case Lfalse:
//                    strcpy( buffer, "SDAI.FALSE" );
//                    break;
//                default:
//                    strcpy( buffer, "SDAI.UNKNOWN" );
//                    break;
//            }
//            break;
//        case string_:
//            if( TYPEis_encoded( e->type ) ) {
//                sprintf( buffer, "\"%s\"", e->symbol.name );
//            } else {
//                sprintf( buffer, "%s", e->symbol.name );
//            }
//            break;
//        case entity_:
//        case identifier_:
//        case attribute_:
//        case enumeration_:
//            strcpy( buffer, e->symbol.name );
//            break;
//        case query_:
//            sprintf( buffer, "QUERY ( %s <* ", e->u.query->local->name->symbol.name );
//            EXPRstring_swift( SELF, buffer + strlen( buffer ), e->u.query->aggregate );
//            strcat( buffer, " | " );
//            EXPRstring_swift( SELF, buffer + strlen( buffer ), e->u.query->expression );
//            strcat( buffer, " )" );
//            break;
//        case self_:
//            strcpy( buffer, "SELF" );
//            break;
//        case funcall_:
//            sprintf( buffer, "%s( ", e->symbol.name );
//            i = 0;
//            LISTdo( e->u.funcall.list, arg, Expression )
//            i++;
//            if( i != 1 ) {
//                strcat( buffer, ", " );
//            }
//            EXPRstring_swift( SELF, buffer + strlen( buffer ), arg );
//            LISTod
//            strcat( buffer, " )" );
//            break;
//
//        case op_:
//            EXPRop_string_swift( SELF, buffer, &e->e );
//            break;
//        case aggregate_:
//            strcpy( buffer, "[" );
//            i = 0;
//            LISTdo( e->u.list, arg, Expression ) {
//                bool repeat = arg->type->u.type->body->flags.repeat;
//                /* if repeat is true, the previous Expression repeats and this one is the count */
//                i++;
//                if( i != 1 ) {
//                    if( repeat ) {
//                        strcat( buffer, " : " );
//                    } else {
//                        strcat( buffer, ", " );
//                    }
//                }
//                EXPRstring_swift( SELF, buffer + strlen( buffer ), arg );
//            } LISTod
//            strcat( buffer, "]" );
//            break;
//        case oneof_:
//            strcpy( buffer, "ONEOF ( " );
//
//            i = 0;
//            LISTdo( e->u.list, arg, Expression ) {
//                i++;
//                if( i != 1 ) {
//                    strcat( buffer, ", " );
//                }
//                EXPRstring_swift( SELF, buffer + strlen( buffer ), arg );
//            } LISTod
//
//            strcat( buffer, " )" );
//            break;
//        default:
//            sprintf( buffer, "EXPRstring: unknown expression, type %d", TYPEis( e->type ) );
//            fprintf( stderr, "%s", buffer );
//    }
//}
//
//void EXPRop_string_swift( Scope SELF, char * buffer, struct Op_Subexpression * oe ) {
//    EXPRstring_swift( SELF, buffer, oe->op1 );
//    switch( oe->op_code ) {
//        case OP_DOT:
//            strcat( buffer, "." );
//            break;
//        case OP_GROUP:
//            strcat( buffer, "\\" );
//            break;
//        default:
//            strcat( buffer, "(* unknown op-expression *)" );
//    }
//    EXPRstring_swift( SELF, buffer + strlen( buffer ), oe->op2 );
//}
