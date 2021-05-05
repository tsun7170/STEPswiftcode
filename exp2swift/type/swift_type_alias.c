//
//  swift_type_alias.c
//  exp2swift
//
//  Created by Yoshida on 2020/10/25.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#include <stdlib.h>
#include <assert.h>

//#include <express/error.h>
//#include <sc_memmgr.h>

#include "exppp.h"
#include "pp.h"
#include "resolve.h"
#include "pretty_where.h"
//#include "pretty_expr.h"
//#include "pretty_type.h"

#include "swift_type_alias.h"
#include "swift_type.h"
#include "swift_files.h"
#include "swift_schema.h"



void typeAliasDefinition_swift( Schema schema, Type type, Type original, int level) {
	char buf[BUFSIZ];
	
	raw("//MARK: - Defined data type (type alias)\n");
	indent_swift(level);
	raw( "public struct %s: ", TYPE_swiftName(type,type->superscope,buf));
	wrap("%s__", SCHEMA_swiftName(schema, buf));
	raw( "%s__type {\n", TYPE_swiftName(type,type->superscope,buf));

	{	int level2 = level+nestingIndent_swift;
		indent_swift(level2);
		raw( "public typealias Supertype = %s\n", TYPE_swiftName(original, type->superscope, buf));
		indent_swift(level2);
		raw("public typealias FundamentalType = Supertype.FundamentalType\n");
		indent_swift(level2);
		raw("public typealias Value = Supertype.Value\n");

		Type fundamental = TYPEget_fundamental_type(type);
		if( !TYPEis_enumeration(fundamental) && !TYPEis_select(fundamental) ){
			indent_swift(level2);
			raw("public typealias SwiftType = Supertype.SwiftType\n");
		}
		if( TYPEis_aggregation_data_type(fundamental) || 
			 (TYPEis_select(fundamental) && (TYPE_retrieve_aggregate_base(fundamental, NULL)!=NULL)) ){
			indent_swift(level2);
			raw("public typealias ELEMENT = Supertype.ELEMENT\n");
			indent_swift(level2);
			raw("public func makeIterator() -> FundamentalType.Iterator { return self.asFundamentalType.makeIterator() }\n");
		}
		
		
		indent_swift(level2);
		raw("public static var typeName: String = ");
		wrap("\"%s\"\n", TYPE_canonicalName(type,schema->superscope,buf));
		
		indent_swift(level2);
		raw("public static var bareTypeName: String = ");
		wrap("\"%s\"\n", TYPE_canonicalName(type,NO_QUALIFICATION,buf));

		indent_swift(level2);
		raw( "public var rep: Supertype\n" );

		indent_swift(level2);
		raw( "public init(fundamental: FundamentalType) {\n" );
		indent_swift(level2+nestingIndent_swift);
		raw( "rep = Supertype(fundamental: fundamental)\n" );
		indent_swift(level2);
		raw("}\n\n");
		
		indent_swift(level2);
		raw("public init?<G: SDAIGenericType>(fromGeneric generic: G?) {\n");
		indent_swift(level2+nestingIndent_swift);
		raw("guard let repval = Supertype(fromGeneric: generic) else { return nil }\n");
		indent_swift(level2+nestingIndent_swift);
		raw("rep = repval\n");
		indent_swift(level2);
		raw("}\n");

		TYPEwhereDefinitions_swift(type, level2);
		TYPEwhereRuleValidation_swift(type, level2);
	}

	indent_swift(level);
	raw("}\n");
}

void typeAliasExtension_swift( Schema schema, Type type, Type original, int level) {
	char typebuf[BUFSIZ];
	const char* typename = TYPE_swiftName(type,type->superscope,typebuf);

	char schemabuf[BUFSIZ];
	const char* schemaname = SCHEMA_swiftName(schema, schemabuf);

	char buf[BUFSIZ];

	raw("\n\n//MARK: - DEFINED TYPE HIERARCHY\n");

	indent_swift(level);
	raw( "public protocol %s__%s__type: ", schemaname, typename);
	wrap("%s__%s__subtype {}\n\n", schemaname, TYPE_swiftName(original, type->superscope, buf));
	 
	indent_swift(level);
	raw( "public protocol %s__%s__subtype: ", schemaname, typename);
	wrap("%s__%s__type\n", schemaname, typename);
//	indent_swift(level);
//	wrap("where Supertype: %s__%s__type\n", schemaname, typename);
	indent_swift(level);
	raw( "{}\n");
}
