//
//  swift_named_aggregate_type.c
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

#include "swift_named_aggregate_type.h"
#include "swift_type.h"
#include "swift_files.h"
#include "swift_schema.h"

void namedAggregateTypeDefinition_swift( Schema schema, Type type, int level) {
	char buf[BUFSIZ];
	
	raw("//MARK: - Defined data type (named aggregate type)\n");
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
		raw("public typealias Value = Supertype.Value\n");
		indent_swift(level2);
		raw("public typealias SwiftType = Supertype.SwiftType\n");
		indent_swift(level2);
		raw("public typealias ELEMENT = Supertype.ELEMENT\n");
		indent_swift(level2);
		raw("public func makeIterator() -> FundamentalType.Iterator { return self.asFundamentalType.makeIterator() }\n");

		indent_swift(level2);
		raw("public static var typeName: String = ");
		wrap("\"%s\"\n", TYPE_canonicalName(type,schema->superscope,buf));

		indent_swift(level2);
		raw( "public var rep: Supertype\n\n" );
		
		indent_swift(level2);
		raw( "public init(fundamental: FundamentalType) {\n" );
		indent_swift(level2+nestingIndent_swift);
		raw( "rep = Supertype(fundamental: fundamental)\n" );		
		indent_swift(level2);
		raw("}\n\n");

		indent_swift(level2);
		raw("public init?<G: SDAIGenericType>(fromGeneric generic: G?) {\n");
		indent_swift(level2+nestingIndent_swift);
		raw("guard let repval = generic?.");
		switch( TYPEis(type) ){
			case array_:
				if(TYPEis_optional(type)){
					raw("arrayOptionalValue(elementType: ELEMENT.self)");
				}
				else{
					raw("arrayValue(elementType: ELEMENT.self)");
				}
				break;
			case list_:
				raw("listValue(elementType: ELEMENT.self)");
				break;
			case bag_:
				raw("bagValue(elementType: ELEMENT.self)");
				break;
			case set_:
				raw("setValue(elementType: ELEMENT.self)");
				break;
			default:
				raw("UNKNOWNTYPE");
				break;
		}
		raw(" else { return nil }\n");
		indent_swift(level2+nestingIndent_swift);
		raw("rep = repval\n");
		indent_swift(level2);
		raw("}\n");
		
	}

	indent_swift(level);
	raw("}\n");
}


void namedAggregateTypeExtension_swift( Schema schema, Type type, int level) {
	char typebuf[BUFSIZ];
	const char* typename = TYPE_swiftName(type,type->superscope,typebuf);

	char schemabuf[BUFSIZ];
	const char* schemaname = SCHEMA_swiftName(schema, schemabuf);

//char buf[BUFSIZ];

	raw("\n\n//MARK: - DEFINED TYPE HIERARCHY\n");
	
	indent_swift(level);
	raw( "public protocol %s__%s__type: ", schemaname, typename);
	positively_wrap();
	wrap("SDAI__%s__subtype {}\n\n", builtinTYPE_body_swiftname(type) );
	 
	indent_swift(level);
	raw( "public protocol %s__%s__subtype: ", schemaname, typename);
	wrap("%s__%s__type\n", schemaname, typename);
//	indent_swift(level);
//	raw( "where Supertype == %s\n", TYPE_swiftName(type,schema->superscope,buf));
	indent_swift(level);
	raw( "{}\n");
}
