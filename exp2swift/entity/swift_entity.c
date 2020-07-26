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

const char* ENTITY_canonicalName( Entity e, char buf[BUFSIZ] ) {
	char* to = buf;
	int remain = BUFSIZ-1;
	for(char* from = e->symbol.name; *from; ++from ) {
		if( islower(*from) ) {
			(*to) = toupper(*from);
		}
		else {
			(*to) = (*from);
		}
		++to;
		--remain;
		if( remain == 0 ) break;
	}
	(*to) = 0;
	return buf;
}

const char* ENTITY_swiftProtocolName( Entity e, char buf[BUFSIZ]) {
	snprintf(buf, BUFSIZ, "%s_protocol", ENTITY_swiftName(e));
	return buf;
}

const char* partialEntity_swiftName( Entity e, char buf[BUFSIZ] ) {
	snprintf(buf, BUFSIZ, "_%s", ENTITY_swiftName(e));
	return buf;	
}

const char* attribute_swiftName( Variable attr ) {
	return ATTRget_name_string(attr);
}

const char* partialEntityAttribute_swiftName( Variable attr, char buf[BUFSIZ] ) {
	snprintf(buf, BUFSIZ, "_%s", attribute_swiftName(attr));
	return buf;
}

const char* dynamicAttribute_swiftProtocolName( Variable original, char buf[BUFSIZ] ) {
	snprintf(buf, BUFSIZ, "%s__%s__provider", ENTITY_swiftName(original->defined_in), attribute_swiftName(original));
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



//MARK: - entity information

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
			Variable effective_definition = ENTITYfind_attribute_effective_definition(leaf, ATTRget_name_string(v));
			DictionaryEntry de;
			Variable overrider;
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
	indent_swift(level);
	raw("ENTITY(%d)\t%s\n", ++entity_no, entity->symbol.name);

	LISTdo( entity->u.entity->attributes, attr, Generic ) {
		attribute_out(leaf, attr, level + nestingIndent_swift);
	} LISTod;

	raw("\n");
}

static void listAllAttributes( Entity leaf, Entity entity, int level ) {
#if 0
//	if( entity->search_id == __SCOPE_search_id ) return;
//	entity->search_id = __SCOPE_search_id;
	if( SCOPE_search_visited(entity) ) return;
	
	LISTdo( entity->u.entity->supertypes, super, Entity ) {
		listAllAttributes(leaf, super, level);
	}
	LISTod;
	
	listLocalAttributes(leaf, entity, level);
#endif
	
	Linked_List supertypes = ENTITYget_super_entity_list(leaf);
	
	LISTdo(supertypes, super, Entity ) {
		listLocalAttributes(leaf, super, level);
	} LISTod;
	
//	listLocalAttributes(leaf, entity, level);
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
	attributeHead_swift("internal", attr, level, attrName);
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

static void explicitAttributeRedefinition_swift(Entity entity, Variable attr, int level) {
	char attrName[BUFSIZ];
	attributeHead_swift("//override", attr, level, attrName);
	raw("\t//EXPLICIT REDEFINITION(%s)", ENTITY_swiftName(attr->original_attribute->defined_in));
	if( VARis_observed(attr) ) raw(" //OBSERVED");
}

static void explicitDynamicAttributeDefinition_swift(Entity entity, Variable attr, int level) {
	char attrName[BUFSIZ];
	attributeHead_swift("internal", attr, level, attrName);
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
}

static void derivedAttributeGetterDefinitionHead(Variable attr, char *attrName, Entity entity, int level) {
	indent_swift(level);
	raw("internal func %s__getter(SELF: %s) -> ", attrName, ENTITY_swiftName(entity) );
	
	if( VARget_optional(attr) ) {
		raw("( ");
	}
	TYPE_head_swift(attr->type, level);
	if( VARget_optional(attr) ) {
		raw(" )?");
	}
}

static void derivedRedefinitionGetterDefinitionHead(Variable attr, char *attrName, Entity entity, int level) {
	indent_swift(level);
	raw("internal func %s__getter(complex: SDAI.ComplexEntity) -> ", attrName );
	
	if( VARget_optional(attr) ) {
		raw("( ");
	}
	TYPE_head_swift(attr->type, level);
	if( VARget_optional(attr) ) {
		raw(" )?");
	}
}

static void explicitDynamicAttributeProtocolDefinition_swift(Entity entity, Variable attr, int level) {
	char buf[BUFSIZ];
	
	indent_swift(level);
	wrap("internal protocol %s {\n", dynamicAttribute_swiftProtocolName(attr, buf));
	
	{	int level2 = level + nestingIndent_swift;

		char attrName[BUFSIZ];
		partialEntityAttribute_swiftName(attr, attrName);
		derivedRedefinitionGetterDefinitionHead(attr, attrName, entity, level2);
		raw("\n");
	}
	
	indent_swift(level);
	wrap("}\n");
}

static void recoverSELFfromComplex_swift(Entity entity, int level) {
	indent_swift(level);
	wrap("let SELF = complex.entityReference( %s.self )!\n", ENTITY_swiftName(entity));
}

static void derivedAttributeDefinition_swift(Entity entity, Variable attr, int level) {
	indent_swift(level);
	raw("//\tDERIVE\n");

	char attrName[BUFSIZ];
	partialEntityAttribute_swiftName(attr, attrName);
	
	derivedAttributeGetterDefinitionHead(attr, attrName, entity, level);
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
	raw("//\tDERIVE REDEFINITION(%s)", ENTITY_swiftName(original_attr->defined_in));
	if( VARis_observed(original_attr) ) raw(" //OBSERVED ORIGINAL");
	raw("\n");

	char attrName[BUFSIZ];
	partialEntityAttribute_swiftName(attr, attrName);
	
	derivedRedefinitionGetterDefinitionHead(original_attr, attrName, entity, level);
	raw(" {\n");
	{	int level2 = level+nestingIndent_swift;
		
		recoverSELFfromComplex_swift(entity, level2);

		indent_swift(level2);
		raw("return %s__getter(SELF: SELF)\n", attrName);
	}
	indent_swift(level);
	raw("}\n");
	
	derivedAttributeGetterDefinitionHead(original_attr, attrName, entity, level);
	raw(" {\n");
	{	int level2 = level+nestingIndent_swift;
		
		recoverSELFfromComplex_swift(entity, level2);

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
	raw("//\tINVERSE\n");
	
	char attrName[BUFSIZ];
	attributeHead_swift("internal private(set)", attr, level, attrName);
	raw("\n");
	
	char attrBaseType[BUFSIZ];
	TYPEhead_string_swift(ATTRget_base_type(attr), attrBaseType);
	
	// addedReference observer func
	indent_swift(level);
	raw("internal func %s__observeAddedReference(in complex: ComplexEntity) {\n", attrName);
	
	{	int level2 = level+nestingIndent_swift;

		indent_swift(level2);
		wrap("let entityRef: %s? = complex\n", attrBaseType);
			
		indent_swift(level2);
		if( TYPEis_aggregate(attr->type)) {
			wrap("self.%s.add(member: entityRef )\n", attrName );
		}
		else {
			wrap("self.%s = entityRef", attrName );
			if( !VARget_optional(attr) ) {
				raw("!");
			}
			raw("\n");
		}
	}
	indent_swift(level);
	raw("} // INVERSE ATTR SUPPORT(ADDER)\n");

	// removedReference observer func
	indent_swift(level);
	raw("internal func %s__observeRemovedReference(in complex: ComplexEntity) {\n", attrName);
	
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		wrap("let entityRef: %s? = complex\n", attrBaseType);

		indent_swift(level2);
		if( TYPEis_aggregate(attr->type)) {
			wrap("self.%s.remove(member: entityRef )\n", attrName );
		}
		else {
			if( VARget_optional(attr) ) {
				wrap("self.%s = nil\n", attrName );
			}
		}
	}
	
	indent_swift(level);
	raw("} // INVERSE ATTR SUPPORT(REMOVER)\n");

}


static void localAttributeDefinitions_swift
 ( Entity entity, int level, Linked_List attr_overrides, Linked_List dynamic_attrs ) {
	 Linked_List local_attributes = entity->u.entity->attributes;
	 
	raw("\n");
	indent_swift(level);
	raw("//ATTRIBUTES\n");
	
	LISTdo( local_attributes, attr, Variable ){
		if( VARis_redeclaring(attr) ) {
			if( VARis_derived(attr) ) {
				derivedAttributeRedefinition_swift(entity, attr, level);
				LISTadd_last(attr_overrides, attr);
			}
			else {
				explicitAttributeRedefinition_swift(entity, attr, level);
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
					LISTadd_last(dynamic_attrs, attr);
				}
				else {
					explicitStaticAttributeDefinition_swift(entity, attr, level);
				}
			}
		}
		raw("\n");
	}LISTod;
	 if( LISTempty(local_attributes) ) {
		 indent_swift(level);
		 raw("// (no local attributes)\n\n");
	 }
}


static void whereDefinitions_swift( Entity entity, int level ) {
	Linked_List where_rules = TYPEget_where(entity);
	if( LISTempty(where_rules) ) return;
	
	indent_swift(level);
	raw("//WHERE RULES\n");

	char whereLabel[BUFSIZ];
	LISTdo( where_rules, where, Where ){
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


static void partialEntityDefinition_swift
 ( Entity entity, int level, Linked_List attr_overrides, Linked_List dynamic_attrs ) {
	raw("\n\n");
	raw("//MARK: - Partial Entity\n");

	indent_swift(level);
	{	
		char buf[BUFSIZ];
		wrap("public class %s : SDAI.PartialEntity {\n", partialEntity_swiftName(entity, buf));
	}
	
	localAttributeDefinitions_swift(entity, level+nestingIndent_swift, attr_overrides, dynamic_attrs);
	whereDefinitions_swift(entity, level+nestingIndent_swift);
	
	indent_swift(level);
	raw("}\n");
}

static void partialEntityAttrOverrideProtocolConformance_swift
 ( Entity entity, int level, Linked_List attr_overrides ) {
	 char buf[BUFSIZ];
	
	 indent_swift(level);
	 wrap("extension %s :", partialEntity_swiftName(entity, buf));
	 char* sep = "";
	 LISTdo( attr_overrides, attr, Variable ) {
		 assert(attr->original_attribute);
		 wrap("%s %s", sep, dynamicAttribute_swiftProtocolName(attr->original_attribute, buf));
		 sep = ",";
	 }LISTod;
	 wrap(" {}\n");
}

//MARK: - entity reference

static void supertypeReferenceDefinition_swift( Entity entity, int level ) {
	raw("\n");
	indent_swift(level);
	raw("//SUPERTYPES\n");
	
	Linked_List supertypes = ENTITYget_super_entity_list(entity);
	
	int entity_index = 0;
	LISTdo( supertypes, super, Entity ) {
		if( super == entity ) continue;
		indent_swift(level);
		wrap("public let SUPER_%s: %s \t// [%d]\n", 
				 ENTITY_swiftName(super), ENTITY_swiftName(super), entity_index++);
	}LISTod;
}

static void partialEntityReferenceDefinition_swift( Entity entity, int level ) {
	raw("\n");
	indent_swift(level);
	raw("//PARTIAL ENTITY\n");
	
	char buf[BUFSIZ];
	indent_swift(level);
	wrap("public let partialEntity: %s\n", partialEntity_swiftName(entity, buf));

}


static void superEntityReference_swift(Entity entity, Entity supertype) {
	if( supertype == entity ) {
		wrap("self");
	}
	else {
		wrap("SUPER_%s", ENTITY_swiftName(supertype) );
	}
}

static void partialEntityReference_swift(Entity entity, Entity partial) {
	superEntityReference_swift(entity, partial);
	wrap(".partialEntity");
}

//MARK: Attirbute References

static void attributeRefHead_swift(Entity entity, char* access, Variable attr, int level, char* label) {	
	indent_swift(level);
	raw("//in ENTITY: %s(%s)\t//%s\n", 
			(entity==attr->defined_in ? "SELF" : "SUPER"), ENTITY_swiftName(attr->defined_in), label);

	indent_swift(level);
	raw("%s var %s: ", access, attribute_swiftName(attr) );
	
	if( VARget_optional(attr) ) {
		raw("( ");
	}
	
	TYPE_head_swift(attr->type, level+nestingIndent_swift);
	
	if( VARget_optional(attr) ) {
		raw(" )?");
	}
	
	raw(" {\n");
}
static void simpleAttribureObserversCall_swift(Entity entity, Entity partial, Variable attr, int level) {
	char buf[BUFSIZ];
	
	indent_swift(level);
	partialEntityReference_swift(entity, partial);
	wrap(".%s__observer(", partialEntityAttribute_swiftName(attr, buf));
	wrap(" SELF:self, removing:oldValue, adding:newValue )\n");
	
}

static void aggregateAttributeObserversCall_swift(Entity entity, Entity partial, Variable attr, int level) {
	char buf[BUFSIZ];
	
	indent_swift(level);
	partialEntityReference_swift(entity, partial);
	wrap(".%s__aggregateObserver(", partialEntityAttribute_swiftName(attr, buf));
	wrap(" SELF:self, oldAggregate:&oldValue, newAggregate:&newValue )\n");
}


static void explicitSetter_swift(Variable attr, Entity entity, int level, Entity partial, char *partialAttr) {
	indent_swift(level);
	raw("set(newValue) {\n");
	{	int level2 = level+nestingIndent_swift;
		
		if( VARis_observed(attr) ) {
			indent_swift(level2);
			wrap("let oldValue = self.%s\n", attribute_swiftName(attr) );
			
			if( TYPEis_aggregate(attr->type)) {
				aggregateAttributeObserversCall_swift(entity, partial, attr, level2);
			}
			else {
				simpleAttribureObserversCall_swift(entity, partial, attr, level2);
			}
		}
		
		indent_swift(level2);
		partialEntityReference_swift(entity, partial);
		wrap(".%s = newValue\n", partialAttr );
	}
	indent_swift(level);
	raw("}\n");
}

static void explicitStaticGetter_swift(Entity entity, int level, Entity partial, char *partialAttr) {
	indent_swift(level);
	raw("get {\n");
	{	int level2 = level+nestingIndent_swift;
		indent_swift(level2);
		raw("return ");
		partialEntityReference_swift(entity, partial);
		wrap(".%s\n", partialAttr );
	}
	indent_swift(level);
	raw("}\n");
}

static void explicitStaticGetterSetter_swift(Variable attr, Entity entity, int level, Entity partial) {
	char partialAttr[BUFSIZ];
	partialEntityAttribute_swiftName(attr, partialAttr);
	
	// getter
	explicitStaticGetter_swift(entity, level, partial, partialAttr);
	
	// setter
	explicitSetter_swift(attr, entity, level, partial, partialAttr);
}

static void explicitDynamicGetterSetter_swift(Variable attr, Entity entity, int level, Entity partial, Variable original) {
	char partialAttr[BUFSIZ];
	partialEntityAttribute_swiftName(attr, partialAttr);
	
	// getter
	indent_swift(level);
	raw("get {\n");
	{	int level2 = level+nestingIndent_swift;

		indent_swift(level2);
		wrap("if let resolved = self.complexEntity.resolvePartialEntityInstance(from: [");
		DictionaryEntry de;
		Variable overrider;
		char buf[BUFSIZ];
		assert(original->overriders != NULL);
		DICTdo_init(original->overriders, &de);
		while( 0 != ( overrider = DICTdo( &de ) ) ) {
			if( !VARis_derived(overrider) ) continue;
			wrap("\"%s\", ", ENTITY_canonicalName(overrider->defined_in, buf));
		}
		wrap("]) as? %s {\n", dynamicAttribute_swiftProtocolName(original, buf) );
		{	int level3 = level2+nestingIndent_swift;
			
			indent_swift(level3);
			wrap("return resolved.%s__getter(complex: self.complexEntity)\n", partialAttr );
		}
		wrap("}\n");

		indent_swift(level2);
		wrap("else {\n");
		{	int level3 = level2+nestingIndent_swift;
			
			indent_swift(level3);
			raw("return ");
			partialEntityReference_swift(entity, original->defined_in);
			wrap(".%s\n", partialAttr );
		}
		indent_swift(level2);
		wrap("}\n");
	}
	indent_swift(level);
	raw("}\n");
	
	// setter
	explicitSetter_swift(attr, entity, level, partial, partialAttr);
}


static void explicitAttributeReference_swift(Entity entity, Variable attr, int level) {
	Entity partial = attr->defined_in;
	
	if( VARis_dynamic(attr) ) {
		attributeRefHead_swift( entity, "public", attr, level, 
													 (VARis_observed(attr) 
														? "EXPLICIT(DYNAMIC)(OBSERVED)" 
														: "EXPLICIT(DYNAMIC)") );
		{	int level2 = level+nestingIndent_swift;
			
			explicitDynamicGetterSetter_swift(attr, entity, level2, partial, attr);
		}
		indent_swift(level);
		raw("}\n");
	}
	else {
		attributeRefHead_swift( entity, "public", attr, level, 
													 (VARis_observed(attr) 
														? "EXPLICIT(OBSERVED)" 
														: "EXPLICIT") );
		{	int level2 = level+nestingIndent_swift;
			
			explicitStaticGetterSetter_swift(attr, entity, level2, partial);
		}
		indent_swift(level);
		raw("}\n");
	}
}


static void derivedGetter_swift(Variable attr, Entity entity, int level, Entity partial) {
	char partialAttr[BUFSIZ];
	
	// getter
	indent_swift(level);
	raw("get {\n");
	{	int level2 = level+nestingIndent_swift;
		indent_swift(level2);
		raw("return ");
		partialEntityReference_swift(entity, partial);
		wrap(".%s__getter(SELF: ", partialEntityAttribute_swiftName(attr, partialAttr) );
		superEntityReference_swift(entity, partial);
		raw(")\n");
	}
	indent_swift(level);
	raw("}\n");
}

static void explicitAttributeOverrideReference_swift(Entity entity, Variable attr, int level) {
	Variable original = VARget_redeclaring_attr(attr);
	Entity partial = original->defined_in;
	
	if( VARis_dynamic(original) ) {
		attributeRefHead_swift( entity, "public", attr, level, 
													 (VARis_observed(attr) 
														? "EXPLICIT REDEF(DYNAMIC)(OBSERVED)" 
														: "EXPLICIT REDEF(DYNAMIC)") );
		{	int level2 = level+nestingIndent_swift;
			
			explicitDynamicGetterSetter_swift(attr, entity, level2, partial, original);
		}
		indent_swift(level);
		raw("}\n");
	}
	else {
		attributeRefHead_swift( entity, "public", attr, level, 
													 (VARis_observed(attr) 
														? "EXPLICIT REDEF(OBSERVED)" 
														: "EXPLICIT REDEF") );
		{	int level2 = level+nestingIndent_swift;
			
			explicitStaticGetterSetter_swift(attr, entity, level2, partial);		
		}
		indent_swift(level);
		raw("}\n");
	}
	
}


static void derivedAttributeReference_swift(Entity entity, Variable attr, int level) {
	Entity partial = attr->defined_in;
	
	attributeRefHead_swift( entity, "public", attr, level, "DERIVE" );
	{	int level2 = level+nestingIndent_swift;
		
		derivedGetter_swift(attr, entity, level2, partial);
	}
	indent_swift(level);
	raw("}\n");
}

static void derivedAttributeOverrideReference_swift(Entity entity, Variable attr, int level) {
	Entity partial = attr->defined_in;
	
	assert(VARis_dynamic(attr));
	attributeRefHead_swift( entity, "public", attr, level, "EXPLICIT REDEF(DERIVE)" );
	{	int level2 = level+nestingIndent_swift;

		derivedGetter_swift(attr, entity, level2, partial);
	}
	indent_swift(level);
	raw("}\n");
}

static void inverseAttributeReference_swift(Entity entity, Variable attr, int level) {
	Entity partial = attr->defined_in;
	char partialAttr[BUFSIZ];
	partialEntityAttribute_swiftName(attr, partialAttr);

	attributeRefHead_swift( entity, "public", attr, level, "INVERSE" );
	
	{	int level2 = level+nestingIndent_swift;
		
		// getter
		explicitStaticGetter_swift(entity, level2, partial, partialAttr);
	}
	
	indent_swift(level);
	raw("}\n");
}

static void entityAttributeReferences_swift( Entity entity, int level ){
	raw("\n");
	indent_swift(level);
	raw("//ATTRIBUTES\n");
	
	Dictionary all_attrs = ENTITYget_all_attributes(entity);
	DictionaryEntry de;
	Linked_List attr_defs;
	Variable attr;
	
	DICTdo_init( all_attrs, &de );
	while( 0 != ( attr_defs = ( Linked_List )DICTdo( &de ) ) ) {
		attr = LISTget_first(attr_defs);
		
		char* attrName = VARget_simple_name(attr);

		if( ENTITYget_attr_ambiguous_count(entity, attrName) > 1 ) {
			indent_swift(level);
			wrap("// %s: (AMBIGUOUS REF)\n\n", attrName);
			continue;
		}
		
		if( VARis_redeclaring(attr) ) {
			if( VARis_derived(attr) ) {
				derivedAttributeOverrideReference_swift(entity, attr, level);
			}
			else {
				explicitAttributeOverrideReference_swift(entity, attr, level);
			}
		}
		else {
			if( VARis_derived(attr) ) {
				derivedAttributeReference_swift(entity, attr, level);
			}
			else if( VARis_inverse(attr) ){
				inverseAttributeReference_swift(entity, attr, level);
			}
			else {	// explicit
				explicitAttributeReference_swift(entity, attr, level);
			}
		}
		raw("\n");
	}
}

static void entityReferenceDefinition_swift( Entity entity, int level ) {
	raw("\n\n");
	raw("//MARK: - Entity Reference\n");
	indent_swift(level);
	wrap("public class %s : SDAI.EntityReference {\n", ENTITY_swiftName(entity));
	
	{	int level2 = level+nestingIndent_swift;
		partialEntityReferenceDefinition_swift(entity, level2);
		supertypeReferenceDefinition_swift(entity, level2);
		entityAttributeReferences_swift(entity, level2);
	}
	
	indent_swift(level);
	raw("}\n");
}



//MARK: - main entry point

void ENTITY_swift( Entity entity, int level, Linked_List dynamic_attrs ) {
	// EXPRESS summary
	beginExpress_swift("ENTITY DEFINITION");
	ENTITY_out(entity, level);
	endExpress_swift();	
	
	raw("\n//MARK: - ALL DEFINED ATTRIBUTES\n/*\n");
	entity_no = 0;
//	__SCOPE_search_id++;
//	SCOPE_begin_search();
	listAllAttributes(entity, entity, level);
//	SCOPE_end_search();
	raw("\n*/\n");
	
	// partial entity definition
	Linked_List attr_overrides = LISTcreate();
	partialEntityDefinition_swift(entity, level, attr_overrides, dynamic_attrs);
	if( !LISTempty(attr_overrides) ) {
		partialEntityAttrOverrideProtocolConformance_swift(entity, level, attr_overrides);
	}
	LISTfree(attr_overrides);

	// entity reference definition
	entityReferenceDefinition_swift(entity, level);
	
}

void ENTITY_swiftProtocol(Entity entity, int level, Linked_List dynamic_attrs ) {
	if( LISTempty(dynamic_attrs) ) return;
	
	raw("\n");
	raw("//MARK: - Entity Dynamic Attribute Protocols\n");

	LISTdo(dynamic_attrs, attr, Variable) {
		explicitDynamicAttributeProtocolDefinition_swift(entity, attr, level);
	}LISTod;
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

