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
#include "swift_algorithm.h"

//MARK: - entity reference

static void supertypeReferenceDefinition_swift(Entity entity, int level ) {
	int entity_index = 0;

	/*
	 public let super_<SUPERENTITY>: <SUPERENTITY> 	// [n]	 
	 */	
	raw("\n");
	indent_swift(level);
	raw("//SUPERTYPES\n");
	
	Linked_List supertypes = ENTITYget_super_entity_list(entity);
	
	char buf1[BUFSIZ];
	char buf2[BUFSIZ];
	LISTdo( supertypes, super, Entity ) {
		indent_swift(level);
		wrap("public let %s%s: %s \t// [%d]\n", 
				 superEntity_swiftPrefix,
				 ENTITY_swiftName(super, NO_QUALIFICATION, buf1), 
				 ENTITY_swiftName(super, entity, buf2), 
				 ++entity_index);
	}LISTod;

	/*
	 public let sub_<SUPERENTITY>: <SUPERENTITY>? 	// [n]	 
	 */	
	raw("\n");
	indent_swift(level);
	raw("//SUBTYPES\n");
	
	Linked_List subtypes = ENTITYget_sub_entity_list(entity);
	
	LISTdo( subtypes, sub, Entity ) {
		if( sub == entity )continue;
		indent_swift(level);
		wrap("public let %s%s: %s? \t// [%d]\n", 
				 subEntity_swiftPrefix,
				 ENTITY_swiftName(sub, NO_QUALIFICATION, buf1), 
				 ENTITY_swiftName(sub, entity, buf2), 
				 ++entity_index);
	}LISTod;
}

static void partialEntityReferenceDefinition_swift( Entity entity, int level ) {
	/*
	 public let partialEntity: _<entity>
	 */
	
	raw("\n");
	indent_swift(level);
	raw("//PARTIAL ENTITY\n");
	
	char buf[BUFSIZ];
	indent_swift(level);
	wrap("public let partialEntity: %s\n", partialEntity_swiftName(entity, buf));

}


static void superEntityReference_swift(Entity entity, Entity supertype, bool is_subtype_attr, bool can_wrap) {
	if( supertype == entity ) {
		wrap_if(can_wrap,"self");
	}
	else {
		char buf[BUFSIZ];
		wrap_if(can_wrap,"%s%s", (is_subtype_attr ? subEntity_swiftPrefix : superEntity_swiftPrefix), ENTITY_swiftName(supertype, NO_QUALIFICATION, buf) );
	}
}

static void partialEntityReference_swift(Entity entity, Entity partial, bool is_subtype_attr, bool can_wrap) {
	superEntityReference_swift(entity, partial, is_subtype_attr, can_wrap);
	if( is_subtype_attr )raw("?");
	wrap(".partialEntity");
}

//MARK: - Attirbute References

static void attributeRefHead_swift(Entity entity, char* access, Variable attr, bool is_subtype_attr, int level, char* label) {	
	 
	 indent_swift(level);
	 {
		 char buf[BUFSIZ];
		 raw("//in ENTITY: %s(%s)\t//%s\n", 
				 (entity==attr->defined_in ? "SELF" : (is_subtype_attr ? "SUB" : "SUPER")), 
				 ENTITY_swiftName(attr->defined_in, NO_QUALIFICATION, buf), 
				 label);
		 
		 indent_swift(level);
		 raw("%s var %s: ", access, attribute_swiftName(attr,buf) );
	 }
	 
	variableType_swift(entity, attr, (is_subtype_attr ? YES_FORCE_OPTIONAL : NO_FORCE_OPTIONAL), NOT_IN_COMMENT);
	 
	 raw(" {\n");
 }

static void simpleAttribureObserversCall_swift(Entity partial, Variable attr, bool is_subtype_attr, int level) {
	/*
	 _<partial>._<attr>__observer( SELF:self, removing:oldValue, adding:newValue )
	 */
	
	char buf[BUFSIZ];
	
	bool optional = (VARis_optional_by_large(attr) || is_subtype_attr);
	
	indent_swift(level);
	raw("%s",partialEntity_swiftName(partial, buf));
	wrap(".%s__observer(", partialEntityAttribute_swiftName(attr, buf));
	
	if( TYPEis_entity(attr->type) ){
		wrap(" SELF: self, removing: oldValue, adding: newValue )\n");
	}
	else {
		// attr type should be select type
		if( optional ){
			wrap(" SELF: self, removing: oldValue?.asFundamentalType.entityReference, adding: newValue?.asFundamentalType.entityReference )\n");
		}
		else{
			wrap(" SELF: self, removing: oldValue.asFundamentalType.entityReference, adding: newValue.asFundamentalType.entityReference )\n");
		}

	}
	
}

static const char* aggregateAttributeObserversCall_swift(Entity partial, Variable attr, bool is_subtype_attr, int level) {
	/*
	 newAggregate = _<partial>._<attr>__aggregateObserver( SELF:self, oldAggregate:oldValue, newAggregate:newValue )
	 */
	
	char buf[BUFSIZ];
	const char* newAggregate = "newAggregate";
	indent_swift(level);
	raw("let %s = %s",newAggregate,partialEntity_swiftName(partial, buf));
	wrap(".%s__aggregateObserver(", partialEntityAttribute_swiftName(attr, buf));
	wrap(" SELF:self, oldAggregate:oldValue, newAggregate:newValue )\n");
	return newAggregate;
}

//MARK: - explicit static attribute access
static void explicitSetter_swift(Variable attr, bool is_subtype_attr, Entity entity, int level, Entity partial, char *partialAttr) {
	/*
	 set(newValue) {
		 let oldValue = self.<ATTR>
			// observer call //
		 self.partialEntity.<partialAttr> = newValue
	 		// or //
	   super_<PARTIAL>.partialEntity.<partialAttr> = <ORIGINAL TYPE>(newValue)
	 }
	 
	 */
	
	const char* newValue = "newValue";
	indent_swift(level);
	raw("set(%s) {\n", newValue);
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		if( is_subtype_attr ){
			raw("guard let partial = ");
			superEntityReference_swift(entity, attr->defined_in, is_subtype_attr, YES_WRAP);
			raw("?");
			wrap(".");
			partialEntityReference_swift(entity, partial, false, NO_WRAP);
			wrap(" else { return }\n");
		}
		else{
			raw("let partial = ");
			partialEntityReference_swift(entity, partial, false, NO_WRAP);
			raw("\n");
		}
		
		if( attribute_need_observer(attr) ) {
			indent_swift(level2);
			{
				char buf[BUFSIZ];
				wrap("let oldValue = self.%s\n", attribute_swiftName(attr,buf) );
			}
			
			if( TYPEis_aggregation_data_type(attr->type)) {
				newValue = aggregateAttributeObserversCall_swift(partial, attr, is_subtype_attr, level2);
			}
			else {
				simpleAttribureObserversCall_swift(partial, attr, is_subtype_attr, level2);
			}
		}
		
		indent_swift(level2);
		if( VARis_redeclaring(attr) ){
//			if( is_subtype_attr ){
//				superEntityReference_swift(entity, attr->defined_in, is_subtype_attr, YES_WRAP);
//				raw("?");
//				wrap(".");
//			}
//			partialEntityReference_swift(entity, partial, false, NO_WRAP);

			wrap("partial.%s = ", partialAttr );
			if( !VARis_optional_by_large(attr) ){
				if( TYPEis_logical(VARget_type(attr)) ){
					wrap("SDAI.LOGICAL(");
				}
				else{
					wrap("SDAI.UNWRAP(");
				}
			}
			Variable original = VARget_original_attr(attr);
			aggressively_wrap();
//			variableType_swift(entity, original, NO_FORCE_OPTIONAL, WO_COMMENT);
			TYPE_head_swift(entity, original->type, WO_COMMENT);
			raw("(%s)", newValue );
			if( !VARis_optional_by_large(attr) ){
				raw(")");
			}
			raw("\n");
		}
		else {
//			partialEntityReference_swift(entity, partial, is_subtype_attr, YES_WRAP);
			if( VARis_optional_by_large(attr) ){
				wrap("partial.%s = %s\n", partialAttr, newValue );
			} 
			else {
				if( TYPEis_logical(VARget_type(attr)) ){
					wrap("partial.%s = SDAI.LOGICAL(%s)\n", partialAttr, newValue );
				}
				else{
					wrap("partial.%s = SDAI.UNWRAP(%s)\n", partialAttr, newValue );
				}
			}
		}
	}
	indent_swift(level);
	raw("}\n");
}

static void explicitStaticGetter_swift(Variable attr, bool is_subtype_attr, Entity entity, int level, Entity partial, char *partialAttr) {
	/*
	 get {
		 return self.partialEntity.<partialAttr>
	 }
	// or //	 
	 get {
		 return <ATTR TYPE>(super_<PARTIAL>.partialEntity.<partialAttr>)
	 }
	 */
	
	indent_swift(level);
	raw("get {\n");
	{	int level2 = level+nestingIndent_swift;
		indent_swift(level2);
		raw("return ");
		if( !is_subtype_attr && !VARis_optional_by_large(attr) ){
			raw("SDAI.UNWRAP( ");
		}
		else if( TYPEis_logical(VARget_type(attr)) ){
			raw("SDAI.LOGICAL( ");
		}
		if( VARis_redeclaring(attr) ){
//			variableType_swift(entity, attr, NO_FORCE_OPTIONAL, WO_COMMENT);
			TYPE_head_swift(entity, attr->type, WO_COMMENT);
			raw("( ");
			if( is_subtype_attr ){
				superEntityReference_swift(entity, attr->defined_in, is_subtype_attr,YES_WRAP);
				raw("?");
				wrap(".");
			}
			partialEntityReference_swift(entity, partial, false, NO_WRAP);
			wrap(".%s )", partialAttr );
		}
		else {
			partialEntityReference_swift(entity, partial, is_subtype_attr, YES_WRAP);
			wrap(".%s", partialAttr );
		}
		if( !is_subtype_attr && !VARis_optional_by_large(attr) ){
			raw(" )");
		}
		else if( TYPEis_logical(VARget_type(attr)) ){
			raw(" )");
		}
		raw("\n");
	}
	indent_swift(level);
	raw("}\n");
}

static void explicitStaticGetterSetter_swift(Variable attr, bool is_subtype_attr, Entity entity, int level, Entity partial) {
	char partialAttr[BUFSIZ];
	partialEntityAttribute_swiftName(attr, partialAttr);
	
	// getter
	explicitStaticGetter_swift(attr, is_subtype_attr, entity, level, partial, partialAttr);
	
	// setter
	explicitSetter_swift(attr, is_subtype_attr, entity, level, partial, partialAttr);
}

//MARK: - explicit dynamic attribute access
static void explicitDynamicGetterSetter_swift(Variable attr, bool is_subtype_attr, Entity entity, int level, Entity partial, Variable original) {
	/*
	 get {
		 if let resolved = self.complexEntity.resolvePartialEntityInstance(from: [
			 <overrider entity names>...]) as? <ORIGINAL>__<ATTR>__provider {
			 return <ATTR TYPE>(resolved._<attr>__getter(complex: self.complexEntity))
	 // or //
	     return resolved._<original>__getter(complex: self.complexEntity)
		 }
		 else {
			 return self.partialEntity._<attr>
		 }
	 }
	 set(newValue) {
	 	// same as explicit static attribute setter //
	 }
	 
	 */
	
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
			if( attr == original ){
				wrap("return resolved" );
				wrap(".%s__getter(", partialAttr );
				wrap("complex: self.complexEntity)\n", partialAttr );
			}
			else {
				wrap("return " );
				if( !is_subtype_attr && !VARis_optional_by_large(attr) ){
					wrap("SDAI.UNWRAP( ");
				}
				else if( TYPEis_logical(VARget_type(attr)) ){
					raw("SDAI.LOGICAL( ");
				}
//				variableType_swift(entity, attr, NO_FORCE_OPTIONAL, WO_COMMENT);
				TYPE_head_swift(entity, attr->type, WO_COMMENT);
				wrap("(resolved" );
				wrap(".%s__getter(", partialAttr );
				wrap("complex: self.complexEntity))" );
				if( !is_subtype_attr && !VARis_optional_by_large(attr) ){
					raw(" )");
				}
				else if( TYPEis_logical(VARget_type(attr)) ){
					raw(" )");
				}
				raw("\n");
			} 
		}
		indent_swift(level2);
		wrap("}\n");

		indent_swift(level2);
		wrap("else {\n");
		{	int level3 = level2+nestingIndent_swift;
			
			indent_swift(level3);
			raw("return ");
			if( attr == original ){
				partialEntityReference_swift(entity, original->defined_in, is_subtype_attr, YES_WRAP);
				wrap(".%s\n", partialAttr );
			}
			else {
				if( !is_subtype_attr && !VARis_optional_by_large(attr) ){
					wrap("SDAI.UNWRAP( ");
				}
				else if( TYPEis_logical(VARget_type(attr)) ){
					raw("SDAI.LOGICAL( ");
				}
//				variableType_swift(entity, attr, NO_FORCE_OPTIONAL, NOT_IN_COMMENT);
				TYPE_head_swift(entity, attr->type, WO_COMMENT);
				raw("(");

				if( is_subtype_attr ){
					superEntityReference_swift(entity, attr->defined_in, is_subtype_attr, YES_WRAP);
					raw("?");
					wrap(".");
				}
				partialEntityReference_swift(entity, original->defined_in, false, NO_WRAP);
				wrap(".%s)", partialAttr );
				if( !is_subtype_attr && !VARis_optional_by_large(attr) ){
					raw(" )");
				}
				else if( TYPEis_logical(VARget_type(attr)) ){
					raw(" )");
				}
				raw("\n");
			}
			
		}
		indent_swift(level2);
		wrap("}\n");
	}
	indent_swift(level);
	raw("}\n");
	
	// setter
	explicitSetter_swift(attr, is_subtype_attr, entity, level, partial, partialAttr);
}


static void explicitAttributeReference_swift(Entity entity, Variable attr, bool is_subtype_attr, int level) {
	Entity partial = attr->defined_in;
	
	if( VARis_dynamic(attr) ) {
		attributeRefHead_swift( entity, "public", attr, is_subtype_attr, level, 
													 (VARis_observed(attr) 
														? "EXPLICIT(DYNAMIC)(OBSERVED)" 
														: "EXPLICIT(DYNAMIC)") );
		{	int level2 = level+nestingIndent_swift;
			
			explicitDynamicGetterSetter_swift(attr, is_subtype_attr, entity, level2, partial, attr);
		}
		indent_swift(level);
		raw("}\n");
	}
	else {
		attributeRefHead_swift( entity, "public", attr, is_subtype_attr, level, 
													 (VARis_observed(attr) 
														? "EXPLICIT(OBSERVED)" 
														: "EXPLICIT") );
		{	int level2 = level+nestingIndent_swift;
			
			explicitStaticGetterSetter_swift(attr, is_subtype_attr, entity, level2, partial);
		}
		indent_swift(level);
		raw("}\n");
	}
}

//MARK: - derived attribute access
static void derivedGetter_swift(Variable attr, bool is_subtype_attr, Entity entity, int level, Entity partial) {
	/*
	 get {
		 return self.partialEntity._<attr>__getter(SELF: self)
	 }
	 
	 */
	
	char partialAttr[BUFSIZ];
	
	// getter
	indent_swift(level);
	raw("get {\n");
	{	int level2 = level+nestingIndent_swift;
		indent_swift(level2);
		if( is_subtype_attr ){
			raw("guard let origin");
			raw(" = ");
			superEntityReference_swift(entity, partial, is_subtype_attr, YES_WRAP);
			raw(" else { return nil }\n");
		}
		else {
			raw("let origin");
			raw(" = ");
			superEntityReference_swift(entity, partial, is_subtype_attr, YES_WRAP);
			raw("\n");

		}
		indent_swift(level2);
		raw("return ");
		if( !is_subtype_attr && !VARis_optional_by_large(attr) ){
			raw("SDAI.UNWRAP( ");
		}
		else if( TYPEis_logical(VARget_type(attr)) ){
			raw("SDAI.LOGICAL( ");
		}
		if( entity == partial ){
//			partialEntityReference_swift(entity, partial, is_subtype_attr);
			wrap("origin.partialEntity.%s__getter(SELF: origin", partialEntityAttribute_swiftName(attr, partialAttr) );
//			superEntityReference_swift(entity, partial, is_subtype_attr);
			raw(")");
		}
		else {
//			variableType_swift(entity, attr, NO_FORCE_OPTIONAL, WO_COMMENT);
			TYPE_head_swift(entity, attr->type, WO_COMMENT);
			raw("(");
//			partialEntityReference_swift(entity, partial, is_subtype_attr);
			wrap("origin.partialEntity.%s__getter(SELF: origin", partialEntityAttribute_swiftName(attr, partialAttr) );
//			superEntityReference_swift(entity, partial, is_subtype_attr);
			raw("))");
		}
		if( !is_subtype_attr && !VARis_optional_by_large(attr) ){
			raw(" )");
		}
		else if( TYPEis_logical(VARget_type(attr)) ){
			raw(" )");
		}
		raw("\n");
	}
	indent_swift(level);
	raw("}\n");
}

//MARK: explicit redef
static void explicitAttributeOverrideReference_swift(Entity entity, Variable attr, bool is_subtype_attr, int level) {
	Variable original = VARget_original_attr(attr);
	Entity partial = original->defined_in;
	
	if( VARis_dynamic(original) ) {
		attributeRefHead_swift( entity, "public", attr, is_subtype_attr, level, 
													 (VARis_observed(attr) 
														? "EXPLICIT REDEF(DYNAMIC)(OBSERVED)" 
														: "EXPLICIT REDEF(DYNAMIC)") );
		{	int level2 = level+nestingIndent_swift;
			
			explicitDynamicGetterSetter_swift(attr, is_subtype_attr, entity, level2, partial, original);
		}
		indent_swift(level);
		raw("}\n");
	}
	else {
		attributeRefHead_swift( entity, "public", attr, is_subtype_attr, level, 
													 (VARis_observed(attr) 
														? "EXPLICIT REDEF(OBSERVED)" 
														: "EXPLICIT REDEF") );
		{	int level2 = level+nestingIndent_swift;
			
			explicitStaticGetterSetter_swift(attr, is_subtype_attr, entity, level2, partial);		
		}
		indent_swift(level);
		raw("}\n");
	}
	
}


static void derivedAttributeReference_swift(Entity entity, Variable attr, bool is_subtype_attr, int level) {
	Entity partial = attr->defined_in;
	
	attributeRefHead_swift( entity, "public", attr, is_subtype_attr, level, "DERIVE" );
	{	int level2 = level+nestingIndent_swift;
		
		derivedGetter_swift(attr, is_subtype_attr, entity, level2, partial);
	}
	indent_swift(level);
	raw("}\n");
}

static void derivedAttributeOverrideReference_swift(Entity entity, Variable attr, bool is_subtype_attr, int level) {
	Entity partial = attr->defined_in;
	
	assert(VARis_dynamic(attr));
	attributeRefHead_swift( entity, "public", attr, is_subtype_attr, level, "EXPLICIT REDEF(DERIVE)" );
	{	int level2 = level+nestingIndent_swift;

		derivedGetter_swift(attr, is_subtype_attr, entity, level2, partial);
	}
	indent_swift(level);
	raw("}\n");
}

//MARK: - inverse attrivute access
static void inverseAttributeReference_swift(Entity entity, Variable attr, bool is_subtype_attr, int level) {
	Entity partial = attr->defined_in;
	char partialAttr[BUFSIZ];
	partialEntityAttribute_swiftName(attr, partialAttr);

	attributeRefHead_swift( entity, "public", attr, is_subtype_attr, level, "INVERSE" );
	
	{	int level2 = level+nestingIndent_swift;
		
		// getter
		explicitStaticGetter_swift(attr, is_subtype_attr, entity, level2, partial, partialAttr);
	}
	
	indent_swift(level);
	raw("}\n");
}

//MARK: - attribute access main entry point
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
		bool is_subtype_attr = LISTget_first_aux(attr_defs);
		
		if( ENTITYget_attr_ambiguous_count(entity, attrName) > 1 ) {
			if( attr->defined_in != entity ){
				indent_swift(level);
				char buf[BUFSIZ];
				wrap("// %s: (%d AMBIGUOUS REFs)\n\n", as_attributeSwiftName_n(attrName, buf, BUFSIZ), LISTget_length(attr_defs));
				continue;
			}
		}
		
//		LISTadd_last(effective_attrs, attr);
		LISTadd_last_marking_aux(effective_attrs, attr, (Generic)is_subtype_attr);
	}
	
	LISTdo(effective_attrs, attr, Variable) {
		bool is_subtype_attr = LIST_aux;
		if( VARis_redeclaring(attr) ) {
			if( VARis_derived(attr) ) {
				derivedAttributeOverrideReference_swift(entity, attr, is_subtype_attr, level);
			}
			else {
				explicitAttributeOverrideReference_swift(entity, attr, is_subtype_attr, level);
			}
		}
		else {
			if( VARis_derived(attr) ) {
				derivedAttributeReference_swift(entity, attr, is_subtype_attr, level);
			}
			else if( VARis_inverse(attr) ){
				inverseAttributeReference_swift(entity, attr, is_subtype_attr, level);
			}
			else {	// explicit
				explicitAttributeReference_swift(entity, attr, is_subtype_attr, level);
			}
		}
		raw("\n");
	}LISTod;
}

//MARK: - initializer
static void entityReferenceInitializers_swift( Entity entity, int level ){
	raw("\n");
	indent_swift(level);
	raw("//INITIALIZERS\n");
	
	/*
	 public convenience init?(_ entityRef: SDAI.EntityReference?) {
		 let complex = entityRef?.complexEntity
		 self.init(complex: complex)
	 }
	 */
	indent_swift(level);
	raw( "public convenience init?(_ entityRef: SDAI.EntityReference?) {\n");
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw( "let complex = entityRef?.complexEntity\n");
		indent_swift(level2);
		raw( "self.init(complex: complex)\n");
	}
	indent_swift(level);
	raw( "}\n\n");
	
	/*
	 public required init?(complex complexEntity: SDAI.ComplexEntity?) {
		 guard let partial = complexEntity?.partialEntityInstance(<partialEntity>.self) else { return nil }
		 self.partialEntity = partial
		 self.super_<superType> = (complexEntity?.entityReference(<superType>.self))!
	 	 ...
		 super.init(complex: complexEntity)
	 }	 
	 */
	indent_swift(level);
	raw( "public required init?(complex complexEntity: SDAI.ComplexEntity?) {\n");
	{	int level2=level+nestingIndent_swift;
		char buf[BUFSIZ];
		
		indent_swift(level2);
		raw( "guard let partial = complexEntity?.partialEntityInstance(%s.self) else { return nil }\n", 
				partialEntity_swiftName(entity, buf));
		indent_swift(level2);
		raw( "self.partialEntity = partial\n\n");
		
		LISTdo( ENTITYget_super_entity_list(entity), super, Entity ) {
			indent_swift(level2);
			wrap("self.%s%s = ", 
					 superEntity_swiftPrefix,
					 ENTITY_swiftName(super, NO_QUALIFICATION, buf) );
			wrap("( complexEntity?.entityReference(%s.self) )!\n", 
					 ENTITY_swiftName(super, entity, buf) );
		}LISTod;
		raw("\n");
		
		LISTdo( ENTITYget_sub_entity_list(entity), sub, Entity ) {
			if( sub == entity )continue;
			indent_swift(level2);
			wrap("self.%s%s = ", 
					 subEntity_swiftPrefix,
					 ENTITY_swiftName(sub, NO_QUALIFICATION, buf) );
			wrap("complexEntity?.entityReference(%s.self)\n", 
					 ENTITY_swiftName(sub, entity, buf) );
		}LISTod;
		raw("\n");
		
		indent_swift(level2);
		raw( "super.init(complex: complexEntity)\n");
	}
	indent_swift(level);
	raw( "}\n\n");
	
	/*
	 public required convenience init?<S: SDAIGenericType>(fromGeneric generic: G?) {
		 guard let entityRef = generic?.entityReference else { return nil }
		 self.init(complex: entityRef.complexEntity)
	 }
	 */
	indent_swift(level);
	raw( "public required convenience init?<G: SDAIGenericType>(fromGeneric generic: G?) {\n");
	{	int level2=level+nestingIndent_swift;
		
		indent_swift(level2);
		raw( "guard let entityRef = generic?.entityReference else { return nil }\n" );
		indent_swift(level2);
		raw( "self.init(complex: entityRef.complexEntity)\n");
	}
	indent_swift(level);
	raw( "}\n\n");
	
	/*
	 public convenience init?<S: SDAISelectType>(_ select: S?) { self.init(possiblyFrom: select) }
	 public convenience init?(_ complex: SDAI.ComplexEntity?) { self.init(complex: complex) }
	 */
	indent_swift(level);
	raw( "public convenience init?<S: SDAISelectType>(_ select: S?) { self.init(possiblyFrom: select) }\n");
	indent_swift(level);
	raw( "public convenience init?(_ complex: SDAI.ComplexEntity?) { self.init(complex: complex) }\n\n");
	
	if( LISTget_length(ENTITYget_super_entity_list(entity)) == 1 ) {
	/*
	 // ENTITY REFERENCE constructor from PARTIAL ENTITY for simple entity type
	 public convenience init?(_ partial:_partialentitytype) {
		 let complex = SDAI.ComplexEntity(entities:[partial])
		 self.init(complex: complex)
	 }
	 */
		indent_swift(level);
		raw("//SIMPLE ENTITY REFERENCE\n");
		
		char buf[BUFSIZ];
		indent_swift(level);
		raw("public convenience init?(_ partial:%s) {\n", partialEntity_swiftName(entity, buf));

		{	int level2 = level+nestingIndent_swift;
			indent_swift(level2);
			raw("let complex = SDAI.ComplexEntity(entities:[partial])\n");

			indent_swift(level2);
			raw("self.init(complex: complex)\n");
		}
		
		indent_swift(level);
		raw("}\n\n");	
	}

	
	
//	/*
//	 //EXPRESS IMPLICIT PARTIAL ENTITY CONSTRUCTOR
//	 public convenience init(ATTRi:ATTRTYPE, ...){
//	 	let partial = _partialentitytype(ATTRi:ATTRi, ...)
//	  let complex = SDAI.ComplexEntity(entities:[partial])
//	 	self.init(complex: complex)!
//	 }
//	 */
//	indent_swift(level);
//	raw("//EXPRESS IMPLICIT PARTIAL ENTITY CONSTRUCTOR\n");
//	
//	Linked_List params = ENTITYget_constructor_params(entity);
//	
//	indent_swift(level);
//	raw("public convenience init(");
//	ALGargs_swift(entity, NO_FORCE_OPTIONAL, params, NO_DROP_SINGLE_LABEL, level);
//	raw(") {\n");
//
//	{	int level2 = level+nestingIndent_swift;
//		char buf[BUFSIZ];
//		
//		indent_swift(level2);
//		raw("let partial = %s(", partialEntity_swiftName(entity, buf));
//		char* sep = "";
//		LISTdo(params, attr, Variable) {
//			raw(sep);
//			raw("%s: %s", variable_swiftName(attr, buf), buf);
//			sep = ", ";
//		}LISTod;
//		raw(")\n");
//		
//		indent_swift(level2);
//		raw("let complex = SDAI.ComplexEntity(entities:[partial])\n");
//
//		indent_swift(level2);
//		raw("self.init(complex: complex)!\n");
//	}
//	
//	indent_swift(level);
//	raw("}\n\n");	
}

//MARK: - entity reference defition main entry point
void entityReferenceDefinition_swift( Entity entity, int level ) {
	raw("\n\n");
	raw("//MARK: - Entity Reference\n");
	indent_swift(level);
	{
		char buf[BUFSIZ];
		wrap("public class %s : SDAI.EntityReference {\n", 
				 ENTITY_swiftName(entity, NO_QUALIFICATION, buf));
	}
	
	{	int level2 = level+nestingIndent_swift;
		partialEntityReferenceDefinition_swift(entity, level2);
		supertypeReferenceDefinition_swift(entity, level2);
		entityAttributeReferences_swift(entity, level2);
		entityReferenceInitializers_swift(entity, level2);
	}
	
	indent_swift(level);
	raw("}\n");
}

