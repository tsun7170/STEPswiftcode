//
//  swift_type.c
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//


#include <stdlib.h>
#include <assert.h>

#include <express/error.h>
#include <sc_memmgr.h>
#include "symbol.h"

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
#include "decompose_expression.h"

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

const char* TYPE_canonicalName( Type t, Scope current, char buf[BUFSIZ] ) {
	int qual_len = accumulate_qualification(t->superscope, current, buf);
	
	assert(t->symbol.name != NULL);
	char buf2[BUFSIZ];
	snprintf(&buf[qual_len], BUFSIZ-qual_len, "%s" ,canonical_swiftName(t->symbol.name, buf2));
	return buf;
}

const char* TYPE_swiftName( Type t, Scope current, char buf[BUFSIZ] ) {
	assert(TYPEis_valid(t));
	int qual_len = accumulate_qualification(t->superscope, current, buf);
	
	const char* prefix = "";
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
	
	assert(t->symbol.name != NULL);
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

void TYPE_head_swift( Scope current, Type t, SwiftOutCommentOption in_comment, bool leaf_unowned ) {
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
		if( leaf_unowned ){
			wrap( "SDAI.UnownedWrap<%s>", TYPE_swiftName(t,current,buf) );
		}
		else {
			wrap( "%s", TYPE_swiftName(t,current,buf) );
		}
	} else {
		TYPE_body_swift( current, t, in_comment, leaf_unowned );
	}
}

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

void TYPE_body_swift( Scope current, Type t, SwiftOutCommentOption in_comment, bool leaf_unowned ) {
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
			if( leaf_unowned ) wrap("SDAI.UnownedWrap<");
			if( t == Type_Entity ){
				wrap( "SDAI.GENERIC_ENTITY" );
			}
			else {
				char buf[BUFSIZ];
				wrap( "%s", ENTITY_swiftName(tb->entity, current, buf) );
			}
			if( leaf_unowned ) wrap(">");
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
//			wrap("SDAIGenericType");
		}
			break;
			
	//MARK:aggregate_
		case aggregate_:
		{
			if( tb->tag == NULL ){
				wrap("SDAI.AGGREGATE");
				if( tb->base != NULL ){
					raw( "<" );
					TYPE_head_swift( current, tb->base, in_comment, leaf_unowned );
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
			TYPE_head_swift( current, tb->base, in_comment, leaf_unowned );
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
					wrap( "/*runtime*/SDAI.GENERIC" );
					break;
			}
			break;
			
		default:
			switch (in_comment) {
				case YES_IN_COMMENT:
				case WO_COMMENT:
					wrap( "unknown type %d", tb->type );
					break;
					
				case NOT_IN_COMMENT:
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

const char* TYPEhead_string_swift( Scope current, Type t, SwiftOutCommentOption in_comment, bool leaf_unowned, char buf[BUFSIZ]) {
	if( prep_buffer(buf, BUFSIZ) ) {
		abort();
	}
	TYPE_head_swift(current, t, in_comment, leaf_unowned);
	finish_buffer();
	return ( buf );
}

bool TYPEis_swiftAssignable(Type lhstype, Type rhstype) {
	if( lhstype == rhstype ) return (!TYPEis_partial_entity(rhstype));
	if( (lhstype == NULL)||(rhstype == NULL) )return false;
	if( lhstype == Type_Entity && TYPEis_entity(rhstype) ) return true;

	TypeBody lhstb = TYPEget_body(lhstype);
	TypeBody rhstb = TYPEget_body(rhstype);
	
	bool equal_symbol = names_are_equal( lhstype->symbol.name, rhstype->symbol.name );
	
	if( !equal_symbol ) return false;
	
	if( lhstb==NULL && rhstb==NULL ){
		return true;
	}
	if( lhstb==NULL || rhstb==NULL ){
		return false;
	}
	
	switch (lhstb->type) {
		case indeterminate_:
			break;
			
		case integer_:
		case real_:
		case string_:
		case binary_:
		case boolean_:
			if(lhstb->type != rhstb->type)return false;
			return  equal_symbol;

		case number_:
			if(!equal_symbol) return false;
			switch( rhstb->type) {
				case number_:
					return true;
				default:
					return false;
			}

		case logical_:
			if(!equal_symbol) return false;
			switch( rhstb->type) {
				case logical_:
					return true;
				default:
					return false;
			}
			
		case entity_:
			return (lhstb->entity == rhstb->entity) && (rhstb->flags.partial == false);
			
		case generic_:
		case aggregate_:
			return (lhstb->type == rhstb->type) && TYPEs_are_equal(lhstb->tag, rhstb->tag);

		case array_:
		case bag_:
		case set_:
		case list_:
			if( lhstb->type != rhstb->type )return false;
			if( lhstb->flags.unique != rhstb->flags.unique )return false;
			if( lhstb->flags.optional != rhstb->flags.optional )return false;
			if( !EXPs_are_equal(lhstb->upper, rhstb->upper) )return false;
			if( !EXPs_are_equal(lhstb->lower, rhstb->lower) )return false;
			return TYPEs_are_equal(lhstb->base, rhstb->base);
			
		case enumeration_: 
			break;
		case select_:
			break;
		default:
			break;
	}
	return false;
}

bool TYPEis_observable_aggregate(Type t) {
	if( !TYPEis_aggregation_data_type(t) ) return false;
	Type elementType = TYPEget_nonaggregate_base_type(t);
	switch (TYPEis(elementType)) {
		case entity_:
		case select_:
			return true;
			
		default:
			return false;
	}
}


//MARK: - where rule
const char* whereRuleLabel_swiftName( Where w, char buf[BUFSIZ] ) {
	if( w-> label ) {
		snprintf(buf, BUFSIZ, "WHERE_%s", w->label->name );
	}
	else {
		snprintf(buf, BUFSIZ, "WHERE_%d", w->serial );
	}
	return buf;
}

void TYPEwhereDefinitions_swift( Scope scope, int level ) {
	Linked_List where_rules = TYPEget_where(scope);
	if( LISTis_empty(where_rules) ) return;
	bool is_entity = scope->u_tag == scope_is_entity;
	
	raw("\n");
	indent_swift(level);
	if( is_entity ){
		raw("//MARK: WHERE RULES (ENTITY)\n");
	}
	else {
		raw("//MARK: WHERE RULES (DEFINED TYPE)\n");
	}

	char whereLabel[BUFSIZ];
	char buf[BUFSIZ];
	LISTdo( where_rules, where, Where ){
		indent_swift(level);
		wrap("public static func %s(SELF: %s?) -> SDAI.LOGICAL {\n", 
				 whereRuleLabel_swiftName(where, whereLabel), 
				 is_entity ? 	ENTITY_swiftName(scope, NO_QUALIFICATION, buf) : 
				 							TYPE_swiftName(scope, NO_QUALIFICATION, buf) );
		
		{	int level2 = level+nestingIndent_swift;
			
			indent_swift(level2);
			raw("guard let SELF = SELF else { return SDAI.UNKNOWN }\n");
			
			int tempvar_id = 1;
			Linked_List tempvars;
			Expression simplified = EXPR_decompose(where->expr, Type_Logical, &tempvar_id, &tempvars);
			EXPR_tempvars_swift(scope, tempvars, level2);
			
			indent_swift(level2);
			raw("return ");
			if( EXPRresult_is_optional(simplified, CHECK_DEEP) != no_optional ){
				TYPE_head_swift(scope, Type_Logical, WO_COMMENT, LEAF_OWNED);	// wrap with explicit type cast
				raw("(");
				EXPR_swift(scope, simplified, Type_Logical, NO_PAREN);			
				raw(")");
			}
			else {
				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, scope, simplified, Type_Logical, NO_PAREN, OP_UNKNOWN, YES_WRAP);
			}
			raw("\n");
			EXPR_delete_tempvar_definitions(tempvars);
		}
		
		indent_swift(level);
		raw("}\n");
	}LISTod;
//	raw("\n");
}

void TYPEwhereRuleValidation_swift( Type type, int level ) {
	Linked_List where_rules = TYPEget_where(type);
	if( LISTis_empty(where_rules) ) return;

	raw("\n");
	indent_swift(level);
	raw("//WHERE RULE VALIDATION (DEFINED TYPE)\n");

	indent_swift(level);
	raw("public static func validateWhereRules(instance:Self?, prefix:SDAI.WhereLabel) -> [SDAI.WhereLabel:SDAI.LOGICAL] {\n");
	
	{	int level2 = level+nestingIndent_swift;
		
		char typename[BUFSIZ];TYPE_swiftName(type, NO_QUALIFICATION, typename);
		indent_swift(level2);
		raw("let prefix2 = prefix + \"\\\\%s\"\n", typename);
		
		indent_swift(level2);
		raw("var result = Supertype.validateWhereRules(instance:instance?.rep, prefix:prefix2)\n\n");
		
		LISTdo( where_rules, where, Where ){
			char whereLabel[BUFSIZ];
			indent_swift(level2);
			raw("result[prefix2 + \".%s\"] = %s.%s(SELF: instance)\n", 
					whereRuleLabel_swiftName(where, whereLabel), typename, whereLabel);
		}LISTod;

		indent_swift(level2);
		raw("return result\n");		
	}
	
	indent_swift(level);
	raw("}\n\n");
	
}

void insert_select_type_member_tree( Scope scope, Type select_type, const char* referencing, int level ){
	if( SCOPE_search_visited(select_type) )return;
	
	char select_type_name[BUFSIZ];
	TYPEhead_string_swift(scope, select_type, WO_COMMENT, LEAF_OWNED, select_type_name);
	
	indent_swift(level);
	raw("members.insert(SDAI.STRING(%s.typeName)) // -> %s\n", select_type_name, referencing);

	TypeBody typebody = TYPEget_body(select_type);
	if( typebody->referenced_by_select == NULL )return;
	LISTdo(typebody->referenced_by_select, refby, Type){
		insert_select_type_member_tree(scope, refby, select_type_name, level);
	}LISTod;
}

void TYPEinsert_select_type_members_swift( Type type, int level ){
	TypeBody typebody = TYPEget_body(type);
	if( typebody->referenced_by_select == NULL )return;
	indent_swift(level);
	raw("//SELECT data types (indirectly) referencing the current type as a member of the select list\n");
	
	SCOPE_begin_search();
	LISTdo(typebody->referenced_by_select, refby, Type){
		insert_select_type_member_tree(type, refby, "Self", level);
	}LISTod;
	SCOPE_end_search();
}
