//
//  swift_named_aggregate_type.c
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
#include "pretty_where.h"
//#include "pretty_expr.h"
#include "pretty_type.h"

#include "swift_named_aggregate_type.h"
#include "swift_type.h"
#include "swift_files.h"
#include "swift_schema.h"
#include "swift_expression.h"

void namedAggregateTypeDefinition_swift( Schema schema, Type type, int level) {
	int level2 = level  + nestingIndent_swift;
	int level3 = level2 + nestingIndent_swift;
	
	char buf[BUFSIZ];
	
	raw("//MARK: - Defined data type (named aggregate type)\n");
	
	// markdown
	raw("\n/** Defined data type (named aggregate type)\n");
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
    TypeBody basetype_body = TYPEget_body( type );

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
		raw( "public var rep: Supertype\n\n" );
		
		indent_swift(level2);
		raw("/// initialize from the fundamental type value\n");
		indent_swift(level2);
		raw( "public init(fundamental: FundamentalType) {\n" );

    indent_swift(level3);
    raw( "rep = Supertype(from: fundamental.asSwiftType, " );

    emit_aggregateBoundSpec(type, type->superscope, EMIT_SELF, "");
    raw( ")\n" );

    indent_swift(level2);
		raw("}\n\n");

		indent_swift(level2);
		raw("/// initialize from SDAI generic type value\n");
    {
      indent_swift(level2);
      raw("public init?<G: SDAI.GenericType>(fromGeneric generic: G?) {\n");
      
      indent_swift(level3);
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
      raw( "rep = Supertype(from: repval.asSwiftType, " );

      emit_aggregateBoundSpec(type, type->superscope, EMIT_SELF, "");
      raw( ")\n" );
      
      
      indent_swift(level2);
      raw("}\n");
    }
    {
      indent_swift(level2);
      raw("public init?<G: SDAI.GenericType>(_ generic: G?) {\n");

      indent_swift(level3);
      raw("self.init(fromGeneric:generic)\n");

      indent_swift(level2);
      raw("}\n");
    }

		TYPEwhereDefinitions_swift(type, level2);
		TYPEwhereRuleValidation_swift(type, level2);
	}

	indent_swift(level);
	raw("}\n");
}


void namedAggregateTypeExtension_swift( Schema schema, Type type, int level) {
	char typebuf[BUFSIZ];
	const char* typename = namedType_swiftName(type,type->superscope, SWIFT_QUALIFIER, typebuf);

	char schemabuf[BUFSIZ];
	const char* schemaname = SCHEMA_swiftName(schema, schemabuf);

	raw("\n\n//MARK: - DEFINED TYPE HIERARCHY\n");
	
  //__TypeBehavior protocol
	indent_swift(level);
	raw( "extension %s.TypeHierarchy {\n public protocol %s__TypeBehavior: ",
      schemaname,
      typename
      );
	positively_wrap();
	if( TYPEis_entityYieldingAggregate(type) ){
		wrap("SDAI.EntityReferenceYielding, ");		
	}
	wrap("SDAI.TypeHierarchy.%s__Subtype {}\n}\n\n",
       builtinTYPE_body_swiftname(type)
       );

  // __Subtype protocol
	indent_swift(level);
	raw( "extension %s.TypeHierarchy {\n public protocol %s__Subtype: ",
      schemaname,
      typename
      );
	wrap("%s__TypeBehavior\n",
       typename
       );

	indent_swift(level);
	raw( "{}\n}\n\n");
}
