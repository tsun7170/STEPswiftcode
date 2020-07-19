//
//  swift_entity.c
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#include <assert.h>
#include <stdlib.h>

#include "exppp.h"
#include "pp.h"
#include "pretty_expr.h"
#include "pretty_subtype.h"
#include "pretty_where.h"
#include "pretty_type.h"
#include "pretty_entity.h"

#include "swift_entity.h"
#include "swift_files.h"
#include "swift_type.h"
#include "swift_expression.h"


const char* ENTITY_swiftName( Entity e ) {
	return e->symbol.name;
}

char ENTITY_swiftNameInitial( Entity e) {
	return toupper( ENTITY_swiftName(e)[0] );
}

const char* attribute_swiftName( Variable attr ) {
	return ATTRget_name_string(attr);
}

const char* partialEntity_swiftName( Entity e, char buf[BUFSIZ] ) {
	snprintf(buf, BUFSIZ, "_%s", ENTITY_swiftName(e));
	return buf;	
}

const char* partialEntityAttribute_swiftName( Variable attr, char buf[BUFSIZ] ) {
	snprintf(buf, BUFSIZ, "_%s", ATTRget_name_string(attr));

	return buf;
}

const char* whereLabel_swiftName( Where w, char buf[BUFSIZ] ) {
	if( w-> label ) {
		snprintf(buf, BUFSIZ, "WHERE_%s", w->label->name );
	}
	else {
		snprintf(buf, BUFSIZ, "WHERE_%d", w->serial );
	}
	return buf;
}




enum AttrType {
	explicit,
	derived,
	inverse
};

static void attribute_out( Entity leaf, Variable v, int level ) {
	enum AttrType attr_type = explicit;
	if( VARis_derived(v) ) attr_type = derived;	
	if( VARis_inverse(v) ) attr_type = inverse;


	indent_swift(level);
	if( VARis_redeclaring(v) ) {
		raw("REDCR: ");
	}
	else {
		raw("ATTR:  ");
	}
	raw( "%s,\t", ATTRget_name_string(v) );

	raw("TYPE: ");
	if( VARget_optional( v ) ) {
		wrap( "OPTIONAL " );
	}
	raw(ATTRget_type_string(v));
	
	switch (attr_type) {
		case explicit:
			raw(" -- EXPLICIT");
			break;
		case derived:
			raw(" -- DERIVED");
			break;
		case inverse:
			raw(" -- INVERSE");
			break;
	}
	if(VARis_dynamic(v)) {
		raw(" (DYNAMIC)");
	}
	if( ENTITYget_attr_ambiguous_count(leaf, ATTRget_name_string(v)) > 1 ) {
		raw("\t(AMBIGUOUS)");
	}
	raw("\n");

	{	int level2 = level+nestingIndent_swift;

		if(  VARis_derived(v) ) {
			indent_swift(level2);
			raw( ":= " );
			EXPR_out( VARget_initializer(v), 0 );
			raw("\n");
		}
		if( VARis_observed(v) ){
			indent_swift(level2);
			raw("-- observed by\n");
			{	int level3 = level2+nestingIndent_swift;

				int observer_no = 0;
				LISTdo( VARget_observers(v), observingAttr, Variable) {				
					indent_swift(level3);
					raw("ENTITY(%d): %s,\t",++observer_no,ENTITYget_name( observingAttr->defined_in) );
					raw("ATTR: ");
					EXPR_out(observingAttr->name,0);
					raw(",\t" "TYPE: ");
					TYPE_head_out( observingAttr->type, NOLEVEL );
					raw("\n");
				} LISTod
			}
		}
		else if( VARis_inverse(v) ) {
			indent_swift(level2);
			raw( "FOR " );
			raw( VARget_name(VARget_inverse(v))->symbol.name );
			raw( ";\n" );
		}
		
		if( VARis_redeclaring(v) ) {
			indent_swift(level2);
			//		raw("-- OVERRIDING ENTITY: %s\n",  _VARget_redeclaring_entity_name_string(v));
			raw("-- OVERRIDING ENTITY: %s\n", ENTITYget_name(VARget_redeclaring_attr(v)->defined_in) );
		}
		if( VARis_overriden(v)) {
			indent_swift(level2);
			raw("-- possibly overriden by\n");
			DictionaryEntry de;
			Variable overrider;
			Variable effective_definition = ENTITYfind_attribute_effective_definition(leaf, ATTRget_name_string(v));
			DICTdo_init(v->overriders, &de);
			while( 0 != ( overrider = DICTdo( &de ) ) ) {
				indent_swift(level2);
				if( overrider == effective_definition ) {
					raw("*** ");
				}
				else {
					raw("    ");
				}
				raw("ENTITY: %s,\t", ENTITYget_name(overrider->defined_in));
				raw("TYPE: ");
				if(VARget_optional(v)) {
					raw("OPTIONAL ");
				}
				raw(ATTRget_type_string(overrider));
				if( VARis_derived(overrider) ){
					raw("\t(as DERIVED)");
				}
				raw("\n");
			}
		}
	}
	raw( "\n" );
}

static int entity_no;

static void listLocalAttributes( Entity leaf, Entity entity, int level ) {
	raw("\n");
	indent_swift(level);
	raw("ENTITY(%d)\t%s\n", ++entity_no, entity->symbol.name);

	LISTdo( entity->u.entity->attributes, attr, Generic ) {
		attribute_out(leaf, attr, level + nestingIndent_swift);
	}
	LISTod;

}

static void listAllAttributes( Entity leaf, Entity entity, int level, int search_id ) {
	if( entity->search_id == search_id ) return;
	entity->search_id = search_id;
	
	LISTdo( entity->u.entity->supertypes, super, Entity ) {
		listAllAttributes(leaf, super, level, search_id);
	}
	LISTod;
	
	listLocalAttributes(leaf, entity, level);
}


//MARK: - partial entity swift definition

static void attributeHead_swift(char* access, Variable attr, int level, char attrNameBuf[BUFSIZ]) {
	indent_swift(level);
	raw("%s var %s: ",access,partialEntityAttribute_swiftName(attr, attrNameBuf));
	if( VARget_optional(attr) ) {
		raw("( ");
	}
	TYPE_head_swift(attr->type, level+nestingIndent_swift);
	if( VARget_optional(attr) ) {
		raw(" )?");
	}
}

static void simpleAttribureObservers_swift(Entity entity, Variable attr, int level) {
	char buf[BUFSIZ];

	indent_swift(level);
	wrap("fileprivate static func %s__observer(", partialEntityAttribute_swiftName(attr, buf));
	wrap("SELF: SDAI.EntityReference, ");
	wrap("removing: SDAI.EntityReference?, adding: SDAI.EntityReference?) {\n");
	{	int level2 = level + nestingIndent_swift;

		indent_swift(level2);
		raw("guard removing != adding else { return }\n");
		indent_swift(level2);
		raw("let selfComplex = SELF.complexEntity\n");
		indent_swift(level2);
		raw("if let oldComplex = removing?.complexEntity {\n");
		{	int level3 = level2 + nestingIndent_swift;
			
			LISTdo( VARget_observers(attr), observingAttr, Variable) {
				Entity observingEntity = observingAttr->defined_in;
				
				indent_swift(level3);
				raw("//OBSERVING ENTITY: %s\n", ENTITY_swiftName(observingEntity));
				
				indent_swift(level3);
				raw("oldComplex.partialEntityInstance(%s.self)?", partialEntity_swiftName(observingEntity, buf));
				wrap(".%s__observeRemovedReference(in: selfComplex)\n", partialEntityAttribute_swiftName(observingAttr, buf));
			} LISTod
			
		}
		indent_swift(level2);
		raw("}\n");

		indent_swift(level2);
		raw("if let newComplex = adding?.complexEntity {\n");
		{	int level3 = level2 + nestingIndent_swift;
			
			LISTdo( VARget_observers(attr), observingAttr, Variable) {
				Entity observingEntity = observingAttr->defined_in;
				
				indent_swift(level3);
				raw("//OBSERVING ENTITY: %s\n", ENTITY_swiftName(observingEntity));
				
				indent_swift(level3);
				raw("newComplex.partialEntityInstance(%s.self)?", partialEntity_swiftName(observingEntity, buf));
				wrap(".%s__observeAddedReference(in: selfComplex)\n", partialEntityAttribute_swiftName(observingAttr, buf));
			} LISTod
			
		}
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw("}\n");

}

static void aggregateAttributeObservers_swift(Entity entity, Variable attr, int level) {
	char buf[BUFSIZ];
	
	indent_swift(level);
	wrap("fileprivate static func %s__aggregateObserver<AGG:SDAIAggregationType>(", partialEntityAttribute_swiftName(attr, buf));
	wrap("SELF: SDAI.EntityReference, ");
	wrap("oldAggregate: inout AGG?, newAggregate: inout AGG?) {\n");

	{	int level2 = level + nestingIndent_swift;
		indent_swift(level2);
		raw("oldAggregate?.observer = nil\n");
		indent_swift(level2);
		raw("newAggregate?.observer = { [unowned SELF] (removing, adding) in %s", partialEntity_swiftName(entity, buf));
		wrap(".%s__observer(SELF:SELF, removing:removing, adding:adding) }\n", partialEntityAttribute_swiftName(attr, buf));
	}
	indent_swift(level);
	raw("}\n");
	
	simpleAttribureObservers_swift(entity, attr, level);
}

static void explicitStaticAttributeDefinition_swift(Entity entity, Variable attr, int level) {
	char attrName[BUFSIZ];
	attributeHead_swift("public", attr, level, attrName);
	if( VARis_observed(attr) ) raw(" //OBSERVED");
	raw("\n");
	
	if( VARis_observed(attr) ) {
		if( TYPEis_aggregate(attr->type)) {
			aggregateAttributeObservers_swift(entity, attr, level);
		}
		else {
			simpleAttribureObservers_swift(entity, attr, level);
		}
	}
}

static void explicitDynamicAttributeDefinition_swift(Entity entity, Variable attr, int level) {
	char attrName[BUFSIZ];
	attributeHead_swift("public", attr, level, attrName);
	raw(" //DYNAMIC");
	if( VARis_observed(attr) ) raw(" //OBSERVED");
	raw(" \n");
	
	if( VARis_observed(attr) ) {
		if( TYPEis_aggregate(attr->type)) {
			aggregateAttributeObservers_swift(entity, attr, level);
		}
		else {
			simpleAttribureObservers_swift(entity, attr, level);
		}
	}

	indent_swift(level);
	raw("public func %s__getter(SELF: %s) -> ", attrName, ENTITY_swiftName(entity) );
	
	if( VARget_optional(attr) ) {
		raw("( ");
	}
	TYPE_head_swift(attr->type, level+nestingIndent_swift);
	if( VARget_optional(attr) ) {
		raw(" )?");
	}
	raw(" {\n");
	
	indent_swift(level+nestingIndent_swift);
	raw("return self.%s\n", attrName);
	
	indent_swift(level);
	raw("}\n");
}

static void derivedAttributeDefinition_swift(Entity entity, Variable attr, int level) {
	indent_swift(level);
	raw("//DERIVE\n");

	char attrName[BUFSIZ];
	partialEntityAttribute_swiftName(attr, attrName);
	
	indent_swift(level);
	raw("public func %s__getter(SELF: %s) -> ", attrName, ENTITY_swiftName(entity) );
	
	if( VARget_optional(attr) ) {
		raw("( ");
	}
	TYPE_head_swift(attr->type, level);
	if( VARget_optional(attr) ) {
		raw(" )?");
	}
	raw(" {\n");
	
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw("let value = ");
		EXPR_swift(entity, VARget_initializer(attr), 0 );
		raw("\n");
		
		indent_swift(level2);
		raw("return value\n", attrName);
	}
	
	indent_swift(level);
	raw("}\n");
}


static void derivedAttributeRedefinition_swift(Entity entity, Variable attr, int level) {
	Variable original_attr = VARget_redeclaring_attr(attr);
	
	indent_swift(level);
	raw("//DERIVE REDEFINITION(%s)", ENTITY_swiftName(original_attr->defined_in));
	if( VARis_observed(original_attr) ) raw(" //OBSERVED ORIGINAL");
	raw("\n");

	char attrName[BUFSIZ];
	partialEntityAttribute_swiftName(attr, attrName);
	
	indent_swift(level);
	raw("public func %s__getter(SELF: %s) -> ", attrName, ENTITY_swiftName(entity) );
	
	if( VARget_optional(attr) ) {
		raw("( ");
	}
	TYPE_head_swift(attr->type, level);
	if( VARget_optional(attr) ) {
		raw(" )?");
	}
	raw(" {\n");
	
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw("let value = ");
		EXPR_swift(entity, VARget_initializer(attr), NO_PAREN );
		raw("\n");
		
		if( VARis_observed(original_attr) ) {
			indent_swift(level2);
			raw("SELF.SUPER_%s.%s = value\n", ENTITY_swiftName(original_attr->defined_in), attrName );
		}
		
		indent_swift(level2);
		raw("return value\n");
	}
	
	
	indent_swift(level);
	raw("}\n");
}


static void inverseAttributeDefinition_swift(Variable attr, int level) {
	indent_swift(level);
	raw("//INVERSE\n");
	
	char attrName[BUFSIZ];
	attributeHead_swift("public private(set)", attr, level, attrName);
	raw("\n");
	
	char attrBaseType[BUFSIZ];
	TYPEhead_string_swift(ATTRget_base_type(attr), attrBaseType);
	
	// addedReference observer func
	indent_swift(level);
	raw("public func %s__observeAddedReference(in complex: ComplexEntity) {\n", attrName);
	
	indent_swift(level+nestingIndent_swift);
	if( TYPEis_aggregate(attr->type)) {
		raw("self.%s.add(member: %s.makeReference(from: complex) )\n", 
				attrName, 
				attrBaseType );
	}
	else {
		raw("self.%s = %s.makeReference(from: complex)", 
				attrName, 
				attrBaseType );
		if( !VARget_optional(attr) ) {
			raw("!");
		}
		raw("\n");
	}
	indent_swift(level);
	raw("} // INVERSE ATTR SUPPORT\n");

	// removedReference observer func
	indent_swift(level);
	raw("public func %s__observeRemovedReference(in complex: ComplexEntity) {\n", attrName);
	
	indent_swift(level+nestingIndent_swift);
	if( TYPEis_aggregate(attr->type)) {
		raw("self.%s.remove(member: %s.makeReference(from: complex) )\n", 
				attrName, 
				attrBaseType );
	}
	else {
		if( VARget_optional(attr) ) {
			raw("self.%s = nil\n", attrName );
		}
	}
	indent_swift(level);
	raw("} // INVERSE ATTR SUPPORT\n");

}


static void localAttributeDefinitions_swift( Entity entity, int level ){
	raw("\n");
	indent_swift(level);
	raw("//ATTRIBUTES\n");
	
	LISTdo( entity->u.entity->attributes, attr, Variable ){
		if( VARis_redeclaring(attr) ) {
			if( VARis_derived(attr) ) {
				derivedAttributeRedefinition_swift(entity, attr, level);
			}
		}
		else {
			if( VARis_derived(attr) ) {
				derivedAttributeDefinition_swift(entity, attr, level);
			}
			else if( VARis_inverse(attr) ){
				inverseAttributeDefinition_swift(attr, level);
			}
			else {	// explicit
				if( VARis_dynamic(attr) ) {
					explicitDynamicAttributeDefinition_swift(entity, attr, level);
				}
				else {
					explicitStaticAttributeDefinition_swift(entity, attr, level);
				}
			}
		}
		raw("\n");
	}LISTod;	
}


static void whereDefinitions_swift( Entity entity, int level ) {
	raw("\n");
	indent_swift(level);
	raw("//WHERE RULES\n");

	char whereLabel[BUFSIZ];
	LISTdo( TYPEget_where(entity), where, Where ){
		indent_swift(level);
		raw("public func %s(SELF: %s) -> LOGICAL {\n", 
				 whereLabel_swiftName(where, whereLabel), 
				 ENTITY_swiftName(entity) );
		
		indent_swift(level+nestingIndent_swift);
		EXPR_swift(entity, where->expr, 0);
		
		indent_swift(level);
		raw("}\n");
	}LISTod
}


static void partialEntityDefinition_swift( Entity entity, int level ) {
	raw("\n");
	indent_swift(level);
	{	char buf[BUFSIZ];
		wrap("public class %s : SDAI.PartialEntity {\n", partialEntity_swiftName(entity, buf));
	}
	
	localAttributeDefinitions_swift(entity, level+nestingIndent_swift);
	whereDefinitions_swift(entity, level+nestingIndent_swift);
	
	indent_swift(level);
	raw("}\n");
}

static void entityReferenceDefinition_swift( Entity entity, int level ) {
	raw("\n");
	indent_swift(level);
	{	//char buf[BUFSIZ];
		wrap("public class %s : SDAI.EntityReference {\n", ENTITY_swiftName(entity));
	}
	
//	localAttributesDefinition_swift(entity, level+nestingIndent_swift);
	
	indent_swift(level);
	raw("}\n");
}


void ENTITY_swift( Entity e, int level ) {
	beginExpress_swift("ENTITY DEFINITION");
	ENTITY_out(e, level);
	endExpress_swift();	
	
	raw("\n/* ALL DEFINED ATTRIBUTES");
	entity_no = 0;
	listAllAttributes(e, e, 1, ++__SCOPE_search_id);
	raw("\n*/\n");
	
	partialEntityDefinition_swift(e, level);
	
	entityReferenceDefinition_swift(e, level);
	
}

void ENTITYunique_swift( Linked_List u, int level ) {
    int i;
    int max_indent;
    Symbol * sym;

    if( !u ) {
        return;
    }

    raw( "%*sUNIQUE\n", level, "" );

    /* pass 1 */
    max_indent = 0;
    LISTdo( u, list, Linked_List ) {
        if( 0 != ( sym = ( Symbol * )LISTget_first( list ) ) ) {
            int length;
            length = strlen( sym->name );
            if( length > max_indent ) {
                max_indent = length;
            }
        }
    } LISTod

    level += exppp_nesting_indent;
    indent2 = level + max_indent + strlen( ": " ) + exppp_continuation_indent;

    LISTdo( u, list, Linked_List ) {
        i = 0;
        LISTdo_n( list, e, Expression, b ) {
            i++;
            if( i == 1 ) {
                /* print label if present */
                if( e ) {
                    raw( "%*s%-*s : ", level, "", max_indent, ( ( Symbol * )e )->name );
                } else {
                    raw( "%*s%-*s   ", level, "", max_indent, "" );
                }
            } else {
                if( i > 2 ) {
                    raw( ", " );
                }
                EXPR_out( e, 0 );
            }
        } LISTod
        raw( ";\n" );
    } LISTod
}

