//
//  swift_scope_types.c
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

#include "swift_scope_types.h"
#include "swift_files.h"
#include "swift_type.h"
#include "swift_schema.h"

void SCOPEtypeList_swift( Scope s, int level ) {
	DictionaryEntry dictEntry;
	Type type;
	
	if( exppp_alphabetize == false ) {
		DICTdo_type_init( s->symbol_table, &dictEntry, OBJ_TYPE );
		while( 0 != ( type = ( Type )DICTdo( &dictEntry ) ) ) {
			TYPE_swift( type, level );
		}
	} else {
		Linked_List sorted = LISTcreate();
		
		DICTdo_type_init( s->symbol_table, &dictEntry, OBJ_TYPE );
		while( 0 != ( type = ( Type )DICTdo( &dictEntry ) ) ) {
			SCOPEadd_inorder( sorted, type );
		}
		
		LISTdo( sorted, ty, Type ) {
			TYPE_swift( ty, level );
		} LISTod
		
		LISTfree( sorted );
	}
}


static void schemaLevelType_swift( Schema schema, Type type) {
	int level = 0;

	openSwiftFileForType(type);
	
	raw("\n"
			"extension %s {\n", SCHEMA_swiftName(schema));
	
	TYPE_swift( type, level + nestingIndent_swift );
	
	raw("}\n");
}

void SCHEMAtypeList_swift( Schema schema ) {
	DictionaryEntry dictEntry;
	
	if( exppp_alphabetize == false ) {
		Type type;
		DICTdo_type_init( schema->symbol_table, &dictEntry, OBJ_TYPE );
		while( 0 != ( type = ( Type )DICTdo( &dictEntry ) ) ) {
			schemaLevelType_swift(schema,type);
		}
	} 
	else {
		Linked_List sorted = LISTcreate();
		
		{
			Type type;
			DICTdo_type_init( schema->symbol_table, &dictEntry, OBJ_TYPE );
			while( 0 != ( type = ( Type )DICTdo( &dictEntry ) ) ) {
				SCOPEadd_inorder( sorted, type );
			}
		}
		
		LISTdo( sorted, type, Type ) {
			schemaLevelType_swift(schema,type);
		} LISTod
		
		LISTfree( sorted );
	}
}
