//
//  swift_expression.c
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
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
#include "swift_files.h"
#include "decompose_expression.h"
#include "swift_schema.h"

//#define YES_PAD	true
//#define NO_PAD	false
#define OPCODE_FROM_EXPRESS	( char * )0

#define NEED_OPTIONAL_OPERAND	true
#define NO_NEED_OPTIONAL_OPERAND	false


//#define EXPRop2_swift(	SELF1,SELF2,oe,string,paren,pad) \
//				EXPRop2__swift(	SELF1,SELF2,oe,string,paren,pad,		OP_UNKNOWN, YES_WRAP)
//
//#define EXPRop_swift(		SELF,oe,paren) \
//				EXPRop__swift(	SELF,oe,paren,		OP_UNKNOWN, YES_WRAP)

static void EXPR__swift( Scope SELF, Expression e, Type target_type, 
												bool paren, unsigned int previous_op, bool can_wrap );


static void EXPRop__swift( Scope SELF, struct Op_Subexpression * oe, Type target_type,
													bool paren, unsigned int previous_op, bool can_wrap );

static void EXPRop2__swift( Scope SELF1, Scope SELF2, struct Op_Subexpression * eo, char * opcode, 
													 Type op1target_type,Type op2target_type,
													 bool paren, unsigned int previous_op, bool can_wrap, bool need_optional_operand );

static void EXPRop1_swift( Scope SELF, struct Op_Subexpression * eo, char * opcode, Type op1target_type,
													bool paren, bool can_wrap, bool need_optional_operand );


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
	if( !tb->upper ) return;
	if( in_comment == WO_COMMENT ) return;
	
		wrap( "%s[", in_comment==YES_IN_COMMENT ? "" : "/*" );
		EXPR_swift(SELF, tb->lower, Type_Integer, YES_PAREN );
		raw( ":" );
		EXPR_swift(SELF, tb->upper, Type_Integer, YES_PAREN );
		raw( "]%s ", in_comment==YES_IN_COMMENT ? "" : "*/" );
}

//MARK: - special operators
static void EXPRopGroup_swift( Scope SELF, struct Op_Subexpression * oe, bool can_wrap ) {
		char buf[BUFSIZ];
		Entity ent = SCOPEfind(SELF, (char*)canonical_dictName(oe->op2->symbol.name, buf), SCOPE_FIND_ENTITY);
		type_optionality optional = EXPRresult_is_optional(oe->op1, CHECK_SHALLOW);
		
		switch (optional) {
			case no_optional:
				break;
				case yes_optional:
				break;
			case unknown:
				wrap_if(can_wrap, "SDAI.FORCE_OPTIONAL(");
				break;
		}
		EXPR__swift( SELF, oe->op1, oe->op1->return_type, YES_PAREN, oe->op_code, YES_WRAP );
		switch (optional) {
			case no_optional:
				break;
				case yes_optional:
				raw("?");
				break;
			case unknown:
				raw(")?");
				break;
		}

		wrap_if(can_wrap, ".GROUP_REF(");

		if( ent != NULL ){
			wrap_if(can_wrap, ENTITY_swiftName(ent, SELF, buf));
		}
		else {
			// symbol not found; probably error in source
			raw("/*UNRESOLVED*/");
			EXPR__swift( SELF, oe->op2, oe->op2->return_type, NO_PAREN, oe->op_code, NO_WRAP );	// should be an entity reference
		}
		
	raw(".self)");
}

static void EXPRopDot_swift( Scope SELF, struct Op_Subexpression * oe, bool can_wrap ) {
	switch( EXPRresult_is_optional(oe->op1, CHECK_SHALLOW) ){
		case no_optional:
			EXPR__swift( SELF, oe->op1, oe->op1->return_type, YES_PAREN, oe->op_code, can_wrap );
			break;
		case yes_optional:
			EXPR__swift( SELF, oe->op1, oe->op1->return_type, YES_PAREN, oe->op_code, can_wrap );
			raw("?");
			break;
		case unknown:
			raw("SDAI.FORCE_OPTIONAL(");
			EXPR__swift( SELF, oe->op1, oe->op1->return_type, YES_PAREN, oe->op_code, can_wrap );
			raw(")?");			
			break;
	}
	wrap_if(YES_WRAP, ".");
	EXPR__swift( NULL, oe->op2, oe->op2->return_type, YES_PAREN, oe->op_code, NO_WRAP );
}

//MARK: USEDIN()
static void emit_usedin_call( Scope SELF, Expression e, bool can_wrap ){
	wrap_if(can_wrap, "SDAI.USEDIN(" );
	Expression arg1 = LISTget_first(e->u.funcall.list);
	Expression arg2 = LISTget_second(e->u.funcall.list);
	
	wrap("T: ");
	EXPRassignment_rhs_swift(YES_RESOLVING_GENERIC, SELF, arg1, Type_Generic, NO_PAREN, OP_UNKNOWN, YES_WRAP);

	if( TYPEis_string(arg2->type) ){
		if( (arg2 != NULL) && (strlen(arg2->symbol.name) > 0) ){
			raw(", ");
			
			char* attrsep = strrchr(arg2->symbol.name, '.');
			assert( attrsep != NULL );
			int symlen = (int)(attrsep - arg2->symbol.name);
			assert( symlen < BUFSIZ );
			char* attr_name = attrsep+1;
			
			char buf[BUFSIZ];
			canonical_dictName_n(arg2->symbol.name, buf, symlen);
			char* entsep = strrchr(buf, '.');
			assert(entsep != NULL);
			char* entity_name = entsep+1;

			*entsep = 0;
			char* schema_name = buf;

			char buf2[BUFSIZ];
			wrap("ROLE: \\%s", as_schemaSwiftName_n(schema_name, buf2, BUFSIZ));
			wrap(".%s", as_entitySwiftName_n(entity_name, buf2, BUFSIZ));
			wrap(".%s", as_attributeSwiftName_n(attr_name, buf2, BUFSIZ));
		}
	}
	else {
		raw(", ");
		wrap("R: ");
		EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, SELF, arg2, Type_String, NO_PAREN, OP_UNKNOWN, YES_WRAP);
	}
	raw(")");
}

//MARK: NAGATE op
static void EXPRopNEGATE_swift( Scope SELF, struct Op_Subexpression * oe, Type op1target_type, bool paren, bool can_wrap ) {
	if( EXP_is_literal(oe->op1) ){
		wrap_if(can_wrap, " -" );
		EXPR__swift(SELF, oe->op1, oe->op1->return_type, paren, OP_UNKNOWN, NO_WRAP);
	}
	else {
		EXPRop1_swift(SELF, oe, " -", op1target_type, paren, NO_WRAP, NO_NEED_OPTIONAL_OPERAND);
	}
}


static void emit_CONTAINS(Scope SELF, bool can_wrap, struct Op_Subexpression *oe) {
	// op1 IN op2
	Type op1type = oe->op1->return_type;	// lhs: should be basetype of rhs
	Type op2type = oe->op2->return_type;	// rhs: aggregation type
	Type op2basetype = TYPE_retrieve_aggregate_base(op2type, NULL);
	if( op2basetype == NULL || TYPEis_runtime(op2basetype) ){
		op2basetype = op1type;
		op2type = TYPEcreate_aggregate(TYPEis(op2type), op2basetype, TYPEget_body(op2type)->lower, TYPEget_body(op2type)->upper, TYPEget_body(op2type)->flags.unique, TYPEget_body(op2type)->flags.optional);
	}
	
	wrap_if(can_wrap, "SDAI.aggregate(");
	EXPR__swift( SELF, oe->op2, op2type, YES_PAREN, oe->op_code, YES_WRAP );
	raw(", ");
	wrap("contains: ");
	EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, SELF, oe->op1, op2basetype, NO_PAREN, oe->op_code, YES_WRAP);
	raw(")");
}

//MARK: CONTAINS(), TYPEOF()
static void EXPRopIn_swift( Scope SELF, struct Op_Subexpression * oe, bool can_wrap ) {
	if( TYPEis(oe->op1->type)==string_ && TYPEis(oe->op2->type)==funcall_ && oe->op2->u.funcall.function==func_typeof ) {
		// TYPEOF( IS:)
		Expression arg = LISTget_first(oe->op2->u.funcall.list);
		char typename[BUFSIZ]; canonical_swiftName(oe->op1->symbol.name, typename);
		char* sep = NULL;
		bool builtin_type = (sep = strchr(typename, '.')) == NULL;
		
		if( builtin_type ){
			if( names_are_equal(typename, "LIST") || names_are_equal(typename, "ARRAY") || names_are_equal(typename, "BAG") || names_are_equal(typename, "SET") ){
				emit_CONTAINS(SELF, can_wrap, oe);		
				return;
			}
		}
		
		wrap_if(can_wrap, "SDAI.TYPEOF(");
		EXPR_swift( SELF, arg, arg->return_type, NO_PAREN );
		raw(", ");
		
		char buf[BUFSIZ];
		assert(oe->op1->symbol.name != NULL);
		
		if( !builtin_type ){
			Generic x = SCOPEfind(SELF, (char*)canonical_dictName(sep+1, buf), SCOPE_FIND_TYPE | SCOPE_FIND_ENTITY);
			if( x != NULL ){
				if( DICT_type == OBJ_TYPE ){
					wrap_if(YES_WRAP, "IS: %s.self", TYPE_swiftName((Type)x, SELF, buf) );
				}
				else {
					assert(DICT_type == OBJ_ENTITY);
					wrap_if(YES_WRAP, "IS: %s.self", ENTITY_swiftName((Entity)x, SELF, buf) );
				}
				
			}
			else {
				// unknown type/entity; probably error in source
				wrap_if(YES_WRAP, "IS: %s.self", typename);
			}
		}
		else {
			wrap_if(YES_WRAP, "IS: SDAI.%s.self", typename);
		}
		raw(")");
		return;
	}

	// .CONTAINS()
		emit_CONTAINS(SELF, can_wrap, oe);		
}

//MARK: ISLIKE()
static void EXPRopLike_swift( Scope SELF, struct Op_Subexpression * oe, bool can_wrap ) {
	switch (EXPRresult_is_optional(oe->op1, CHECK_DEEP)) {
		case no_optional:
			EXPR__swift( SELF, oe->op1, oe->op1->return_type, YES_PAREN, oe->op_code, YES_WRAP );
			break;
		case yes_optional:
			EXPR__swift( SELF, oe->op1, oe->op1->return_type, YES_PAREN, oe->op_code, YES_WRAP );
			raw("?");
			break;
		case unknown:
			raw("SDAI.FORCE_OPTIONAL(");
			EXPR__swift( SELF, oe->op1, oe->op1->return_type, YES_PAREN, oe->op_code, YES_WRAP );
			raw(")?");
			break;
	}
	wrap_if(YES_WRAP, ".ISLIKE( PATTERN: ");
	EXPR__swift( SELF, oe->op2, Type_String, NO_PAREN, oe->op_code, YES_WRAP );
	raw(" )");
}

//MARK: QUERY()
static void EXPRquery_swift(Scope SELF, bool can_wrap, Expression e) {
	switch(EXPRresult_is_optional(e->u.query->aggregate, CHECK_SHALLOW)) {
		case no_optional:
			EXPR__swift( SELF, e->u.query->aggregate, e->u.query->aggregate->return_type, YES_PAREN, OP_UNKNOWN, can_wrap );
			break;
		case yes_optional:
			EXPR__swift( SELF, e->u.query->aggregate, e->u.query->aggregate->return_type, YES_PAREN, OP_UNKNOWN, can_wrap );
			raw("?");
			break;
		case unknown:
			raw("SDAI.FORCE_OPTIONAL(");
			EXPR__swift( SELF, e->u.query->aggregate, e->u.query->aggregate->return_type, YES_PAREN, OP_UNKNOWN, can_wrap );
			raw(")?");					
			break;
	}
	{
		char buf[BUFSIZ];
		aggressively_wrap();
		raw( ".QUERY{ %s in \n", variable_swiftName(e->u.query->local,buf));
	}
	
	int indent2 = captureWrapIndent();
	int level = indent2+nestingIndent_swift;
	
	int tempvar_id = 1;
	Linked_List tempvars;
	Expression simplified = EXPR_decompose(e->u.query->expression, Type_Logical, &tempvar_id, &tempvars);
	EXPR_tempvars_swift(SELF, tempvars, level);

	indent_swift(level);
	raw("return ");
	EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, SELF, simplified, Type_Logical, NO_PAREN, OP_UNKNOWN, YES_WRAP);
	raw( " }" );
	
	restoreWrapIndent(indent2);
	EXPR_delete_tempvar_definitions(tempvars);
}

//MARK: OP_CONCAT (complex entity instance construction operator)
void EXPRopCONCAT__swift( Scope SELF1, Scope SELF2, struct Op_Subexpression * oe, char * opcode, 
										Type op1target_type, Type op2target_type,
										bool paren, unsigned int previous_op, bool can_wrap, bool need_optional_operand ) {
	
	bool can_wrap2 = YES_WRAP;
	
	if( paren && ( oe->op_code != previous_op ) ) {
		wrap_if(can_wrap, "( " );
		can_wrap = YES_WRAP;
	}
	
	//OPERAND1
//	raw("/*");TYPE_head_swift(SELF1, oe->op1->return_type, YES_IN_COMMENT);raw("*/");	// DEBUG

		EXPR__swift(SELF1, oe->op1, op1target_type, YES_PAREN, oe->op_code, can_wrap);

	
	//OP CODE
	assert(opcode != NULL);
	wrap_if(can_wrap, "%s", opcode);
	positively_wrap();	

	
	//OPERAND2
//	raw("/*"); TYPE_head_swift(SELF2, oe->op2->return_type, YES_IN_COMMENT); raw("*/"); // DEBUG

		EXPR__swift(SELF2, oe->op2, op2target_type, YES_PAREN, oe->op_code, can_wrap2);

	if( paren && ( oe->op_code != previous_op ) ) {
		raw( " )" );
	}
}




//MARK: - aggregate initializer

static void EXPR_aggregate_initializer_swift(Scope SELF, bool can_wrap, Expression e, Type target_type) {
	if(LISTis_empty(e->u.list)){
		wrap_if(can_wrap, "SDAI.EMPLY_AGGREGATE" );
		return;
	}
	
		Type basetype = TYPE_retrieve_aggregate_base(target_type, NULL);
	
	Type common_type = NULL;
	
	// pre-check //
	LISTdo( e->u.list, arg, Expression ) {
		if( TYPEis(arg->type) == op_ && ARY_EXPget_operator(arg) == OP_REPEAT ){
			arg = ARY_EXPget_operand(arg);
		}
		common_type = TYPEget_common(arg->return_type, common_type);
	} LISTod;
	
	if( basetype == NULL || TYPEis_runtime(basetype) ){
		basetype = common_type;
	}
	if( basetype == NULL ){
		basetype = Type_Runtime;
	}
	
	// code generation //
//	raw("/*");	TYPE_head_swift(SELF, target_type, YES_IN_COMMENT); raw("*/");	//DEBUG
	if( !TYPEis_runtime(basetype) ){
//	raw("/*[");	TYPE_head_swift(SELF, common_type, WO_COMMENT); raw("]*/");	//DEBUG
		wrap_if(can_wrap, "([" );
	}
	else{
		wrap_if(can_wrap, "[" );
	}
	char* sep = "";
	
	LISTdo( e->u.list, arg, Expression ) {
		Expression repeat = NULL;
		if( TYPEis(arg->type) == op_ && ARY_EXPget_operator(arg) == OP_REPEAT ){
			repeat = BIN_EXPget_operand(arg);
			arg = ARY_EXPget_operand(arg);
		}
		
		raw(sep);
		aggressively_wrap();
		
		wrap_if(YES_WRAP,"SDAI.AIE(");
//		raw("/*");TYPE_head_swift(SELF, arg->return_type, WO_COMMENT);raw("*/"); //DEBUG
		EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, SELF, arg, basetype, NO_PAREN, OP_UNKNOWN, YES_WRAP);
		
		if( repeat != NULL ){
			raw(", ");
			wrap_if(YES_WRAP,"repeat: ");
			EXPR__swift( SELF, repeat, Type_Integer, NO_PAREN, OP_UNKNOWN, YES_WRAP );
		}
		raw(")");
		sep = ", ";
	} LISTod;
	
	if( !TYPEis_runtime(basetype) ){
		raw( "] " );
		wrap( "as [SDAI.AggregationInitializerElement<" );
		TYPE_head_swift(SELF, basetype, WO_COMMENT, LEAF_OWNED);
			raw(">])");
	}
	else{
		raw("]");
	}

}


//MARK: - expression return type check
static type_optionality operator_returns_optional(struct Op_Subexpression * oe, bool deep) {
	switch( oe->op_code ) {
			//MARK: - Arithmetic Op
		case OP_NEGATE:
			return EXP_is_literal(oe->op1) ? no_optional : yes_optional;
			
		case OP_PLUS:
		case OP_MINUS:
		case OP_TIMES:
		case OP_REAL_DIV:
		case OP_EXP:
		case OP_DIV:
		case OP_MOD:
			return (EXP_is_literal(oe->op1) && EXP_is_literal(oe->op2)) ? no_optional : yes_optional;
			
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
			return no_optional;
			
			
			//MARK: - Logical Op
		case OP_NOT:
		case OP_AND:
		case OP_OR:
		case OP_XOR:
			return no_optional;
			
			//MARK: - String Op
		case OP_SUBCOMPONENT:
			return yes_optional;
			
			//MARK: - Aggregate Op
		case OP_ARRAY_ELEMENT:
			return yes_optional;
			
			//MARK: - Reference Op
		case OP_DOT:
			if( deep ){
				if( TYPEis_select(oe->op1->return_type) ) return yes_optional;
				
				switch (EXPRresult_is_optional(oe->op1, deep)) {
					case no_optional:
						return EXPRresult_is_optional(oe->op2, deep);
						
					case yes_optional:
						return yes_optional;
						
					case unknown:
						if( EXPRresult_is_optional(oe->op2, deep) == yes_optional ) return yes_optional;
						return unknown;
				}
			}
			else {
				return EXPRresult_is_optional(oe->op2, deep);
			}
			
		case OP_GROUP:
			return yes_optional;
			
			//MARK: - Complex Entity Constructor
		case OP_CONCAT:
			return no_optional;
			
		case OP_REPEAT:
			switch (EXPRresult_is_optional(oe->op1, deep)) {
				case no_optional:
					return EXPRresult_is_optional(oe->op2, deep);
					
				case yes_optional:
					return yes_optional;
					
				case unknown:
					if( EXPRresult_is_optional(oe->op2, deep) == yes_optional ) return yes_optional;
					return unknown;
			}

			
			
			//case OP_ANDOR:
			
		default:
			fprintf( stderr, "ERROR - unknown op-expression\n" );
			abort();
	}
}

//MARK: - optionality check
type_optionality EXPRresult_is_optional(Expression e, bool deep) {
	switch( TYPEis( e->type ) ) {
		case indeterminate_:
		case integer_:
		case real_:
		case binary_:
		case logical_:
		case boolean_:
		case string_:
			return no_optional;
			
		case attribute_:
			return VARis_optional_by_large(e->u.variable)? yes_optional : no_optional;
			
		case entity_:
			return no_optional;
			
		case identifier_:
			if( e->u_tag == expr_is_variable ){
				return VARis_optional_by_large(e->u.variable)? yes_optional : no_optional;
			}
			else if( e->u_tag == expr_is_user_defined ){
				// expr is _TEMPVAR_
				return (type_optionality)e->u.user_defined;
			}
			return unknown;	// attribute of generic entity
			
		case enumeration_:
			return no_optional;
			
		case query_:
			return EXPRresult_is_optional(e->u.query->aggregate, CHECK_DEEP);
			
		case self_:
			return no_optional;	
			
		case funcall_:
			if( e->u.funcall.function->u_tag != scope_is_func ) return no_optional;
			
			if( FUNCreturn_indeterminate(e->u.funcall.function) ) return yes_optional;
			return no_optional;
			
		case op_:
			return operator_returns_optional(&e->e, deep);
			
		case aggregate_:
			return no_optional;

		default:
			fprintf( stderr, "%s:%d: ERROR - unknown expression, type %d", e->symbol.filename, e->symbol.line, TYPEis( e->type ) );
			abort();
	}
}


//MARK: - main entry point

/**
 * if paren == 1, parens are usually added to prevent possible rebind by
 *    higher-level context.  If op is similar to previous op (and
 *    precedence/associativity is not a problem) parens may be omitted.
 * if paren == 0, then parens may be omitted without consequence
 */
void EXPR__swift( Scope SELF, Expression e, Type target_type, bool paren, unsigned int previous_op, bool can_wrap  ) {
		
	switch( TYPEis( e->type ) ) {
			//MARK: indeterminate_
		case indeterminate_:
			wrap_if(can_wrap, "nil" );
			break;
			
			//MARK: integer_
		case integer_:
			if( isLITERAL_INFINITY(e) ) {	//*TY2020/08/26
//				wrap_if(can_wrap, "SDAI.INDETERMINATE" );
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
				wrap_if(can_wrap, "%#.10e", e->u.real  );
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
//		case entity_:
		case identifier_:
//		case enumeration_:			
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
				if(VARis_redeclaring(v)){
					v = v->original_attribute;
				}
				Entity ent = v->defined_in;
				if( ENTITYis_a(SELF, ent) ) {
					wrap_if(can_wrap, "SELF.");
					can_wrap = false;
				}
//				else {
//					ENTITYis_a(SELF, ent); // for debug
//				}
			}
		{
			char buf[BUFSIZ];
			wrap_if(can_wrap, canonical_swiftName(e->symbol.name,buf) );
		}
			break;

		case enumeration_:			
		case entity_:
		{
			char buf[BUFSIZ];
			if( (e->type->symbol.name != NULL) && (strcmp(e->symbol.name, e->type->symbol.name) == 0) ){
				wrap_if(can_wrap, TYPE_swiftName(e->type, SELF, buf));
			}
			else {
				// probably enum case label
				wrap_if(can_wrap, canonical_swiftName(e->symbol.name,buf) );
			}
		}
			break;
			
			//MARK: query_
		case query_:
			EXPRquery_swift(SELF, can_wrap, e);
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
				
				if(can_wrap && LISTget_length(e->u.funcall.list) > 2 ) aggressively_wrap();
				wrap_if(can_wrap, "%s%s(", qual, func );
			}
			
			if( !LISTis_empty(e->u.funcall.list) ) {
				LISTget_length(e->u.funcall.list) > 1 ? aggressively_wrap() : positively_wrap();
				int oldwrap = captureWrapIndent();
				if( LISTget_length(e->u.funcall.list)<=1 )restoreWrapIndent(oldwrap);
				
				char* sep = "";
				bool noLabel = ( isfunc && (LISTget_second(formals) == NULL) );
				
				assert(LISTget_length(formals) == LISTget_length(e->u.funcall.list));

				Link formals_iter = LISTLINKfirst(formals);
				LISTdo( e->u.funcall.list, arg, Expression ) {
					raw("%s",sep);
					LISTget_length(e->u.funcall.list) > 1 ? aggressively_wrap() : positively_wrap();
					
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
					else if( !VARis_optional_by_large(formal_param) ) {
						if( TYPEis_logical(VARget_type(formal_param)) ){
							raw("SDAI.LOGICAL(");
						}
						else{
							raw("SDAI.UNWRAP(");
						}
						EXPRassignment_rhs_swift(YES_RESOLVING_GENERIC, SELF, arg, formal_param->type, NO_PAREN, OP_UNKNOWN, YES_WRAP);
						raw(")");
					}
					else {
						EXPRassignment_rhs_swift(YES_RESOLVING_GENERIC, SELF, arg, formal_param->type, NO_PAREN, OP_UNKNOWN, YES_WRAP);
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
			EXPRop__swift( SELF, &e->e, target_type, paren, previous_op, can_wrap );
			break;
			
			//MARK: aggregate_
		case aggregate_:
			EXPR_aggregate_initializer_swift(SELF, can_wrap, e, target_type);
			break;
			
//			//MARK: oneof_
//		case oneof_: {

		default:
			fprintf( stderr, "%s:%d: ERROR - unknown expression, type %d\n", e->symbol.filename, e->symbol.line, TYPEis( e->type ) );
			abort();
	}
}

static Type align_aggregate_type( Type source_type, Type target_type) {
	if( TYPEis_runtime(TYPEget_base_type(target_type)) ) return source_type ;
	if( TYPEis_AGGREGATE(source_type) ) return source_type;
	
	Type aligned = TYPEcreate_aggregate(TYPEget_type(source_type), 
																			TYPEget_base_type(target_type), 
																			TYPEget_body(source_type)->lower, 
																			TYPEget_body(source_type)->upper, 
																			TYPEget_unique(source_type), 
																			TYPEget_optional(source_type) );
	return aligned;
}


/** print expression that has op and operands */
void EXPRop__swift( Scope SELF, struct Op_Subexpression * oe,  Type target_type,
									 bool paren, unsigned int previous_op, bool can_wrap) {
    switch( oe->op_code ) {
		//MARK: - Arithmetic Op
			//MARK:OP_NEGATE
			case OP_NEGATE:
				EXPRopNEGATE_swift( SELF, oe, target_type, paren, can_wrap);
//				EXPRop1_swift( SELF, oe, " -", paren, NO_WRAP );
				break;
				
			//MARK:OP_PLUS
			case OP_PLUS:
			{
				Type op1_type = oe->op1->return_type;
				Type op2_type = oe->op2->return_type;
				if( TYPEis_aggregation_data_type(target_type) && TYPEis_aggregation_data_type(op1_type) && TYPEis_aggregation_data_type(op2_type) ){
					op1_type = align_aggregate_type(op1_type, target_type);
					op2_type = align_aggregate_type(op2_type, target_type);
				}
				
				EXPRop2__swift( SELF,SELF, oe, " + ", op1_type,op2_type, paren, previous_op, can_wrap, NEED_OPTIONAL_OPERAND );
			}
				break;
				
				//MARK:OP_TIMES
			case OP_TIMES:
			{
				Type op1_type = oe->op1->return_type;
				Type op2_type = oe->op2->return_type;
				if( TYPEis_aggregation_data_type(target_type) && TYPEis_aggregation_data_type(op1_type) && TYPEis_aggregation_data_type(op2_type) ){
					op1_type = align_aggregate_type(op1_type, target_type);
					op2_type = align_aggregate_type(op2_type, target_type);
				}
			
				EXPRop2__swift( SELF,SELF, oe, " * ", op1_type,op2_type, paren, previous_op, can_wrap, NEED_OPTIONAL_OPERAND );
			}
				break;
				
			//MARK:OP_MINUS
			case OP_MINUS:
			{
				Type op1_type = oe->op1->return_type;
				Type op2_type = oe->op2->return_type;
				if( TYPEis_aggregation_data_type(target_type) && TYPEis_aggregation_data_type(op1_type) && TYPEis_aggregation_data_type(op2_type) ){
					op1_type = align_aggregate_type(op1_type, target_type);
					op2_type = align_aggregate_type(op2_type, target_type);
				}
			
				EXPRop2__swift( SELF,SELF, oe, " - ", op1_type,op2_type, paren, OP_UNKNOWN, can_wrap, NEED_OPTIONAL_OPERAND );
			}
				break;
				
			//MARK:OP_REAL_DIV
			case OP_REAL_DIV:
				EXPRop2__swift( SELF,SELF, oe, " / ", oe->op1->return_type,oe->op2->return_type, paren, OP_UNKNOWN, can_wrap, NEED_OPTIONAL_OPERAND );
				break;
				
			//MARK:OP_EXP
			case OP_EXP:
				EXPRop2__swift( SELF,SELF, oe, " ** ", oe->op1->return_type,oe->op2->return_type, paren, OP_UNKNOWN, can_wrap, NEED_OPTIONAL_OPERAND );
				break;
				
				//MARK:OP_DIV
			case OP_DIV:
				EXPRop2__swift( SELF,SELF, oe, " ./. ", oe->op1->return_type,oe->op2->return_type, paren, OP_UNKNOWN, can_wrap, NEED_OPTIONAL_OPERAND );
				break;
				
				//MARK:OP_MOD
			case OP_MOD:
				EXPRop2__swift( SELF,SELF, oe, " % ", oe->op1->return_type,oe->op2->return_type, paren, OP_UNKNOWN, can_wrap, NEED_OPTIONAL_OPERAND );
				break;
				
				
		//MARK: - Relational Op
			//MARK:OP_EQUAL
			case OP_EQUAL:
				EXPRop2__swift( SELF,SELF, oe, " .==. ", oe->op1->return_type,oe->op2->return_type, paren, OP_UNKNOWN, can_wrap, NEED_OPTIONAL_OPERAND );
				break;
				
			//MARK:OP_NOT_EQUAL
			case OP_NOT_EQUAL:
				EXPRop2__swift( SELF,SELF, oe, " .!=. ", oe->op1->return_type,oe->op2->return_type, paren, OP_UNKNOWN, can_wrap, NEED_OPTIONAL_OPERAND );
				break;
				
			//MARK:OP_GREATER_THAN
			case OP_GREATER_THAN:
				EXPRop2__swift( SELF,SELF, oe, " > ", oe->op1->return_type,oe->op2->return_type, paren, OP_UNKNOWN, can_wrap, NEED_OPTIONAL_OPERAND );
				break;
				
			//MARK:OP_LESS_THAN
			case OP_LESS_THAN:
				EXPRop2__swift( SELF,SELF, oe, " < ", oe->op1->return_type,oe->op2->return_type, paren, OP_UNKNOWN, can_wrap, NEED_OPTIONAL_OPERAND );
				break;
				
			//MARK:OP_GREATER_EQUAL
			case OP_GREATER_EQUAL:
				EXPRop2__swift( SELF,SELF, oe, " >= ", oe->op1->return_type,oe->op2->return_type, paren, OP_UNKNOWN, can_wrap, NEED_OPTIONAL_OPERAND );
				break;
				
			//MARK:OP_LESS_EQUAL
			case OP_LESS_EQUAL:
				EXPRop2__swift( SELF,SELF, oe, " <= ", oe->op1->return_type,oe->op2->return_type, paren, OP_UNKNOWN, can_wrap, NEED_OPTIONAL_OPERAND );
				break;
				
			//MARK:OP_INST_EQUAL
			case OP_INST_EQUAL:
				EXPRop2__swift( SELF,SELF, oe, " .===. ", oe->op1->return_type,oe->op2->return_type, paren, OP_UNKNOWN, can_wrap, NEED_OPTIONAL_OPERAND );
				break;
				
			//MARK:OP_INST_NOT_EQUAL
			case OP_INST_NOT_EQUAL:
				EXPRop2__swift( SELF,SELF, oe, " .!==. ", oe->op1->return_type,oe->op2->return_type, paren, OP_UNKNOWN, can_wrap, NEED_OPTIONAL_OPERAND );
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
				EXPRop1_swift( SELF, oe, " !", oe->op1->return_type, paren, NO_WRAP, NO_NEED_OPTIONAL_OPERAND );
				break;
				
			//MARK:OP_AND
			case OP_AND:
				EXPRop2__swift( SELF,SELF, oe, " && ", oe->op1->return_type,oe->op2->return_type, paren, previous_op, can_wrap, NO_NEED_OPTIONAL_OPERAND );
				break;
				
			//MARK:OP_OR
			case OP_OR:
				EXPRop2__swift( SELF,SELF, oe, " || ", oe->op1->return_type,oe->op2->return_type, paren, previous_op, can_wrap, NO_NEED_OPTIONAL_OPERAND );
				break;
				
			//MARK:OP_XOR
			case OP_XOR:
				EXPRop2__swift( SELF,SELF, oe, " .!=. ", oe->op1->return_type,oe->op2->return_type, paren, OP_UNKNOWN, can_wrap, NO_NEED_OPTIONAL_OPERAND );
				break;
				
		//MARK: - String Op
			//MARK:OP_SUBCOMPONENT
			case OP_SUBCOMPONENT:
				switch (EXPRresult_is_optional(oe->op1, CHECK_SHALLOW)) {
					case no_optional:
						EXPR__swift( SELF, oe->op1,oe->op1->return_type, YES_PAREN, OP_UNKNOWN, can_wrap );
						break;
					case yes_optional:
						EXPR__swift( SELF, oe->op1,oe->op1->return_type, YES_PAREN, OP_UNKNOWN, can_wrap );
						raw("?");
						break;
					case unknown:
						raw("SDAI.FORCE_OPTIONAL(");
						EXPR__swift( SELF, oe->op1,oe->op1->return_type, YES_PAREN, OP_UNKNOWN, can_wrap );
						raw(")?");
						break;
				}
				wrap_if(can_wrap, "[" );
				EXPR__swift( SELF, oe->op2, Type_Integer, NO_PAREN, OP_UNKNOWN, YES_WRAP );
				raw(" ... " );
				EXPR__swift( SELF, oe->op3, Type_Integer, NO_PAREN, OP_UNKNOWN, YES_WRAP );
				raw( "]" );
				break;
				
				
		//MARK: - Aggregate Op
			//MARK:OP_ARRAY_ELEMENT
			case OP_ARRAY_ELEMENT:
				switch (EXPRresult_is_optional(oe->op1, CHECK_SHALLOW)) {
					case no_optional:
						EXPR__swift( SELF, oe->op1,oe->op1->return_type, YES_PAREN, OP_UNKNOWN, can_wrap );
						break;
					case yes_optional:
						EXPR__swift( SELF, oe->op1,oe->op1->return_type, YES_PAREN, OP_UNKNOWN, can_wrap );
						raw("?");
						break;
					case unknown:
						raw("SDAI.FORCE_OPTIONAL(");
						EXPR__swift( SELF, oe->op1,oe->op1->return_type, YES_PAREN, OP_UNKNOWN, can_wrap );
						raw(")?");
						break;
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
				EXPRopCONCAT__swift( SELF,SELF, oe, " .||. ", oe->op1->return_type,oe->op2->return_type, paren, previous_op, can_wrap, NO_NEED_OPTIONAL_OPERAND );
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
										Type op1target_type, Type op2target_type,
										bool paren, unsigned int previous_op, bool can_wrap, bool need_optional_operand ) {
	bool both_literal = EXP_is_literal(oe->op1) && EXP_is_literal(oe->op2);
	
	bool can_wrap2 = YES_WRAP;
	if( oe->op_code == OP_DOT ) {
//		can_wrap = YES_WRAP;
		can_wrap2 = NO_WRAP;
	}
	
	if( paren && ( oe->op_code != previous_op ) ) {
		wrap_if(can_wrap, "( " );
		can_wrap = YES_WRAP;
	}
	
	//OPERAND1
//	raw("/*");TYPE_head_swift(SELF1, oe->op1->return_type, YES_IN_COMMENT);raw("*/");	// DEBUG

	if( both_literal || TYPEis_AGGREGATE(oe->op1->type) ){
		EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, SELF1, oe->op1, op1target_type, YES_PAREN, oe->op_code, can_wrap);
	}
	else if( !need_optional_operand ){
		EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, SELF1, oe->op1, op1target_type, YES_PAREN, oe->op_code, can_wrap);
	}
	else{
		switch ( EXPRresult_is_optional(oe->op1, CHECK_DEEP) ) {
			case yes_optional:
				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, SELF1, oe->op1, op1target_type, YES_PAREN, oe->op_code, can_wrap);
				break;
			case no_optional:				
			case unknown:
				raw("SDAI.FORCE_OPTIONAL(");
				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, SELF1, oe->op1, op1target_type, NO_PAREN, oe->op_code, can_wrap);
				raw(")");
				break;
		}
	}

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
//	raw("/*"); TYPE_head_swift(SELF2, oe->op2->return_type, YES_IN_COMMENT); raw("*/"); // DEBUG

	if( both_literal || TYPEis_AGGREGATE(oe->op2->type) ){
		EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, SELF2, oe->op2, op2target_type, YES_PAREN, oe->op_code, can_wrap2);
	}
	else if( !need_optional_operand ){
		EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, SELF2, oe->op2, op2target_type, YES_PAREN, oe->op_code, can_wrap2);
	}
	else {
		switch ( EXPRresult_is_optional(oe->op2, CHECK_DEEP) ) {
			case yes_optional:
				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, SELF2, oe->op2, op2target_type, YES_PAREN, oe->op_code, can_wrap2);
				break;
			case no_optional:				
			case unknown:
				raw("SDAI.FORCE_OPTIONAL(");
				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, SELF2, oe->op2, op2target_type, NO_PAREN, oe->op_code, can_wrap2);
				raw(")");
				break;
		}
	}

	if( paren && ( oe->op_code != previous_op ) ) {
		raw( " )" );
	}
}

/** Print out a one-operand operation.  If there were more than two of these
 * I'd generalize it to do padding, but it's not worth it.
 */
static void EXPRop1_swift( Scope SELF, struct Op_Subexpression * oe, char * opcode, Type op1target_type, 
													bool paren, bool can_wrap, bool need_optional_operand ) {
    if( paren ) {
        wrap_if(can_wrap, "( " );
    }
	
    wrap_if(can_wrap, "%s", opcode );
	
//    EXPR__swift( SELF, oe->op1, oe->op1->return_type, YES_PAREN, OP_UNKNOWN, can_wrap );
#if 0
	raw("/*");
	TYPE_head_swift(SELF1, oe->op1->return_type, YES_IN_COMMENT);
	raw("*/");
#endif
	if( EXP_is_literal(oe->op1) || TYPEis_AGGREGATE(oe->op1->type) ){
		EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, SELF, oe->op1, op1target_type, YES_PAREN, oe->op_code, can_wrap);
	}
	else if( !need_optional_operand ){
		EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, SELF, oe->op1, op1target_type, YES_PAREN, oe->op_code, can_wrap);
	}
	else{
		switch ( EXPRresult_is_optional(oe->op1, CHECK_DEEP) ) {
			case yes_optional:
				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, SELF, oe->op1, op1target_type, YES_PAREN, oe->op_code, can_wrap);
				break;
			case no_optional:				
			case unknown:
				raw("SDAI.FORCE_OPTIONAL(");
				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, SELF, oe->op1, op1target_type, NO_PAREN, oe->op_code, can_wrap);
				raw(")");
				break;
		}
	}
	
    if( paren ) {
        raw( " )" );
    }
}

static Type resolve_generic_type(Type lhs_generic, Type rhs_concrete) {
	assert(lhs_generic != NULL);
	if( rhs_concrete == NULL )return lhs_generic;
	
	Type base_type = TYPEget_base_type(lhs_generic);
	if( base_type != NULL ){
		enum type_enum aggr_type = TYPEis(lhs_generic);
		if( aggr_type == aggregate_ && TYPEis_aggregation_data_type(rhs_concrete) ) aggr_type = TYPEis(rhs_concrete);
		
		if( TYPEcontains_generic(base_type) ) base_type = resolve_generic_type(base_type, TYPEget_base_type(rhs_concrete));
		
		Expression bound1 = TYPEget_body(lhs_generic)->lower;
		if( bound1 == NULL ) bound1 = TYPEget_body(rhs_concrete)->lower; 
		
		Expression bound2 = TYPEget_body(lhs_generic)->upper;
		if( bound2 == NULL ) bound2 = TYPEget_body(rhs_concrete)->upper;
		
		bool unique = TYPEget_unique(lhs_generic);
		if( unique == false ) unique = TYPEget_unique(rhs_concrete);
		
		bool optional = TYPEget_optional(lhs_generic);
		if( optional == false ) optional = TYPEget_optional(rhs_concrete);
		
		Type resolved = TYPEcreate_aggregate(aggr_type, base_type, bound1, bound2, unique, optional);
		return resolved;
	}
	
	if( TYPEis_generic(lhs_generic) ){
		return rhs_concrete;
	}
	
	return lhs_generic;
}

void EXPRaggregate_init_swift(bool resolve_generic, Scope SELF, Expression rhs, Type lhsType , bool leaf_unowned) {
	TypeBody lhs_tb = TYPEget_body( lhsType );

	if( resolve_generic && TYPEcontains_generic(lhsType) ){
		lhsType = resolve_generic_type(lhsType, rhs->return_type);
	}
	
	// rhs: ?
	if( isLITERAL_INFINITY(rhs) ) {
		raw("(nil as ");
		TYPE_head_swift(SELF, lhsType, WO_COMMENT, leaf_unowned);
		raw("?)");
		return;
	}

	// rhs: [AIE...]
	if( TYPEis_AGGREGATE(rhs->type) ) {
		TYPE_head_swift(SELF, lhsType, WO_COMMENT, leaf_unowned);
		raw("(");
		if( lhs_tb->upper ){
			positively_wrap();
			wrap("bound1: SDAI.UNWRAP(");
			EXPRassignment_rhs_swift(resolve_generic, SELF, lhs_tb->lower, Type_Integer, NO_PAREN, OP_UNKNOWN, YES_WRAP);
			raw("), ");
			
			if( TYPEis_array(lhsType) ){
				wrap("bound2: SDAI.UNWRAP(");
				EXPRassignment_rhs_swift(resolve_generic, SELF, lhs_tb->upper, Type_Integer, NO_PAREN, OP_UNKNOWN, YES_WRAP);
				raw("), ");
			}
			else{
				wrap("bound2: ");
				EXPRassignment_rhs_swift(resolve_generic, SELF, lhs_tb->upper, Type_Integer, NO_PAREN, OP_UNKNOWN, YES_WRAP);
				raw(", ");
			}
			aggressively_wrap();
		}
//		raw("/*");TYPE_head_swift(SELF, rhs->return_type, YES_IN_COMMENT); raw("*/"); // DEBUG
		EXPR_swift(SELF, rhs, lhsType, NO_PAREN);
		raw(")");
		return;
	}

	// lhs.type == rhs.type
	if( TYPEis_swiftAssignable(lhsType, rhs->return_type) ){
		EXPR_swift(SELF, rhs, lhsType, NO_PAREN);
		return;
	}
	
	// rhs: select type
	if( TYPEis_select(rhs->return_type) ){
		TYPE_head_swift(SELF, lhsType, WO_COMMENT, leaf_unowned);
		raw("(");
		raw("/*");TYPE_head_swift(SELF, rhs->return_type, YES_IN_COMMENT, leaf_unowned); raw("*/"); // DEBUG
		EXPR_swift(SELF, rhs, lhsType, NO_PAREN);
		raw(")");
		return;
	}
	
	// rhs <> lhs type
	if( TYPEis_AGGREGATE(lhsType) && TYPEis_runtime(TYPEget_base_type(lhsType)) ){
		EXPR_swift(SELF, rhs, lhsType, NO_PAREN);
		return;
	}
	
	TYPE_head_swift(SELF, lhsType, WO_COMMENT, leaf_unowned);
	raw("(");
	if( lhs_tb->upper ){
		positively_wrap();
		wrap("bound1: SDAI.UNWRAP(");
		EXPRassignment_rhs_swift(resolve_generic, SELF, lhs_tb->lower, Type_Integer, NO_PAREN, OP_UNKNOWN, YES_WRAP);
		raw("), ");
		
		if( TYPEis_array(lhsType) ){
			wrap("bound2: SDAI.UNWRAP(");
			EXPRassignment_rhs_swift(resolve_generic, SELF, lhs_tb->upper, Type_Integer, NO_PAREN, OP_UNKNOWN, YES_WRAP);
			raw("), ");
		}
		else{
			wrap("bound2: ");
			EXPRassignment_rhs_swift(resolve_generic, SELF, lhs_tb->upper, Type_Integer, NO_PAREN, OP_UNKNOWN, YES_WRAP);
			raw(", ");
		}
		aggressively_wrap();
	}
	if( TYPEis_generic(rhs->return_type) ){
		wrap("fromGeneric: ");
	}
	else if( TYPEis_AGGREGATE(rhs->return_type) && TYPEget_body(rhs->return_type)->tag != NULL ){
		wrap("fromGeneric: ");		
	}
	else if( TYPEis_aggregation_data_type(rhs->return_type) && 
					(TYPEis_runtime(TYPEget_base_type(rhs->return_type))||TYPEis_generic(TYPEget_base_type(rhs->return_type))) ){
		wrap("generic: ");
	}
	raw("/*");TYPE_head_swift(SELF, rhs->return_type, YES_IN_COMMENT, leaf_unowned); raw("*/"); // DEBUG
	
	EXPR_swift(SELF, rhs, lhsType, NO_PAREN);
	raw(")");
}

void EXPRassignment_rhs_swift(bool resolve_generic, Scope SELF, Expression rhs, Type lhsType, bool paren, unsigned int previous_op, bool can_wrap) {
	bool rhs_is_partial = TYPEis_partial_entity(rhs->return_type);
	
	if( (!rhs_is_partial) && (lhsType == NULL) ) {
		raw("/*null*/");
		EXPR__swift(SELF, rhs, rhs->return_type, paren, previous_op, can_wrap);
		return;
	}
	if( (!rhs_is_partial) && TYPEis_runtime(lhsType) ){
		raw("/*runtime*/");
		EXPR__swift(SELF, rhs, rhs->return_type, paren, previous_op, can_wrap);
		return;
	}
	
	if( isLITERAL_INFINITY(rhs) ) {
		raw("(nil as ");
		TYPE_head_swift(SELF, lhsType, WO_COMMENT, LEAF_OWNED);
		raw("?)");
		return;
	}

	if( resolve_generic && TYPEcontains_generic(lhsType) ) {
		if( TYPEis_aggregation_data_type(lhsType) ) {
			EXPRaggregate_init_swift(resolve_generic, SELF, rhs, lhsType, LEAF_OWNED);
			return;
		}
		
		if( !rhs_is_partial ){
			//	raw("/*generic*/");
			EXPR__swift(SELF, rhs, rhs->return_type, paren, previous_op, can_wrap);
			return;
		}
		
		TYPE_head_swift(SELF, rhs->return_type, WO_COMMENT, LEAF_OWNED);	// wrap with explicit type cast to entity reference
		raw("(");
		raw("/*partial entity*/");
		EXPR__swift(SELF, rhs, lhsType, paren, previous_op, can_wrap);
			raw(")");
			return;
	}
	
	if( EXP_is_literal(rhs) ){
		TYPE_head_swift(SELF, lhsType, WO_COMMENT, LEAF_OWNED);	// wrap with explicit type cast
		raw("(");
	
//	raw("/*");TYPE_head_swift(SELF, rhs->return_type, WO_COMMENT);raw("*/"); // DEBUG	
	
		EXPR__swift(SELF, rhs, lhsType, paren, previous_op, can_wrap);
		raw(")");
		return;
	}
	
	if( TYPEis_swiftAssignable(lhsType, rhs->return_type) ) {
		EXPR__swift(SELF, rhs, lhsType, paren, previous_op, can_wrap);
		return;
	}
	
	if( TYPEis_aggregation_data_type(lhsType) ) {
//		raw("/*%s lhs type*/",TYPEis_runtime(lhsType)? "runtime":"unknown");
		EXPRaggregate_init_swift(resolve_generic, SELF, rhs, lhsType, LEAF_OWNED);
		return;
	}
	
	if( (!rhs_is_partial) && (lhsType == Type_Entity) ) {
		EXPR__swift(SELF, rhs, lhsType, paren, previous_op, can_wrap);
		return;
	}
	
	TYPE_head_swift(SELF, lhsType, WO_COMMENT, LEAF_OWNED);	// wrap with explicit type cast
	raw("(");
	if( TYPEis_generic(rhs->return_type)||TYPEis_runtime(rhs->return_type) ){
		raw("fromGeneric: ");
	}
	if( rhs_is_partial ){
		raw("/*partial entity*/");
	}
	else {
		raw("/*");TYPE_head_swift(SELF, rhs->return_type, WO_COMMENT, LEAF_OWNED);raw("*/"); // DEBUG	
	}
	
	EXPR__swift(SELF, rhs, lhsType, paren, previous_op, can_wrap);
		raw(")");
		return;
}

Expression EXPRempty_aggregate_initializer(void) {
	static Expression empty_aggregate_initializer = NULL;
	
	if( empty_aggregate_initializer != NULL )return empty_aggregate_initializer;
	
	empty_aggregate_initializer = EXP_new();
	empty_aggregate_initializer->type = Type_Aggregate;
	empty_aggregate_initializer->return_type = Type_Runtime;
	empty_aggregate_initializer->u.list = LISTcreate();
	return empty_aggregate_initializer;
}
