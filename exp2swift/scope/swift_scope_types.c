//
//  swift_scope_types.c
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

#include "swift_scope_types.h"
#include "swift_files.h"
#include "swift_type.h"
#include "swift_schema.h"


void SCOPEtypeList_swift(Schema schema, Scope s, int level ) {
	DictionaryEntry dictEntry;
	Type type;
	
	DICTdo_type_init( s->symbol_table, &dictEntry, OBJ_TYPE );
	while( 0 != ( type = ( Type )DICTdo( &dictEntry ) ) ) {
		TYPEdefinition_swift(schema, type, level );
	}
}


static void schemaLevelType_swift( Schema schema, Type type) {
	int level = 0;

	openSwiftFileForType(schema,type);

	raw("\n"
			"import SwiftSDAIcore\n");
	
	{	char buf[BUFSIZ];
		raw("\n"
				"extension %s {\n", SCHEMA_swiftName(schema, buf));
	}
	
	TYPEdefinition_swift(schema, type, level + nestingIndent_swift );
	
	raw("}\n");
	
	TYPEextension_swift( schema, type, level );
}

void SCHEMAtypeList_swift( Schema schema ) {
	DictionaryEntry dictEntry;
	
	Type type;
	DICTdo_type_init( schema->symbol_table, &dictEntry, OBJ_TYPE );
	while( 0 != ( type = ( Type )DICTdo( &dictEntry ) ) ) {
		schemaLevelType_swift(schema,type);
	}
}
