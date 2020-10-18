//
//  swift_type.c
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//


#include <stdlib.h>
#include <assert.h>

#include <express/error.h>
#include <sc_memmgr.h>

#include "exppp.h"
#include "pp.h"
#include "pretty_where.h"
#include "pretty_expr.h"
#include "pretty_type.h"

#include "swift_type.h"
#include "swift_entity.h"
#include "swift_expression.h"
#include "swift_files.h"
#include "swift_symbol.h"
#include "swift_select_type.h"
#include "swift_enum_type.h"

// returns length of qualification string
int accumulate_qualification(Scope target, Scope current, char buf[BUFSIZ]) {
	int qual_len = 0;
	if( current == NO_QUALIFICATION ) return qual_len;
	if( target == current ) return qual_len;
	if( target->superscope == NULL ) return qual_len;	// target is not enclosed by current
	
	// check if target is enclosed by current
	if( target->superscope == current ) {
		int prnlen;
		snprintf(&buf[qual_len], BUFSIZ-qual_len, "%s.%n",target->symbol.name, &prnlen);
		qual_len += prnlen;
	}
	else {
		qual_len = accumulate_qualification(target->superscope, current, buf);
		if( qual_len > 0 ) {
			int prnlen;
			snprintf(&buf[qual_len], BUFSIZ-qual_len, "%s.%n",target->symbol.name, &prnlen);
			qual_len += prnlen;
		}
	}
	return qual_len;
}

const char* TYPE_swiftName( Type t, Scope current, char buf[BUFSIZ] ) {
	int qual_len = accumulate_qualification(t->superscope, current, buf);
	char buf2[BUFSIZ];
	snprintf(&buf[qual_len], BUFSIZ-qual_len, "%s", canonical_swiftName(t->symbol.name, buf2));
	return buf;
}

char TYPE_swiftNameInitial( Type t ) {
	return toupper( t->symbol.name[0] );
}

const char* enumCase_swiftName( Expression expr, char buf[BUFSIZ] ) {
	return canonical_swiftName(expr->symbol.name, buf);
}

const char* selectCase_swiftName( Type selection, char buf[BUFSIZ] ) {
	return canonical_swiftName(selection->symbol.name, buf);
}

//MARK: - type definitions
static void simpleTypeAlias_swift( Type type, Type original, int level) {
	char buf[BUFSIZ];
	indent_swift(level);
	raw( "public typealias %s = ", TYPE_swiftName(type,type->superscope,buf) );
	wrap( " %s", TYPE_swiftName(original, type->superscope, buf) );
	raw("\n");
}

static void typeArias_swift( Type type, int level, bool in_comment ) {
	indent_swift(level);
	{
		char buf[BUFSIZ];
		raw( "public typealias %s = ", TYPE_swiftName(type, type->superscope, buf) );
	}
	TYPE_body_swift(type->superscope, type, in_comment);
	raw("\n");
}


//MARK: - type definition entry point

void TYPEdefinition_swift( Type t, int level ) {
	beginExpress_swift("TYPE DEFINITION");
	TYPE_out(t, level);
	endExpress_swift();
	
	// swift code generation
	assert(t->symbol.name);
	if( TYPEget_head( t ) ) {
		simpleTypeAlias_swift( t, TYPEget_head(t), level );
	} 
	else {
		TypeBody tb = TYPEget_body( t );
		switch( tb->type ) {
			case enumeration_: 
				enumTypeDefinition_swift(t, level);
				break;
				
			case select_:
				selectTypeDefinition_swift(t, level);
				break;
				
			default:
				typeArias_swift(t, level, NOT_IN_COMMENT);
		}
	}
	
	if( t->where && !LISTis_empty(t->where) ) {
		raw("/*");
		WHERE_out( t->where, level );
		raw("*/\n");
	}
}

extern void TYPEextension_swift( Schema schema, Type t, int level ) {
	if( TYPEget_head( t ) ) return;
	
		TypeBody tb = TYPEget_body( t );
		switch( tb->type ) {
			case enumeration_: 
				enumTypeExtension_swift(schema, t, level);
				break;
				
			case select_:
				selectTypeExtension_swift(schema, t, level);
				break;
				
			default:
				break;
		}

}


//MARK: - type references

void TYPE_head_swift( Scope current, Type t, bool in_comment ) {
	if( t->symbol.name ) {
		char buf[BUFSIZ];
		wrap( "%s", TYPE_swiftName(t,current,buf) );
	} else {
		TYPE_body_swift( current, t, in_comment );
	}
}

//static void TYPEunique_swift( TypeBody tb, bool in_comment ) {
//    if( tb->flags.unique ) {
//			if( in_comment ) {
//				wrap( "UNIQUE" );
//			}
//			else {
//				wrap( "/*UNIQUE*/" );
//			}
//    }
//}

//static void TYPEoptional_swift( TypeBody tb ) {
//    if( tb->flags.optional ) {
//        wrap( "? " );
//    }
//}


void TYPE_body_swift( Scope current, Type t, bool in_comment ) {
	bool first_time = true;
	
	Expression expr;
	DictionaryEntry de;
	
	TypeBody tb = TYPEget_body( t );
	
	switch( tb->type ) {
			//MARK:integer_
		case integer_:
			wrap( "SDAI.INTEGER" );
			break;
			
			//MARK:real_
		case real_:
			wrap( "SDAI.REAL" );
			break;
			
			//MARK:string_
		case string_:
			wrap( "SDAI.STRING" );
			break;
			
			//MARK:binary_
		case binary_:
			wrap( "SDAI.BINARY" );
			break;
			
			//MARK:boolean_
		case boolean_:
			wrap( "SDAI.BOOLEAN" );
			break;
			
			//MARK:logical_
		case logical_:
			wrap( "SDAI.LOGICAL" );
			break;
			
			//MARK:number_
		case number_:
			wrap( "SDAI.NUMBER" );
			break;
			
			//MARK:entity_
		case entity_:
		{
			char buf[BUFSIZ];
			wrap( "%s", ENTITY_swiftName(tb->entity,"","",current,buf) );
		}
			break;
			
			//MARK:generic_
		case generic_:
		{
			char buf[BUFSIZ];
			assert(tb->tag);
			wrap( "%s", TYPE_swiftName(tb->tag,NO_QUALIFICATION,buf) );
		}
			break;
			
			//MARK:aggregate_
		case aggregate_:
		{
			char buf[BUFSIZ];
			assert(tb->tag);
			wrap( "%s", TYPE_swiftName(tb->tag,NO_QUALIFICATION,buf) );
//			TYPEoptional_swift(tb);
//			EXPRbounds_swift( NULL, tb, in_comment );
//			TYPEunique_swift(tb, in_comment);
		}
			break;

			//MARK:array_,bag_,set_,list_
		case array_:
		case bag_:
		case set_:
		case list_:
			switch( tb->type ) {
				case array_:
					if( tb->flags.unique ){
						wrap( "SDAI.UNIQUE_ARRAY" );
					}
					else {
						wrap( "SDAI.ARRAY" );
					}
					break;
					
				case bag_:
					wrap( "SDAI.BAG" );
					break;
					
				case set_:
					wrap( "SDAI.SET" );
					break;
					
				case list_:
					if( tb->flags.unique ){
						wrap( "SDAI.UNIQUE_LIST" );
					}
					else {
						wrap( "SDAI.LIST" );
					}
					break;
					
				default:
					fprintf( stderr, "exppp: Reached default case, %s:%d", __FILE__, __LINE__ );
					abort();
			}
			
			raw( "<" );
			if( tb->flags.optional ) raw("(");
			TYPE_head_swift( current, tb->base, in_comment );
			if( tb->flags.optional ) raw(")?");
			raw(">");
//			TYPEoptional_swift(tb);
			EXPRbounds_swift( NULL, tb, in_comment );
//			TYPEunique_swift(tb, in_comment);
			break;
			
			//MARK:enumeration_
		case enumeration_: {
			int i, count = 0;
			
			/*
			 * write names out in original order by first bucket sorting
			 * to a temporary array.  This is trivial since all buckets
			 * will get filled with one and only one object.
			 */
			DICTdo_type_init( t->symbol_table, &de, OBJ_EXPRESSION );
			while( 0 != DICTdo(&de) ) {
				count++;
			}
			if(count>0){
				Expression * enumCases = ( Expression * )sc_calloc( count, sizeof( Expression ) );
				DICTdo_type_init( t->symbol_table, &de, OBJ_EXPRESSION );
				while( 0 != ( expr = ( Expression )DICTdo( &de ) ) ) {
					assert(expr->u.integer >= 1);
					assert(expr->u.integer <= count);
					assert(enumCases[expr->u.integer - 1] == NULL);
					enumCases[expr->u.integer - 1] = expr;
				}
				
				wrap( "%sENUMERATION OF\n", in_comment ? "" : "/* " );
				
				for( i = 0; i < count; i++ ) {
					/* finish line from previous enum item */
					if( !first_time ) {
						raw( ",\n" );
					}
					
					/* start new enum item */
					if( first_time ) {
//						raw( "%*s( ", level, "" );
						first_time = false;
					} else {
//						raw( "%*s ", level, "" );
					}
					assert(enumCases[i] != NULL);
					char buf[BUFSIZ];
					raw( enumCase_swiftName( enumCases[i], buf ) );
				}
				raw( " )%s", in_comment ? "" : " */" );
				sc_free( ( char * )enumCases );
			}
		}
			break;
			
			//MARK:select_
		case select_:
		{
			int level = 0;
			wrap( "%sSELECT\n", in_comment ? "" : "/* " );
			LISTdo( tb->list, selection, Type )
			/* finish line from previous entity */
			if( !first_time ) {
				raw( ",\n" );
			}
			
			/* start new entity */
			if( first_time ) {
				raw( "%*s( ", level, "" );
				first_time = false;
			} else {
				raw( "%*s ", level, "" );
			}
			char buf[BUFSIZ];
			raw( selectCase_swiftName(selection,buf) );
			LISTod
			
			/* if empty, force a left paren */
			if( first_time ) {
				ERRORreport_with_symbol( ERROR_select_empty, &error_sym, t->symbol.name );
				raw( "%*s( ", level, "" );
			}
			raw( " )%s", in_comment ? "" : " */" );
		}
			break;
			
		default:
			if( in_comment ) {
				wrap( "unknown type %d", tb->type );
			}
			else {
				wrap( " /* unknown type %d */", tb->type );
			}
	}
	
	if( tb->precision ) {
		wrap( "%s( ", in_comment ? "" : "/* " );
		EXPR_out( tb->precision, 0 );
		raw( " )%s", in_comment ? "" : " */" );
	}
	if( tb->flags.fixed ) {
		if( in_comment ) {
			wrap( "FIXED" );
		}
		else {
			wrap( "/* FIXED */" );
		}
	}
}

const char* TYPEhead_string_swift( Scope current, Type t, bool in_comment, char buf[BUFSIZ]) {
	if( prep_buffer(buf, BUFSIZ) ) {
		abort();
	}
	TYPE_head_swift(current, t, in_comment);
	finish_buffer();
	return ( buf );
}
