//
//  swift_schema.c
//  exp2swift
//
//  Created by Yoshida on 2020/06/13.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#include <stdlib.h>
#include <errno.h>

#include <sc_version_string.h>
#include <express/express.h>

#include <exppp/exppp.h>
#include "pp.h"
#include "pretty_ref.h"
#include "pretty_scope.h"
#include "pretty_schema.h"

//#if defined( _WIN32 ) || defined ( __WIN32__ )
//#  define unlink _unlink
//#else
//#  include <unistd.h> /* for unlink */
//#endif

#include "swift_schema.h"
#include "swift_files.h"
#include "swift_scope_consts.h"
#include "swift_scope_types.h"
#include "swift_scope_entities.h"
#include "swift_scope_algs.h"
#include "swift_symbol.h"
#include "swift_entity.h"
#include "swift_rule.h"


const char* SCHEMA_swiftName( Schema schema, char buf[BUFSIZ]) {
//	return canonical_swiftName(schema->symbol.name, buf);
	return as_schemaSwiftName_n(schema->symbol.name, buf, BUFSIZ);
}

const char* as_schemaSwiftName_n( const char* symbol_name, char* buf, int bufsize ){
	return canonical_swiftName_n(symbol_name, buf, bufsize);
}

const char* schema_nickname(Schema schema, char buf[BUFSIZ] ) {
	char* underline = strchr(schema->symbol.name, '_');
	
	if( underline != NULL) {
		int nicklen = (int)(underline - schema->symbol.name);
		snprintf(buf, BUFSIZ, "%.*s", nicklen, schema->symbol.name);
	}
	else {
		snprintf(buf, BUFSIZ, "%s", schema->symbol.name);
	}
	return buf;
}

//MARK: - SDAI Dictionary Schema Support
static void create_schema_definition(Schema schema, Linked_List constList, int level) {

	indent_swift(level);
	raw("private static func createSchemaDefinition() -> SDAIDictionarySchema.SchemaDefinition {\n");
	
	{	int level2 = level+nestingIndent_swift;
		
		char buf[BUFSIZ];
		indent_swift(level2);
		raw("let schemaDef = SDAIDictionarySchema.SchemaDefinition(name: \"%s\", schema: %s.self)\n\n",
				canonical_swiftName(schema->symbol.name, buf), SCHEMA_swiftName(schema, buf));
		
		DictionaryEntry dictEntry;
		
		indent_swift(level2);
		raw("//MARK: ENTITY DICT\n");
		Entity entity;
		DICTdo_type_init( schema->symbol_table, &dictEntry, OBJ_ENTITY );
		while( 0 != (entity = DICTdo(&dictEntry)) ) {
			indent_swift(level2);
			raw("schemaDef.addEntity(entityDef: %s.entityDefinition)\n",
					ENTITY_swiftName(entity, NO_QUALIFICATION, buf));
		}	

		raw("\n");
		indent_swift(level2);
		raw("//MARK: GLOBAL RULE DICT\n");		
		Rule rule;
		DICTdo_type_init( schema->symbol_table, &dictEntry, OBJ_RULE );
		while( 0 != (rule = DICTdo(&dictEntry)) ) {
			indent_swift(level2);
			raw("schemaDef.addGlobalRule(name: \"%s\", rule: %s)\n", RULE_swiftName(rule, buf), buf);
		}

		raw("\n");
		indent_swift(level2);
		raw("//MARK: GLOBAL CONSTANT DICT\n");		
		LISTdo(constList, var, Variable) {
			indent_swift(level2);
			raw("schemaDef.addConstant(name: \"%s\", value: %s)\n", variable_swiftName(var, buf), buf);
		}LISTod;

		raw("\n");
		indent_swift(level2);
		raw("return schemaDef\n");
	}
	
	indent_swift(level);
	raw("}\n");
}

//MARK: - main entry point
void SCHEMA_swift( Schema schema ) {
	
	openSwiftFileForSchema(schema);
		
	raw("\n"
			"import SwiftSDAIcore\n");

	int level = 0;
	int level2 = level+nestingIndent_swift;
	
	first_line = false;
	beginExpress_swift("SCHEMA DEFINITION");
	raw( "SCHEMA %s;\n", schema->symbol.name );
	
	if( schema->u.schema->usedict || schema->u.schema->use_schemas
		 || schema->u.schema->refdict || schema->u.schema->ref_schemas ) {
		raw( "\n" );
	}
	
	REFout( schema->u.schema->usedict, schema->u.schema->use_schemas, "USE", level + exppp_nesting_indent );
	REFout( schema->u.schema->refdict, schema->u.schema->ref_schemas, "REFERENCE", level + exppp_nesting_indent );
	SCOPEconsts_out( schema, level + exppp_nesting_indent );
	raw(	"\n"
			"END_SCHEMA;"	);
	tail_comment( schema->symbol );
	endExpress_swift();
	
	// swift code generation
	raw("//MARK: - SCHEMA\n");
	{	char buf[BUFSIZ];
		raw("public enum %s: SDAISchema {\n", SCHEMA_swiftName(schema, buf));
		
		indent_swift(level2);
		raw("public static let schemaDefinition = createSchemaDefinition()\n");
	}
	
	Linked_List constList = LISTcreate();
	SCOPEconstList_swift( false, schema, constList, level2 );
	
	create_schema_definition(schema, constList, level2);
	LISTfree(constList);
	
	raw("}\n");
	raw("/* END_SCHEMA */\n");

	SCHEMAtypeList_swift( schema );
	SCHEMAentityList_swift( schema );
	SCHEMAalgList_swift( schema );

	
	return;
}
