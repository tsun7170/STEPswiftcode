//
//  swift_enum_type.c
//  exp2swift
//
//  Created by Yoshida on 2020/09/20.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#include <stdlib.h>
#include <assert.h>

//#include <express/error.h>
#include <sc_memmgr.h>

//#include "exppp.h"
#include "pp.h"

#include "swift_enum_type.h"
#include "swift_type.h"
#include "swift_files.h"


//MARK: - enum type

void enumTypeDefinition_swift(Type type, int level) {
	char enumbuf[BUFSIZ];
	int count = DICTcount_type(type->symbol_table, OBJ_EXPRESSION);
	Element* enumCases = NULL;
	
	char typebuf[BUFSIZ];
	const char* typename = TYPE_swiftName(type,type->superscope,typebuf);
	indent_swift(level);
	wrap( "public enum %s : SDAI.ENUMERATION, SDAIEnumerationType {\n", typename );
	
	if(count>0) {
		int level2 = level+nestingIndent_swift;
		enumCases = (Element*)sc_calloc( count, sizeof(Element) );
		
		// sort enum cases
		DictionaryEntry dictEntry;
		DICTdo_type_init( type->symbol_table, &dictEntry, OBJ_EXPRESSION );
		Element tuple;
		while( 0 != ( tuple = DICTdo_tuple(&dictEntry) ) ) {
			Expression expr = tuple->data;
			assert(expr->u.integer >= 1);
			assert(expr->u.integer <= count);
			assert(enumCases[expr->u.integer - 1] == NULL);
			enumCases[expr->u.integer - 1] = tuple;
		}
		
		for( int i=0; i<count; ++i ) {
			assert(enumCases[i] != NULL);
			indent_swift(level2);
			raw( "case %s\n", enumCase_swiftName( enumCases[i]->data, enumbuf ) );
		}
	}
	
	indent_swift(level);
	raw( "}\n\n" );
	
	for( int i=0; i<count; ++i ) {
		assert(enumCases!=NULL && enumCases[i] != NULL);
		if( enumCases[i]->type == OBJ_AMBIG_ENUM ) continue;
		const char* enumname = enumCase_swiftName( enumCases[i]->data, enumbuf );
		indent_swift(level);
		raw( "public static let %s = ", enumname );
		wrap("%s.", typename);
		wrap("%s\n", enumname);
	}
	
	if( enumCases != NULL )sc_free( (char*)enumCases );
}


void enumTypeExtension_swift(Schema schema, Type type, int level) {
}
