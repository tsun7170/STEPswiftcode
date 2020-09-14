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

//MARK: - select type

static void listSelectionEntityAttributes(Type select_type, Entity entity, int level) {
	Dictionary all_attrs_from_entity = ENTITYget_all_attributes(entity);
	DictionaryEntry de;
	Linked_List attr_defs_from_entity;
	
	DICTdo_init( all_attrs_from_entity, &de );
	while( 0 != ( attr_defs_from_entity = ( Linked_List )DICTdo( &de ) ) ) {
		Variable attr = LISTget_first(attr_defs_from_entity);
		char* attr_name = VARget_simple_name(attr);

		indent_swift(level);
		raw("ATTR:  %s: ",attr_name);

		if( ENTITYget_attr_ambiguous_count(entity, attr_name) > 1 ) {
			wrap("(AMBIGUOUS(ENTITY LEVEL))\n");			
		}
		else {
			TYPE_head_out(attr->type, level);
			
			if( SELECTget_attr_ambiguous_count(select_type, attr_name) > 1 ) {
				if( VARis_redeclaring(attr) ) attr = attr->original_attribute;
				wrap("(AMBIGUOUS(SELECT LEVEL), ");			
				wrap("ORIGIN = %s)", attr->defined_in->symbol.name);			
			}
			else if( !SELECTattribute_is_unique(select_type, attr_name) ) {
				raw(" ***");
			}
			raw("\n");
		}
	}

}

static void listSelectionSelectAttributes(Type select_type, Type select, int level) {
	Dictionary all_attrs_from_select = SELECTget_all_attributes(select_type);
	DictionaryEntry de;
	Linked_List attr_defs;
	
	DICTdo_init(all_attrs_from_select, &de);
	while( 0 != (attr_defs = DICTdo(&de)) ) {
		Variable attr = LISTget_first(attr_defs);
		char* attr_name = VARget_simple_name(attr);
		
		indent_swift(level);
		raw("ATTR:  %s: ",attr_name);
		
		if( SELECTget_attr_ambiguous_count(select_type, attr_name) > 1 ) {
			wrap("(AMBIGUOUS(SELECT LEVEL), ");
			wrap("ORIGINS = ");
			char* sep = "";
			LISTdo(attr_defs, amb, Variable) {
				if( VARis_redeclaring(amb) ) amb = amb->original_attribute;
				Entity origin = amb->defined_in;
				raw(sep);
				wrap(origin->symbol.name);
				sep = ", ";
			}LISTod;
			raw(")");
		}
		else {
			attr = SELECTfind_attribute_effective_definition(select_type, attr_name);
			TYPE_head_out(attr->type, level);
			
			if( !SELECTattribute_is_unique(select_type, attr_name) ) {
				raw(" ***");
			}
		}
		raw("\n");		
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
		wrap("%s! {\n", buf);
		
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
		raw("public var %s%s: ", superEntity_swiftPrefix, canonical_swiftName(entity_name, buf)); 
		wrap("%s! {\n", buf);
		
		{	int level2 = level+nestingIndent_swift;
			
			indent_swift(level2);
			raw("switch self {\n");
			int unhandled = selection_count;
			
			LISTdo(origins, selection, Type) {
				if( TYPEis_select(selection) ) {
					indent_swift(level2);
					raw("case .%s(let select): return select",selectCase_swiftName(selection, buf));
					wrap(".%s%s\n",superEntity_swiftPrefix,canonical_swiftName(entity_name, buf));
				}
				else {
					Entity entity = TYPEget_body(selection)->entity;
					
					indent_swift(level2);
					raw("case .%s(let entity): return entity",ENTITY_swiftName(entity, "", "", NULL, buf));
					if( entity->symbol.name != entity_name ) {
						wrap(".%s%s",superEntity_swiftPrefix,canonical_swiftName(entity_name, buf));
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

static void selectTypeAttributeReference_swift(Type select_type, int level) {
	TypeBody typeBody = TYPEget_body(select_type);
	int selection_count = LISTget_length(typeBody->list);

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
		
		Variable attr = SELECTfind_attribute_effective_definition(select_type, attr_name);
		
		indent_swift(level);
		char buf[BUFSIZ];
		raw("public var %s: ", attribute_swiftName(attr,buf) );
		variableType_swift(NULL, attr, YES_FORCE_OPTIONAL, NOT_IN_COMMENT);
		raw(" {\n");

		{	int level2 = level+nestingIndent_swift;
			
			indent_swift(level2);
			raw("switch self {\n");
			int unhandled = selection_count;
			
			LISTdo_links(attr_defs, def) {
				Type selection = def->aux;
				if( TYPEis_select(selection) ) {
					indent_swift(level2);
					raw("case .%s(let select): return select",selectCase_swiftName(selection, buf));
					wrap(".%s\n", attribute_swiftName(attr,buf));
				}
				else {
					Entity entity = TYPEget_body(selection)->entity;
					
					indent_swift(level2);
					raw("case .%s(let entity): return entity",ENTITY_swiftName(entity, "", "", NULL, buf));
					wrap(".%s\n", attribute_swiftName(attr,buf));
				}
				--unhandled;
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

void selectTypeDefinition_swift(Type select_type,  int level) {
	
	listAllSelectionAttributes(select_type, level);
	TypeBody typeBody = TYPEget_body(select_type);
	
	// swift enum definition
	{
		char buf[BUFSIZ];
		indent_swift(level);
		wrap( "public enum %s : SDAISelectType {\n", TYPE_swiftName(select_type,select_type->superscope,buf) );
		
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
		}
	}
	
	indent_swift(level);
	raw( "}\n\n" );
	
}

void selectTypeExtension_swift(Schema schema, Type select_type,  int level) {
	// swift enum extension
	indent_swift(level);
	{
		char buf[BUFSIZ];
		wrap( "public extension %s {\n", TYPE_swiftName(select_type,schema->superscope,buf) );
	}

	{	int level2 = level+nestingIndent_swift;
		
		selectTypeGroupReference_swift(select_type, level2);
		selectTypeAttributeReference_swift(select_type, level2);
	}
	
	indent_swift(level);
	raw( "}\n\n" );
}
