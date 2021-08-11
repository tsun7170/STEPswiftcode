//
//  swift_scope_algs.c
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

#include "pp.h"
#include "exppp.h"
#include "pretty_expr.h"
#include "pretty_rule.h"
#include "pretty_type.h"
#include "pretty_func.h"
#include "pretty_entity.h"
#include "pretty_proc.h"
#include "pretty_scope.h"

#include "swift_scope_algs.h"
#include "swift_files.h"
#include "swift_rule.h"
#include "swift_func.h"
#include "swift_proc.h"
#include "swift_schema.h"


/** print the rules in a scope */
void SCOPEruleList_swift(Schema schema, Scope s, int level ) {
	Rule rule;
	DictionaryEntry de;
	
	DICTdo_type_init( s->symbol_table, &de, OBJ_RULE );
	while( 0 != ( rule = ( Rule )DICTdo( &de ) ) ) {
		RULE_swift(schema, rule, level );
	}
}

/** print the functions in a scope */
void SCOPEfuncList_swift(Schema schema, Scope s, int level ) {
	Function func;
	DictionaryEntry de;
	
	DICTdo_type_init( s->symbol_table, &de, OBJ_FUNCTION );
	while( 0 != ( func = ( Function )DICTdo( &de ) ) ) {
		FUNC_swift(schema, true, func, level );
	}
}

/* print the procs in a scope */
static void SCOPEprocList_swift(Schema schema, Scope s, int level ) {
	Procedure proc;
	DictionaryEntry de;
	
	DICTdo_type_init( s->symbol_table, &de, OBJ_PROCEDURE );
	while( 0 != ( proc = ( Procedure )DICTdo( &de ) ) ) {
		PROC_swift(schema, true, proc, level );
	}
}


void SCOPEalgList_swift(Schema schema, Scope s, int level ) {
    SCOPEruleList_swift(schema, s, level );
    SCOPEfuncList_swift(schema, s, level );
    SCOPEprocList_swift(schema, s, level );
}


//MARK: - SCHEMA LEVEL DEFINITION

static void schemaLevelRule_swift( Schema schema, Rule rule ) {
	int level = 0;

	openSwiftFileForRule(schema,rule);
	
	raw("\n"
			"import SwiftSDAIcore\n");
	
	{	char buf[BUFSIZ];
		raw("\n"
				"extension %s {\n", SCHEMA_swiftName(schema, buf));
	}
	
	{	int level2 = level + nestingIndent_swift;
		
		RULE_swift(schema, rule, level2);
	}
	raw("}\n");
}

static void schemaLevelFunction_swift( Schema schema, Function func ) {
	int level = 0;

	
	openSwiftFileForFunction(schema,func);
	
	raw("\n"
			"import SwiftSDAIcore\n");

	{	char buf[BUFSIZ];
		raw("\n"
				"extension %s {\n", SCHEMA_swiftName(schema, buf));
	}
	
	{	int level2 = level + nestingIndent_swift;
		
		FUNC_swift(schema, false, func, level2 );
	}
	raw("}\n\n");
	
	FUNC_result_cache_var_swift(schema, func, level);
	raw("\n");
}

static void schemaLevelProcedure_swift( Schema schema, Procedure proc ) {
	int level = 0;

	openSwiftFileForProcedure(schema,proc);

	raw("\n"
			"import SwiftSDAIcore\n");

	{	char buf[BUFSIZ];
		raw("\n"
				"extension %s {\n", SCHEMA_swiftName(schema, buf));
	}
	
	{	int level2 = level + nestingIndent_swift;
		
		PROC_swift(schema, false, proc, level2 );
	}
	raw("}\n");
}

void SCHEMAalgList_swift( Schema schema ) {
	DictionaryEntry dictEntry;
	
	Rule rule;
	DICTdo_type_init( schema->symbol_table, &dictEntry, OBJ_RULE );
	while( 0 != ( rule = ( Rule )DICTdo( &dictEntry ) ) ) {
		schemaLevelRule_swift(schema, rule);
	}
	
	Function func;
	DICTdo_type_init( schema->symbol_table, &dictEntry, OBJ_FUNCTION );
	while( 0 != ( func = ( Function )DICTdo( &dictEntry ) ) ) {
		schemaLevelFunction_swift(schema, func);
	}
	
	Procedure proc;
	DICTdo_type_init( schema->symbol_table, &dictEntry, OBJ_PROCEDURE );
	while( 0 != ( proc = ( Procedure )DICTdo( &dictEntry ) ) ) {
		schemaLevelProcedure_swift(schema, proc);
	}
	
}
