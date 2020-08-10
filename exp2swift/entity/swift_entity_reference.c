//
//  swift_entity_reference.c
//  exp2swift
//
//  Created by Yoshida on 2020/07/26.
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
#include "swift_symbol.h"

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
				 ENTITY_swiftName(super), ENTITY_swiftName(super), ++entity_index);
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

//MARK: - Attirbute References

static void attributeRefHead_swift(Entity entity, char* access, Variable attr, int level, char* label) {	
	indent_swift(level);
	raw("//in ENTITY: %s(%s)\t//%s\n", 
			(entity==attr->defined_in ? "SELF" : "SUPER"), ENTITY_swiftName(attr->defined_in), label);

	indent_swift(level);
	raw("%s var %s: ", access, attribute_swiftName(attr) );
	
	variableType_swift(attr, false, level+nestingIndent_swift);
	
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
		char* sep = "";
		char buf[BUFSIZ];
		assert(original->overriders != NULL);
		DICTdo_init(original->overriders, &de);
		while( 0 != ( overrider = DICTdo( &de ) ) ) {
			if( !VARis_derived(overrider) ) continue;
			raw("%s",sep);
			wrap("\"%s\"",  ENTITY_canonicalName(overrider->defined_in, buf));
			sep = ", ";
		}
		wrap("])");
		wrap(" as? %s {\n", dynamicAttribute_swiftProtocolName(original, buf) );
		{	int level3 = level2+nestingIndent_swift;
			
			indent_swift(level3);
			wrap("return resolved" );
			wrap(".%s__getter(", partialAttr );
			wrap("complex: self.complexEntity)\n", partialAttr );
		}
		indent_swift(level2);
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
	Linked_List effective_attrs = LISTcreate();
	
	DICTdo_init( all_attrs, &de );
	while( 0 != ( attr_defs = ( Linked_List )DICTdo( &de ) ) ) {
		Variable attr = LISTget_first(attr_defs);
		char* attrName = VARget_simple_name(attr);

		if( ENTITYget_attr_ambiguous_count(entity, attrName) > 1 ) {
			indent_swift(level);
			wrap("// %s: (AMBIGUOUS REF)\n\n", attrName);
			continue;
		}
		
		LISTadd_last(effective_attrs, attr);
	}
	
	LISTdo(effective_attrs, attr, Variable) {
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
	}LISTod;
}

void entityReferenceDefinition_swift( Entity entity, int level ) {
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

