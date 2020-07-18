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

    if( exppp_alphabetize == false ) {
        DICTdo_type_init( s->symbol_table, &dictEntry, OBJ_ENTITY );
        while( 0 != ( entity = ( Entity )DICTdo( &dictEntry ) ) ) {
            ENTITY_swift( entity, level );
        }
    } else {
        Linked_List alpha = LISTcreate();

        DICTdo_type_init( s->symbol_table, &dictEntry, OBJ_ENTITY );
        while( 0 != ( entity = ( Entity )DICTdo( &dictEntry ) ) ) {
            SCOPEadd_inorder( alpha, entity );
        }

        LISTdo( alpha, en, Entity ) {
            ENTITY_swift( en, level );
        } LISTod

        LISTfree( alpha );
    }
}

static void schemaLevelEntity_swift( Schema schema, Entity entity) {
	int level = 0;
	
	openSwiftFileForEntity(entity);
	
	
	raw("\n"
			"extension %s {\n", SCHEMA_swiftName(schema));

	ENTITY_swift(entity, level + nestingIndent_swift);
	
	raw("}\n");
}

void SCHEMAentityList_swift( Schema schema ) {
	DictionaryEntry dictEntry;
	
	if( exppp_alphabetize == false ) {
		Entity entity;
		DICTdo_type_init( schema->symbol_table, &dictEntry, OBJ_ENTITY );
		while( 0 != ( entity = ( Entity )DICTdo( &dictEntry ) ) ) {
			schemaLevelEntity_swift(schema, entity);
		}
	} 
	else {
		Linked_List sorted = LISTcreate();
		
		{
			Entity entity;
			DICTdo_type_init( schema->symbol_table, &dictEntry, OBJ_ENTITY );
			while( 0 != ( entity = ( Entity )DICTdo( &dictEntry ) ) ) {
				SCOPEadd_inorder( sorted, entity );
			}
		}
		
		LISTdo( sorted, entity, Entity ) {
			schemaLevelEntity_swift(schema, entity);
		} LISTod
		
		LISTfree( sorted );
	}
	
}
