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
#include "swift_schema.h"


//MARK: - enum type

void enumTypeDefinition_swift(Schema schema, Type type, int level) {
	char buf[BUFSIZ];
	int count = DICTcount_type(type->symbol_table, OBJ_EXPRESSION);
	Element* enumCases = NULL;
	
	char typebuf[BUFSIZ];
	const char* typename = TYPE_swiftName(type,type->superscope,typebuf);
	indent_swift(level);
	wrap( "public enum %s : SDAI.ENUMERATION, SDAIValue, ", typename );
	wrap( "%s__%s__type {\n", SCHEMA_swiftName(schema, buf),typename );
	
	{	int level2 = level+nestingIndent_swift;
		
		if(count>0) {
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
				raw( "case %s\n", enumCase_swiftName( enumCases[i]->data, buf ) );
			}
		}
		
		
		raw("\n");
		indent_swift(level2);
		raw("// SDAIGenericType\n");
		indent_swift(level2);
		raw("public var typeMembers: Set<SDAI.STRING> {\n");
		indent_swift(level2+nestingIndent_swift);
		raw("return [SDAI.STRING(Self.typeName)]\n");
		indent_swift(level2);
		raw( "}\n\n" );
		 
		indent_swift(level2);
		raw("// SDAIUnderlyingType\n");
		indent_swift(level2);
		raw("public typealias FundamentalType = Self\n");
		indent_swift(level2);
		raw("public static var typeName: String = ");
		wrap("\"%s.%s\"\n", SCHEMA_swiftName(schema, buf), typename);
		indent_swift(level2);
		raw("public var asFundamentalType: FundamentalType { return self }\n");
		indent_swift(level2);
		raw("public init(fundamental: FundamentalType) {\n");
		indent_swift(level2+nestingIndent_swift);
		raw("self = fundamental\n");
		indent_swift(level2);
		raw( "}\n" );
	}
	
	indent_swift(level);
	raw( "}\n\n" );
	
	
	raw("//MARK: -enum case symbol promotions\n");
	for( int i=0; i<count; ++i ) {
		assert(enumCases!=NULL && enumCases[i] != NULL);
		const char* enumname = enumCase_swiftName( enumCases[i]->data, buf );
		indent_swift(level);
		
		Generic x = DICTlookup(type->superscope->enum_table, ((Expression)(enumCases[i]->data))->symbol.name);
		assert(x);
		if( DICT_type == OBJ_AMBIG_ENUM ){
			raw( "// ambiguous:     %s\n", enumname );
		} 
		else {
			assert(DICT_type == OBJ_ENUM);
			raw( "public static let %s = ", enumname );
			wrap("%s", typename);
			wrap(".%s\n", enumname);
		}
	}
	
	if( enumCases != NULL )sc_free( (char*)enumCases );
}


void enumTypeExtension_swift(Schema schema, Type type, int level) {
	char typebuf[BUFSIZ];
	const char* typename = TYPE_swiftName(type,type->superscope,typebuf);

	char schemabuf[BUFSIZ];
	const char* schemaname = SCHEMA_swiftName(schema, schemabuf);

	raw("\n\n//MARK: - ENUMERATION TYPE HIERARCHY\n");
	indent_swift(level);
	raw( "public protocol %s__%s__type: ", schemaname, typename);
	wrap("SDAIEnumerationType ");
	wrap("{}\n\n");
	
	
	indent_swift(level);
	raw( "public protocol %s__%s__subtype: ", schemaname, typename );
	wrap("%s__%s__type, SDAIDefinedType ", schemaname, typename );
	force_wrap();
	indent_swift(level);
	raw("where ");
//	int oldindent = captureWrapIndent();
	raw("Supertype == %s.%s\n", schemaname, typename);
//	force_wrap();
//	wrap( "Supertype.FundamentalType == %s.%s\n", schemaname, typename);
//	restoreWrapIndent(oldindent);
	indent_swift(level);
	raw( "{}\n\n");
}
