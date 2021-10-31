//
//  swift_func.c
//  exp2swift
//
//  Created by Yoshida on 2020/07/27.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

#include <assert.h>
#include <stdlib.h>

#include "exppp.h"
#include "pp.h"
#include "pretty_expr.h"
#include "pretty_func.h"


#include "swift_func.h"
#include "swift_files.h"
#include "swift_type.h"
#include "swift_algorithm.h"
#include "swift_statement.h"
#include "swift_symbol.h"
#include "swift_schema.h"


char 				FUNC_swiftNameInitial( Function func ) {
	return toupper( func->symbol.name[0] );
}

const char* FUNC_swiftName( Function func, char buf[BUFSIZ] ) {
	return canonical_swiftName(func->symbol.name, buf);
}

const char* FUNCcall_swiftName( Expression fcall, char buf[BUFSIZ] ) {
	return canonical_swiftName(fcall->symbol.name, buf);
}

const char* FUNC_cache_swiftName( Function func, char buf[BUFSIZ] ) {
	snprintf(buf, BUFSIZ, "_%s__cache", func->symbol.name);
	return buf;
}

//MARK: - function return value cache

void FUNC_result_cache_var_swift( Schema schema, Function func, int level ) {
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("//MARK: - function result cache\n");
	
	indent_swift(level);
	raw("private var %s = SDAI.FunctionResultCache(", FUNC_cache_swiftName(func, buf));
	wrap("controller: %s.schemaDefinition)\n", SCHEMA_swiftName(schema, buf));
}

static void func_result_cache_lookup_swift( Schema schema, Function func, int level ) {
	int level2 = level + nestingIndent_swift;
	
	char buf[BUFSIZ];
	
	//
	indent_swift(level);
	raw("// CACHE LOOKUP\n");

	//
	indent_swift(level);
	raw("let _params = SDAI.ParameterList( ");	
	char* sep = "";

	LISTdo(func->u.func->parameters, formalp, Variable) {
		raw("%s", sep);
		positively_wrap();
		wrap("%s", variable_swiftName(formalp,buf));
		sep = ", ";
	}LISTod;

	raw(" )\n");
	
	//
	indent_swift(level);
	raw("if case .available(let _cached_value) = %s.cachedValue(params: _params) {\n", FUNC_cache_swiftName(func, buf) );
	
	//
	bool return_optional = FUNCreturn_indeterminate(func);

	indent_swift(level2);
	raw("return _cached_value as%s ", return_optional ? "?" : "!" );
	TYPE_head_swift(func->superscope, FUNCget_return_type(func), WO_COMMENT, LEAF_OWNED);
	raw("\n");

	//
	indent_swift(level);
	raw("}\n");
}

//MARK: - main entry point

void FUNC_swift( Schema schema, bool nested, Function func, int level ) {
	if(!nested) {
		// EXPRESS summary
		beginExpress_swift("FUNCTION DEFINITION");
		FUNC_out(func, level);
		endExpress_swift();	
	}
	
	// function head
	indent_swift(level);
	char* access = (nested ? "//NESTED FUNCTION" : "public static ");
	raw("%s\n", access);
	indent_swift(level);
	{
		char buf[BUFSIZ];
		raw("func %s", FUNC_swiftName(func,buf));
	}

	Linked_List generics = LISTcreate();
	Linked_List aggregates = LISTcreate();
	ALGget_generics(func, generics, aggregates);
	if(!LISTis_empty(generics) || !LISTis_empty(aggregates)) {
		//generic function
		char buf[BUFSIZ];
		char* sep = "";
		raw("<");
		LISTdo(generics, gtag, Type) {
			wrap("%s%s: SDAIGenericType",sep,TYPE_swiftName(gtag,NO_QUALIFICATION,buf));
			sep=", ";
		}LISTod;
		LISTdo(aggregates, atag, Type) {
			wrap("%s%s: SDAIAggregationType",sep,TYPE_swiftName(atag,NO_QUALIFICATION,buf));
			sep=", ";
		}LISTod;
		raw(">");
	}
	
	// parameters
	raw("(");
	ALGargs_swift( func->superscope, NO_FORCE_OPTIONAL, func->u.func->parameters, YES_DROP_SINGLE_LABEL, level );
	raw(") ");
	
	// return type
	aggressively_wrap();
	bool return_optional = FUNCreturn_indeterminate(func);
	raw("-> ");
	optionalType_swift(func->superscope, func->u.func->return_type, return_optional, NOT_IN_COMMENT);
	
	if(!LISTis_empty(aggregates)) {
		// constraint for aggregate element type
		raw("\n");
		indent_swift(level);
		char buf[BUFSIZ];
		char* sep = "where ";
		LISTdo(aggregates, atag, Type) {
			Type aggr = atag->u.type->head;	
			positively_wrap();
			wrap("%s%s.ELEMENT == ",sep,TYPE_swiftName(atag,NULL,buf));
			TYPE_head_swift(func->superscope, TYPEget_base_type(aggr), WO_COMMENT, LEAF_OWNED);
			sep = ", ";
		}LISTod;
	}
	wrap(" {\n\n");
	
	// function body
	{	int level2 = level+nestingIndent_swift;
		
		if( !nested ){
			func_result_cache_lookup_swift(schema, func, level2);
			raw("\n");
		}
		
		ALGvarnize_args_swift(func->u.func->parameters, level2);
		ALGscope_declarations_swift(schema, func, level2);
		int tempvar_id = 1;
		STMTlist_swift(func, func->u.func->body, &tempvar_id, level2);
	}
	
	indent_swift(level);
	if( nested ) {
		char buf[BUFSIZ];
		raw("} //END FUNCTION %s\n\n", FUNC_swiftName(func,buf));
	}
	else {
		raw("}\n\n");
	}
	
	LISTfree(generics);
	LISTfree(aggregates);
}
