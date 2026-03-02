//
//  swift_type_alias.c
//  exp2swift
//
//  Created by Yoshida on 2020/10/25.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
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
#include "pretty_type.h"

#include "swift_type_alias.h"
#include "swift_type.h"
#include "swift_files.h"
#include "swift_schema.h"



void typeAliasDefinition_swift( Schema schema, Type type, Type underlying, int level) {
	int level2 = level  + nestingIndent_swift;
	int level3 = level2 + nestingIndent_swift;
	
	char buf[BUFSIZ];
	
	raw("//MARK: - Defined data type (type alias)\n");
	
	// markdown
	raw("\n/** Defined data type (type alias)\n");
	raw("- EXPRESS source code:\n");
	raw("```express\n");
	TYPE_out(type, level);
	raw("\n```\n");
	raw("*/\n");
	
	indent_swift(level);
	raw( "public struct %s: ",
      namedType_swiftName(type,type->superscope, SWIFT_QUALIFIER, buf)
      );
//	wrap("%s__", SCHEMA_swiftName(schema, buf));
	raw( "TypeHierarchy.%s__TypeBehavior {\n",
      namedType_swiftName(type,type->superscope, SWIFT_QUALIFIER, buf)
      );

	{	
		indent_swift(level2);
		raw( "public typealias Supertype = %s\n",
        namedType_swiftName(underlying, type->superscope, SWIFT_QUALIFIER, buf)
        );
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
		raw("public static let typeName: String = ");
		wrap("\"%s\"\n",
         namedType_canonicalName(type,schema->superscope, SWIFT_QUALIFIER, buf)
         );

		indent_swift(level2);
		raw("public static let bareTypeName: String = ");
		wrap("\"%s\"\n",
         namedType_canonicalName(type,NO_QUALIFICATION, SWIFT_QUALIFIER, buf)
         );

		indent_swift(level2);
		raw("public var typeMembers: Set<SDAI.STRING> {\n");
		{
			indent_swift(level3);
			raw("var members = rep.typeMembers\n");

			indent_swift(level3);
			raw("members.insert(SDAI.STRING(Self.typeName))\n");
			
			TYPEinsert_select_type_members_swift(type, level3);
			
			indent_swift(level3);
			raw("return members\n");
		}
		indent_swift(level2);
		raw( "}\n\n" );
		
		indent_swift(level2);
		raw( "public var rep: Supertype\n" );

		indent_swift(level2);
		raw("/// initialize from the fundamental type value\n");
		indent_swift(level2);
		raw( "public init(fundamental: FundamentalType) {\n" );
		indent_swift(level2+nestingIndent_swift);
		raw( "rep = Supertype(fundamental: fundamental)\n" );
		indent_swift(level2);
		raw("}\n\n");
		
		indent_swift(level2);
		raw("/// initialize from SDAI generic type value\n");
		indent_swift(level2);
		raw("public init?<G: SDAI.GenericType>(fromGeneric generic: G?) {\n");
		indent_swift(level2+nestingIndent_swift);
		raw("guard let repval = Supertype.convert(fromGeneric: generic) else { return nil }\n");
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

void typeAliasExtension_swift( Schema schema, Type type, Type underlying, int level) {
	char typebuf[BUFSIZ];
	const char* typename = namedType_swiftName(type,type->superscope, SWIFT_QUALIFIER, typebuf);

	char schemabuf[BUFSIZ];
	const char* schemaname = SCHEMA_swiftName(schema, schemabuf);

	char buf[BUFSIZ];

	raw("\n\n//MARK: - DEFINED TYPE HIERARCHY\n");

  //__TypeBehavior protocol
	indent_swift(level);
	raw( "extension %s.TypeHierarchy {\n public protocol %s__TypeBehavior: ",
      schemaname,
      typename
      );
	wrap("%s__Subtype {}\n}\n\n",
       namedType_swiftName(underlying, type->superscope, SWIFT_QUALIFIER, buf)
       );

  // __Subtype protocol
	indent_swift(level);
	raw( "extension %s.TypeHierarchy {\n public protocol %s__Subtype: ",
      schemaname,
      typename);
	wrap("%s__TypeBehavior\n",
       typename);

	indent_swift(level);
	raw( "{}\n}\n\n");
}
