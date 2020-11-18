//
//  swift_entity_partial.c
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
#include "swift_algorithm.h"
#include "swift_symbol.h"
#include "swift_schema.h"


static void attributeHead_swift
 (Entity entity, char* access, Variable attr, int level, SwiftOutCommentOption in_comment, char buf[BUFSIZ]) {
	indent_swift(level);
	raw("%s var %s: ",access,partialEntityAttribute_swiftName(attr, buf));
	
	variableType_swift(entity, attr, NO_FORCE_OPTIONAL, in_comment);
}

//MARK: - attribute observers

static void emitObserveRemovedReference(Variable attr, int level) {
	char buf[BUFSIZ];
	LISTdo( VARget_observers(attr), observingAttr, Variable) {
		Entity observingEntity = observingAttr->defined_in;
		
		indent_swift(level);
		raw("//OBSERVING ENTITY: %s\n", ENTITY_swiftName(observingEntity, "", "", NO_QUALIFICATION, buf));
		
		indent_swift(level);
		raw("oldComplex.partialEntityInstance(%s.self)?", partialEntity_swiftName(observingEntity, buf));
		wrap(".%s__observeRemovedReference(in: selfComplex)\n", partialEntityAttribute_swiftName(observingAttr, buf));
	} LISTod
}

static void emitObserveAddedReference(Variable attr, int level) {
	char buf[BUFSIZ];
	LISTdo( VARget_observers(attr), observingAttr, Variable) {
		Entity observingEntity = observingAttr->defined_in;
		
		indent_swift(level);
		raw("//OBSERVING ENTITY: %s\n", ENTITY_swiftName(observingEntity, "", "", NO_QUALIFICATION, buf));
		
		indent_swift(level);
		raw("newComplex.partialEntityInstance(%s.self)?", partialEntity_swiftName(observingEntity, buf));
		wrap(".%s__observeAddedReference(in: selfComplex)\n", partialEntityAttribute_swiftName(observingAttr, buf));
	} LISTod
}

static void simpleAttribureObservers_swift(Entity entity, Variable attr, int level) {
	 /*
		internal static func _<attr>__observer(SELF: SDAI.EntityReference, 
			removing: SDAI.EntityReference?, adding: SDAI.EntityReference?) {
			guard removing != adding else { return }
			let selfComplex = SELF.complexEntity
			if let oldComplex = removing?.complexEntity {
				//OBSERVING ENTITY: <observer>
				oldComplex.partialEntityInstance(_<observer>.self)?
					._<observer attr>__observeRemovedReference(in: selfComplex)
			}
			if let newComplex = adding?.complexEntity {
				//OBSERVING ENTITY: <observer>
				newComplex.partialEntityInstance(_<observer>.self)?
					._<observer attr>__observeAddedReference(in: selfComplex)
			}
		}		
		*/
	 
	char buf[BUFSIZ];

	indent_swift(level);
	wrap("internal static func %s__observer(", partialEntityAttribute_swiftName(attr, buf));
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
						
			if( VARis_observed(attr)){
				emitObserveRemovedReference(attr, level3);
			}

			if( VARis_overriden(attr)) {
				DictionaryEntry de;
				Variable overrider;
				DICTdo_init(attr->overriders, &de);
				while( 0 != ( overrider = DICTdo( &de ) ) ) {
					if( VARis_observed(overrider)){
						emitObserveRemovedReference(overrider, level3);
					}
				}
			}
		}
		indent_swift(level2);
		raw("}\n");

		indent_swift(level2);
		raw("if let newComplex = adding?.complexEntity {\n");
		{	int level3 = level2 + nestingIndent_swift;
			
			if( VARis_observed(attr)){
				emitObserveAddedReference(attr,  level3);
			}

			if( VARis_overriden(attr)) {
				DictionaryEntry de;
				Variable overrider;
				DICTdo_init(attr->overriders, &de);
				while( 0 != ( overrider = DICTdo( &de ) ) ) {
					if( VARis_observed(overrider)){
						emitObserveAddedReference(overrider, level3);
					}
				}
			}			
		}
		indent_swift(level2);
		raw("}\n");
	}
	indent_swift(level);
	raw("}\n");

}

static void aggregateAttributeObservers_swift(Entity entity, Variable attr, int level) {
	 /*
		internal static func _<attr>__aggregateObserver(SELF: SDAI.EntityReference, 
			oldAggregate: <ATTR TYPE>?, newAggregate: <ATTR TYPE>?) -> <ATTR TYPE>? {
			oldAggregate?.teardown()
			var configured = newAggregate
		  configured?.observer = { [unowned SELF] (removing, adding) in _<entity>
				._<attr>__observer(SELF:SELF, removing:removing, adding:adding) }
			return configured
		}		
		*/
	 
	char buf[BUFSIZ];
	
	indent_swift(level);
	wrap("internal static func %s__aggregateObserver(", partialEntityAttribute_swiftName(attr, buf));
	wrap("SELF: SDAI.EntityReference, ");
	aggressively_wrap();
	wrap("oldAggregate: ");
	variableType_swift(entity, attr, NO_FORCE_OPTIONAL, NOT_IN_COMMENT);
	raw(", ");
	aggressively_wrap();
	wrap("newAggregate: ");
	variableType_swift(entity, attr, NO_FORCE_OPTIONAL, NOT_IN_COMMENT);
	raw(") ");
	aggressively_wrap();
	wrap("-> ");
	variableType_swift(entity, attr, NO_FORCE_OPTIONAL, NOT_IN_COMMENT);
	raw(" {\n");

	const char* optional_ref = (TYPEis_optional(attr->type) ? "?" : "");
	
	{	int level2 = level + nestingIndent_swift;
		indent_swift(level2);
		raw("oldAggregate%s.teardown()\n", optional_ref);
		indent_swift(level2);
		raw("var configured = newAggregate\n");
		indent_swift(level2);
		raw("configured%s.observer = { [unowned SELF] (removing, adding) in ", optional_ref );
		wrap(partialEntity_swiftName(entity, buf));
		wrap(".%s__observer(SELF:SELF, removing:removing, adding:adding) }\n", partialEntityAttribute_swiftName(attr, buf));
		indent_swift(level2);
		raw("return configured\n");
	}
	indent_swift(level);
	raw("}\n");
	
	simpleAttribureObservers_swift(entity, attr, level);
}

//MARK: - explicit attribute
static void explicitStaticAttributeDefinition_swift(Entity entity, Variable attr, int level) {
	char buf[BUFSIZ];
	
	attributeHead_swift(entity, "internal", attr, level, NOT_IN_COMMENT, buf);
	if( VARis_observed(attr) ) raw(" //OBSERVED");
	raw("\n");
	
	if( attribute_need_observer(attr) ) {
		if( TYPEis_aggregate(attr->type)) {
			aggregateAttributeObservers_swift(entity, attr, level);
		}
		else {
			simpleAttribureObservers_swift(entity, attr, level);
		}
	}
}

static void explicitAttributeRedefinition_swift(Entity entity, Variable attr, int level) {
	char buf[BUFSIZ];
	
	attributeHead_swift(entity, "//override", attr, level, YES_IN_COMMENT, buf);
	raw("\t//EXPLICIT REDEFINITION(%s)", 
			ENTITY_swiftName(attr->original_attribute->defined_in, "", "", NO_QUALIFICATION, buf));
	if( VARis_observed(attr) ) raw(" //OBSERVED");
	raw("\n");
}

static void explicitDynamicAttributeDefinition_swift(Entity entity, Variable attr, int level) {
	char buf[BUFSIZ];
	
	attributeHead_swift(entity, "internal", attr, level, NOT_IN_COMMENT, buf);
	raw(" //DYNAMIC");
	if( VARis_observed(attr) ) raw(" //OBSERVED");
	raw(" \n");
	
	if( attribute_need_observer(attr) ) {
		if( TYPEis_aggregate(attr->type)) {
			aggregateAttributeObservers_swift(entity, attr, level);
		}
		else {
			simpleAttribureObservers_swift(entity, attr, level);
		}
	}
}

//MARK: - derived attribute
static void derivedAttributeGetterDefinitionHead(Entity entity, Variable attr, const char *attrName, int level) {
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("internal func %s__getter(SELF: %s) -> ", 
			attrName, ENTITY_swiftName(entity, NO_PREFIX, NO_POSTFIX, NO_QUALIFICATION, buf) );
	
	variableType_swift(entity, attr, NO_FORCE_OPTIONAL, NOT_IN_COMMENT);
}

static void derivedRedefinitionGetterDefinitionHead(Scope scope, Entity entity, char* access, Variable attr, const char* attrName, int level) {	 
	indent_swift(level);
	raw("%sfunc %s__getter(complex: SDAI.ComplexEntity) -> ", access, attrName );
	
	variableType_swift(scope, attr, NO_FORCE_OPTIONAL, NOT_IN_COMMENT);
}

void explicitDynamicAttributeProtocolDefinition_swift(Schema schema, Entity entity, Variable attr, int level) {
	char buf[BUFSIZ];
	
	indent_swift(level);
	wrap("internal protocol %s {\n", dynamicAttribute_swiftProtocolName(attr, buf));
	
	{	int level2 = level + nestingIndent_swift;

		const char* attrName = partialEntityAttribute_swiftName(attr, buf);
		derivedRedefinitionGetterDefinitionHead(schema->superscope, entity, "", attr, attrName, level2);
		raw("\n");
	}
	
	indent_swift(level);
	wrap("}\n");
}

static void recoverSELFfromComplex_swift(Entity entity, int level) {
	char buf[BUFSIZ];
	
	indent_swift(level);
	wrap("let SELF = complex.entityReference( %s.self )!\n", 
			 ENTITY_swiftName(entity, NO_PREFIX, NO_POSTFIX, NO_QUALIFICATION, buf));
}

static void derivedAttributeDefinition_swift(Entity entity, Variable attr, int level) {
	indent_swift(level);
	raw("//\tDERIVE\n");

	char attrName[BUFSIZ];
	partialEntityAttribute_swiftName(attr, attrName);
	
	derivedAttributeGetterDefinitionHead(entity, attr, attrName, level);
	raw(" {\n");
	{	int level2 = level+nestingIndent_swift;
				
		indent_swift(level2);
		raw("let value = ");
		EXPR_swift(entity, VARget_initializer(attr), NO_PAREN );
		raw("\n");
		
		indent_swift(level2);
		raw("return ");
		variableType_swift(entity, attr, NO_FORCE_OPTIONAL, NOT_IN_COMMENT);
		if( VARis_optional(attr) ){
			raw("(value)\n", attrName);
		}
		else {
			raw("(SDAI.UNWRAP(value))\n", attrName);
		}
	}
	indent_swift(level);
	raw("}\n");
}


static void derivedAttributeRedefinition_swift(Entity entity, Variable attr, int level) {
	Variable original_attr = VARget_redeclaring_attr(attr);
	
	char buf[BUFSIZ];
	const char* entityName = ENTITY_swiftName(original_attr->defined_in, NO_PREFIX, NO_POSTFIX, NO_QUALIFICATION, buf);
	
	char buf2[BUFSIZ];
	const char* attrName = partialEntityAttribute_swiftName(attr, buf2);
	
	indent_swift(level);
	raw("//\tDERIVE REDEFINITION(%s)", entityName);
	if( attribute_need_observer(original_attr) ) raw(" //OBSERVED ORIGINAL");
	raw("\n");
	
	derivedRedefinitionGetterDefinitionHead(entity, entity, "internal ", original_attr, attrName, level);
	raw(" {\n");
	{	int level2 = level+nestingIndent_swift;
		
		recoverSELFfromComplex_swift(entity, level2);

		indent_swift(level2);
		raw("return ");
		variableType_swift(entity, original_attr, NO_FORCE_OPTIONAL, NOT_IN_COMMENT);
		raw("(%s__getter(SELF: SELF))\n", attrName);
	}
	indent_swift(level);
	raw("}\n");
	
	derivedAttributeGetterDefinitionHead(entity, attr, attrName, level);
	raw(" {\n");
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw("let value = ");
		EXPR_swift(entity, VARget_initializer(attr), NO_PAREN );
		raw("\n");
		
		if( attribute_need_observer(original_attr) ) {
			indent_swift(level2);
			raw("SELF.%s%s.%s = ", superEntity_swiftPrefix, entityName, attrName );
			variableType_swift(entity, original_attr, NO_FORCE_OPTIONAL, NOT_IN_COMMENT);
			raw("(value)\n");
		}
		
		indent_swift(level2);
		raw("return ");
		variableType_swift(entity, attr, NO_FORCE_OPTIONAL, NOT_IN_COMMENT);
		raw("(value)\n");
	}
	indent_swift(level);
	raw("}\n");
}

//MARK: - inverse attribute
static void inverseAttributeDefinition_swift(Entity entity, Variable attr, int level) {
	/*
	 internal private(set) var _<attr>: <ATTR TYPE> 
	 internal func _<attr>__observeAddedReference(in complex: SDAI.ComplexEntity) {
		 self._<attr>.add(member: complex.entityReference(<ATTR BASETYPE>.self) )
	 //or//
	   self._<attr> = complex.entityReference(<ATTR BASETYPE>.self)
	 } // INVERSE ATTR SUPPORT(ADDER)
	 internal func _<attr>__observeRemovedReference(in complex: SDAI.ComplexEntity) {
		 self._<attr>.remove(member: complex.entityReference(<ATTR BASETYPE>.self) )
	 //or//
		 self._<attr> = nil
	 } // INVERSE ATTR SUPPORT(REMOVER)
	 
	 */
	
	indent_swift(level);
	raw("//\tINVERSE\n");
	
	char attrName[BUFSIZ];
	attributeHead_swift(entity, "internal private(set)", attr, level, NOT_IN_COMMENT, attrName);
	raw("\n");
	
	char attrBaseType[BUFSIZ];
	TYPEhead_string_swift(entity, ATTRget_base_type(attr), NOT_IN_COMMENT, attrBaseType);
	
	// addedReference observer func
	indent_swift(level);
	raw("internal func %s__observeAddedReference(in complex: SDAI.ComplexEntity) {\n", attrName);
	
	{	int level2 = level+nestingIndent_swift;

//		indent_swift(level2);
//		wrap("let entityRef: %s? = complex\n", attrBaseType);
			
		indent_swift(level2);
		if( TYPEis_aggregate(attr->type)) {
			wrap("self.%s.add(member: complex.entityReference(%s.self) )\n", attrName, attrBaseType );
		}
		else {
			wrap("self.%s = complex.entityReference(%s.self)", attrName, attrBaseType );
			if( !VARis_optional(attr) ) {
				raw("!");
			}
			raw("\n");
		}
	}
	indent_swift(level);
	raw("} // INVERSE ATTR SUPPORT(ADDER)\n");

	// removedReference observer func
	indent_swift(level);
	raw("internal func %s__observeRemovedReference(in complex: SDAI.ComplexEntity) {\n", attrName);
	
	{	int level2 = level+nestingIndent_swift;
		
//		indent_swift(level2);
//		wrap("let entityRef: %s? = complex\n", attrBaseType);

		indent_swift(level2);
		if( TYPEis_aggregate(attr->type)) {
			wrap("self.%s.remove(member: complex.entityReference(%s.self) )\n", attrName, attrBaseType );
		}
		else {
			if( VARis_optional(attr) ) {
				wrap("self.%s = nil\n", attrName );
			}
		}
	}
	
	indent_swift(level);
	raw("} // INVERSE ATTR SUPPORT(REMOVER)\n");

}

//MARK: - local attributes list
static void localAttributeDefinitions_swift( Entity entity, int level, Linked_List attr_overrides, Linked_List dynamic_attrs ) {
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
				inverseAttributeDefinition_swift(entity, attr, level);
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
	 if( LISTis_empty(local_attributes) ) {
		 indent_swift(level);
		 raw("// (no local attributes)\n\n");
	 }
}

//MARK: - where rule
static void whereDefinitions_swift( Entity entity, int level ) {
	Linked_List where_rules = TYPEget_where(entity);
	if( LISTis_empty(where_rules) ) return;
	
	indent_swift(level);
	raw("//WHERE RULES\n");

	char whereLabel[BUFSIZ];
	char buf[BUFSIZ];
	LISTdo( where_rules, where, Where ){
		indent_swift(level);
		wrap("public func %s(SELF: %s) -> SDAI.LOGICAL {\n", 
				 whereRuleLabel_swiftName(where, whereLabel), 
				 ENTITY_swiftName(entity, NO_PREFIX, NO_POSTFIX, NO_QUALIFICATION, buf) );
		
		{	int level2 = level+nestingIndent_swift;
			indent_swift(level2);
			raw("SDAI.LOGICAL( ");
			EXPR_swift(entity, where->expr, NO_PAREN);			
			raw(" )\n");
		}
		
		indent_swift(level);
		raw("}\n");
	}LISTod;
	raw("\n");
}

//MARK: - unique rule
static Variable unique_attribute( Expression expr ) {
	assert(expr->u_tag == expr_is_variable);
	return expr->u.variable;
}

static void simpleUniquenessRule_swift( Entity entity, Linked_List unique, int level) {
	Expression attr_expr = LISTget_second(unique);
	
	indent_swift(level);
	raw("return AnyHashable( ");
	EXPR_swift(entity, attr_expr, NO_PAREN);
	raw(" )\n");
}

static void jointUniquenessRule_swift( Entity entity, Linked_List unique, int joint_count, int level) {
	indent_swift(level);
	wrap("var attributes: Array<AnyHashable> = []\n\n");
	
	int attr_no = -1;
	LISTdo(unique, attr_expr, Expression) {
		if( ++attr_no == 0 ) continue;	// skip label
		
		Variable attr = unique_attribute(attr_expr);
		if( VARis_optional(attr) ) {
			indent_swift(level);
			wrap("guard let attr%d = ", attr_no);
			EXPR_swift(entity, attr_expr, NO_PAREN);
			wrap(" else { return nil }\n");
		}
		else {
			indent_swift(level);
			wrap("let attr%d = ", attr_no);
			EXPR_swift(entity, attr_expr, NO_PAREN);
			raw("\n");
		}
		
		indent_swift(level);
		wrap("attributes.append( AnyHashable(attr%d) )\n", attr_no);		
	}LISTod;
	
	raw("\n");
	indent_swift(level);
	wrap("return AnyHashable( attributes )\n");
}

static void uniqueRule_swift( Entity entity, int serial, Linked_List unique, int level ) {
	char buf[BUFSIZ];
	int joint_count = LISTget_length(unique) - 1;
	
	indent_swift(level);
	wrap("public func %s(complex: SDAI.ComplexEntity) -> AnyHashable? {\n", 
			 uniqueRuleLabel_swiftName(serial, unique, buf) );

	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		wrap("guard let SELF = complex.entityReference( %s.self ) ", 
				 ENTITY_swiftName(entity, NO_PREFIX, NO_POSTFIX, NO_QUALIFICATION, buf));
		wrap("else { return nil }\n\n");

		if( joint_count == 1 ) {
			simpleUniquenessRule_swift(entity, unique, level2);
		}
		else {
			jointUniquenessRule_swift(entity, unique, joint_count, level2);
		}
	}

	raw("\n");
	indent_swift(level);
	raw("}\n");	
	
}

static void uniqueDefinitions_swift( Entity entity, int level ) {
	Linked_List unique_rules = ENTITYget_uniqueness_list(entity);
	if( LISTis_empty(unique_rules) ) return;
	
	indent_swift(level);
	raw("//UNIQUE RULES\n");
	
	int serial = 0;
	LISTdo(unique_rules, unique, Linked_List) {
		uniqueRule_swift(entity, ++serial, unique, level);
	}LISTod;
	
	raw("\n");
}

//MARK: - constructor
static void expressConstructor( Entity entity, int level ) {
	
	indent_swift(level);
	raw("//EXPRESS IMPLICIT PARTIAL ENTITY CONSTRUCTOR\n");
	
	Linked_List params = ENTITYget_constructor_params(entity);
	
	indent_swift(level);
	raw("public%s init(", LISTis_empty(params) ? " override" : "");
	ALGargs_swift(entity, NO_FORCE_OPTIONAL, params, NO_DROP_SINGLE_LABEL, level);
	raw(") {\n");

	{	int level2 = level+nestingIndent_swift;
		char buf[BUFSIZ];
		LISTdo(params, attr, Variable) {
			indent_swift(level2);
			raw("self.%s = ", partialEntityAttribute_swiftName(attr, buf));
			raw("%s\n", variable_swiftName(attr,buf));
		}LISTod;
		indent_swift(level2);
		raw("super.init()\n");
	}
	
	indent_swift(level);
	raw("}\n\n");
}

//MARK: - partial entity swift definition

void partialEntityDefinition_swift
 ( Entity entity, int level, Linked_List attr_overrides, Linked_List dynamic_attrs ) {
	 raw("\n\n");
	 raw("//MARK: - Partial Entity\n");
	 
	 {	char buf[BUFSIZ];
		 
		 indent_swift(level);
		 wrap("public class %s : SDAI.PartialEntity {\n", partialEntity_swiftName(entity, buf));
	 }
	 
	 {	int level2 = level+nestingIndent_swift;
		 
		 localAttributeDefinitions_swift(entity, level2, attr_overrides, dynamic_attrs);
		 whereDefinitions_swift(entity, level2);
		 uniqueDefinitions_swift(entity, level2);
		 expressConstructor(entity, level2);
	 }
	 
	 indent_swift(level);
	 raw("}\n");
 }

void partialEntityAttrOverrideProtocolConformance_swift
 ( Schema schema, Entity entity, int level, Linked_List attr_overrides ) {
	 if( LISTis_empty(attr_overrides) ) return;
	 
	 raw("\n");
	 raw("//MARK: - partial Entity Dynamic Attribute Protocol Conformances\n");

	 char buf[BUFSIZ];

	 indent_swift(level);
	 raw("extension %s", SCHEMA_swiftName(schema, buf));
	 wrap(".%s :", partialEntity_swiftName(entity, buf));
	 char* sep = "";
	 LISTdo( attr_overrides, attr, Variable ) {
		 assert(attr->original_attribute);
		 raw(sep);
		 wrap(dynamicAttribute_swiftProtocolName(attr->original_attribute, buf));
		 sep = ", ";
	 }LISTod;
	 wrap(" {}\n");
}
