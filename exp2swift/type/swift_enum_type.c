//
//  swift_enum_type.c
//  exp2swift
//
//  Created by Yoshida on 2020/09/20.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

#include <stdlib.h>
#include <assert.h>

//#include <express/error.h>
#include <sc_memmgr.h>

//#include "exppp.h"
#include "pp.h"
#include "pretty_where.h"
#include "pretty_type.h"

#include "swift_enum_type.h"
#include "swift_type.h"
#include "swift_files.h"
#include "swift_schema.h"

//MARK: WHERE rule validation
static void enumWhereRuleValidation_swift( Type type, int level ) {
	Linked_List where_rules = TYPEget_where(type);

	raw("\n");
	indent_swift(level);
	raw("//WHERE RULE VALIDATION (ENUMERATION TYPE)\n");

	indent_swift(level);
	raw("public static func validateWhereRules(instance:Self?, prefix:SDAI.WhereLabel, round: SDAI.ValidationRound) -> [SDAI.WhereLabel:SDAI.LOGICAL] {\n");
	
	{	int level2 = level+nestingIndent_swift;
		
		if( LISTis_empty(where_rules) ) {
			indent_swift(level2);
			raw("return [:]\n");		
		} 
		else {
			char typename[BUFSIZ];TYPE_swiftName(type, NO_QUALIFICATION, typename);
			indent_swift(level2);
			raw("let prefix2 = prefix + \"\\\\%s\"\n", typename);
			
			indent_swift(level2);
			raw("var result: [SDAI.WhereLabel:SDAI.LOGICAL] = [:]\n");
			
			LISTdo( where_rules, where, Where ){
				char whereLabel[BUFSIZ];
				indent_swift(level2);
				raw("result[prefix2 + \".%s\"] = %s.%s(SELF: instance)\n", 
						whereRuleLabel_swiftName(where, whereLabel), typename, whereLabel);
			}LISTod;
			
			indent_swift(level2);
			raw("return result\n");		
		}
	}
	
	indent_swift(level);
	raw("}\n\n");
	
}

//MARK: - enum type

void enumTypeDefinition_swift(Schema schema, Type type, int level) {
	char buf[BUFSIZ];
	int count = DICTcount_type(type->symbol_table, OBJ_EXPRESSION);
	Element* enumCases = NULL;
	
	// markdown
	raw("\n/** ENUMERATION type\n");
	raw("- EXPRESS:\n");
	raw("```express\n");
	TYPE_out(type, level);
	raw("\n```\n");
	raw("*/\n");
	
	char typename[BUFSIZ];
	TYPE_swiftName(type,type->superscope,typename);
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
				
				//markdown
				indent_swift(level2);
				raw("/// ENUMERATION case in ``%s``\n", typename);
				
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
		raw("public var entityReference: SDAI.EntityReference? {nil}\n");
		indent_swift(level2);
		raw("public var stringValue: SDAI.STRING? {nil}\n");
		indent_swift(level2);
		raw("public var binaryValue: SDAI.BINARY? {nil}\n");
		indent_swift(level2);
		raw("public var logicalValue: SDAI.LOGICAL? {nil}\n");
		indent_swift(level2);
		raw("public var booleanValue: SDAI.BOOLEAN? {nil}\n");
		indent_swift(level2);
		raw("public var numberValue: SDAI.NUMBER? {nil}\n");
		indent_swift(level2);
		raw("public var realValue: SDAI.REAL? {nil}\n");
		indent_swift(level2);
		raw("public var integerValue: SDAI.INTEGER? {nil}\n");
		indent_swift(level2);
		raw("public var genericEnumValue: SDAI.GenericEnumValue? { SDAI.GenericEnumValue(self) }\n");
		indent_swift(level2);
		raw("public func arrayOptionalValue<ELEM:SDAIGenericType>(elementType:ELEM.Type) -> SDAI.ARRAY_OPTIONAL<ELEM>? {nil}\n");
		indent_swift(level2);
		raw("public func arrayValue<ELEM:SDAIGenericType>(elementType:ELEM.Type) -> SDAI.ARRAY<ELEM>? {nil}\n");
		indent_swift(level2);
		raw("public func listValue<ELEM:SDAIGenericType>(elementType:ELEM.Type) -> SDAI.LIST<ELEM>? {nil}\n");
		indent_swift(level2);
		raw("public func bagValue<ELEM:SDAIGenericType>(elementType:ELEM.Type) -> SDAI.BAG<ELEM>? {nil}\n");
		indent_swift(level2);
		raw("public func setValue<ELEM:SDAIGenericType>(elementType:ELEM.Type) -> SDAI.SET<ELEM>? {nil}\n");
		indent_swift(level2);
		raw("public func enumValue<ENUM:SDAIEnumerationType>(enumType:ENUM.Type) -> ENUM? { return self as? ENUM }\n");
		raw("\n");		
		
		indent_swift(level2);
		raw("// SDAIUnderlyingType\n");

		indent_swift(level2);
		raw("public typealias FundamentalType = Self\n");

		indent_swift(level2);
		raw("public static var typeName: String = ");
		wrap("\"%s\"\n", TYPE_canonicalName(type,schema->superscope,buf));
		
		indent_swift(level2);
		raw("public var asFundamentalType: FundamentalType { return self }\n\n");

		indent_swift(level2);
		raw("public init(fundamental: FundamentalType) {\n");
		indent_swift(level2+nestingIndent_swift);
		raw("self = fundamental\n");
		indent_swift(level2);
		raw("}\n\n" );

		indent_swift(level2);
		raw("public init?<G: SDAIGenericType>(fromGeneric generic: G?) {\n");
		indent_swift(level2+nestingIndent_swift);
		raw("guard let enumval = generic?.enumValue(enumType: Self.self) else { return nil }\n");
		indent_swift(level2+nestingIndent_swift);
		raw("self = enumval\n");
		indent_swift(level2);
		raw("}\n");

		
		
		indent_swift(level2);
		raw("// InitializableByP21Parameter\n");
	
		indent_swift(level2);
		raw("public static var bareTypeName: String = ");
		wrap("\"%s\"\n\n", TYPE_canonicalName(type,NO_QUALIFICATION,buf));

		indent_swift(level2);
		raw("public	init?(p21untypedParam: P21Decode.ExchangeStructure.UntypedParameter, from exchangeStructure: P21Decode.ExchangeStructure) {\n");
		{	int level3 = level2 + nestingIndent_swift;

			indent_swift(level3);
			raw("switch p21untypedParam {\n");

			indent_swift(level3);
			raw("case .enumeration(let enumcase):\n");
			{	int level4 = level3 + nestingIndent_swift;
				
				indent_swift(level4);
				raw("switch enumcase {\n");
				
				for( int i=0; i<count; ++i ) {
					indent_swift(level4);
					raw( "case \"%s\": self = .%s\n", enumCase_swiftName( enumCases[i]->data, buf ), buf );
				}			
				
				indent_swift(level4);
				raw("default:\n");
				indent_swift(level4+nestingIndent_swift);
				raw("exchangeStructure.error = \"unexpected p21parameter enum case(\\(enumcase)) while resolving \\(Self.bareTypeName) value\"\n");
				indent_swift(level4+nestingIndent_swift);
				raw("return nil\n");
				
				indent_swift(level4);
				raw("}\n\n");
			}
			
			indent_swift(level3);
			raw("case .rhsOccurenceName(let rhsname):\n");
			{	int level4 = level3 + nestingIndent_swift; int level5 = level4 + nestingIndent_swift;
				
				indent_swift(level4);
				raw("switch rhsname {\n");
				indent_swift(level4);
				raw("case .constantValueName(let name):\n");
				indent_swift(level5);
				raw("guard let generic = exchangeStructure.resolve(constantValueName: name) else {exchangeStructure.add(errorContext: \"while resolving \\(Self.bareTypeName) value\"); return nil }\n");
				indent_swift(level5);
				raw("guard let enumValue = generic.enumValue(enumType:Self.self) else { exchangeStructure.error = \"constant value(\\(name): \\(generic)) is not compatible with \\(Self.bareTypeName)\"; return nil }\n");
				indent_swift(level5);
				raw("self = enumValue\n\n");
				
				indent_swift(level4);
				raw("case .valueInstanceName(let name):\n");
				indent_swift(level5);
				raw("guard let param = exchangeStructure.resolve(valueInstanceName: name) else {exchangeStructure.add(errorContext: \"while resolving \\(Self.bareTypeName) value from \\(rhsname)\"); return nil }\n");
				indent_swift(level5);
				raw("self.init(p21param: param, from: exchangeStructure)\n\n");
					
				indent_swift(level4);
				raw("default:\n");
				indent_swift(level5);
				raw("exchangeStructure.error = \"unexpected p21parameter(\\(p21untypedParam)) while resolving \\(Self.bareTypeName) value\"\n");
				indent_swift(level5);
				raw("return nil\n");

				indent_swift(level4);
				raw("}\n\n");
			}			
			
			indent_swift(level3);
			raw("case .noValue:\n");
			indent_swift(level3+nestingIndent_swift);
			raw("return nil\n\n");
			
			indent_swift(level3);
			raw("default:\n");
			indent_swift(level3+nestingIndent_swift);
			raw("exchangeStructure.error = \"unexpected p21parameter(\\(p21untypedParam)) while resolving \\(Self.bareTypeName) value\"\n");
			indent_swift(level3+nestingIndent_swift);
			raw("return nil\n");
			
			indent_swift(level3);
			raw("}\n");
		}		
		indent_swift(level2);
		raw("}\n\n");

		indent_swift(level2);
		raw("public init(p21omittedParamfrom exchangeStructure: P21Decode.ExchangeStructure) {\n");
		indent_swift(level2+nestingIndent_swift);
		assert(count > 0);
		raw("self = .%s\n",enumCase_swiftName( enumCases[0]->data, buf ));
		indent_swift(level2);
		raw("}\n");

		
		TYPEwhereDefinitions_swift(type, level2);
		enumWhereRuleValidation_swift(type, level2);
	}
	
	indent_swift(level);
	raw( "}\n\n" );
	
	
	indent_swift(level);
	raw("//MARK: -enum case symbol promotions\n");
	for( int i=0; i<count; ++i ) {
		assert(enumCases!=NULL && enumCases[i] != NULL);
		const char* enumname = enumCase_swiftName( enumCases[i]->data, buf );
		
		Generic x = DICTlookup(type->superscope->enum_table, ((Expression)(enumCases[i]->data))->symbol.name);
		assert(x);
		if( DICT_type == OBJ_AMBIG_ENUM ){
			indent_swift(level);
			raw( "// ambiguous:     %s\n", enumname );
		} 
		else {
			assert(DICT_type == OBJ_ENUM);
			
			//markdown
			indent_swift(level);
			raw("/// promoted ENUMERATION case in ``%s``\n", typename);

			indent_swift(level);
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
	wrap("%s__%s__type, SDAIDefinedType\n", schemaname, typename );
	indent_swift(level);
	wrap("where Supertype: %s__%s__type\n", schemaname, typename);
	indent_swift(level);
	raw( "{}\n\n");
}
