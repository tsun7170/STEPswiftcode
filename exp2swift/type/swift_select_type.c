//
//  swift_select_type.c
//  exp2swift
//
//  Created by Yoshida on 2020/09/03.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
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

//MARK: - underlying reference codes
static void selectTypeGroupReference_swift(Type select_type, int level) {
	char buf[BUFSIZ];
	
	TypeBody typeBody = TYPEget_body(select_type);
	int selection_count = LISTget_length(typeBody->list);
	
	char* mark = "//MARK: - NON-ENTITY UNDERLYING TYPE REFERENCES\n";
	
	int num_selections = LISTget_length(typeBody->list);
	
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

			if( num_selections > 1 ){
				indent_swift(level2);
				raw("default: return nil\n");
			}
			
			indent_swift(level2);				
			raw("}\n");
		}
		
		indent_swift(level);
		raw("}\n\n");
	} LISTod;				
	
	mark = "//MARK: - ENTITY UNDERLYING TYPE REFERENCES\n";
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
		char buf[BUFSIZ];
		const char* attr_name = DICT_key;
		if( SELECTget_attr_ambiguous_count(select_type, attr_name) > 1 ) {
			indent_swift(level);
			raw("//MARK: var %s: (AMBIGUOUS)\n\n", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));			
			continue;
		}
		
		if( mark ) {
			indent_swift(level);
			raw(mark);
			mark = NULL;
		}
		
		Type attr_type = SELECTfind_common_type(attr_defs);
		if( attr_type == NULL ) {
			indent_swift(level);
			raw("//MARK: var %s: (NO COMMON TYPE)\n\n", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));			
			continue;
		}

		// markdown
		indent_swift(level);
		raw("/// attribute of SELECT type ``%s``\n", TYPE_swiftName(select_type, select_type->superscope, buf));
		LISTdo(typebody->list, selection_case, Type){
			if( TYPEis_entity(selection_case) ) {
				Entity case_entity = TYPEget_body(selection_case)->entity;
				Variable base_attr = ENTITYfind_attribute_effective_definition(case_entity, attr_name);
				if( base_attr != NULL ){
					indent_swift(level);
					raw("/// - origin: ENTITY( ``%s`` )\n", ENTITY_swiftName(case_entity, NO_QUALIFICATION, buf));
				}
			}
			else if( TYPEis_select(selection_case) ) {
				Linked_List base_attr_defs = DICTlookup(SELECTget_all_attributes(selection_case), attr_name);
				Type base_attr_type = base_attr_defs!=NULL ? SELECTfind_common_type(base_attr_defs) : NULL;
				if( SELECTget_attr_ambiguous_count(selection_case, attr_name) > 1 ) continue;
				if(base_attr_type != NULL ){
					indent_swift(level);
					raw("/// - origin: SELECT( ``%s`` )\n", TYPE_swiftName(selection_case, NO_QUALIFICATION, buf));					
				}
			}				
		}LISTod;

		indent_swift(level);
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
						
						if( TYPEis_swiftAssignable(attr_type, base_attr->type) ){
							wrap("entity.%s\n", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));
						}
						else {
							TYPE_head_swift(NULL, attr_type, WO_COMMENT, LEAF_OWNED);
							raw("(");
							wrap("entity.%s)\n", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));
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
					Linked_List base_attr_defs = DICTlookup(SELECTget_all_attributes(selection_case), attr_name);
					Type base_attr_type = base_attr_defs!=NULL ? SELECTfind_common_type(base_attr_defs) : NULL;
					
					if( SELECTget_attr_ambiguous_count(selection_case, attr_name) > 1 ){
						indent_swift(level2);
						--unhandled;
						raw("case .%s/*(let select)*/: return nil // AMBIGUOUS ATTRIBUTE ",selectCase_swiftName(selection_case, buf));
						raw("for %s\n", TYPE_swiftName(selection_case, NO_QUALIFICATION, buf));
					}
					else if(base_attr_type != NULL ){
						indent_swift(level2);
						--unhandled;
						raw("case .%s(let select): return ",selectCase_swiftName(selection_case, buf));
						
						if( TYPEis_swiftAssignable(attr_type, base_attr_type) ){
							wrap("select.%s\n", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));
						}
						else if( TYPEis_free_generic(base_attr_type) ){
							TYPE_head_swift(NULL, attr_type, WO_COMMENT, LEAF_OWNED);
							raw(".convert(fromGeneric: ");
							wrap("select.%s)\n", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));
						}
						else {
							TYPE_head_swift(NULL, attr_type, WO_COMMENT, LEAF_OWNED);
							raw("(");
							wrap("select.%s)\n", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));
						}
					}
				}				
			}LISTod;
						
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
		
		const char* ifhead = "if";
		bool emitted = false;

		LISTdo( typeBody->list, selection, Type ) {
			if( TYPEis_entity(selection) )continue;

			if( !emitted ){
				indent_swift(level2);
				raw("guard let underlyingType = underlyingType else { return nil }\n\n");
			}			

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

			if( !emitted ){
				indent_swift(level2);
				raw("guard let underlyingType = underlyingType else { return nil }\n\n");
			}			

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
				if( !emitted ){
				 indent_swift(level2);
				 raw("guard let underlyingType = underlyingType else { return nil }\n\n");
			 }			

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
		
		const char* ifhead = "if";
		bool emitted = false;
		
		LISTdo( typeBody->list, selection, Type ) {
			if( TYPEis_entity(selection) ){
				if(!emitted) {
					indent_swift(level2);
					raw("guard let complex = complex else { return nil }\n\n");
				}
				
				indent_swift(level2);
				raw("%s let base = complex.entityReference(%s.self) {", 
						ifhead, TYPE_swiftName(selection, select_type->superscope, buf));
				raw("self = .%s(base) }\n", selectCase_swiftName(selection, buf));
				
				ifhead = "else if";			
				emitted = true;
			}
			else if( TYPEis_select(selection) ){
				if(!emitted) {
					indent_swift(level2);
					raw("guard let complex = complex else { return nil }\n\n");
				}
				
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
	 public init?<G: SDAIGenericType>(fromGeneric generic: G?) {
	 guard let select = generic else { return nil }
	 
	 	if let fundamental = select as? Self {
	 		self.init(fundamental: fundamental)
	 	}
		else if let base = <selection>.convert(fromGeneric: select) { 
	 		self = .<selection>(base)
	 }
		...
	 	else { return nil }
	 } 
	 	 */
	indent_swift(level);
	raw("public init?<G: SDAIGenericType>(fromGeneric generic: G?) {\n");
	{	int level2=level+nestingIndent_swift;
		indent_swift(level2);
		raw("guard let select = generic else { return nil }\n\n");
		
		indent_swift(level2);
		raw("if let fundamental = select as? Self {\n");
		indent_swift(level2+nestingIndent_swift);
		raw("self.init(fundamental: fundamental)\n");
		indent_swift(level2);
		raw("}\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("else if let base = %s.convert(fromGeneric: select) {\n", 
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
	
	
	indent_swift(level);
	raw("// InitializableByP21Parameter\n");
	
	indent_swift(level);
	raw("public static var bareTypeName: String = ");
	wrap("\"%s\"\n\n", TYPE_canonicalName(select_type,NO_QUALIFICATION,buf));

	/*
	public init?(p21typedParam: P21Decode.ExchangeStructure.TypedParameter, from exchangeStructure: P21Decode.ExchangeStructure) {
		guard let keyword = p21typedParam.keyword.asStandardKeyword else { exchangeStructure.error = "unexpected p21parameter(\(p21typedParam)) while resolving \(Self.bareTypeName) select value"; return nil }
		
		switch(keyword) {
			case <selection>.bareTypeName:
				guard let base = <selection>(p21typedParam: p21typedParam, from: exchangeStructure) else { exchangeStructure.add(errorContext: "while resolving \(Self.bareTypeName) select value"); return nil }
				self = .<selection>(base)
				
				...
			default:
				exchangeStructure.error = "unexpected p21parameter(\(p21typedParam)) while resolving \(Self.bareTypeName) select value"
				return nil
		}		
	}
	*/
	indent_swift(level);
	raw("public init?(p21typedParam: P21Decode.ExchangeStructure.TypedParameter, from exchangeStructure: P21Decode.ExchangeStructure) {\n");
	{	int level2 = level + nestingIndent_swift; int level3 = level2 + nestingIndent_swift;
		
		indent_swift(level2);
		raw("guard let keyword = p21typedParam.keyword.asStandardKeyword else { exchangeStructure.error = \"unexpected p21parameter(\\(p21typedParam)) while resolving \\(Self.bareTypeName) select value\"; return nil }\n\n");
		
		indent_swift(level2);
		raw("switch(keyword) {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			if( TYPEis_entity(selection) )continue;

			indent_swift(level2);
			raw("case %s.bareTypeName:\n",TYPE_swiftName(selection, select_type->superscope, buf));
			indent_swift(level3);
			raw("guard let base = %s(p21typedParam: p21typedParam, from: exchangeStructure) else { exchangeStructure.add(errorContext: \"while resolving \\(Self.bareTypeName) select value\"); return nil }\n", buf);
			indent_swift(level3);
			raw("self = .%s(base)\n\n",selectCase_swiftName(selection, buf));
		}LISTod;

		indent_swift(level2);
		raw("default:\n");
		indent_swift(level3);
		raw("exchangeStructure.error = \"unexpected p21parameter(\\(p21typedParam)) while resolving \\(Self.bareTypeName) select value\"\n");
		indent_swift(level3);
		raw("return nil\n");
		
		indent_swift(level2);
		raw("}\n");		
	}
	indent_swift(level);
	raw("}\n\n");
	
	/*
	public init?(p21untypedParam: P21Decode.ExchangeStructure.UntypedParameter, from exchangeStructure: P21Decode.ExchangeStructure) {
		switch p21untypedParam {
		case .rhsOccurenceName(let rhsname):
			switch rhsname {
			case .constantEntityName(let name):
				guard let entity = exchangeStructure.resolve(constantEntityName: name) else {exchangeStructure.add(errorContext: "while resolving \(Self.bareTypeName) instance"); return nil }
				self.init(possiblyFrom: entity.complexEntity)
				
			case .entityInstanceName(let name):
				guard let complex = exchangeStructure.resolve(entityInstanceName: name) else {exchangeStructure.add(errorContext: "while resolving \(Self.bareTypeName) instance"); return nil }
				self.init(possiblyFrom: complex)
			
			default:
				exchangeStructure.error = "unexpected p21parameter(\(p21untypedParam)) while resolving \(Self.bareTypeName) select instance"
				return nil
			}
						
		default:
			exchangeStructure.error = "unexpected p21parameter(\(p21untypedParam)) while resolving \(Self.bareTypeName) select instance"
			return nil
		}
	}
	*/
	indent_swift(level);
	raw("public init?(p21untypedParam: P21Decode.ExchangeStructure.UntypedParameter, from exchangeStructure: P21Decode.ExchangeStructure) {\n");
	{	int level2 = level + nestingIndent_swift; int level3 = level2 + nestingIndent_swift; int level4 = level3 + nestingIndent_swift;
		
		indent_swift(level2);
		raw("switch p21untypedParam {\n");
		indent_swift(level2);
		raw("case .rhsOccurenceName(let rhsname):\n");
		indent_swift(level3);
		raw("switch rhsname {\n");
		indent_swift(level3);
		raw("case .constantEntityName(let name):\n");
		indent_swift(level4);
		raw("guard let entity = exchangeStructure.resolve(constantEntityName: name) else {exchangeStructure.add(errorContext: \"while resolving \\(Self.bareTypeName) instance\"); return nil }\n");
		indent_swift(level4);
		raw("self.init(possiblyFrom: entity.complexEntity)\n\n");
		
		indent_swift(level3);
		raw("case .entityInstanceName(let name):\n");
		indent_swift(level4);
		raw("guard let complex = exchangeStructure.resolve(entityInstanceName: name) else {exchangeStructure.add(errorContext: \"while resolving \\(Self.bareTypeName) instance\"); return nil }\n");
		indent_swift(level4);
		raw("self.init(possiblyFrom: complex)\n\n");
		
		indent_swift(level3);
		raw("default:\n");
		indent_swift(level4);
		raw("exchangeStructure.error = \"unexpected p21parameter(\\(p21untypedParam)) while resolving \\(Self.bareTypeName) select instance\"\n");
		indent_swift(level4);
		raw("return nil\n");
		indent_swift(level3);
		raw("}\n\n");
		
		indent_swift(level2);
		raw("default:\n");
		indent_swift(level3);
		raw("exchangeStructure.error = \"unexpected p21parameter(\\(p21untypedParam)) while resolving \\(Self.bareTypeName) select instance\"\n");
		indent_swift(level3);
		raw("return nil\n");
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw("}\n\n");

	/*
	public init?(p21omittedParamfrom exchangeStructure: P21Decode.ExchangeStructure) {
		return nil
	}
	*/
	indent_swift(level);
	raw("public init?(p21omittedParamfrom exchangeStructure: P21Decode.ExchangeStructure) {\n");
	{	int level2 = level + nestingIndent_swift;

		indent_swift(level2);
		raw("return nil\n");
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
	 
	 init?<G: SDAIGenericType>(fromGeneric generic: G?) { 
	 	self.init(fundamental: FundamentalType.convert(fromGeneric: generic))
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
	raw("init?<G: SDAIGenericType>(fromGeneric generic: G?) {\n");
	indent_swift(level+nestingIndent_swift);
	raw("self.init(fundamental: FundamentalType.convert(fromGeneric: generic))\n");
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

	/////////
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
	raw("}\n\n");


	/////////
	indent_swift(level);
	raw("public func isValueEqualOptionally<T: SDAIValue>(to rhs: T?) -> Bool? {\n");
	{	int level2 = level+nestingIndent_swift;
		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s(let selection): return selection.value.isValueEqualOptionally(to: rhs)\n", selectCase_swiftName(selection, buf));
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
		
	indent_swift(level);
	raw("}\n\n");

	
	/////////
	indent_swift(level);
	raw("public func hashAsValue(into hasher: inout Hasher, visited complexEntities: inout Set<SDAI.ComplexEntity>) {\n");
	{	int level2 = level+nestingIndent_swift;
		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s(let selection): selection.value.hashAsValue(into: &hasher, visited: &complexEntities)\n", selectCase_swiftName(selection, buf));
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
		
	indent_swift(level);
	raw("}\n\n");
	
	/////////
	indent_swift(level);
	raw("public func isValueEqual<T: SDAIValue>(to rhs: T, visited comppairs: inout Set<SDAI.ComplexPair>) -> Bool {\n");
	{	int level2 = level+nestingIndent_swift;
		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s(let selection): return selection.value.isValueEqual(to: rhs, visited: &comppairs)\n", selectCase_swiftName(selection, buf));
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
		
	indent_swift(level);
	raw("}\n\n");
	
	/////////
	indent_swift(level);
	raw("public func isValueEqualOptionally<T: SDAIValue>(to rhs: T?, visited comppairs: inout Set<SDAI.ComplexPair>) -> Bool? {\n");
	{	int level2 = level+nestingIndent_swift;
		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s(let selection): return selection.value.isValueEqualOptionally(to: rhs, visited: &comppairs)\n", selectCase_swiftName(selection, buf));
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
		
	indent_swift(level);
	raw("}\n\n");

	
}

//MARK: - type members
static void selectTypeMembers_swift(Type select_type,  int level) {
	TypeBody typeBody = TYPEget_body(select_type);
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("//MARK: SDAIGenericType\n");
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

//MARK: - value conversions
static void selectSimpleValueConversion_swift(Type select_type, const char* varname, const char* vartype, int level) {
	TypeBody typeBody = TYPEget_body(select_type);
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("public var %s: %s? {\n", varname, vartype);
	{	int level2 = level+nestingIndent_swift;
		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s(let selection): return selection.%s\n", selectCase_swiftName(selection, buf), varname);
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw( "}\n" );
}

static void selectAggregateValueConversion_swift(Type select_type, const char* varname, const char* vartype, int level) {
	TypeBody typeBody = TYPEget_body(select_type);
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("public func %s<ELEM:SDAIGenericType>(elementType:ELEM.Type) -> %s<ELEM>? {\n", varname, vartype);
	{	int level2 = level+nestingIndent_swift;
		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s(let selection): return selection.%s(elementType:elementType)\n", selectCase_swiftName(selection, buf), varname);
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw( "}\n" );
}

static void selectEnumValueConversion_swift(Type select_type, int level) {
	TypeBody typeBody = TYPEget_body(select_type);
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("public func enumValue<ENUM:SDAIEnumerationType>(enumType:ENUM.Type) -> ENUM? {\n");
	{	int level2 = level+nestingIndent_swift;
		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s(let selection): return selection.enumValue(enumType:enumType)\n", selectCase_swiftName(selection, buf));
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw( "}\n" );
}

//MARK: - SDAIObservableAggregateElement
static void selectObservableAggregateElementConformance_swift(Type select_type,  int level) {
	int level2 = level+nestingIndent_swift;
	int level3 = level2+nestingIndent_swift;
	
	TypeBody typeBody = TYPEget_body(select_type);
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("//MARK: SDAIObservableAggregateElement\n");
	
	// entityReferences
	indent_swift(level);
	raw("public var entityReferences: AnySequence<SDAI.EntityReference> {\n");
	{	
		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s", selectCase_swiftName(selection, buf));

			if( TYPEis_entity(selection)){
				raw("(let entity): return entity.entityReferences\n");
			}
			else if( TYPEis_select(selection)){
				raw("(let select): return select.entityReferences\n");
			}
			else if( TYPEis_observable_aggregate(selection)){
				raw("(let observable): return observable.entityReferences\n");
			}
			else {
				raw(": return AnySequence<SDAI.EntityReference>(EmptyCollection<SDAI.EntityReference>())\n");
			}
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw( "}\n\n" );
	
	// configure()
	indent_swift(level);
	raw("public mutating func configure(with observer: SDAI.EntityReferenceObserver) {\n");
	{
		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s", selectCase_swiftName(selection, buf));

			if( TYPEis_entity(selection)){
				raw("(let entity): \n");
				
				indent_swift(level3);
				raw("entity.configure(with: observer)\n");
				indent_swift(level3);
				raw("self = .%s(entity)\n", buf);
			}
			else if( TYPEis_select(selection)){
				raw("(var select): \n");
				
				indent_swift(level3);
				raw("select.configure(with: observer)\n");
				indent_swift(level3);
				raw("self = .%s(select)\n", buf);
			}
			else if( TYPEis_observable_aggregate(selection)){
				raw("(var observable): \n");
				
				indent_swift(level3);
				raw("observable.configure(with: observer)\n");
				indent_swift(level3);
				raw("self = .%s(observable)\n", buf);
			}
			else {
//				indent_swift(level3);
				raw(": break\n", buf);
			}
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw( "}\n\n" );

	// teardownObserver()
	indent_swift(level);
	raw("public mutating func teardownObserver() {\n");
	{
		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s", selectCase_swiftName(selection, buf));

			if( TYPEis_entity(selection)){
				raw("(let entity): \n");
				
				indent_swift(level3);
				raw("entity.teardownObserver()\n");
				indent_swift(level3);
				raw("self = .%s(entity)\n", buf);
			}
			else if( TYPEis_select(selection)){
				raw("(var select): \n");
				
				indent_swift(level3);
				raw("select.teardownObserver()\n");
				indent_swift(level3);
				raw("self = .%s(select)\n", buf);
			}
			else if( TYPEis_observable_aggregate(selection)){
				raw("(var observable): \n");
				
				indent_swift(level3);
				raw("observable.teardownObserver()\n");
				indent_swift(level3);
				raw("self = .%s(observable)\n", buf);
			}
			else {
//				indent_swift(level3);
				raw(": break\n", buf);
			}
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw( "}\n" );
	
}

//MARK: - SDAIGenericTypeBase
static void selectGenericTypeBaseConformance_swift( Type select_type, int level) {
	int level2 = level+nestingIndent_swift;
	char buf[BUFSIZ];

	TypeBody typeBody = TYPEget_body(select_type);

	indent_swift(level);
	raw("//MARK: SDAIGenericTypeBase\n");

	indent_swift(level);
	raw("public func copy() -> Self {\n");
	{
		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s(let selection): ", selectCase_swiftName(selection, buf));
			raw("return .%s(selection.copy())\n", buf);
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw("}\n");
	
	
}

//MARK: - SDAIUnderlyingType
static void selectUnderlyingTypeConformance_swift(int level, Schema schema, Type select_type) {
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("//MARK: SDAIUnderlyingType\n");
	
	indent_swift(level);
	raw("public typealias FundamentalType = Self\n");
	
	indent_swift(level);
	raw("public static var typeName: String = ");
	wrap("\"%s\"\n", TYPE_canonicalName(select_type,schema->superscope,buf));
	
	indent_swift(level);
	raw("public var asFundamentalType: FundamentalType { return self }\n");
}

//MARK: - aggregation type conformance

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

static void selectAggregationBehaviorForwarding_swift(Type select_type, const char* var_name, const char* var_type, const char* base_attr_name, int level) {
	TypeBody typeBody = TYPEget_body(select_type);
	char buf[BUFSIZ];

	indent_swift(level);
	raw("public var %s: %s {\n", var_name, var_type);
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw("switch self {\n");
		
		bool exhausted = true;
		LISTdo( typeBody->list, selection, Type ) {
			if( TYPEis_aggregation_data_type(selection) ){
				indent_swift(level2);
				raw("case .%s(let aggregate): ", selectCase_swiftName(selection, buf));
				raw("return aggregate.%s\n", base_attr_name);
			}
			else if( TYPEis_select(selection) ){
				indent_swift(level2);
				raw("case .%s(let selection): ", selectCase_swiftName(selection, buf));
				raw("return selection.%s\n", var_name);
			}
			else {
				exhausted = false;				
			}
		} LISTod;		
		
		if( !exhausted ){
			indent_swift(level2);
			raw("default: return nil\n");
		}
		
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw("}\n");
}


static void selectAggregationBehaviorConformance_swift(Type select_type, int level) {
	indent_swift(level);
	raw("//MARK: SDAIAggregationBehavior\n");
	selectAggregationBehaviorForwarding_swift(select_type, "aggregationHiBound", "Int?", "hiBound", level);
	selectAggregationBehaviorForwarding_swift(select_type, "aggregationHiIndex", "Int?", "hiIndex", level);
	selectAggregationBehaviorForwarding_swift(select_type, "aggregationLoBound", "Int?", "loBound", level);
	selectAggregationBehaviorForwarding_swift(select_type, "aggregationLoIndex", "Int?", "loIndex", level);
	selectAggregationBehaviorForwarding_swift(select_type, "aggregationSize", "Int?", 	 "size", 		level);
}

static void selectAggregateTypeConformance_swift(Type select_type, Type aggregate_base, int level) {
	TypeBody typeBody = TYPEget_body(select_type);
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("//MARK: SDAIAggregationType\n");

	indent_swift(level);
	wrap("public typealias ELEMENT = %s\n", TYPEhead_string_swift(select_type->superscope, aggregate_base, NOT_IN_COMMENT, LEAF_OWNED, buf));

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
			wrap("return AnyIterator<ELEMENT?>(selection.lazy.map({SDAI.FORCE_OPTIONAL($0)}).makeIterator())\n");
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw("}\n");

	raw("\n");
	indent_swift(level);
	raw("public func CONTAINS(elem: ELEMENT?) -> SDAI.LOGICAL {\n");
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
	raw("public typealias RESULT_AGGREGATE = SDAI.BAG<ELEMENT>\n");
	indent_swift(level);
	raw("public func QUERY(logical_expression: @escaping (ELEMENT) -> SDAI.LOGICAL ) -> RESULT_AGGREGATE {\n");
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
	raw("public var observer: SDAI.EntityReferenceObserver? {\n");
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw("get {return nil}\n");
		indent_swift(level2);
		raw("set {}\n");
	}
	indent_swift(level);
	raw("}\n");

	
	raw("\n");
	indent_swift(level);
	raw("// SDAIAggregationSequence\n");
	indent_swift(level);
	raw("public var asAggregationSequence: AnySequence<ELEMENT> {\n");
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw("switch self {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s(let selection): \n", selectCase_swiftName(selection, buf));
			{	int level3 = level2+nestingIndent_swift;
				indent_swift(level3);
				raw("return selection.asAggregationSequence\n");
			}
		} LISTod;		
		
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw("}\n");

}

//MARK: - WHERE rule validation
static void SELECTwhereRuleValidation_swift( Type select_type, int level ) {
	char typename[BUFSIZ]; TYPE_swiftName(select_type, select_type->superscope, typename);

	raw("\n");
	indent_swift(level);
	raw("//MARK: WHERE RULE VALIDATION (SELECT TYPE)\n");

	indent_swift(level);
	raw("public static func validateWhereRules(instance:Self?, prefix:SDAI.WhereLabel) -> [SDAI.WhereLabel:SDAI.LOGICAL] {\n");
	
	{	int level2 = level+nestingIndent_swift;
		char buf[BUFSIZ];
		
		indent_swift(level2);
		raw("var result: [SDAI.WhereLabel:SDAI.LOGICAL] = [:]\n");

		// underlying case type validation
		indent_swift(level2);
		raw("switch instance {\n");
		LISTdo( TYPEget_body(select_type)->list, selection, Type ) {			
			indent_swift(level2);
			raw("case .%s(let selectValue): ",selectCase_swiftName(selection, buf));
			wrap("result = %s.validateWhereRules(instance:selectValue, ", 
					 TYPE_swiftName(selection, select_type->superscope, buf)
					 );
			wrap("prefix:prefix + \"\\\\%s\")\n", 
					 TYPE_canonicalName(selection,select_type->superscope,buf)
					 );
		} LISTod;				
		
		indent_swift(level2);
		raw("case nil: break\n");
		indent_swift(level2);
		raw("}\n\n");

		// selftype where validations
		Linked_List where_rules = TYPEget_where(select_type);
		LISTdo( where_rules, where, Where ){
			char whereLabel[BUFSIZ];
			indent_swift(level2);
			raw("result[prefix + \".%s\"] = %s.%s(SELF: instance)\n", 
					whereRuleLabel_swiftName(where, whereLabel), typename, whereLabel);
		}LISTod;

		indent_swift(level2);
		raw("return result\n");		
	}
	
	indent_swift(level);
	raw("}\n\n");
	
}

//MARK: - main entry points	
void selectTypeDefinition_swift(Schema schema, Type select_type,  int level) {
	
	listAllSelectionAttributes(select_type, level);
	TypeBody typeBody = TYPEget_body(select_type);
	Type common_aggregate_base = TYPE_retrieve_aggregate_base(select_type, NULL);

	// markdown
	raw("\n/** SELECT type\n");
	raw("- EXPRESS:\n");
	raw("```express\n");
	TYPE_out(select_type, level);
	raw("\n```\n");
	raw("*/\n");
	
	// swift enum definition
		char buf[BUFSIZ];
				
		char typename[BUFSIZ];
		TYPE_swiftName(select_type,select_type->superscope,typename);
	
		indent_swift(level);
		wrap( "public enum %s : SDAIValue, ", typename );
		positively_wrap();
		raw(  "%s__", SCHEMA_swiftName(schema, buf) );
		raw(  "%s__type {\n", typename );
		
		{	int level2 = level+nestingIndent_swift;
			
			LISTdo( typeBody->list, selection, Type ) {
				const char* case_kind;
				if( TYPEis_entity(selection) ) {
					case_kind = "ENTITY";
				}
				else if( TYPEis_select(selection) ) {
					case_kind = "SELECT";
				}
				else if( TYPEis_enumeration(selection) ) {
					case_kind = "ENUM";
				}
				else {
					case_kind = "TYPE";
				}
				
				
				// markdown
				raw("\n");
				indent_swift(level2);
				raw("/// SELECT case ``%s`` (%s) in ``%s``\n",
						TYPE_swiftName(selection, select_type->superscope, buf),
						case_kind,
						typename );
				
				indent_swift(level2);
				raw( "case %s(", selectCase_swiftName(selection, buf) );
				wrap( "%s)\t// (%s)\n", 
						 TYPE_swiftName(selection, select_type->superscope, buf),
						 case_kind );
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

			// SDAIGenericTypeBase
			selectGenericTypeBaseConformance_swift(select_type, level2);
			raw("\n");

			// SDAIGenericType
			selectTypeMembers_swift(select_type, level2);
			raw("\n");

			selectSimpleValueConversion_swift(select_type, "entityReference", "SDAI.EntityReference", level2);
			selectSimpleValueConversion_swift(select_type, "stringValue", "SDAI.STRING", level2);
			selectSimpleValueConversion_swift(select_type, "binaryValue", "SDAI.BINARY", level2);
			selectSimpleValueConversion_swift(select_type, "logicalValue", "SDAI.LOGICAL", level2);
			selectSimpleValueConversion_swift(select_type, "booleanValue", "SDAI.BOOLEAN", level2);
			selectSimpleValueConversion_swift(select_type, "numberValue", "SDAI.NUMBER", level2);
			selectSimpleValueConversion_swift(select_type, "realValue", "SDAI.REAL", level2);
			selectSimpleValueConversion_swift(select_type, "integerValue", "SDAI.INTEGER", level2);
			selectSimpleValueConversion_swift(select_type, "genericEnumValue", "SDAI.GenericEnumValue", level2);
			
			selectAggregateValueConversion_swift(select_type, "arrayOptionalValue", "SDAI.ARRAY_OPTIONAL", level2);
			selectAggregateValueConversion_swift(select_type, "arrayValue", "SDAI.ARRAY", level2);
			selectAggregateValueConversion_swift(select_type, "listValue", "SDAI.LIST", level2);
			selectAggregateValueConversion_swift(select_type, "bagValue", "SDAI.BAG", level2);
			selectAggregateValueConversion_swift(select_type, "setValue", "SDAI.SET", level2);
			
			selectEnumValueConversion_swift(select_type, level2);
			raw("\n");
			
			// SDAIUnderlyingType
			selectUnderlyingTypeConformance_swift(level2, schema, select_type);
			raw("\n");
			
			// SDAIObservableAggregateElement
			selectObservableAggregateElementConformance_swift(select_type, level2);
			raw("\n");
			
			// SDAIAggregationBehavior
			selectAggregationBehaviorConformance_swift(select_type, level2);
			
			// SDAIAggregationType
			if( common_aggregate_base != NULL ){
				raw("\n");
				selectAggregateTypeConformance_swift(select_type, common_aggregate_base, level2);				
			}
			
			// where rules
			TYPEwhereDefinitions_swift(select_type, level2);
			SELECTwhereRuleValidation_swift(select_type, level2);
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
		raw(", ");
		wrap("SDAIAggregationType");
//	}
//	if( common_aggregate_base != NULL ){
		if( common_aggregate_base == Type_Entity ){
			wrap("where ELEMENT == SDAI.EntityReference");
		}
		else {
			char buf[BUFSIZ];
//			wrap("where ELEMENT == %s", TYPE_swiftName(common_aggregate_base,schema->superscope,buf));
			wrap("where ELEMENT == %s", TYPEhead_string_swift(schema->superscope, common_aggregate_base, NOT_IN_COMMENT, LEAF_OWNED, buf));
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
