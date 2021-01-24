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
#include "swift_type_alias.h"
#include "swift_named_simple_type.h"
#include "swift_named_aggregate_type.h"

// returns length of qualification string
int accumulate_qualification(Scope target, Scope current, char buf[BUFSIZ]) {
	int qual_len = 0;
	if( target == NULL ) return qual_len;
	if( current == NO_QUALIFICATION ) return qual_len;
	if( target == current ) return qual_len;
	if( target->superscope == NULL ) return qual_len;	// target is not enclosed by current
	
	// check if target is enclosed by current
	if( target->superscope == current ) {
		int prnlen;
		char cbuf[BUFSIZ];
		snprintf(&buf[qual_len], BUFSIZ-qual_len, "%s.%n",canonical_swiftName(target->symbol.name,cbuf), &prnlen);
		qual_len += prnlen;
	}
	else {
		qual_len = accumulate_qualification(target->superscope, current, buf);
		if( qual_len > 0 ) {
			int prnlen;
			char cbuf[BUFSIZ];
			snprintf(&buf[qual_len], BUFSIZ-qual_len, "%s.%n",canonical_swiftName(target->symbol.name,cbuf), &prnlen);
			qual_len += prnlen;
		}
	}
	return qual_len;
}

const char* TYPE_swiftName( Type t, Scope current, char buf[BUFSIZ] ) {
	int qual_len = accumulate_qualification(t->superscope, current, buf);
	
	const char* prefix = "";
//	if( TYPEget_head(t) ){
//		prefix = "r";
//	}
//	else 
	if( TYPEget_body(t) == NULL ){
		prefix = "g";
	}
	else{
		switch (TYPEis(t)) {
			case entity_:
				prefix = "e";
				break;
			case select_:
				prefix = "s";
				break;
			case enumeration_:
				prefix = "n";
				break;
			case generic_:
			case aggregate_:
				prefix = "g";
				break;
				
			default:
				prefix = "t";
				break;
		}
	}
	
	char buf2[BUFSIZ];
	snprintf(&buf[qual_len], BUFSIZ-qual_len, "%s%s", prefix,canonical_swiftName(t->symbol.name, buf2));
	return buf;
}

char TYPE_swiftNameInitial( Type t ) {
	return toupper( t->symbol.name[0] );
}

const char* enumCase_swiftName( Expression expr, char buf[BUFSIZ] ) {
	return canonical_swiftName(expr->symbol.name, buf);
}

const char* selectCase_swiftName( Type selection, char buf[BUFSIZ] ) {
	buf[0] = '_';
	canonical_swiftName_n(selection->symbol.name, buf+1, BUFSIZ-2);
	return buf;
}

//MARK: - type definitions

//static void typeArias_swift( Type type, int level, SwiftOutCommentOption in_comment ) {
//	indent_swift(level);
//	{
//		char buf[BUFSIZ];
//		raw( "public typealias %s = ", TYPE_swiftName(type, type->superscope, buf) );
//	}
//	TYPE_body_swift(type->superscope, type, in_comment);
//	raw("\n");
//}


//MARK: - type definition entry point

void TYPEdefinition_swift(Schema schema, Type t, int level ) {
	beginExpress_swift("TYPE DEFINITION");
	TYPE_out(t, level);
	endExpress_swift();
	
	// swift code generation
	assert(t->symbol.name);
	if( TYPEget_head( t ) ) {
		typeAliasDefinition_swift( schema, t, TYPEget_head(t), level );
	} 
	else {
		TypeBody tb = TYPEget_body( t );
		switch( tb->type ) {
			case enumeration_: 
				enumTypeDefinition_swift(schema, t, level);
				break;
				
			case select_:
				selectTypeDefinition_swift(schema, t, level);
				break;
				
			case array_:
			case bag_:
			case set_:
			case list_:
				namedAggregateTypeDefinition_swift(schema, t, level);
				break;
				
			default:
				namedSimpleTypeDefinition_swift(schema, t, level);
				break;
		}
	}
	
	if( t->where && !LISTis_empty(t->where) ) {
		raw("/*");
		WHERE_out( t->where, level );
		raw("*/\n");
	}
}

extern void TYPEextension_swift( Schema schema, Type t, int level ) {
	if( TYPEget_head( t ) ) {
		typeAliasExtension_swift( schema, t, TYPEget_head(t), level );
	}
	else {
		TypeBody tb = TYPEget_body( t );
		switch( tb->type ) {
			case enumeration_: 
				enumTypeExtension_swift(schema, t, level);
				break;
				
			case select_:
				selectTypeExtension_swift(schema, t, level);
				break;
				
			case array_:
			case bag_:
			case set_:
			case list_:
				namedAggregateTypeExtension_swift(schema, t, level);
				break;
				
			default:
				namedSimpleTypeExtension_swift(schema, t, level);
				break;
		}
	}
}


//MARK: - type references

void TYPE_head_swift( Scope current, Type t, SwiftOutCommentOption in_comment ) {
	if( t == NULL ){
		switch (in_comment) {
			case YES_IN_COMMENT:
				wrap("NULLTYPE");
				break;
			case NOT_IN_COMMENT:
				wrap("/*NULLTYPE*/");
				break;
			case WO_COMMENT:
				break;
		}
		return;
	}

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

const char* builtinTYPE_body_swiftname( Type t ) {
	TypeBody tb = TYPEget_body( t );
	
	switch( tb->type ) {
		case indeterminate_: return( "INDETERMINATE" );
		case integer_:	return( "INTEGER" );
		case real_:			return( "REAL" );
		case string_:		return( "STRING" );
		case binary_:		return( "BINARY" );
		case boolean_:	return( "BOOLEAN" );
		case logical_:	return( "LOGICAL" );
		case number_:		return( "NUMBER" );
			
		case bag_:			return( "BAG" );
		case set_:			return( "SET" );
		case array_:
			if( tb->flags.optional ){
				if( tb->flags.unique ) return( "ARRAY_OPTIONAL_UNIQUE" );
				return( "ARRAY_OPTIONAL" );
			} 
			if( tb->flags.unique ) return( "ARRAY_UNIQUE" );
			return( "ARRAY" );
		case list_:
			if( tb->flags.unique ) return("LIST_UNIQUE");
			return( "LIST" );
			
		default:
			break;
	}
	return( "<NON_BUILTIN_TYPE>" );
}

void TYPE_body_swift( Scope current, Type t, SwiftOutCommentOption in_comment ) {
//	bool first_time = true;
	
//	Expression expr;
//	DictionaryEntry de;
	
	TypeBody tb = TYPEget_body( t );
	
	switch( tb->type ) {
	//MARK: SIMPLE TYPES
		case indeterminate_:
			wrap("/*INDETERMINATE*/");
			break;
			
		case integer_:
		case real_:
		case string_:
		case binary_:
		case boolean_:
		case logical_:
		case number_:
			wrap( "SDAI." );
			raw( builtinTYPE_body_swiftname(t) );
			break;
			
	//MARK:entity_
		case entity_:
		{
			if( t == Type_Entity ){
				wrap( "SDAI.GENERIC_ENTITY" );
			}
			else {
				char buf[BUFSIZ];
				wrap( "%s", ENTITY_swiftName(tb->entity, current, buf) );
			}
		}
			break;
			
	//MARK:generic_
		case generic_:
		{
			if( tb->tag == NULL ){
				wrap("SDAI.GENERIC");
			}
			else {
				char buf[BUFSIZ];
				wrap( "%s", TYPE_swiftName(tb->tag,NO_QUALIFICATION,buf) );
			}
		}
			break;
			
	//MARK:aggregate_
		case aggregate_:
		{
			if( tb->tag == NULL ){
				wrap("SDAI.AGGREGATE");
				if( tb->base != NULL ){
					raw( "<" );
					TYPE_head_swift( current, tb->base, in_comment );
					raw(">");
				}
				else {
					raw("<SDAI.GENERIC>");
				}
			}
			else {
				char buf[BUFSIZ];
				wrap( "%s", TYPE_swiftName(tb->tag,NO_QUALIFICATION,buf) );
			}
		}
			break;

	//MARK: array_,bag_,set_,list_
		case array_:
		case bag_:
		case set_:
		case list_:
			wrap("SDAI.");
			raw( builtinTYPE_body_swiftname(t) );			
			raw( "<" );
			TYPE_head_swift( current, tb->base, in_comment );
			raw(">");
			EXPRbounds_swift( NULL, tb, in_comment );
			break;
			
		case runtime_:
			switch (in_comment) {
				case YES_IN_COMMENT:
					wrap( "runtime" );
					break;
					
				case NOT_IN_COMMENT:
				case WO_COMMENT:
					wrap( " /*runtime*/" );
					break;
			}
			break;
			
		default:
			switch (in_comment) {
				case YES_IN_COMMENT:
					wrap( "unknown type %d", tb->type );
					break;
					
				case NOT_IN_COMMENT:
				case WO_COMMENT:
					wrap( " /* unknown type %d */", tb->type );
					break;
			}
	}
	
	if( in_comment != WO_COMMENT ){
		if( tb->precision ) {
			wrap( "%s( ", in_comment==YES_IN_COMMENT ? "" : "/* " );
			EXPR_out( tb->precision, 0 );
			raw( " )%s", in_comment==YES_IN_COMMENT ? "" : " */" );
		}
		if( tb->flags.fixed ) {
			if( in_comment==YES_IN_COMMENT ) {
				wrap( "FIXED" );
			}
			else {
				wrap( "/* FIXED */" );
			}
		}
	}
}

const char* TYPEhead_string_swift( Scope current, Type t, SwiftOutCommentOption in_comment, char buf[BUFSIZ]) {
	if( prep_buffer(buf, BUFSIZ) ) {
		abort();
	}
	TYPE_head_swift(current, t, in_comment);
	finish_buffer();
	return ( buf );
}
