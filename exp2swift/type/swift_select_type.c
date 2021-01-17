//
//  swift_select_type.c
//  exp2swift
//
//  Created by Yoshida on 2020/09/03.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#include <stdlib.h>
#include <assert.h>

#include <express/error.h>
#include <sc_memmgr.h>
#include <resolve.h>

#include "exppp.h"
#include "pp.h"
#include "pretty_where.h"
#include "pretty_expr.h"
#include "pretty_type.h"

#include "swift_select_type.h"
#include "swift_type.h"
#include "swift_entity.h"
#include "swift_expression.h"
#include "swift_files.h"
#include "swift_symbol.h"
#include "swift_algorithm.h"
#include "swift_schema.h"

//MARK: - select type

static void listSelectionEntityAttributes(Type select_type, Entity case_entity, int level) {
	Dictionary all_attrs_from_entity = ENTITYget_all_attributes(case_entity);
	DictionaryEntry de;
	Linked_List attr_defs_from_entity;
	
	DICTdo_init( all_attrs_from_entity, &de );
	while( 0 != (attr_defs_from_entity=DICTdo(&de)) ) {
		Variable attr = LISTget_first(attr_defs_from_entity);
		char* attr_name = VARget_simple_name(attr);

		indent_swift(level);
		raw("ATTR:  %s: ",attr_name);

		if( ENTITYget_attr_ambiguous_count(case_entity, attr_name) > 1 ) {
			wrap("(AMBIGUOUS (CASE LEVEL))\n");			
		}
		else {
			TYPE_head_out(attr->type, level);
			
			if( SELECTget_attr_ambiguous_count(select_type, attr_name) > 1 ) {
//				if( VARis_redeclaring(attr) ) attr = attr->original_attribute;
				wrap(" (AMBIGUOUS (SELECT LEVEL))");			
//				wrap("ORIGIN = %s)", attr->defined_in->symbol.name);			
			}
			else if( !SELECTattribute_is_unique(select_type, attr_name) ) {
				raw(" *** Multiple Select Case Sources ***");
			}
			raw("\n");
		}
	}

}

static void listSelectionSelectAttributes(Type select_type, Type case_select_type, int level) {
	Dictionary all_attrs_from_select = SELECTget_all_attributes(case_select_type);
	DictionaryEntry de;
	Linked_List attr_defs;
	
	DICTdo_init(all_attrs_from_select, &de);
	while( 0 != (attr_defs = DICTdo(&de)) ) {
		Variable attr = LISTget_first(attr_defs);
		char* attr_name = VARget_simple_name(attr);
		
		indent_swift(level);
		raw("ATTR:  %s: ",attr_name);
		
		if( SELECTget_attr_ambiguous_count(case_select_type, attr_name) > 1 ) {
			wrap("(AMBIGUOUS (CASE LEVEL))\n");
//			wrap("ORIGINS = ");
//			char* sep = "";
//			LISTdo(attr_defs, amb, Variable) {
//				if( VARis_redeclaring(amb) ) amb = amb->original_attribute;
//				Entity origin = amb->defined_in;
//				raw(sep);
//				wrap(origin->symbol.name);
//				sep = ", ";
//			}LISTod;
//			raw(")\n");
		}
		else {
			attr = SELECTfind_attribute_effective_definition(case_select_type, attr_name);
			TYPE_head_out(attr->type, level);
			
			if( SELECTget_attr_ambiguous_count(select_type, attr_name) > 1 ) {
				wrap(" (AMBIGUOUS (SELECT LEVEL))");			
			}
			else if( !SELECTattribute_is_unique(case_select_type, attr_name) ) {
				raw(" *** Multiple Select Case Sources ***");
			}
			raw("\n");		
		}
	}
}

static void listAllSelectionAttributes(Type select_type, int level) {
	raw("\n//MARK: - ALL DEFINED ATTRIBUTES FOR SELECT\n/*\n");
	
	TypeBody typeBody = TYPEget_body(select_type);

	LISTdo( typeBody->list, selection, Type ) {
		indent_swift(level);
		raw("%s ", selection->symbol.name);

		if( TYPEis_entity(selection) ) {
			Entity entity = TYPEget_body(selection)->entity;
			raw("(*ENTITY*):\n");
			listSelectionEntityAttributes(select_type, entity, level);
		}
		else if( TYPEis_select(selection) ) {
			raw("(*SELECT*):\n");
			listSelectionSelectAttributes(select_type, selection, level);
		}
		else {
			if( TYPEis_enumeration(selection) ) {
				raw("(*ENUM*): ");
			}
			else {
				raw("(*TYPE*): ");
			}
			TYPE_head_out(selection, level);
			raw("\n");
		}
		raw("\n");
	}LISTod;
	
	raw("*/\n");
}

//MARK: - group reference codes
static void selectTypeGroupReference_swift(Type select_type, int level) {
	char buf[BUFSIZ];
	
	TypeBody typeBody = TYPEget_body(select_type);
	int selection_count = LISTget_length(typeBody->list);
	
	char* mark = "//MARK: - NON-ENTITY TYPE GROUP REFERENCES\n";
	
	LISTdo( typeBody->list, selection, Type ) {
		if( TYPEis_entity(selection) ) continue;
		
		if( mark ) {
			indent_swift(level);
			raw(mark);
			mark = NULL;
		}
		
		indent_swift(level);
		raw("public var %s%s: ", superEntity_swiftPrefix, TYPE_swiftName(selection, select_type->superscope, buf)); 
		wrap("%s? {\n", buf);
		
		{	int level2 = level+nestingIndent_swift;
			
			indent_swift(level2);
			raw("switch self {\n");
			
			indent_swift(level2);
			raw("case .%s(let selectValue): return selectValue\n",selectCase_swiftName(selection, buf));
			indent_swift(level2);
			raw("default: return nil\n");
			
			indent_swift(level2);				
			raw("}\n");
		}
		
		indent_swift(level);
		raw("}\n\n");
	} LISTod;				
	
	mark = "//MARK: - ENTITY TYPE GROUP REFERENCES\n";
	Dictionary all_supers = SELECTget_super_entity_list(select_type);
	DictionaryEntry de;
	Linked_List origins;
	
	DICTdo_init(all_supers, &de);
	while( 0 != (origins = DICTdo(&de))) {
		const char* entity_name = DICT_key;
		
		if( mark ) {
			indent_swift(level);
			raw(mark);
			mark = NULL;
		}
		
		indent_swift(level);
		raw("public var %s%s: ", superEntity_swiftPrefix, as_entitySwiftName_n(entity_name, buf, sizeof(buf))); 
		wrap("%s? {\n", buf);
		
		{	int level2 = level+nestingIndent_swift;
			
			indent_swift(level2);
			raw("switch self {\n");
			int unhandled = selection_count;
			
			LISTdo(origins, selection, Type) {
				if( TYPEis_select(selection) ) {
					indent_swift(level2);
					raw("case .%s(let select): return select",selectCase_swiftName(selection, buf));
					wrap(".%s%s\n",superEntity_swiftPrefix,as_entitySwiftName_n(entity_name, buf, sizeof(buf)));
				}
				else {
					Entity entity = TYPEget_body(selection)->entity;
					
					indent_swift(level2);
					raw("case .%s(let entity): return entity",selectCase_swiftName(selection, buf));
					if( entity->symbol.name != entity_name ) {
						wrap(".%s%s",superEntity_swiftPrefix,as_entitySwiftName_n(entity_name, buf, sizeof(buf)));
					}
					raw("\n");
				}
				--unhandled;
			}LISTod;
			
			if( unhandled > 0 ) {
				indent_swift(level2);
				raw("default: return nil\n");
			}
			
			indent_swift(level2);				
			raw("}\n");
		}
		
		indent_swift(level);
		raw("}\n\n");
	}
}
static void selectTypeGroupReference_swiftProtocol(Schema schema, Type select_type, int level) {
	char buf[BUFSIZ];
	char schemabuf[BUFSIZ];
	const char* schemaname = SCHEMA_swiftName(schema, schemabuf);
	
	TypeBody typeBody = TYPEget_body(select_type);
	
	char* mark = "//MARK: GROUP REFERENCES\n";
	
	LISTdo( typeBody->list, selection, Type ) {
		if( TYPEis_entity(selection) ) continue;
		
		if( mark ) {
			indent_swift(level);
			raw(mark);
			mark = NULL;
		}
		
		indent_swift(level);
		raw("var %s%s: ", superEntity_swiftPrefix, TYPE_swiftName(selection, select_type->superscope, buf)); 
		wrap("%s.%s? { get }\n", schemaname,buf);
	} LISTod;				
	
	Dictionary all_supers = SELECTget_super_entity_list(select_type);
	DictionaryEntry de;
//	Linked_List origins;
	const char* entity_name;
	
	DICTdo_init(all_supers, &de);
	while( 0 != (entity_name = DICTdo_key(&de))) {
//		const char* entity_name = DICT_key;
		
		if( mark ) {
			indent_swift(level);
			raw(mark);
			mark = NULL;
		}
		
		indent_swift(level);
		raw("var %s%s: ", superEntity_swiftPrefix, as_entitySwiftName_n(entity_name, buf, sizeof(buf))); 
		wrap("%s.%s? { get }\n", schemaname,buf);
	}
}
static void selectSubtypeGroupReference_swift(Schema schema, Type select_type, int level) {
	char buf[BUFSIZ];
	char schemabuf[BUFSIZ];
	const char* schemaname = SCHEMA_swiftName(schema, schemabuf);
	
	TypeBody typeBody = TYPEget_body(select_type);
	
	char* mark = "//MARK: GROUP REFERENCES\n";
	
	LISTdo( typeBody->list, selection, Type ) {
		if( TYPEis_entity(selection) ) continue;
		
		if( mark ) {
			indent_swift(level);
			raw(mark);
			mark = NULL;
		}
		
		indent_swift(level);
		raw("var %s%s: ", superEntity_swiftPrefix, TYPE_swiftName(selection, select_type->superscope, buf)); 
		wrap("%s.%s? { rep.%s%s }\n", schemaname, buf, superEntity_swiftPrefix, buf);
	} LISTod;				
	
	Dictionary all_supers = SELECTget_super_entity_list(select_type);
	DictionaryEntry de;
//	Linked_List origins;
	const char* entity_name;
	
	DICTdo_init(all_supers, &de);
	while( 0 != (entity_name = DICTdo_key(&de))) {
//		const char* entity_name = DICT_key;
		
		if( mark ) {
			indent_swift(level);
			raw(mark);
			mark = NULL;
		}
		
		indent_swift(level);
		raw("var %s%s: ", superEntity_swiftPrefix, as_entitySwiftName_n(entity_name, buf, sizeof(buf))); 
		wrap("%s.%s? { rep.%s%s }\n", schemaname, buf, superEntity_swiftPrefix, buf);
	}
}

//MARK: - attribute reference codes
static void selectTypeAttributeReference_swift(Type select_type, int level) {
	TypeBody typebody = TYPEget_body(select_type);
	int selection_case_count = LISTget_length(typebody->list);

	Dictionary all_attrs = SELECTget_all_attributes(select_type);
	DictionaryEntry de;
	Linked_List attr_defs;
	
	char* mark = "//MARK: - ENTITY ATTRIBUTE REFERENCES\n";

	DICTdo_init(all_attrs, &de);
	while ( 0 != (attr_defs = DICTdo(&de))) {
		const char* attr_name = DICT_key;
		if( SELECTget_attr_ambiguous_count(select_type, attr_name) > 1 ) continue;
		
		if( mark ) {
			indent_swift(level);
			raw(mark);
			mark = NULL;
		}
		
		Type attr_type = SELECTfind_common_type(attr_defs);
		if( attr_type == NULL ) continue;
			
		indent_swift(level);
		char buf[BUFSIZ];
		raw("public var %s: ", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));
		optionalType_swift(NO_QUALIFICATION, attr_type, YES_FORCE_OPTIONAL, NOT_IN_COMMENT);
		raw(" {\n");

		{	int level2 = level+nestingIndent_swift;
			
			indent_swift(level2);
			raw("switch self {\n");
			int unhandled = selection_case_count;
			
			LISTdo(typebody->list, selection_case, Type){
				if( TYPEis_entity(selection_case) ) {
					Entity case_entity = TYPEget_body(selection_case)->entity;

					Variable base_attr = ENTITYfind_attribute_effective_definition(case_entity, attr_name);
					if( base_attr != NULL ){
						indent_swift(level2);
						--unhandled;
						raw("case .%s(let entity): return ",selectCase_swiftName(selection_case, buf));
						
						if( TYPEs_are_equal(attr_type, base_attr->type) ){
							wrap("entity.%s\n", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));
						}
						else {
							TYPE_head_swift(NULL, attr_type, WO_COMMENT);
							wrap("(entity.%s)\n", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));
						}
					}
					else if( ENTITYget_attr_ambiguous_count(case_entity, attr_name) > 1 ){
						indent_swift(level2);
						--unhandled;
						raw("case .%s/*(let entity)*/: return nil // AMBIGUOUS ATTRIBUTE ",selectCase_swiftName(selection_case, buf));
						raw("for %s\n", TYPE_swiftName(selection_case, NO_QUALIFICATION, buf));
					}
				}

				else if( TYPEis_select(selection_case) ) {
					Variable base_attr = SELECTfind_attribute_effective_definition(selection_case, attr_name);
					if(base_attr != NULL ){
						indent_swift(level2);
						--unhandled;
						raw("case .%s(let select): return ",selectCase_swiftName(selection_case, buf));
						
						if( TYPEs_are_equal(attr_type, base_attr->type) ){
							wrap("select.%s\n", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));
						}
						else {
							TYPE_head_swift(NULL, attr_type, WO_COMMENT);
							wrap("(select.%s)\n", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));
						}
					}
					else if( SELECTget_attr_ambiguous_count(selection_case, attr_name) > 1 ){
						indent_swift(level2);
						--unhandled;
						raw("case .%s/*(let select)*/: return nil // AMBIGUOUS ATTRIBUTE ",selectCase_swiftName(selection_case, buf));
						raw("for %s\n", TYPE_swiftName(selection_case, NO_QUALIFICATION, buf));
					}
				}				
			}LISTod;
			
//			
//			LISTdo_links(attr_defs, def) {
//				Type selection = def->aux;
//				if( TYPEis_select(selection) ) {
//					Variable base_attr = SELECTfind_attribute_effective_definition(selection, attr_name);
//					assert(base_attr != NULL);
//					
//					indent_swift(level2);
//					raw("case .%s(let select): return ",selectCase_swiftName(selection, buf));
//			
//					if( TYPEs_are_equal(attr_type, base_attr->type) ){
//						wrap("select.%s\n", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));
//					}
//					else {
//						TYPE_head_swift(NULL, attr_type, WO_COMMENT);
//						wrap("(select.%s)\n", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));
//					}
//				}
//				
//				else {
//					Entity entity = TYPEget_body(selection)->entity;
//					Variable base_attr = ENTITYfind_attribute_effective_definition(entity, attr_name);
//					if( base_attr != NULL ){
//						indent_swift(level2);
//						raw("case .%s(let entity): return ",selectCase_swiftName(selection, buf));
//						
//						if( TYPEs_are_equal(attr_type, base_attr->type) ){
//							wrap("entity.%s\n", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));
//						}
//						else {
//							TYPE_head_swift(NULL, attr_type, WO_COMMENT);
//							wrap("(entity.%s)\n", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));
//						}
//					}
//					else {
//						indent_swift(level2);
//						raw("// case .%s(let entity): return nil // AMBIGUOUS ATTRIBUTE (ENTITY LEVEL)\n",selectCase_swiftName(selection, buf));
//						continue;
//					}
//				}
//				--unhandled;
//			}LISTod;		
			
			if( unhandled > 0 ){
				indent_swift(level2);
				raw("default: return nil\n");
			}

			indent_swift(level2);
			raw("}\n");
		}
		
		indent_swift(level);
		raw("}\n\n");
	}
}
static void selectTypeAttributeReference_swiftProtocol(Schema schema, Type select_type, int level) {
	Dictionary all_attrs = SELECTget_all_attributes(select_type);
	DictionaryEntry de;
	Linked_List attr_defs;
//	const char* attr_name;
	
	char* mark = "//MARK: ENTITY ATTRIBUTE REFERENCES\n";

	DICTdo_init(all_attrs, &de);
	while ( 0 != (attr_defs = DICTdo(&de))) {
		const char* attr_name = DICT_key;
		if( SELECTget_attr_ambiguous_count(select_type, attr_name) > 1 ) continue;
		
		Type attr_type = SELECTfind_common_type(attr_defs);
		if( attr_type == NULL ) continue;
		
		if( mark ) {
			indent_swift(level);
			raw(mark);
			mark = NULL;
		}
		
		indent_swift(level);
		char buf[BUFSIZ];
		raw("var %s: ", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));
		optionalType_swift(schema->superscope, attr_type, YES_FORCE_OPTIONAL, NOT_IN_COMMENT);
		raw(" { get }\n");
	}
}
static void selectSubtypeAttributeReference_swift(Schema schema, Type select_type, int level) {
	Dictionary all_attrs = SELECTget_all_attributes(select_type);
	DictionaryEntry de;
	Linked_List attr_defs;
//	const char* attr_name;
	
	char* mark = "//MARK: ENTITY ATTRIBUTE REFERENCES\n";

	DICTdo_init(all_attrs, &de);
	while ( 0 != (attr_defs = DICTdo(&de))) {
		const char* attr_name = DICT_key;
		if( SELECTget_attr_ambiguous_count(select_type, attr_name) > 1 ) continue;
		
		Type attr_type = SELECTfind_common_type(attr_defs);
		if( attr_type == NULL ) continue;
		
		if( mark ) {
			indent_swift(level);
			raw(mark);
			mark = NULL;
		}
		
		indent_swift(level);
		char attr_swift_name[BUFSIZ];
		raw("var %s: ", as_attributeSwiftName_n(attr_name, attr_swift_name, BUFSIZ));
		optionalType_swift(schema->superscope, attr_type, YES_FORCE_OPTIONAL, NOT_IN_COMMENT);

		raw(" { rep.%s }\n", attr_swift_name);
	}
}

//MARK: - constructor codes
static void selectTypeConstructor_swift(Type select_type,  int level) {
	
	TypeBody typeBody = TYPEget_body(select_type);
	char buf[BUFSIZ];

	indent_swift(level);
	raw("//MARK: - CONSTRUCTORS\n");
	
	/*
	 public init(fundamental: FundamentalType) { self = fundamental }
	 */
	indent_swift(level);
	raw("public init(fundamental: FundamentalType) { self = fundamental }\n\n");

	
	/*
	 public init?<T: SDAIUnderlyingType>(possiblyFrom underlyingType: T?){
	 	guard let underlyingType = underlyingType else { return nil }

	 	if let fundamental = underlyingType.asFundamentalType as? <selection>.FundamentalType {
	 		self = .<selection>( <selection>(fundamental) )
	 	}
	 	...
	 	else if let base = <selection>(possiblyFrom: underlyingType) {
	 		self = .<selection>(base)
	 	}
	 	else { return nil }
	 }

	 */
	
	indent_swift(level);
	raw("public init?<T: SDAIUnderlyingType>(possiblyFrom underlyingType: T?){\n");
	{	int level2 = level+nestingIndent_swift;
		indent_swift(level2);
		raw("guard let underlyingType = underlyingType else { return nil }\n\n");
		
		const char* ifhead = "if";
		bool emitted = false;

		LISTdo( typeBody->list, selection, Type ) {
			if( TYPEis_entity(selection) )continue;
			indent_swift(level2);
			raw("%s let base = underlyingType as? %s {\n",
					ifhead,TYPE_swiftName(selection, select_type->superscope, buf));
			{	int level3 = level2+nestingIndent_swift;
				indent_swift(level3);
				raw("self = .%s(base)\n",selectCase_swiftName(selection, buf));
			}
			indent_swift(level2);
			raw("}\n");

			ifhead = "else if";			
			emitted = true;
		}LISTod;
		
		LISTdo( typeBody->list, selection, Type ) {
			if( TYPEis_entity(selection) )continue;
			if( TYPEis_select(selection) ){
				indent_swift(level2);
				raw("%s let base = %s(possiblyFrom: underlyingType) {\n",
						ifhead,TYPE_swiftName(selection, select_type->superscope, buf));
				{	int level3 = level2+nestingIndent_swift;
					indent_swift(level3);
					raw("self = .%s(base)\n",selectCase_swiftName(selection, buf));
				}
				indent_swift(level2);
				raw("}\n");
				
				ifhead = "else if";			
				emitted = true;
			}
			else{
				indent_swift(level2);
				raw("%s let fundamental = underlyingType.asFundamentalType as? %s.FundamentalType {\n",
						ifhead,TYPE_swiftName(selection, select_type->superscope, buf));
				{	int level3 = level2+nestingIndent_swift;
					indent_swift(level3);
					raw("self = .%s( ",selectCase_swiftName(selection, buf));
					raw("%s(fundamental: fundamental) )\n",TYPE_swiftName(selection, select_type->superscope, buf));
				}
				indent_swift(level2);
				raw("}\n");
				
				ifhead = "else if";			
				emitted = true;
			}
		}LISTod;
		indent_swift(level2);
		if( emitted ){
			raw("else { return nil }\n");
		}
		else{
			raw("return nil\n");
		}
	}
	indent_swift(level);
	raw("}\n\n");
	
	
	/*
	 public init?(possiblyFrom complex: SDAI.ComplexEntity?) {
	 	guard let complex = complex else { return nil }
	 	if let base = complex.entityReference(<selection TYPE>.self) {
			self = .<selection>(base)
	 	}
	 else if let base = complex.entityReference(<selection TYPE>.self) {
		 self = .<selection>(base)
	 }
	 ...
	 else if let base = <selection>(possiblyFrom: complex) {
		 self = .<selection>(base)
	 }
	 else { rerurn nil }
	 }
	 */
	indent_swift(level);
	raw("public init?(possiblyFrom complex: SDAI.ComplexEntity?) {\n");
	{	int level2=level+nestingIndent_swift;
		indent_swift(level2);
		raw("guard let complex = complex else { return nil }\n\n");
		
		const char* ifhead = "if";
		bool emitted = false;
		
		LISTdo( typeBody->list, selection, Type ) {
			if( TYPEis_entity(selection) ){
				indent_swift(level2);
				raw("%s let base = complex.entityReference(%s.self) {", 
						ifhead, TYPE_swiftName(selection, select_type->superscope, buf));
				raw("self = .%s(base) }\n", selectCase_swiftName(selection, buf));
				
				ifhead = "else if";			
				emitted = true;
			}
			else if( TYPEis_select(selection) ){
				indent_swift(level2);
				raw("%s let base = %s(possiblyFrom: complex) {\n",
						ifhead,TYPE_swiftName(selection, select_type->superscope, buf));
				{	int level3 = level2+nestingIndent_swift;
					indent_swift(level3);
					raw("self = .%s(base)\n",selectCase_swiftName(selection, buf));
				}
				indent_swift(level2);
				raw("}\n");
				
				ifhead = "else if";			
				emitted = true;
			}

		} LISTod;		
		indent_swift(level2);
		if( emitted ){
			raw("else { return nil }\n");
		}
		else {
			raw("return nil\n");
		}
	}
	indent_swift(level);
	raw("}\n\n");
	

	/*
	 public init?<S: SDAISelectType>(possiblyFrom select: S?) {
	 guard let select = select else { return nil }
	 
	 	if let fundamental = select.asFundamentalType as? Self {
	 		self.init(fundamental: fundamental)
	 	}
		else if let base = <selection>(possiblyFrom: select) { 
	 		self = .<selection>(base)
	 }
		...
	 	else { return nil }
	 } 
	 	 */
	indent_swift(level);
	raw("public init?<S: SDAISelectType>(possiblyFrom select: S?) {\n");
	{	int level2=level+nestingIndent_swift;
		indent_swift(level2);
		raw("guard let select = select else { return nil }\n\n");
		
		indent_swift(level2);
		raw("if let fundamental = select.asFundamentalType as? Self {\n");
		indent_swift(level2+nestingIndent_swift);
		raw("self.init(fundamental: fundamental)\n");
		indent_swift(level2);
		raw("}\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("else if let base = %s(possiblyFrom: select) {\n", 
					TYPE_swiftName(selection, select_type->superscope, buf));
			{	int level3 = level2+nestingIndent_swift;
				indent_swift(level3);
				raw("self = .%s(base)\n",selectCase_swiftName(selection, buf));
			}
			indent_swift(level2);
			raw("}\n");
		} LISTod;		
		indent_swift(level2);
		raw("else { return nil }\n");
	}
	indent_swift(level);
	raw("}\n\n");
	
}

static void selectTypeConstructor_swiftProtocol(Schema schema, Type select_type,  int level) {
//	
//	TypeBody typeBody = TYPEget_body(select_type);
//	char buf[BUFSIZ];
//
//	indent_swift(level);
//	raw("//MARK: CONSTRUCTORS\n");
//
//	LISTdo( typeBody->list, selection, Type ) {
//		if( TYPEis_entity(selection) )continue;
//		if( TYPEis_select(selection) )continue;
//		
//		indent_swift(level);
//		raw("init(_ selectValue: %s)\n", TYPE_swiftName(selection, schema->superscope, buf));
//	} LISTod;		
//
}

static void selectSubtypeConstructor_swift(Schema schema, Type select_type,  int level) {
	
//	TypeBody typeBody = TYPEget_body(select_type);
//	char buf[BUFSIZ];

	indent_swift(level);
	raw("//MARK: CONSTRUCTORS\n");
	
	/*
	 init?(possiblyFrom complex: SDAI.ComplexEntity?) { 
	 	self.init(fundamental: FundamentalType(possiblyFrom: complex)) 
	 }
	 
	 init?<T: SDAIUnderlyingType>(possiblyFrom underlyingType: T?) { 
	 	self.init(fundamental: FundamentalType(possiblyFrom: underlyingType)) 
	 }
	 
	 init?<S: SDAISelectType>(possiblyFrom select: S?) { 
	 	self.init(fundamental: FundamentalType(possiblyFrom: select))
	 }
	*/
	
	indent_swift(level);
	raw("init?(possiblyFrom complex: SDAI.ComplexEntity?) {\n");
	indent_swift(level+nestingIndent_swift);
	raw("self.init(fundamental: FundamentalType(possiblyFrom: complex))\n");
	indent_swift(level);
	raw("}\n\n");

	indent_swift(level);
	raw("init?<T: SDAIUnderlyingType>(possiblyFrom underlyingType: T?) {\n");
	indent_swift(level+nestingIndent_swift);
	raw("self.init(fundamental: FundamentalType(possiblyFrom: underlyingType))\n");
	indent_swift(level);
	raw("}\n\n");

	
	indent_swift(level);
	raw("init?<S: SDAISelectType>(possiblyFrom select: S?) {\n");
	indent_swift(level+nestingIndent_swift);
	raw("self.init(fundamental: FundamentalType(possiblyFrom: select))\n");
	indent_swift(level);
	raw("}\n");

	
//	LISTdo( typeBody->list, selection, Type ) {
//		if( TYPEis_entity(selection) )continue;
//		if( TYPEis_select(selection) )continue;
//
//		indent_swift(level);
//		raw("init(_ selectValue: %s) { \n", TYPE_swiftName(selection, schema->superscope, buf));
//		
//		{	int level2 = level+nestingIndent_swift;
//			indent_swift(level2);
//			raw("self.init( Supertype(selectValue) ) }\n");
//		}
//	} LISTod;		
	
//	raw("\n");
//	indent_swift(level);
//	raw("init(_ entityRef: SDAI.EntityReference) { self.init( Supertype(entityRef) ) }\n");
//	
//	raw("\n");
//	indent_swift(level);
//	raw("init<S: SDAISelectType>(_ select: S) { self.init( Supertype(select) ) }\n");
}

//MARK: - value comparison code
static void selectTypeValueComparison_swift(Type select_type,  int level) {
	
	TypeBody typeBody = TYPEget_body(select_type);
	char buf[BUFSIZ];

	indent_swift(level);
	raw("//MARK: - SDAIValue\n");

	indent_swift(level);
	raw("public func isValueEqual<T: SDAIValue>(to rhs: T) -> Bool {\n");
	{	int level2 = level+nestingIndent_swift;
		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s(let selection): return selection.value.isValueEqual(to: rhs)\n", selectCase_swiftName(selection, buf));
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
		
	indent_swift(level);
	raw("}\n");
}

//MARK: - type members
static void selectTypeMembers_swift(Type select_type,  int level) {
	TypeBody typeBody = TYPEget_body(select_type);
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("// SDAIGenericType\n");
	indent_swift(level);
	raw("public var typeMembers: Set<SDAI.STRING> {\n");
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw("var members: Set<SDAI.STRING> = [SDAI.STRING(Self.typeName)]\n");

		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s(let selection): members.formUnion(selection.typeMembers)\n", selectCase_swiftName(selection, buf));
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
		indent_swift(level2);
		raw("return members\n");
	}
	indent_swift(level);
	raw( "}\n" );
}

//MARK: - entity reference
static void selectEntityReference_swift(Type select_type,  int level) {
	TypeBody typeBody = TYPEget_body(select_type);
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("// SDAISelectType\n");
	indent_swift(level);
	raw("public var entityReference: SDAI.EntityReference? {\n");
	{	int level2 = level+nestingIndent_swift;

		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s", selectCase_swiftName(selection, buf));

			if( TYPEis_entity(selection)){
				raw("(let selection): return selection\n");
			}
			else if( TYPEis_select(selection)){
				raw("(let selection): return selection.entityReference\n");
			}
			else {
				raw(": return nil\n");
			}
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw( "}\n" );
}

	
static void selectUnderlyingTypeConformance_swift(int level, Schema schema, const char *typename) {
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("// SDAIUnderlyingType\n");
	
	indent_swift(level);
	raw("public typealias FundamentalType = Self\n");
	
	indent_swift(level);
	raw("public static var typeName: String = ");
	wrap("\"%s.%s\"\n", SCHEMA_swiftName(schema, buf), typename);
	
	indent_swift(level);
	raw("public var asFundamentalType: FundamentalType { return self }\n");
	
//	indent_swift(level);
//	raw("public init(_ fundamental: FundamentalType) {\n");
//	indent_swift(level+nestingIndent_swift);
//	raw("self = fundamental\n");
//	indent_swift(level);
//	raw( "}\n" );
}

static void selectVarForwarding_swift(Type select_type, const char* var_name, const char* var_type, int level) {
	TypeBody typeBody = TYPEget_body(select_type);
	char buf[BUFSIZ];

	indent_swift(level);
	raw("public var %s: %s {\n", var_name, var_type);
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s(let selection): ", selectCase_swiftName(selection, buf));
			raw("return selection.%s\n", var_name);
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw("}\n");
}

static void selectAggregateTypeConformance_swift(Type select_type, Type aggregate_base, int level) {
	TypeBody typeBody = TYPEget_body(select_type);
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("// SDAIAggregationType\n");

	indent_swift(level);
	wrap("public typealias ELEMENT = %s\n", TYPE_swiftName(aggregate_base,select_type->superscope,buf));

	selectVarForwarding_swift(select_type, "hiBound", "Int?", level);
	selectVarForwarding_swift(select_type, "hiIndex", "Int",  level);
	selectVarForwarding_swift(select_type, "loBound", "Int",  level);
	selectVarForwarding_swift(select_type, "loIndex", "Int",  level);
	selectVarForwarding_swift(select_type, "size",    "Int",  level);
	
	raw("\n");
	indent_swift(level);
	raw("public subscript(index: Int?) -> ELEMENT? {\n");
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s(let selection): ", selectCase_swiftName(selection, buf));
			raw("return selection[index]\n");
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw("}\n");

	raw("\n");
	indent_swift(level);
	raw("public func makeIterator() -> AnyIterator<ELEMENT?> {\n");
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s(let selection): ", selectCase_swiftName(selection, buf));
			raw("return AnyIterator<ELEMENT?>(selection.makeIterator())\n");
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw("}\n");

	raw("\n");
	indent_swift(level);
	raw("public func CONTAINS(_ elem: ELEMENT?) -> SDAI.LOGICAL {\n");
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s(let selection): ", selectCase_swiftName(selection, buf));
			raw("return selection.CONTAINS(elem)\n");
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw("}\n");

	raw("\n");
	indent_swift(level);
	wrap("public typealias RESULT_AGGREGATE = SDAI.BAG<ELEMENT>\n");
	indent_swift(level);
	raw("public func QUERY(logical_expression: (ELEMENT) -> SDAI.LOGICAL ) -> RESULT_AGGREGATE {\n");
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s(let selection): \n", selectCase_swiftName(selection, buf));
			{	int level3 = level2+nestingIndent_swift;
				indent_swift(level3);
				raw("let result = RESULT_AGGREGATE.SwiftType( selection.QUERY(logical_expression:logical_expression) )\n");
				indent_swift(level3);
				raw("return RESULT_AGGREGATE(from:result)\n");
			}
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw("}\n");

	raw("\n");
	indent_swift(level);
	raw("public var _observer: EntityReferenceObserver? {\n");
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw("get {retutn nil}\n");
		indent_swift(level2);
		raw("set {}\n");
	}
	indent_swift(level);
	raw("}\n");

}

//MARK: - main entry points	
void selectTypeDefinition_swift(Schema schema, Type select_type,  int level) {
	
	listAllSelectionAttributes(select_type, level);
	TypeBody typeBody = TYPEget_body(select_type);
	Type common_aggregate_base = TYPE_retrieve_aggregate_base(select_type, NULL);

	// swift enum definition
	{
		char buf[BUFSIZ];
				
		char typebuf[BUFSIZ];
		const char* typename = TYPE_swiftName(select_type,select_type->superscope,typebuf);
		indent_swift(level);
		wrap( "public enum %s : SDAIValue, ", typename );
		positively_wrap();
		raw(  "%s__", SCHEMA_swiftName(schema, buf) );
		raw(  "%s__type {\n", typename );
		
		{	int level2 = level+nestingIndent_swift;
			
			LISTdo( typeBody->list, selection, Type ) {
				indent_swift(level2);
				raw( "case %s(", selectCase_swiftName(selection, buf) );
				wrap( "%s)", TYPE_swiftName(selection, select_type->superscope, buf) );
				if( TYPEis_entity(selection) ) {
					raw( "\t// %s\n", "(ENTITY)" );
				}
				else if( TYPEis_select(selection) ) {
					raw( "\t// %s\n", "(SELECT)" );
				}
				else if( TYPEis_enumeration(selection) ) {
					raw( "\t// %s\n", "(ENUM)" );
				}
				else {
					raw( "\t// %s\n", "(TYPE)" );
				}
			} LISTod;		
			raw("\n");
			
			selectTypeConstructor_swift(select_type, level2);
			raw("\n");
			
			selectTypeGroupReference_swift(select_type, level2);
			raw("\n");

			selectTypeAttributeReference_swift(select_type, level2);		
			raw("\n");
			
			selectTypeValueComparison_swift(select_type, level2);
			raw("\n");

			selectTypeMembers_swift(select_type, level2);
			raw("\n");
			 
			selectUnderlyingTypeConformance_swift(level2, schema, typename);
			raw("\n");

			selectEntityReference_swift(select_type, level2);
			
			if( common_aggregate_base != NULL ){
				raw("\n");
				selectAggregateTypeConformance_swift(select_type, common_aggregate_base, level2);				
			}
		}
	}
	
	indent_swift(level);
	raw( "}\n\n" );
	
}

void selectTypeExtension_swift(Schema schema, Type select_type,  int level) {
	char typebuf[BUFSIZ];
	const char* typename = TYPE_swiftName(select_type,select_type->superscope,typebuf);

	char schemabuf[BUFSIZ];
	const char* schemaname = SCHEMA_swiftName(schema, schemabuf);
	Type common_aggregate_base = TYPE_retrieve_aggregate_base(select_type, NULL);

	raw("\n\n//MARK: - SELECT TYPE HIERARCHY\n");
	//type protocol
	indent_swift(level);
	raw( "public protocol %s__%s__type: ", schemaname, typename);
	wrap("SDAISelectType");
	if( common_aggregate_base != NULL ){
		wrap(", SDAIAggregationType ");
//	}
//	if( common_aggregate_base != NULL ){
		if( common_aggregate_base == Type_Entity ){
			wrap("where ELEMENT == SDAI.EntityReference");
		}
		else {
			char buf[BUFSIZ];
			wrap("where ELEMENT == %s", TYPE_swiftName(common_aggregate_base,schema->superscope,buf));
		}
	}
	
	raw(" {\n");
	{	int level2 = level+nestingIndent_swift;
		
		selectTypeConstructor_swiftProtocol(schema, select_type, level2);
		raw("\n");
		
		selectTypeGroupReference_swiftProtocol(schema, select_type, level2);
		raw("\n");

		selectTypeAttributeReference_swiftProtocol(schema, select_type, level2);				
	}
	
	indent_swift(level);
	raw("}\n\n");
	
	// subtype protocol
	indent_swift(level);
	raw( "public protocol %s__%s__subtype: ", schemaname, typename );
	wrap("%s__%s__type, SDAIDefinedType\n", schemaname, typename );
	indent_swift(level);
	wrap("where Supertype: %s__%s__type\n", schemaname, typename);
//	force_wrap();
//	indent_swift(level);
//	raw("where ");
////	int oldindent = captureWrapIndent();
//	raw("Supertype == %s.%s\n", schemaname, typename);
//	force_wrap();
//	wrap( "Supertype.FundamentalType == %s.%s\n", schemaname, typename);
//	restoreWrapIndent(oldindent);
	indent_swift(level);
	raw( "{}\n\n");
	
	// subtype protocol extension
	indent_swift(level);
	raw( "public extension %s__%s__subtype {\n", schemaname, typename );

	{	int level2 = level+nestingIndent_swift;
		selectSubtypeConstructor_swift(schema, select_type, level2);
		raw("\n");
		
		selectSubtypeGroupReference_swift(schema, select_type, level2);
		raw("\n");

		selectSubtypeAttributeReference_swift(schema, select_type, level2);				
	}
	
	indent_swift(level);
	raw( "}\n\n");
}
