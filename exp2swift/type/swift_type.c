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

const char* TYPE_swiftName( Type t ) {
	return t->symbol.name;
}

char TYPE_swiftNameInitial( Type t ) {
	return toupper( TYPE_swiftName(t)[0] );
}

const char* enumCase_swiftName( Expression expr ) {
	return expr->symbol.name;
}

const char* selectCase_swiftName( Type selection ) {
	return selection->symbol.name;
}

static void simpleTypeAlias_swift( Type type, Type original, int level) {
	indent_swift(level);
	raw( "public typealias %s = ", TYPE_swiftName(type) );
	wrap( " %s", TYPE_swiftName(original) );
	raw("\n");
}

static void typeArias_swift( Type type, int level ) {
	indent_swift(level);
	raw( "public typealias %s = ", TYPE_swiftName(type) );
	TYPE_body_swift(type, level + nestingIndent_swift);
	raw("\n");
}

static void enumTypeDefinition_swift(Type type, int level) {
	DictionaryEntry dictEntry;
	DICTdo_type_init( type->symbol_table, &dictEntry, OBJ_EXPRESSION );
	int count = 0;
	while( 0 != ( ( Expression )DICTdo( &dictEntry ) ) ) {
		count++;
	}
	
	indent_swift(level);
	wrap( "public enum %s : SDAI.ENUMERATION, SDAIEnumerationType {\n", TYPE_swiftName(type) );
	
	if(count>0) {
		Expression * enumCases = ( Expression * )sc_malloc( (count) * sizeof( Expression ) );
		DICTdo_type_init( type->symbol_table, &dictEntry, OBJ_EXPRESSION );
		Expression expr;
		while( 0 != ( expr = ( Expression )DICTdo( &dictEntry ) ) ) {
			enumCases[expr->u.integer - 1] = expr;
		}
		
		
		for( int i = 0; i < count; i++ ) {
			indent_swift(level + nestingIndent_swift);
			raw( "case %s\n", enumCase_swiftName( enumCases[i] ) );
		}
		
		sc_free( ( char * )enumCases );
	}
	
	indent_swift(level);
	raw( "}\n" );
}

static void selectTypeDefinition_swift(Type type, TypeBody typeBody, int level) {
	indent_swift(level);
	wrap( "public enum %s : SDAI.SELECT, SDAISelectType {\n", TYPE_swiftName(type) );

	LISTdo( typeBody->list, selection, Type )
	indent_swift(level + nestingIndent_swift);
		raw( "case %s(%s)\n", selectCase_swiftName(selection), TYPE_swiftName(selection) );
	LISTod

	indent_swift(level);
	raw( "}\n" );
}


void TYPE_swift( Type t, int level ) {
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
				selectTypeDefinition_swift(t, tb, level);
				break;
				
			default:
				typeArias_swift(t, level);
		}
	}
	
	raw("/*");
	WHERE_out( t->where, level );
	raw("*/\n");
}



/** prints type description (preceded by a space).
 * I.e., the type of an attribute or other object
 */
void TYPE_head_swift( Type t, int level ) {
    if( t->symbol.name ) {
        int old_indent = indent2;
        if( indent2 + ( int ) strlen( t->symbol.name ) > exppp_linelength ) {
            indent2 = ( indent2 + level ) / 2;
        }
        wrap( "%s", TYPE_swiftName(t) );
        indent2 = old_indent;
    } else {
        TYPE_body_swift( t, level );
    }
}

void TYPEunique_swift( TypeBody tb ) {
    if( tb->flags.unique ) {
        wrap( "/*UNIQUE*/" );
    }
}

void TYPEoptional_swift( TypeBody tb ) {
    if( tb->flags.optional ) {
        wrap( "?" );
    }
}


void TYPE_body_swift( Type t, int level ) {
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
			wrap( "%s", ENTITY_swiftName(tb->entity) );
			break;
			
			//MARK:generic_
		case generic_:
			assert(tb->tag);
			wrap( "%s", TYPE_swiftName(tb->tag) );
			break;
			
			//MARK:aggregate_
		case aggregate_:
			assert(tb->tag);
			wrap( "%s", TYPE_swiftName(tb->tag) );
			TYPEoptional_swift(tb);
			EXPRbounds_swift( NULL, tb );
			TYPEunique_swift(tb);
			break;

			//MARK:array_,bag_,set_,list_
		case array_:
		case bag_:
		case set_:
		case list_:
			switch( tb->type ) {
				case array_:
					wrap( "SDAI.ARRAY" );
					break;
					
				case bag_:
					wrap( "SDAI.BAG" );
					break;
					
				case set_:
					wrap( "SDAI.SET" );
					break;
					
				case list_:
					wrap( "SDAI.LIST" );
					break;
					
				default:
					fprintf( stderr, "exppp: Reached default case, %s:%d", __FILE__, __LINE__ );
					abort();
			}
			
			raw( "<" );
			TYPE_head_swift( tb->base, level );
			raw(">");
			TYPEoptional_swift(tb);
			EXPRbounds_swift( NULL, tb );
			TYPEunique_swift(tb);
			break;
			
			//MARK:enumeration_
		case enumeration_: {
			int i, count = 0;
			Expression * enumCases;
			
			/*
			 * write names out in original order by first bucket sorting
			 * to a temporary array.  This is trivial since all buckets
			 * will get filled with one and only one object.
			 */
			DICTdo_type_init( t->symbol_table, &de, OBJ_EXPRESSION );
			while( 0 != ( ( Expression )DICTdo( &de ) ) ) {
				count++;
			}
			if(count>0){
				enumCases = ( Expression * )sc_malloc( (count) * sizeof( Expression ) );
				DICTdo_type_init( t->symbol_table, &de, OBJ_EXPRESSION );
				while( 0 != ( expr = ( Expression )DICTdo( &de ) ) ) {
					enumCases[expr->u.integer - 1] = expr;
				}
				
				wrap( "/* ENUMERATION OF\n" );
				
				for( i = 0; i < count; i++ ) {
					/* finish line from previous enum item */
					if( !first_time ) {
						raw( ",\n" );
					}
					
					/* start new enum item */
					if( first_time ) {
						raw( "%*s( ", level, "" );
						first_time = false;
					} else {
						raw( "%*s ", level, "" );
					}
					raw( enumCase_swiftName( enumCases[i] ) );
				}
				raw( " ) */" );
				sc_free( ( char * )enumCases );
			}
		}
			break;
			
			//MARK:select_
		case select_:
			wrap( "/* SELECT\n" );
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
			raw( selectCase_swiftName(selection) );
			LISTod
			
			/* if empty, force a left paren */
			if( first_time ) {
				ERRORreport_with_symbol( ERROR_select_empty, &error_sym, t->symbol.name );
				raw( "%*s( ", level, "" );
			}
			raw( " ) */" );
			break;
			
		default:
			wrap( " /* unknown type %d */", tb->type );
	}
	
	if( tb->precision ) {
		wrap( "/* ( " );
		EXPR_out( tb->precision, 0 );
		raw( " ) */" );
	}
	if( tb->flags.fixed ) {
		wrap( "/* FIXED */" );
	}
}

const char* TYPEhead_string_swift( Type t, char buf[BUFSIZ]) {
	if( prep_buffer(buf, BUFSIZ) ) {
		abort();
	}
	TYPE_head_swift(t, 0);
	finish_buffer();
	return ( buf );
}
