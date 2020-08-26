//
//  swift_scope_entities.c
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
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

#include "swift_scope_entities.h"
#include "swift_files.h"
#include "swift_entity.h"
#include "swift_schema.h"

void SCOPEentityList_swift( Scope s, int level ) {
	DictionaryEntry dictEntry;
	Entity entity;
	
	DICTdo_type_init( s->symbol_table, &dictEntry, OBJ_ENTITY );
	while( 0 != ( entity = ( Entity )DICTdo( &dictEntry ) ) ) {
		Linked_List dummy = LISTcreate();
		Linked_List dummy2 = LISTcreate();
		ENTITY_swift( entity, level, dummy, dummy2 );
		assert(LISTempty(dummy));
		assert(LISTempty(dummy2));
		LISTfree(dummy);
		LISTfree(dummy2);
	}
}


static void schemaLevelEntity_swift( Schema schema, Entity entity) {
	int level = 0;
	Linked_List dynamic_attrs = LISTcreate();
	Linked_List attr_overrides = LISTcreate();
	
	openSwiftFileForEntity(schema,entity);
	
	raw("\n"
			"import SwiftSDAIcore\n");

	raw("\n"
			"extension %s {\n", SCHEMA_swiftName(schema));
	
	{	int level2 = level + nestingIndent_swift;
		
		ENTITY_swift(entity, level2, dynamic_attrs, attr_overrides);
	}
	raw("}\n");
	
	ENTITY_swiftProtocol(schema, entity, level, dynamic_attrs);
	partialEntityAttrOverrideProtocolConformance_swift(schema, entity, level, attr_overrides);

	LISTfree(dynamic_attrs);
	LISTfree(attr_overrides);
}

void SCHEMAentityList_swift( Schema schema ) {
	DictionaryEntry dictEntry;
	
	Entity entity;
	DICTdo_type_init( schema->symbol_table, &dictEntry, OBJ_ENTITY );
	while( 0 != ( entity = ( Entity )DICTdo( &dictEntry ) ) ) {
		schemaLevelEntity_swift(schema, entity);
	}	
}
