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
			raw(" (AMBIGUOUS (CASE LEVEL))\n");
		}
		else {
			TYPE_head_out(attr->type, level);
			
			if( SELECTget_attr_ambiguous_count(select_type, attr_name) > 1 ) {
//				if( VARis_redeclaring(attr) ) attr = attr->original_attribute;
				raw(" (AMBIGUOUS (SELECT LEVEL))");
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
			raw(" (AMBIGUOUS (CASE LEVEL))\n");
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
				raw(" (AMBIGUOUS (SELECT LEVEL))");
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
		raw("CASE: %s ", selection->symbol.name);

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
static void selectTypeGroupReference_swift
 (
  Schema schema,
  Type select_type, int level)
{
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
    // markdown
    indent_swift(level);
    raw("/// underlying type reference (NON-ENTITY)\n"
        );
    indent_swift(level);
    raw("///\n");
    indent_swift(level);
    raw("/// - defined in: ``%s``\n",
        TYPE_swiftName(select_type, schema->superscope, DOCC_QUALIFIER, buf)
        );

		indent_swift(level);
		raw("public var %s%s: ", superEntity_swiftPrefix, TYPE_swiftName(selection, select_type->superscope, SWIFT_QUALIFIER, buf));
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
    // markdown
    indent_swift(level);
    raw("/// underlying type reference (ENTITY)\n"
        );
    indent_swift(level);
    raw("///\n");
    indent_swift(level);
    raw("/// - defined in: ``%s``\n",
        TYPE_swiftName(select_type, schema->superscope, DOCC_QUALIFIER, buf)
        );

		indent_swift(level);
		raw("public var %s%s: ", superEntity_swiftPrefix, as_entitySwiftName_n(entity_name, buf, sizeof(buf))); 
		wrap("%s.PRef {\n", buf);

		{	int level2 = level+nestingIndent_swift;
			char buf2[BUFSIZ];

			indent_swift(level2);
			raw("switch self {\n");
			int unhandled = selection_count;
			
			LISTdo(origins, selection, Type) {
				if( TYPEis_select(selection) ) {
					indent_swift(level2);
					raw("case .%s(let select): return select",selectCase_swiftName(selection, buf2));
					wrap(".%s%s\n",superEntity_swiftPrefix,as_entitySwiftName_n(entity_name, buf2, sizeof(buf2)));
				}
				else {
					Entity entity = TYPEget_body(selection)->entity;

					indent_swift(level2);
					if( entity->symbol.name == entity_name ) {
						raw("case .%s(let entity): return entity",
								selectCase_swiftName(selection, buf2)
								);
					}
					else {
						raw("case .%s(let entity): return %s.PRef(entity.eval?.%s%s)",
								selectCase_swiftName(selection, buf2),
								buf,
								superEntity_swiftPrefix,
								buf
								);
					}

					raw("\n");
				}
				--unhandled;
			}LISTod;
			
			if( unhandled > 0 ) {
				indent_swift(level2);
				raw("default: return %s.PRef(nil)\n", buf);
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
		raw("var %s%s: ", superEntity_swiftPrefix, TYPE_swiftName(selection, select_type/*->superscope*/, SWIFT_QUALIFIER, buf));
		wrap("%s.%s? { get }\n", schemaname,buf);
	} LISTod;				
	
	Dictionary all_supers = SELECTget_super_entity_list(select_type);
	DictionaryEntry de;
	const char* entity_name;
	
	DICTdo_init(all_supers, &de);
	while( 0 != (entity_name = DICTdo_key(&de))) {
		if( mark ) {
			indent_swift(level);
			raw(mark);
			mark = NULL;
		}
		
		indent_swift(level);
		raw("var %s%s: ", superEntity_swiftPrefix, as_entitySwiftName_n(entity_name, buf, sizeof(buf))); 
		wrap("%s.%s.PRef { get }\n", schemaname, buf);
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
		raw("var %s%s: ", superEntity_swiftPrefix, TYPE_swiftName(selection, select_type->superscope, SWIFT_QUALIFIER, buf));
		wrap("%s.%s? { rep.%s%s }\n", schemaname, buf, superEntity_swiftPrefix, buf);
	} LISTod;				
	
	Dictionary all_supers = SELECTget_super_entity_list(select_type);
	DictionaryEntry de;
	const char* entity_name;
	
	DICTdo_init(all_supers, &de);
	while( 0 != (entity_name = DICTdo_key(&de))) {
		if( mark ) {
			indent_swift(level);
			raw(mark);
			mark = NULL;
		}
		
		indent_swift(level);
		raw("var %s%s: ", superEntity_swiftPrefix, as_entitySwiftName_n(entity_name, buf, sizeof(buf))); 
		wrap("%s.%s.PRef { rep.%s%s }\n", schemaname, buf, superEntity_swiftPrefix, buf);
	}
}

//MARK: - attribute reference codes
static void selectTypeAttributeReference_swift
 (Schema schema,
	Type select_type,
	int level)
{
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
			raw("//MARK: var %s: (AMBIGUOUS SOURCE; ATTRIBUTE ACCESS SUPPRESSED)\n\n", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));
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
			raw("//MARK: var %s: (CONFLICTING ATTRIBUTE TYPE)\n\n", as_attributeSwiftName_n(attr_name, buf, BUFSIZ));
			continue;
		}

		// markdown
		indent_swift(level);
		raw("/// SELECT type attribute\n"	);
    indent_swift(level);
    raw("///\n");
    indent_swift(level);
    raw("/// - defined in: ``%s``\n",
        TYPE_swiftName(select_type, schema->superscope, DOCC_QUALIFIER, buf)
        );

    LISTdo(typebody->list, selection_case, Type){
			if( TYPEis_entity(selection_case) ) {
				Entity case_entity = TYPEget_body(selection_case)->entity;
				Variable base_attr = ENTITYfind_attribute_effective_definition(case_entity, attr_name);
				if( base_attr != NULL ){
					indent_swift(level);
					raw("/// - origin: ENTITY( ``%s`` )\n",
							ENTITY_swiftName(case_entity, schema->superscope, DOCC_QUALIFIER, buf)
							);
				}
			}
			else if( TYPEis_select(selection_case) ) {
				Linked_List base_attr_defs = DICTlookup(SELECTget_all_attributes(selection_case), attr_name);
				Type base_attr_type = base_attr_defs!=NULL ? SELECTfind_common_type(base_attr_defs) : NULL;
				if( SELECTget_attr_ambiguous_count(selection_case, attr_name) > 1 ) continue;
				if(base_attr_type != NULL ){
					indent_swift(level);
					raw("/// - origin: SELECT( ``%s`` )\n",
							TYPE_swiftName(selection_case, schema->superscope, DOCC_QUALIFIER, buf)
							);
				}
			}				
		}LISTod;

		indent_swift(level);
		raw("public var %s: ",
				as_attributeSwiftName_n(attr_name, buf, BUFSIZ)
				);
		optionalType_swift(NO_QUALIFICATION, attr_type, YES_FORCE_OPTIONAL, NOT_IN_COMMENT);
		raw(" {\n");

		{	int level2 = level+nestingIndent_swift;
			bool is_generic_entity = TYPEis_generic_entity(attr_type);

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
							wrap("entity.%s",
									 as_attributeSwiftName_n(attr_name, buf, BUFSIZ)
									 );
							if( TYPEis_logical(attr_type) ){
								raw(" ?? SDAI.UNKNOWN");
							}
							else if( is_generic_entity ){
								raw("?.asGenericEntityReference");
							}
							raw("\n");
						}
						else {
							TYPE_head_swift(NULL, attr_type, WO_COMMENT);
							raw("(");
							wrap("entity.%s)\n",
									 as_attributeSwiftName_n(attr_name, buf, BUFSIZ)
									 );
						}
					}
					else if( ENTITYget_attr_ambiguous_count(case_entity, attr_name) > 1 ){
						indent_swift(level2);
						--unhandled;
						raw("case .%s/*(let entity)*/: return nil // AMBIGUOUS ATTRIBUTE ",selectCase_swiftName(selection_case, buf));
						raw("for %s\n", TYPE_swiftName(selection_case, NO_QUALIFICATION, SWIFT_QUALIFIER, buf));
					}
				}

				else if( TYPEis_select(selection_case) ) {
					Linked_List base_attr_defs = DICTlookup(SELECTget_all_attributes(selection_case), attr_name);
					Type base_attr_type = base_attr_defs!=NULL ? SELECTfind_common_type(base_attr_defs) : NULL;
					
					if( SELECTget_attr_ambiguous_count(selection_case, attr_name) > 1 ){
						indent_swift(level2);
						--unhandled;
						raw("case .%s/*(let select)*/: return nil // AMBIGUOUS ATTRIBUTE ",
								selectCase_swiftName(selection_case, buf)
								);
						raw("for %s\n",
								TYPE_swiftName(selection_case, NO_QUALIFICATION, SWIFT_QUALIFIER, buf)
								)
						;
					}
					else if(base_attr_type != NULL ){
						indent_swift(level2);
						--unhandled;
						raw("case .%s(let select): return ",
								selectCase_swiftName(selection_case, buf)
								);

						if( TYPEis_swiftAssignable(attr_type, base_attr_type) ){
							wrap("select.%s",
									 as_attributeSwiftName_n(attr_name, buf, BUFSIZ)
									 );
							if( is_generic_entity ){
								raw("?.asGenericEntityReference");
							}
						}
						else if( TYPEis_free_generic(base_attr_type) ){
							TYPE_head_swift(NULL, attr_type, WO_COMMENT);
							raw(".convert(fromGeneric: ");
							wrap("select.%s)",
									 as_attributeSwiftName_n(attr_name, buf, BUFSIZ)
									 );
						}
						else {
							TYPE_head_swift(NULL, attr_type, WO_COMMENT);
							raw("(");
							wrap("select.%s)",
									 as_attributeSwiftName_n(attr_name, buf, BUFSIZ)
									 );
						}
						raw("\n");
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
	raw("/// initialize from the fundamental type value\n");
	indent_swift(level);
	raw("public init(fundamental: FundamentalType) { self = fundamental }\n\n");

	
	/*
	 public init?<T: SDAI.UnderlyingType>(possiblyFrom underlyingType: T?){
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
	raw("/// initialize from the underlying type value\n");
	indent_swift(level);
	raw("public init?<T: SDAI.UnderlyingType>(possiblyFrom underlyingType: T?){\n");
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
					ifhead,TYPE_swiftName(selection, select_type->superscope, SWIFT_QUALIFIER, buf));
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
						ifhead,TYPE_swiftName(selection, select_type->superscope, SWIFT_QUALIFIER, buf));
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
						ifhead,TYPE_swiftName(selection, select_type->superscope, SWIFT_QUALIFIER, buf));
				{	int level3 = level2+nestingIndent_swift;
					indent_swift(level3);
					raw("self = .%s( ",selectCase_swiftName(selection, buf));
					raw("%s(fundamental: fundamental) )\n",TYPE_swiftName(selection, select_type->superscope, SWIFT_QUALIFIER, buf));
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
	raw("/// initialize from complex entity\n");
	indent_swift(level);
	raw("public init?(possiblyFrom complex: SDAI.ComplexEntity?) {\n");
	{	int level2=level+nestingIndent_swift;
		
		const char* ifhead = "if";
		bool emitted = false;
		
		LISTdo( typeBody->list, selection_case, Type ) {
			if( TYPEis_entity(selection_case) ){
				if(!emitted) {
					indent_swift(level2);
					raw("guard let complex = complex else { return nil }\n\n");
				}
				
				indent_swift(level2);
				raw("%s let base = complex.entityReference(%s.self) {", 
						ifhead,
						ENTITY_swiftName(ENT_TYPEget_entity(selection_case), select_type->superscope, SWIFT_QUALIFIER, buf)
						);
				raw("self = .%s(base.pRef) }\n",
						selectCase_swiftName(selection_case, buf)
						);

				ifhead = "else if";			
				emitted = true;
			}
			else if( TYPEis_select(selection_case) ){
				if(!emitted) {
					indent_swift(level2);
					raw("guard let complex = complex else { return nil }\n\n");
				}
				
				indent_swift(level2);
				raw("%s let base = %s(possiblyFrom: complex) {\n",
						ifhead,TYPE_swiftName(selection_case, select_type->superscope, SWIFT_QUALIFIER, buf));
				{	int level3 = level2+nestingIndent_swift;
					indent_swift(level3);
					raw("self = .%s(base)\n",selectCase_swiftName(selection_case, buf));
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
	 public init?<G: SDAI.GenericType>(fromGeneric generic: G?) {
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
	raw("/// initialize from SDAI generic type value\n");
	indent_swift(level);
	raw("public init?<G: SDAI.GenericType>(fromGeneric generic: G?) {\n");
	{	int level2=level+nestingIndent_swift;
		indent_swift(level2);
		raw("guard let select = generic else { return nil }\n\n");
		
		indent_swift(level2);
		raw("if let fundamental = select as? Self {\n");
		indent_swift(level2+nestingIndent_swift);
		raw("self.init(fundamental: fundamental)\n");
		indent_swift(level2);
		raw("}\n");
		
		LISTdo( typeBody->list, selection_case, Type ) {
			const char* suffix = "";
			if( TYPEis_entity(selection_case) ) {
//				suffix = ".pRef";
			}
			indent_swift(level2);
			raw("else if let base = %s.convert(fromGeneric: select) {\n", 
					TYPE_swiftName(selection_case, selection_case->superscope, SWIFT_QUALIFIER, buf));

			{	int level3 = level2+nestingIndent_swift;
				indent_swift(level3);
				raw("self = .%s(base%s)\n",
						selectCase_swiftName(selection_case, buf),
						suffix
						);
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
	raw("// Initializable.ByP21Parameter\n");
	
	indent_swift(level);
	raw("public static let bareTypeName: String = ");
	wrap("\"%s\"\n\n", TYPE_canonicalName(select_type,NO_QUALIFICATION, SWIFT_QUALIFIER, buf));

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
	raw("/// initialize from ISO 10303-21 exchange structure typed parameters\n");
	indent_swift(level);
	raw("public init?(");force_wrap();
	raw("p21typedParam: P21Decode.ExchangeStructure.TypedParameter,");force_wrap();
	raw("from exchangeStructure: P21Decode.ExchangeStructure\n");
	indent_swift(level);
	raw(") {\n");
	{	int level2 = level + nestingIndent_swift; int level3 = level2 + nestingIndent_swift;
		
		indent_swift(level2);
		raw("guard let keyword = p21typedParam.keyword.asStandardKeyword else { exchangeStructure.error = \"unexpected p21parameter(\\(p21typedParam)) while resolving \\(Self.bareTypeName) select value\"; return nil }\n\n");
		
		indent_swift(level2);
		raw("switch(keyword) {\n");
		
		LISTdo( typeBody->list, selection, Type ) {
			if( TYPEis_entity(selection) )continue;

			indent_swift(level2);
			raw("case %s.bareTypeName:\n",TYPE_swiftName(selection, select_type->superscope, SWIFT_QUALIFIER, buf));
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
		case .rhsOccurrenceName(let rhsname):
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
	raw("/// initialize from ISO 10303-21 exchange structure untyped parameters\n");
	indent_swift(level);
	raw("public init?(");force_wrap();
	raw("p21untypedParam: P21Decode.ExchangeStructure.UntypedParameter,");force_wrap();
	raw("from exchangeStructure: P21Decode.ExchangeStructure\n");
	indent_swift(level);
	raw(") {\n");
	{	int level2 = level + nestingIndent_swift; int level3 = level2 + nestingIndent_swift; int level4 = level3 + nestingIndent_swift;
		
		indent_swift(level2);
		raw("switch p21untypedParam {\n");
		indent_swift(level2);
		raw("case .rhsOccurrenceName(let rhsname):\n");
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
	raw("/// initialize from ISO 10303-21 exchange structure omitted parameters\n");
	indent_swift(level);
	raw("public init?(p21omittedParamfrom exchangeStructure: P21Decode.ExchangeStructure) {\n");
	{	int level2 = level + nestingIndent_swift;

		indent_swift(level2);
		raw("return nil\n");
	}
	indent_swift(level);
	raw("}\n\n");

}


static void selectSubtypeConstructor_swift(Schema schema, Type select_type,  int level) {
	indent_swift(level);
	raw("//MARK: CONSTRUCTORS\n");
	
	/*
	 init?(possiblyFrom complex: SDAI.ComplexEntity?) { 
	 	self.init(fundamental: FundamentalType(possiblyFrom: complex)) 
	 }
	 
	 init?<T: SDAI.UnderlyingType>(possiblyFrom underlyingType: T?) { 
	 	self.init(fundamental: FundamentalType(possiblyFrom: underlyingType)) 
	 }
	 
	 init?<G: SDAI.GenericType>(fromGeneric generic: G?) { 
	 	self.init(fundamental: FundamentalType.convert(fromGeneric: generic))
	 }
	*/
	
	indent_swift(level);
	raw("/// initialize SELECT subtype from complex entity\n");
	indent_swift(level);
	raw("init?(possiblyFrom complex: SDAI.ComplexEntity?) {\n");
	indent_swift(level+nestingIndent_swift);
	raw("self.init(fundamental: FundamentalType(possiblyFrom: complex))\n");
	indent_swift(level);
	raw("}\n\n");

	indent_swift(level);
	raw("/// initialize SELECT subtype from the underlying type value\n");
	indent_swift(level);
	raw("init?<T: SDAI.UnderlyingType>(possiblyFrom underlyingType: T?) {\n");
	indent_swift(level+nestingIndent_swift);
	raw("self.init(fundamental: FundamentalType(possiblyFrom: underlyingType))\n");
	indent_swift(level);
	raw("}\n\n");

	
	indent_swift(level);
	raw("/// initialize SELECT subtype from SDAI generic type value\n");
	indent_swift(level);
	raw("init?<G: SDAI.GenericType>(fromGeneric generic: G?) {\n");
	indent_swift(level+nestingIndent_swift);
	raw("self.init(fundamental: FundamentalType.convert(fromGeneric: generic))\n");
	indent_swift(level);
	raw("}\n");
}

//MARK: - value comparison code
static void selectTypeValueComparison_swift(Type select_type,  int level) {
	
	TypeBody typeBody = TYPEget_body(select_type);
	char buf[BUFSIZ];

	indent_swift(level);
	raw("//MARK: - SDAI.Value\n");

	/////////
	indent_swift(level);
	raw("public func isValueEqual<T: SDAI.Value>(to rhs: T) -> Bool {\n");
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
	raw("public func isValueEqualOptionally<T: SDAI.Value>(to rhs: T?) -> Bool? {\n");
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
	raw("public func isValueEqual<T: SDAI.Value>(to rhs: T, visited comppairs: inout Set<SDAI.ComplexPair>) -> Bool {\n");
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
	raw("public func isValueEqualOptionally<T: SDAI.Value>(to rhs: T?, visited comppairs: inout Set<SDAI.ComplexPair>) -> Bool? {\n");
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
	int level2 = level+nestingIndent_swift;

	TypeBody typeBody = TYPEget_body(select_type);
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("//MARK: SDAI.GenericType\n");
	indent_swift(level);
	raw("public var typeMembers: Set<SDAI.STRING> {\n");
	{
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
		
		TYPEinsert_select_type_members_swift(select_type, level2);
		
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
	raw("public func %s<ELEM:SDAI.GenericType>(elementType:ELEM.Type) -> %s<ELEM>? {\n", varname, vartype);
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
	raw("public func enumValue<ENUM:SDAI.EnumerationType>(enumType:ENUM.Type) -> ENUM? {\n");
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

//MARK: - EntityReferenceYielding
static void selectEntityReferenceYieldingConformance_swift(Type select_type,  int level) {
	int level2 = level+nestingIndent_swift;
	int level3 = level2+nestingIndent_swift;
	
	TypeBody typeBody = TYPEget_body(select_type);
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("//MARK: SDAI.EntityReferenceYielding\n");

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
			else if( TYPEis_entityYieldingAggregate(selection)){
				raw("(let aggregate): return aggregate.entityReferences\n");
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

  // persistentEntityReferences
  indent_swift(level);
  raw("public var persistentEntityReferences: AnySequence<SDAI.GenericPersistentEntityReference> {\n");
  {
    indent_swift(level2);
    raw("switch self {\n");

    LISTdo( typeBody->list, selection, Type ) {
      indent_swift(level2);
      raw("case .%s", selectCase_swiftName(selection, buf));

      if( TYPEis_entity(selection)){
        raw("(let entity): return entity.persistentEntityReferences\n");
      }
      else if( TYPEis_select(selection)){
        raw("(let select): return select.persistentEntityReferences\n");
      }
      else if( TYPEis_entityYieldingAggregate(selection)){
        raw("(let aggregate): return aggregate.persistentEntityReferences\n");
      }
      else {
        raw(": return AnySequence<SDAI.GenericPersistentEntityReference>(EmptyCollection<SDAI.GenericPersistentEntityReference>())\n");
      }
    } LISTod;

    indent_swift(level2);
    raw("}\n");
  }
  indent_swift(level);
  raw( "}\n\n" );

	/* 	func isHolding(entityReference: SDAI.EntityReference) -> Bool */
	indent_swift(level);
	raw("public func isHolding(entityReference: SDAI.EntityReference) -> Bool {\n");
	{
		indent_swift(level2);
		raw("switch self {\n");

		LISTdo( typeBody->list, selection, Type ) {
			indent_swift(level2);
			raw("case .%s", selectCase_swiftName(selection, buf));

			if( TYPEis_entity(selection)){
				raw("(let entity): \n");

				indent_swift(level3);
				raw("return entity.isHolding(entityReference: entityReference)\n");
			}
			else if( TYPEis_select(selection)){
				raw("(let select): \n");

				indent_swift(level3);
				raw("return select.isHolding(entityReference: entityReference)\n");
			}
			else if( TYPEis_entityYieldingAggregate(selection)){
				raw("(let aggregate): \n");

				indent_swift(level3);
				raw("return aggregate.isHolding(entityReference: entityReference)\n");
			}
			else {
				raw(": return false\n", buf);
			}
		} LISTod;

		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw( "}\n\n" );
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

//MARK: - SDAI.UnderlyingType
static void selectUnderlyingTypeConformance_swift(int level, Schema schema, Type select_type) {
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("//MARK: SDAI.UnderlyingType\n");
	
	indent_swift(level);
	raw("public typealias FundamentalType = Self\n");
	
	indent_swift(level);
	raw("public static let typeName: String = ");
	wrap("\"%s\"\n", TYPE_canonicalName(select_type,schema->superscope, SWIFT_QUALIFIER, buf));

	indent_swift(level);
	raw("public var asFundamentalType: FundamentalType { return self }\n");
}

//MARK: - aggregation type conformance

typedef struct AggregateTypeSpec {
  Type base_type;

  union {
    struct None {
    } none;

    struct ArrayType {
      Expression upper;
      Expression lower;
    } array_type;

    struct NonArrayType {
      bool canbe_bag;
      bool canbe_list;
      bool canbe_set;
    } non_array_type;
  } u;

  enum {
    is_none = 0,
    is_array_type,
    is_non_array_type
  } u_tag;

} AggregateTypeSpec;

// derived from
//  Type TYPE_retrieve_aggregate_base( Type t_root, Type t_agg_base )

static AggregateTypeSpec common_aggregate_spec
 ( Type t_root, AggregateTypeSpec counterpart )
{
  if( t_root == NULL )return counterpart;

  AggregateTypeSpec result_none;
  result_none.base_type = NULL;
  result_none.u_tag = is_none;


  if( TYPEis_select( t_root ) ) {
    /* parse the underlying types */
    LISTdo_links( t_root->u.type->body->list, link ){
      /* the current underlying type */
      Type t_select_case = ( Type ) link->data;

      AggregateTypeSpec case_agg = common_aggregate_spec( t_select_case, counterpart );
      if( case_agg.u_tag != is_none ) {

        // select down case_agg's base_type
        if( (counterpart.u_tag != is_none) &&
           (case_agg.base_type != counterpart.base_type) )
        {
          Type common_base = TYPEget_common(case_agg.base_type, counterpart.base_type);
          /* the underlying select case types did not resolve to a common aggregate base type */
          if( (common_base == NULL) || (case_agg.u_tag != counterpart.u_tag) )
          {
            return result_none;
          }
          case_agg.base_type = common_base;
        }

        // check agg_types
        switch (case_agg.u_tag) {
          case is_array_type:
            if( counterpart.u_tag == is_non_array_type ) {
              return result_none;
            }
            if( counterpart.u_tag == is_array_type ) {
              if( !EXPs_are_equal(case_agg.u.array_type.lower, counterpart.u.array_type.lower) ||
                 !EXPs_are_equal(case_agg.u.array_type.upper, counterpart.u.array_type.upper) ) {
                 return result_none;
              }
            }
            break;

          case is_non_array_type:
            if( counterpart.u_tag == is_array_type ) {
              return result_none;
            }
            if( counterpart.u_tag == is_non_array_type ){
              case_agg.u.non_array_type.canbe_bag  |= counterpart.u.non_array_type.canbe_bag;
              case_agg.u.non_array_type.canbe_list |= counterpart.u.non_array_type.canbe_list;
              case_agg.u.non_array_type.canbe_set  |= counterpart.u.non_array_type.canbe_set;
            }
            break;

          default:
            break;
        }


        counterpart = case_agg;
      }
      else if( TYPEis_aggregation_data_type(t_select_case) ){
        /* the underlying aggregate base types did not resolve to a common type */
        AggregateTypeSpec result;
        result.u_tag = is_none;
        return result_none;
      }
    }LISTod;
    return counterpart;
  }

  if( TYPEis_aggregation_data_type(t_root) ){
    switch (counterpart.u_tag) {
      case is_array_type:
        if( !TYPEis_array(t_root) ) return result_none;

        if( !EXPs_are_equal(TYPEget_body(t_root)->lower, counterpart.u.array_type.lower) ||
           !EXPs_are_equal(TYPEget_body(t_root)->upper, counterpart.u.array_type.upper) ) {
          return result_none;
        }
        break;

      case is_non_array_type:
        switch (TYPEget_type(t_root)) {
          case array_:
            return result_none;
            break;

          case bag_:
            counterpart.u.non_array_type.canbe_bag = true;
            break;

          case list_:
            counterpart.u.non_array_type.canbe_list = true;
            break;

          case set_:
            counterpart.u.non_array_type.canbe_set = true;
            break;

          default:
            break;
        }
        break;

      case is_none:
        if( TYPEis_array(t_root) ) {
          counterpart.u_tag = is_array_type;
          counterpart.u.array_type.lower = TYPEget_body(t_root)->lower;
          counterpart.u.array_type.upper = TYPEget_body(t_root)->upper;
        }
        else {
          counterpart.u_tag = is_non_array_type;
          counterpart.u.non_array_type.canbe_bag = false;
          counterpart.u.non_array_type.canbe_list = false;
          counterpart.u.non_array_type.canbe_set = false;

          switch (TYPEget_type(t_root)) {
            case bag_:
              counterpart.u.non_array_type.canbe_bag = true;
              break;

            case list_:
              counterpart.u.non_array_type.canbe_list = true;
              break;

            case set_:
              counterpart.u.non_array_type.canbe_set = true;
              break;

            default:
              break;
          }
        }
        break;
    }

    Type common_base = TYPEget_common(t_root->u.type->body->base, counterpart.base_type);
    if( common_base == NULL )return result_none;
    
    counterpart.base_type = common_base;
    return  counterpart;
  }

  /* the underlying select case type is neither a select nor an aggregate */
  return result_none;
}

static AggregateTypeSpec SELECTaggregate_spec(Type select_type)
{
  AggregateTypeSpec result_none;
  result_none.base_type = NULL;
  result_none.u_tag = is_none;

  return common_aggregate_spec(select_type, result_none);
}

static void selectVarForwarding_swift
 (Type select_type,
  const char* var_name,
  const char* var_type,
  const char* default_value,
  int level)
{
	TypeBody typeBody = TYPEget_body(select_type);
	char buf[BUFSIZ];

	indent_swift(level);
	raw("public var %s: %s {\n", var_name, var_type);
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw("switch self {\n");

    bool exhausted = true;
		LISTdo( typeBody->list, selection, Type ) {
      AggregateTypeSpec common_aggr = SELECTaggregate_spec(selection);
      if( common_aggr.u_tag != is_none ){
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
      raw("default: return %s\n", default_value);
    }

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


static void selectAggregationBehaviorConformance_swift
 (Type select_type, int level)
{
	indent_swift(level);
	raw("//MARK: SDAIAggregationBehavior\n");
	selectAggregationBehaviorForwarding_swift(select_type, "aggregationHiBound", "Int?", "hiBound", level);
	selectAggregationBehaviorForwarding_swift(select_type, "aggregationHiIndex", "Int?", "hiIndex", level);
	selectAggregationBehaviorForwarding_swift(select_type, "aggregationLoBound", "Int?", "loBound", level);
	selectAggregationBehaviorForwarding_swift(select_type, "aggregationLoIndex", "Int?", "loIndex", level);
	selectAggregationBehaviorForwarding_swift(select_type, "aggregationSize", "Int?", 	 "size", 		level);
}

static void selectAggregateTypeConformance_swift
 (Type select_type,
  AggregateTypeSpec select_aggr,
  int level)
{
	TypeBody typeBody = TYPEget_body(select_type);
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("//MARK: SDAI.AggregationType\n");

	indent_swift(level);
	wrap("public typealias ELEMENT = %s\n", TYPEhead_string_swift(select_type->superscope, select_aggr.base_type, NOT_IN_COMMENT, buf));

	selectVarForwarding_swift(select_type, "hiBound", "Int?", "nil",   level);
	selectVarForwarding_swift(select_type, "hiIndex", "Int",  "0",     level);
	selectVarForwarding_swift(select_type, "loBound", "Int",  "0",     level);
	selectVarForwarding_swift(select_type, "loIndex", "Int",  "0",     level);
	selectVarForwarding_swift(select_type, "size",    "Int",  "0",     level);
	selectVarForwarding_swift(select_type, "isEmpty", "Bool", "true",  level);

	raw("\n");
	indent_swift(level);
	raw("public subscript(index: Int?) -> ELEMENT? {\n");
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw("switch self {\n");

    bool exhausted = true;
		LISTdo( typeBody->list, selection, Type ) {
      AggregateTypeSpec common_aggr = SELECTaggregate_spec(selection);
      if( common_aggr.u_tag != is_none ){
        indent_swift(level2);
        raw("case .%s(let selection): ", selectCase_swiftName(selection, buf));
        raw("return selection[index]\n");
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

	raw("\n");
	indent_swift(level);
	raw("public func makeIterator() -> AnyIterator<ELEMENT?> {\n");
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw("switch self {\n");

    bool exhausted = true;
		LISTdo( typeBody->list, selection, Type ) {
      AggregateTypeSpec common_aggr = SELECTaggregate_spec(selection);
      if( common_aggr.u_tag != is_none ){
        indent_swift(level2);
        raw("case .%s(let selection): ", selectCase_swiftName(selection, buf));
        wrap("return AnyIterator<ELEMENT?>(selection.lazy.map({SDAI.FORCE_OPTIONAL($0)}).makeIterator())\n");
      }
      else {
        exhausted = false;
      }
		} LISTod;

    if( !exhausted ){
      indent_swift(level2);
      raw("default: return AnyIterator<ELEMENT?>(EmptyCollection<ELEMENT?>().makeIterator())\n");
    }

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

    bool exhausted = true;
		LISTdo( typeBody->list, selection, Type ) {
      AggregateTypeSpec common_aggr = SELECTaggregate_spec(selection);
      if( common_aggr.u_tag != is_none ){
        indent_swift(level2);
        raw("case .%s(let selection): ", selectCase_swiftName(selection, buf));
        raw("return selection.CONTAINS(elem)\n");
      }
      else {
        exhausted = false;
      }
		} LISTod;

    if( !exhausted ){
      indent_swift(level2);
      raw("default: return SDAI.FALSE\n");
    }

		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw("}\n");

	raw("\n");
  {
    indent_swift(level);
    if( select_aggr.u_tag == is_array_type ){
      raw("public typealias RESULT_AGGREGATE = SDAI.ARRAY_OPTIONAL<ELEMENT>\n");
    }
    else {
      if( select_aggr.u.non_array_type.canbe_list ){
        raw("public typealias RESULT_AGGREGATE = SDAI.LIST<ELEMENT>\n");
      }
      else if( select_aggr.u.non_array_type.canbe_bag ){
        raw("public typealias RESULT_AGGREGATE = SDAI.BAG<ELEMENT>\n");
      }
      else if( select_aggr.u.non_array_type.canbe_set ){
        raw("public typealias RESULT_AGGREGATE = SDAI.SET<ELEMENT>\n");
      }
      else {
        raw("public typealias RESULT_AGGREGATE = SDAI.LIST<ELEMENT>\n");
      }
    }
  }
	indent_swift(level);
	raw("public func QUERY(logical_expression: @escaping (ELEMENT) -> SDAI.LOGICAL ) -> RESULT_AGGREGATE {\n");
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw("switch self {\n");

    bool exhausted = true;
		LISTdo( typeBody->list, selection, Type ) {
      AggregateTypeSpec common_aggr = SELECTaggregate_spec(selection);
      if( common_aggr.u_tag != is_none ){
        indent_swift(level2);
        raw("case .%s(let selection): \n", selectCase_swiftName(selection, buf));
        {	int level3 = level2+nestingIndent_swift;
          indent_swift(level3);
          raw("let result = RESULT_AGGREGATE.SwiftType( selection.QUERY(logical_expression:logical_expression) )\n");

          indent_swift(level3);
          if( select_aggr.u_tag == is_array_type ){
            raw("return RESULT_AGGREGATE(from:result, ");
            force_wrap();
            raw("bound1: SDAI.UNWRAP(");
            EXPRassignment_rhs_swift
            (NO_RESOLVING_GENERIC, select_type->superscope,
             common_aggr.u.array_type.lower, Type_Integer,
             EMIT_SELF, NO_PAREN, OP_UNKNOWN, NO_WRAP);
            raw("), ");

            raw("bound2:");
            EXPRassignment_rhs_swift
            (NO_RESOLVING_GENERIC, select_type->superscope,
             common_aggr.u.array_type.upper, Type_Integer,
             EMIT_SELF, NO_PAREN, OP_UNKNOWN, NO_WRAP);
            raw(")\n");
          }
          else {
            raw("return RESULT_AGGREGATE(from:result)\n");
          }
        }
      }
      else {
        exhausted = false;
      }
		} LISTod;

    if( !exhausted ){
      indent_swift(level2);
      raw("default: return RESULT_AGGREGATE()\n");
    }

		indent_swift(level2);
		raw("}\n");
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
		
    bool exhausted = true;
		LISTdo( typeBody->list, selection, Type ) {
      AggregateTypeSpec common_aggr = SELECTaggregate_spec(selection);
      if( common_aggr.u_tag != is_none ){
        indent_swift(level2);
        raw("case .%s(let selection): \n", selectCase_swiftName(selection, buf));
        {	int level3 = level2+nestingIndent_swift;
          indent_swift(level3);
          raw("return selection.asAggregationSequence\n");
        }
      }
      else {
        exhausted = false;
      }
		} LISTod;

    if( !exhausted ){
      indent_swift(level2);
      raw("default: return AnySequence<ELEMENT>(EmptyCollection<ELEMENT>())\n");
    }

		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw("}\n");

}

//MARK: - WHERE rule validation
static void SELECTwhereRuleValidation_swift( Type select_type, int level ) {
	char typename[BUFSIZ]; TYPE_swiftName(select_type, select_type->superscope, SWIFT_QUALIFIER, typename);

  raw("\n");
  indent_swift(level);
  raw("//MARK: WHERE RULE VALIDATION (SELECT TYPE)\n");

  indent_swift(level);
  raw("public static func validateWhereRules(instance:Self?, prefix:SDAI.WhereLabel) -> SDAI.WhereRuleValidationRecords {\n");

  {  int level2 = level+nestingIndent_swift;
    char buf[BUFSIZ];
    
    indent_swift(level2);
    raw("var result: SDAI.WhereRuleValidationRecords = [:]\n");

    // underlying case type validation
    indent_swift(level2);
    raw("switch instance {\n");
    LISTdo( TYPEget_body(select_type)->list, selection_case, Type ) {
      indent_swift(level2);
      if( TYPEis_entity(selection_case)) {
        raw("case .%s(let entity): ",
            selectCase_swiftName(selection_case, buf));
        force_wrap();
        wrap("if !entity.shouldPropagateValidation { break }");
        force_wrap();
        wrap("result = %s.validateWhereRules(instance: entity.eval, ",
             ENTITY_swiftName(ENT_TYPEget_entity(selection_case), select_type->superscope, SWIFT_QUALIFIER, buf)
             );
        wrap("prefix:prefix + \"\\\\\\(entity)\")\n",
             TYPE_canonicalName(selection_case,select_type->superscope, SWIFT_QUALIFIER, buf)
             );
      }
      else{
        raw("case .%s(let selectValue): ",
            selectCase_swiftName(selection_case, buf));
        wrap("result = %s.validateWhereRules(instance: selectValue, ",
              TYPE_swiftName(selection_case, select_type->superscope, SWIFT_QUALIFIER, buf)
             );
        wrap("prefix:prefix + \"\\\\%s\")\n",
             TYPE_canonicalName(selection_case,select_type->superscope, SWIFT_QUALIFIER, buf)
             );
      }

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
          whereRuleLabel_swiftName(where, whereLabel),
          typename,
          whereLabel);
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
  AggregateTypeSpec common_aggr = SELECTaggregate_spec(select_type);

	// markdown
	raw("\n/** SELECT type\n");
	raw("- EXPRESS source code:\n");
	raw("```express\n");
	TYPE_out(select_type, level);
	raw("\n```\n");
//  if( common_aggregate_base != NULL ){
//    raw("\n- Common Aggregate Element Type: ") ;
//    TYPE_out(common_aggregate_base, level);
//    raw("\n");
//  }
	raw("*/\n");
	
	// swift enum definition
		char buf[BUFSIZ];
				
		char typename[BUFSIZ];
		TYPE_swiftName(select_type,select_type->superscope, SWIFT_QUALIFIER, typename);

		indent_swift(level);
		wrap( "public enum %s : SDAI.Value, ", typename );
		positively_wrap();
//		raw(  "%s__", SCHEMA_swiftName(schema, buf) );
		raw(  "TypeHierarchy.%s__TypeBehavior {\n", typename );

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
        raw("/// SELECT case (%s)",
            case_kind
            );
        force_wrap();
        raw("///");
        force_wrap();
				raw("/// - target: ``%s`` (%s)",
						TYPE_swiftName(selection, schema->superscope, DOCC_QUALIFIER, buf),
						case_kind
						);
        force_wrap();
				raw("/// - defined in: ``%s``",
						TYPE_swiftName(select_type, schema->superscope, DOCC_QUALIFIER, buf)
						);

        {
          AggregateTypeSpec common_aggr = SELECTaggregate_spec(selection);
          if( common_aggr.u_tag != is_none ){
            force_wrap();
            raw("/// - may yield: aggregate[``%s``]\n",
                TYPEhead_string_swift(select_type->superscope, common_aggr.base_type, YES_IN_COMMENT, buf)
                );
          }
        }
        raw("\n");

				indent_swift(level2);
				raw("@_documentation(visibility:public)\n");
				indent_swift(level2);
				raw( "case %s(",
						selectCase_swiftName(selection, buf)
						);
				wrap( "%s)\t// (%s)\n",
						 TYPE_swiftName(selection, select_type->superscope, SWIFT_QUALIFIER, buf),
//						 suffix,
						 case_kind
						 );
			} LISTod;
			raw("\n");
			
			selectTypeConstructor_swift(select_type, level2);
			raw("\n");
			
			selectTypeGroupReference_swift(schema, select_type, level2);
			raw("\n");

			selectTypeAttributeReference_swift(schema, select_type, level2);		
			raw("\n");
			
			selectTypeValueComparison_swift(select_type, level2);
			raw("\n");

			// SDAIGenericTypeBase
			selectGenericTypeBaseConformance_swift(select_type, level2);
			raw("\n");

			// SDAI.GenericType
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
			
			// SDAI.UnderlyingType
			selectUnderlyingTypeConformance_swift(level2, schema, select_type);
			raw("\n");
			
			// SDAI.EntityReferenceYielding
			selectEntityReferenceYieldingConformance_swift(select_type, level2);
			raw("\n");
			
			// SDAIAggregationBehavior
			selectAggregationBehaviorConformance_swift(select_type, level2);
			
			// SDAI.AggregationType
			if( common_aggr.u_tag != is_none ){
				raw("\n");
				selectAggregateTypeConformance_swift(select_type, common_aggr, level2);				
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
	const char* typename = TYPE_swiftName(select_type,select_type->superscope, SWIFT_QUALIFIER, typebuf);

	char schemabuf[BUFSIZ];
	const char* schemaname = SCHEMA_swiftName(schema, schemabuf);
  AggregateTypeSpec common_aggr = SELECTaggregate_spec(select_type);

	raw("\n\n//MARK: - SELECT TYPE HIERARCHY\n");
	//__TypeBehavior protocol
  indent_swift(level);
  raw( "extension %s.TypeHierarchy {\n", schemaname);
//  indent_swift(level);
//  raw("@_documentation(visibility:public)\n");
	indent_swift(level);
	raw( "public protocol %s__TypeBehavior: ", typename);
	wrap("SDAI.SelectType");
	if( common_aggr.u_tag != is_none ){
		raw(", ");
		wrap("SDAI.AggregationType");
    force_wrap();
		if( common_aggr.base_type == Type_Entity ){
			wrap("where ELEMENT == SDAI.EntityReference");
		}
		else {
			char buf[BUFSIZ];
			wrap("where ELEMENT == %s",
           TYPEhead_string_swift(schema->superscope, common_aggr.base_type, NOT_IN_COMMENT, buf)
           );
		}
	}
	
	raw(" {\n");
	{	int level2 = level+nestingIndent_swift;

		selectTypeGroupReference_swiftProtocol(schema, select_type, level2);
		raw("\n");

		selectTypeAttributeReference_swiftProtocol(schema, select_type, level2);				
	}
	
	indent_swift(level);
	raw("}\n}\n\n");


	// __Subtype protocol
	indent_swift(level);
	raw( "extension %s.TypeHierarchy {\n public protocol %s__Subtype: ", schemaname, typename );
	wrap("%s__TypeBehavior, SDAI.DefinedType\n", typename );

	indent_swift(level);
	wrap("where Supertype: %s__TypeBehavior\n", typename);

	indent_swift(level);
	raw( "{}\n}\n\n");


	// __Subtype protocol extension
	indent_swift(level);
	raw( "public extension %s.TypeHierarchy.%s__Subtype {\n", schemaname, typename );

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
