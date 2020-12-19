//
//  swift_named_simple_type.c
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
//#include "pretty_where.h"
//#include "pretty_expr.h"
//#include "pretty_type.h"

#include "swift_named_simple_type.h"
#include "swift_type.h"
#include "swift_files.h"
#include "swift_schema.h"

void namedSimpleTypeDefinition_swift( Schema schema, Type type, int level) {
	char buf[BUFSIZ];
	
	raw("//MARK: - Defined data type (named simple type)\n");
	indent_swift(level);
	raw( "public struct %s: ", TYPE_swiftName(type,type->superscope,buf));
	wrap("%s__", SCHEMA_swiftName(schema, buf));
	raw( "%s__type {\n", TYPE_swiftName(type,type->superscope,buf));

	{	int level2 = level+nestingIndent_swift;
		indent_swift(level2);
		raw( "public typealias Supertype = " );
		TYPE_body_swift(type->superscope, type, NOT_IN_COMMENT);
		raw("\n");
		indent_swift(level2);
		raw("public typealias FundamentalType = Supertype.FundamentalType\n");

		indent_swift(level2);
		raw("public static var typeName: String = ");
		wrap("\"%s\"\n", TYPE_swiftName(type,schema->superscope,buf));

		indent_swift(level2);
		raw( "public var rep: Supertype\n" );
		indent_swift(level2);
		raw( "public init(fundamental: FundamentalType) {\n" );
		
		{	int level3 = level2+nestingIndent_swift;
			indent_swift(level3);
			raw( "rep = Supertype(fundamental)\n" );
		}
		
		indent_swift(level2);
		raw("}\n");
	}

	indent_swift(level);
	raw("}\n");
}

void namedSimpleTypeExtension_swift( Schema schema, Type type, int level) {
	char buf[BUFSIZ];

	raw("\n\n//MARK: - DEFINED TYPE HIERARCHY\n");

	indent_swift(level);
	raw( "public protocol %s__", SCHEMA_swiftName(schema, buf));
	raw( "%s__type: ", TYPE_swiftName(type,type->superscope,buf));
	wrap("SDAI__");
	raw( "%s__subtype {}\n\n", builtinTYPE_body_swiftname(type) );
	 
	indent_swift(level);
	raw( "public protocol %s__", SCHEMA_swiftName(schema, buf));
	raw( "%s__subtype: ", TYPE_swiftName(type,type->superscope,buf));
	wrap("%s__", SCHEMA_swiftName(schema, buf));
	raw( "%s__type\n", TYPE_swiftName(type,type->superscope,buf));
	indent_swift(level);
	raw( "where Supertype == %s\n", TYPE_swiftName(type,schema->superscope,buf));
	indent_swift(level);
	raw( "{}\n");
}

