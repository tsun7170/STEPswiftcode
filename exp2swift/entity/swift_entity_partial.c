//
//  swift_entity_partial.c
//  exp2swift
//
//  Created by Yoshida on 2020/07/26.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
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
#include "decompose_expression.h"

#define YES_DYNAMIC	true
#define NO_DYNAMIC	false

#define NO_PREFIX	""
#define BACKING_STORAGE_PREFIX "_"

static bool attributeHead_swift
 (Entity entity,
	char* access,
	char* prefix,
	Variable attr,
	bool isDynamic,
	int level,
	SwiftOutCommentOption in_comment,
	char buf[BUFSIZ])
{
	 if(isDynamic) {
		 indent_swift(level);
		 raw("/// - see the corresponding attribute accessor in the entity reference definition for the attribute value dynamic lookup logic\n");
	 }

	indent_swift(level);
	raw("%s var %s%s: ",
			access,
			prefix,
			partialEntityAttribute_swiftName(attr, buf)
			);

	bool optional = variableType_swift(entity, attr, (isDynamic ? YES_FORCE_OPTIONAL : NO_FORCE_OPTIONAL), in_comment); // to handle p21 asterisk encoded attribute value for the explicit attributes redeclared as derived.
	 return optional;
}



//MARK: - explicit attribute
static void explicitStaticAttributeDefinition_swift(Entity entity, Variable attr, int level) {
	/*
	 public internal(set) var <attr>: <attr_type>
	 
	 // or //
	 public internal(set) var <attr>: <attr_type>
	 {
		get {
			return _<attr>?.copy()
		}
	 set {
	   guard newValue != _<attr> else { return }
		 var newValue = newValue
		 let observer = SDAI.EntityReferenceObserver(referencer: self, observerCode: <attr>__observer.self)
		 newValue?.configure(with: observer)
		 observer.observe(removing: SDAI.UNWRAP(seq:_<attr>?.entityReferences),
			 adding: SDAI.UNWRAP(seq: newValue?.entityReferences) )
		 _<attr> = newValue
		}
	 }
	 private var _<attr>: <attr_type>

	 */
	

	char buf[BUFSIZ];
	char partialAttrName[BUFSIZ];
	partialEntityAttribute_swiftName(attr,partialAttrName);

	indent_swift(level);
	raw("/// EXPLICIT ATTRIBUTE %s\n", 
			VARis_observed(attr) ? "(OBSERVED)" : (attribute_need_observer(attr) ? "(SUBTYPE ATTR OBSERVED)" : "")
			);
	indent_swift(level);
	raw("@_documentation(visibility:public)\n");
//	bool optional =
	attributeHead_swift(entity, "public internal(set)",
											NO_PREFIX, attr, VARis_dynamic(attr),
											level, NOT_IN_COMMENT, buf);

	if( attribute_need_observer(attr) ) {
		raw(" // OBSERVED EXPLICIT ATTRIBUTE\n");
	}
	else {
		raw(" // PLAIN EXPLICIT ATTRIBUTE\n");
	}
}

static void explicitAttributeRedefinition_swift(Entity entity, Variable attr, int level) {
	char buf[BUFSIZ];
	
	attributeHead_swift(entity, "/* override", NO_PREFIX, attr, NO_DYNAMIC, level, YES_IN_COMMENT, buf);
	raw("\t//EXPLICIT REDEFINITION(%s)", 
			ENTITY_swiftName(attr->original_attribute->defined_in, NO_QUALIFICATION, SWIFT_QUALIFIER, buf)
			);
	if( VARis_observed(attr) ) raw(" //OBSERVED");
	raw(" */\n");
}

//MARK: - explicit dynamic attribute
static void dynamicAttributeValueProvider_swift(Entity entity, Variable attr, int level) {
	/* 
	 internal static func <attr>__provider(complex: SDAI.ComplexEntity) -> <ENTITY>__<ATTR>__provider? {
		 let resolved = complex.resolvePartialEntityInstance(from:[<entity>.typeIdentity, ...])
		 return resolved as? <ENTITY>__<ATTR>__provider
	 }
	 */
	
	int level2 = level + nestingIndent_swift;
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("internal static func %s__provider(complex: SDAI.ComplexEntity) -> ",
			partialEntityAttribute_swiftName(attr, buf)
			);
	raw("%s? {\n",
			dynamicAttribute_swiftProtocolName(attr, buf)
			);
	{
		indent_swift(level2);
		wrap("let resolved = complex.resolvePartialEntityInstance(from: [");
		DictionaryEntry de;
		Variable overrider;
		char* sep = "";
		assert(attr->overriders != NULL);
		DICTdo_init(attr->overriders, &de);
		while( 0 != (overrider = DICTdo(&de)) ) {
			if( !VARis_derived(overrider) ) continue;
			raw("%s",
					sep
					);
			wrap("%s.typeIdentity",
					 partialEntity_swiftName(overrider->defined_in, buf)
					 );
			sep = ", ";
		}
		wrap("])\n");
		
		indent_swift(level2);
		raw("return resolved as? %s\n",
				dynamicAttribute_swiftProtocolName(attr, buf)
				);
	}
	indent_swift(level);
	raw("}\n");
}

static void explicitDynamicAttributeDefinition_swift(Entity entity, Variable attr, int level) {
	// same as static
	explicitStaticAttributeDefinition_swift(entity, attr, level);
	raw("\n");
	dynamicAttributeValueProvider_swift(entity, attr, level);
}

static void fixupPartialComplexEntityAttributes_swift(Entity entity, Linked_List dynamic_attrs, int level) {
/*
	final public override class func fixupPartialComplexEntityAttributes(partialComplex: SDAI.ComplexEntity, baseComplex: SDAI.ComplexEntity) {
 guard let pe = partialComplex.partialEntityInstance(<entity>.self) else { return }
 
	 if pe.<attr> == nil, self.<attr>__provider(complex: partialComplex) == nil, let base_<attr>__provider = self.<attr>__provider(complex: baseComplex) {
		 pe.<attr> = base_<attr>__provider.<attr>__getter(complex: baseComplex)
	 }
 
	 ...
	 ...
 }
 */

	int level2 = level + nestingIndent_swift;
	int level3 = level2 + nestingIndent_swift;
	char buf[BUFSIZ];

	if( LISTis_empty(dynamic_attrs) )return;
	
	indent_swift(level);
	raw("//PARTIAL COMPLEX ENTITY SUPPORT\n");	
	indent_swift(level);
	raw("final public override class func fixupPartialComplexEntityAttributes(partialComplex: SDAI.ComplexEntity, baseComplex: SDAI.ComplexEntity) {\n");
	{
		indent_swift(level2);
		raw("guard let pe = partialComplex.partialEntityInstance(%s.self) else { return }\n",
				partialEntity_swiftName(entity, buf)
				);

		LISTdo(dynamic_attrs, attr, Variable){
			if( VARis_optional(attr) )continue;
			
			raw("\n");
			indent_swift(level2);
			raw("if pe.%s == nil, ",
					partialEntityAttribute_swiftName(attr, buf)
					);
			wrap("self.%s__provider(complex: partialComplex) == nil, ",
					 buf
					 );
			positively_wrap();
			wrap("let base_%s__provider = self.%s__provider(complex: baseComplex) {\n", 
					 buf,
					 buf
					 );

			indent_swift(level3);
			raw("pe.%s = base_%s__provider.%s__getter(complex: baseComplex)\n",
					buf,
					buf,
					buf
					);

			indent_swift(level2);
			raw("}\n");
		}LISTod;
	}
	indent_swift(level);
	raw("}\n");
}


//MARK: - derived attribute
static void derivedAttributeGetterDefinitionHead(Entity entity, Variable attr, const char *attrName, int level) {
	char buf[BUFSIZ];
	
	indent_swift(level);
	raw("@_documentation(visibility:public)\n");
	indent_swift(level);
	raw("internal func %s__getter(SELF: %s) -> ",
			attrName,
			ENTITY_REFTYPE_swiftName(entity, NO_QUALIFICATION, SWIFT_QUALIFIER, buf)
			);

	variableType_swift(entity, attr, NO_FORCE_OPTIONAL, NOT_IN_COMMENT);
}

static void derivedRedefinitionGetterDefinitionHead(Scope scope, Entity entity, char* access, Variable attr, const char* attrName, int level) {

	indent_swift(level);
	raw("@_documentation(visibility:public)\n");
	indent_swift(level);
	raw("%sfunc %s__getter(complex: SDAI.ComplexEntity) -> ",
			access,
			attrName
			);

	variableType_swift(scope, attr, NO_FORCE_OPTIONAL, NOT_IN_COMMENT);
}

void explicitDynamicAttributeProtocolDefinition_swift(Schema schema, Entity entity, Variable attr, int level) {
	char buf[BUFSIZ];
	
	indent_swift(level);
	wrap("internal protocol %s {\n",
			 dynamicAttribute_swiftProtocolName(attr, buf)
			 );

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
			 ENTITY_swiftName(entity, NO_QUALIFICATION, SWIFT_QUALIFIER, buf)
			 );
}

static void derivedAttributeDefinition_swift(Entity entity, Variable attr, int level) {
	indent_swift(level);
	raw("/// DERIVE ATTRIBUTE\n");

	char attrName[BUFSIZ];
	partialEntityAttribute_swiftName(attr, attrName);
	
	derivedAttributeGetterDefinitionHead(entity, attr, attrName, level);
	raw(" {\n");
	{	int level2 = level+nestingIndent_swift;
		int tempvar_id = 1;
		Linked_List tempvars;
		Expression simplified = EXPR_decompose(entity, VARget_initializer(attr), VARget_type(attr), &tempvar_id, &tempvars);
		EXPR_tempvars_swift(entity, tempvars, level2);
				
		indent_swift(level2);
		raw("return ");
		if( VARis_optional_by_large(attr) ){
			EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, entity, simplified, VARget_type(attr), NO_PAREN, OP_UNKNOWN, YES_WRAP);
		}
		else {
			if( TYPEis_logical(VARget_type(attr)) ){
				raw("SDAI.LOGICAL(");
			}
			else {
				raw("SDAI.UNWRAP(");
			}
			EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, entity, simplified, VARget_type(attr), NO_PAREN, OP_UNKNOWN, YES_WRAP);
			raw(")");
		}
		
		raw("\n");
		EXPR_delete_tempvar_definitions(tempvars);
	}
	indent_swift(level);
	raw("}\n");
}


static void derivedAttributeRedefinition_swift(Entity entity, Variable attr, int level) {
	Variable original_attr = VARget_redeclaring_attr(attr);
	
	char originEntityName[BUFSIZ];
	ENTITY_swiftName(original_attr->defined_in, NO_QUALIFICATION, SWIFT_QUALIFIER, originEntityName);

	char partialAttrName[BUFSIZ];
	partialEntityAttribute_swiftName(attr, partialAttrName);
	
	// provider protocol conformance
	indent_swift(level);
	raw("/// DERIVE ATTRIBUTE REDEFINITION(origin: %s)\n",
			originEntityName
			);
	indent_swift(level);
	raw("/// - attribute value provider protocol conformance wrapper\n");
	derivedRedefinitionGetterDefinitionHead(entity, entity, "internal ", original_attr, partialAttrName, level);
	raw(" {\n");
	{	int level2 = level+nestingIndent_swift;
		
		recoverSELFfromComplex_swift(entity, level2);

		indent_swift(level2);
		raw("return ");
		if( !VARis_optional_by_large(attr) ){
			if( TYPEis_logical(VARget_type(attr)) ){
				raw("SDAI.LOGICAL( ");
			}
			else{
				raw("SDAI.UNWRAP( ");
			}
		}
		TYPE_head_swift(entity, original_attr->type, WO_COMMENT, LEAF_OWNED);
		char buf[BUFSIZ];
		raw("(SELF.%s)", attribute_swiftName(attr, buf));
		if( !VARis_optional_by_large(attr) ){
			raw(" )");
		}
		raw("\n");
	}
	indent_swift(level);
	raw("}\n");
	
	// gut
	indent_swift(level);
	raw("/// DERIVE ATTRIBUTE REDEFINITION(origin: %s)\n", originEntityName);
	indent_swift(level);
	raw("/// - gut of derived attribute logic\n");
	derivedAttributeGetterDefinitionHead(entity, attr, partialAttrName, level);
	raw(" {\n");
	{	int level2 = level+nestingIndent_swift;
		int tempvar_id = 1;
		Linked_List tempvars;
		Expression simplified = EXPR_decompose(entity, VARget_initializer(attr), VARget_type(attr), &tempvar_id, &tempvars);
		EXPR_tempvars_swift(entity, tempvars, level2);
		
		indent_swift(level2);
		raw("let value = ");
		EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, entity, simplified, VARget_type(attr), NO_PAREN, OP_UNKNOWN, YES_WRAP);
		raw("\n");

		indent_swift(level2);
		if( VARis_optional_by_large(attr) ){
			raw("return value\n");
		}
		else {
			if( TYPEis_logical(VARget_type(attr)) ){
				raw("return SDAI.LOGICAL( value )\n");
			}
			else{
				raw("return SDAI.UNWRAP( value )\n");
			}
		}
		EXPR_delete_tempvar_definitions(tempvars);
	}
	indent_swift(level);
	raw("}\n");
}

//MARK: - inverse attribute
static void inverseSimpleAttributeDefinition_swift(Entity entity, Variable attr, int level) {
	/*
	 internal private(set) weak var <attr>: <ATTR TYPE> 

	 internal func <attr>__observeAddedReference(in complex: SDAI.ComplexEntity) {
	 guard let newSource = complex.entityReference(<ATTR TYPE>.self) else { return }
	 self.<attr> = newSource
	 }

	 internal func <attr>__observeRemovedReference(in complex: SDAI.ComplexEntity) {
	 guard let _ = complex.entityReference(<ATTR TYPE>.self) else { return }
	 self.<attr> = nil
	 }

	 internal func <attr>__observe(leavingReferencerOwner complex: SDAI.ComplexEntity) {
	 guard let _ = complex.entityReference(<ATTR TYPE>.self) else { return }
	 self.<attr> = nil
	 }
	 */
	
	char buf[BUFSIZ];
	
	assert(VARis_optional_by_large(attr));

	char partialAttrName[BUFSIZ];
	partialEntityAttribute_swiftName(attr, partialAttrName);
	
	char attrTypeName[BUFSIZ];
	TYPEhead_string_swift(entity, attr->type, NOT_IN_COMMENT, LEAF_OWNED, attrTypeName);
	
	indent_swift(level);
	raw("// INVERSE SIMPLE ATTRIBUTE\n");
	Variable observing_attr = VARget_inverse(attr);
	indent_swift(level);
	raw("// observing %s",
			partialEntity_swiftName(observing_attr->defined_in, buf)
			);
	raw(" .%s\n",
			partialEntityAttribute_swiftName(observing_attr, buf)
			);

	indent_swift(level);
	raw("//internal private(set) weak var %s: %s? \t(SEE CORRESPONDING ENTITY REFERENCE DEFINITION)\n",
			partialAttrName,
			attrTypeName
			);

}

static void inverseAggregateAttributeDefinition_swift(Entity entity, Variable attr, int level) {
	/*
	 internal var <attr>: <AGGR<<ATTR BASETYPE>> {
	  return _<attr>.map{ %0.reference }
	 }
	 private var _<attr>: <AGGR<SDAI.UnownedWrap<<ATTR BASETYPE>>> 
	 
	 internal func <attr>__observeAddedReference(in complex: SDAI.ComplexEntity) {
	 guard let newSource = complex.entityReference(<ATTR BASETYPE>.self) else { return }
		 self._<attr>.add(member: SDAI.UnownedWrap(newSource) )
	 }
	 
	 internal func <attr>__observeRemovedReference(in complex: SDAI.ComplexEntity) {
	 guard let oldSource = complex.entityReference(<ATTR BASETYPE>.self) else { return }
	 let success = self._<attr>.remove(member: SDAI.UnownedWrap(oldSource)) )
	 assert(success, "failed to remove inverse attribute element")
	 }
	 
	 internal func <attr>__observe(leavingReferencerOwner complex: SDAI.ComplexEntity) {
	 guard let oldSource = complex.entityReference(<ATTR BASETYPE>.self) else { return }
	 let success = self._<attr>.removeAll(member: SDAI.UnownedWrap(oldSource)) )
	 assert(success, "failed to remove inverse attribute elements")
	 }
	 */

	char buf[BUFSIZ];
	
	assert(!VARis_optional_by_large(attr));

	char partialAttrName[BUFSIZ];
	partialEntityAttribute_swiftName(attr, partialAttrName);
	
	char attrBaseTypeName[BUFSIZ];
	TYPEhead_string_swift(entity, ATTRget_base_type(attr), NOT_IN_COMMENT, LEAF_OWNED, attrBaseTypeName);

	indent_swift(level);
	raw("// INVERSE AGGREGATE ATTRIBUTE\n");
	Variable observing_attr = VARget_inverse(attr);
	indent_swift(level);
	raw("// observing %s",
			partialEntity_swiftName(observing_attr->defined_in, buf)
			);
	raw(" .%s\n",
			partialEntityAttribute_swiftName(observing_attr, buf)
			);

	indent_swift(level);
	raw("//internal var %s: ",
			partialAttrName
			);
	{
		int saved = never_wrap();
		TYPE_head_swift(entity, attr->type, WO_COMMENT, LEAF_OWNED);
		restore_wrap(saved);
	}
	raw(" \t(SEE CORRESPONDING ENTITY REFERENCE DEFINITION)\n");
}

//MARK: - local attributes list
static void localAttributeDefinitions_swift( Entity entity, int level, /*out*/Linked_List attr_overrides, /*out*/Linked_List dynamic_attrs ) {
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
				if( TYPEis_aggregation_data_type(attr->type) ){
					inverseAggregateAttributeDefinition_swift(entity, attr, level);
				}
				else{
					inverseSimpleAttributeDefinition_swift(entity, attr, level);
				}
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

//MARK: - value comparison supports
static void hashAsValue_swift( Entity entity, int level ) {
	/*
	 public override func hashAsValue(into hasher: inout Hasher, visited complexEntities: inout Set<ComplexEntity>) {
	 		super.hashAsValue(into: &hasher, visited: &complexEntities)
	 
	 		self.attr?.value.hashAsValue(into: &hasher, visited: &complexEntities)
			...	 
	 }
	 */
	
	indent_swift(level);
	raw("public override func hashAsValue(into hasher: inout Hasher, visited complexEntities: inout Set<SDAI.ComplexEntity>) {\n");
	{	int level2 = level+nestingIndent_swift;
		char buf[BUFSIZ];
		
		indent_swift(level2);
		raw("super.hashAsValue(into: &hasher, visited: &complexEntities)\n");
		
		Linked_List local_attributes = entity->u.entity->attributes;
		LISTdo( local_attributes, attr, Variable ){
			if( VARis_redeclaring(attr) ) continue;
			if( VARis_derived(attr) ) continue;
			if( VARis_inverse(attr) ) continue;
			
			indent_swift(level2);
			raw("self.%s%s.value.hashAsValue(into: &hasher, visited: &complexEntities)\n",
					partialEntityAttribute_swiftName(attr, buf),
					(VARis_optional_by_large(attr)||(VARis_dynamic(attr) && !TYPEis_logical(attr->type))) ? "?" : ""
					);
		}LISTod;
	}
	indent_swift(level);
	raw("}\n\n");
}

static void isValueEqual_swift( Entity entity, int level ) {
	/*
	 public override func isValueEqual(to rhs: PartialEntity, visited comppairs: inout Set<ComplexPair>) -> Bool {
	 		guard let rhs = rhs as? Self else { return false }
	 		if !super.isValueEqual(to: rhs, visited: &comppairs) { return false }
	 
	 		if let comp = self.attr?.value.isValueEqualOptionally(to: rhs.attr?.value, visited: &comppairs)	{
	 			if !comp { return false }
	 		}
	 		else { return false }
	 		...
	 
	 		return true
	 }
	 */
	
	indent_swift(level);
	raw("public override func isValueEqual(to rhs: SDAI.PartialEntity, visited comppairs: inout Set<SDAI.ComplexPair>) -> Bool {\n");
	{	int level2 = level+nestingIndent_swift;
		
		indent_swift(level2);
		raw("guard let rhs = rhs as? Self else { return false }\n");
		indent_swift(level2);
		raw("if !super.isValueEqual(to: rhs, visited: &comppairs) { return false }\n");
		
		Linked_List local_attributes = entity->u.entity->attributes;
		LISTdo( local_attributes, attr, Variable ){
			if( VARis_redeclaring(attr) ) continue;
			if( VARis_derived(attr) ) continue;
			if( VARis_inverse(attr) ) continue;
			
			char buf[BUFSIZ];partialEntityAttribute_swiftName(attr, buf);
			char* optional = (VARis_optional_by_large(attr)||(VARis_dynamic(attr) && !TYPEis_logical(attr->type)))? "?" : "";
			
			indent_swift(level2);
			raw("if let comp = self.%s%s.value.isValueEqualOptionally(to: rhs.%s%s.value, visited: &comppairs)	{\n",
					buf,
					optional,
					buf,
					optional
					);
			indent_swift(level2+nestingIndent_swift);
			raw("if !comp { return false }\n");
			indent_swift(level2);
			raw("}\n");
			indent_swift(level2);
			raw("else { return false }\n");
		}LISTod;
		indent_swift(level2);
		raw("return true\n");
	}
	indent_swift(level);
	raw("}\n\n");
}

static void isValueEqualOptionally_swift( Entity entity, int level ) {
	/*
	 public override func isValueEqualOptionally(to rhs: PartialEntity, visited comppairs: inout Set<ComplexPair>) -> Bool? {
	 		guard let rhs = rhs as? Self else { return false }
	    var result: Bool? = true
			if let comp = super.isValueEqualOptionally(to: rhs, visited: &comppairs) { 
	 			if !comp { return false }
	 		}
	 		else { result = nil }
	 
	 		if let comp = self.attr?.value.isValueEqualOptionally(to: rhs.attr?.value, visited: &comppairs) {
			 	if !comp { return false }
	 		}
	 		else { result = nil }
	 		...
	 
	 		return result
	 }
	 */
	
	indent_swift(level);
	raw("public override func isValueEqualOptionally(to rhs: SDAI.PartialEntity, visited comppairs: inout Set<SDAI.ComplexPair>) -> Bool? {\n");
	{	int level2 = level+nestingIndent_swift;

		indent_swift(level2);
		raw("guard let rhs = rhs as? Self else { return false }\n");
		indent_swift(level2);
		raw("var result: Bool? = true\n");
		indent_swift(level2);
		raw("if let comp = super.isValueEqualOptionally(to: rhs, visited: &comppairs) {\n");
		indent_swift(level2+nestingIndent_swift);
		raw("if !comp { return false }\n");
		indent_swift(level2);
		raw("}\n");
		indent_swift(level2);
		raw("else { result = nil }\n");
		
		Linked_List local_attributes = entity->u.entity->attributes;
		LISTdo( local_attributes, attr, Variable ){
			if( VARis_redeclaring(attr) ) continue;
			if( VARis_derived(attr) ) continue;
			if( VARis_inverse(attr) ) continue;
			
			char buf[BUFSIZ];partialEntityAttribute_swiftName(attr, buf);
			char* optional = (VARis_optional_by_large(attr)||(VARis_dynamic(attr) && !TYPEis_logical(attr->type)))? "?" : "";

			indent_swift(level2);
			raw("if let comp = self.%s%s.value.isValueEqualOptionally(to: rhs.%s%s.value, visited: &comppairs) {\n",
					buf,optional, buf,optional );
			indent_swift(level2+nestingIndent_swift);
			raw("if !comp { return false }\n");
			indent_swift(level2);
			raw("}\n");
			indent_swift(level2);
			raw("else { result = nil }\n");
		}LISTod;
		indent_swift(level2);
		raw("return result\n");
	}
	indent_swift(level);
	raw("}\n\n");
}

static void valueComparisonSupports_swift( Entity entity, int level ) {
	raw("\n");
	indent_swift(level);
	raw("//VALUE COMPARISON SUPPORT\n");
	hashAsValue_swift(entity, level);
	isValueEqual_swift(entity, level);
	isValueEqualOptionally_swift(entity, level);
}


//MARK: - unique rule

static void simpleUniquenessRule_swift( Entity entity, Linked_List unique, int level) {
	indent_swift(level);
	raw("//SIMPLE UNIQUE RULE\n\n");
	
	Expression attr_expr = LISTget_second(unique);
	
	switch (EXPRresult_is_optional(entity, attr_expr, CHECK_DEEP)) {
		case no_optional:
			indent_swift(level);
			raw("let attr = ");
			EXPR_swift(entity, attr_expr,attr_expr->return_type, no_optional, NO_PAREN);
			raw("\n");
			break;
			
		case yes_optional:
			indent_swift(level);
			raw("guard let attr = ");
			EXPR_swift(entity, attr_expr,attr_expr->return_type, yes_optional, NO_PAREN);
			wrap(" else { return nil }\n");
			break;
			
		case unknown_optional:
			indent_swift(level);
			raw("guard let attr = SDAI.FORCE_OPTIONAL( ");
			EXPR_swift(entity, attr_expr,attr_expr->return_type, unknown_optional, NO_PAREN);
			wrap( ") else { return nil }\n");
			break;
	}

	indent_swift(level);
	raw("return AnyHashable( attr )\n");
}

static void jointUniquenessRule_swift( Entity entity, Linked_List unique, int joint_count, int level) {
	indent_swift(level);
	raw("//JOINT UNIQUE RULE\n\n");
	
	indent_swift(level);
	wrap("var attributes: Array<AnyHashable> = []\n");
	
	int attr_no = -1;
	LISTdo(unique, attr_expr, Expression) {
		if( ++attr_no == 0 ) continue;	// skip label

		raw("\n");
		switch (EXPRresult_is_optional(entity, attr_expr, CHECK_DEEP)) {
			case no_optional:
			case yes_optional:
				indent_swift(level);
				wrap("guard let attr%d = ",
						 attr_no
						 );
				EXPR_swift(entity, attr_expr,attr_expr->return_type, yes_optional, NO_PAREN);
				wrap(" else { return nil }\n");
				break;
				
			case unknown_optional:
				indent_swift(level);
				wrap("guard let attr%d = SDAI.FORCE_OPTIONAL( ",
						 attr_no
						 );
				EXPR_swift(entity, attr_expr,attr_expr->return_type, unknown_optional, NO_PAREN);
				wrap(") else { return nil }\n");
				break;
		}
		
		indent_swift(level);
		wrap("attributes.append( AnyHashable(attr%d) )\n",
				 attr_no
				 );
	}LISTod;
	
	raw("\n");
	indent_swift(level);
	wrap("return AnyHashable( attributes )\n");
}

static void uniqueRule_swift( Entity entity, int serial, Linked_List unique, int level ) {
	int joint_count = LISTget_length(unique) - 1;
	
	{
		char buf[BUFSIZ];
		indent_swift(level);
		raw("public static func %s(SELF: SDAI.EntityReference) -> AnyHashable? {\n", 
				uniqueRuleLabel_swiftName(serial, unique, buf)
				);
	}

	{	int level2 = level+nestingIndent_swift;
		{
			char buf[BUFSIZ];
			indent_swift(level2);
			raw("guard let SELF = (SELF as? %s)?.pRef else { return nil }\n",
					ENTITY_swiftName(entity, NO_QUALIFICATION, SWIFT_QUALIFIER, buf)
					);
		}
		if( joint_count == 1 ) {
			simpleUniquenessRule_swift(entity, unique, level2);
		}
		else {
			jointUniquenessRule_swift(entity, unique, joint_count, level2);
		}
	}

	indent_swift(level);
	raw("}\n");	
	
}

static void uniqueDefinitions_swift( Entity entity, int level ) {
	Linked_List unique_rules = ENTITYget_uniqueness_list(entity);
	if( LISTis_empty(unique_rules) ) return;
	
	raw("\n");
	indent_swift(level);
	raw("//UNIQUENESS RULES\n");
	
	int serial = 0;
	LISTdo(unique_rules, unique, Linked_List) {
		uniqueRule_swift(entity, ++serial, unique, level);
	}LISTod;
}

//MARK: - constructor
static void expressConstructor( Entity entity, int level ) {
	int level2 = level+nestingIndent_swift;
	
	raw("\n");
	indent_swift(level);
	raw("//EXPRESS IMPLICIT PARTIAL ENTITY CONSTRUCTOR\n");
	
	Linked_List params = ENTITYget_constructor_params(entity);
	
	indent_swift(level);
	raw("/// initialize from explicit attribute values\n");
	indent_swift(level);
	raw("public init(");
	ALGargs_swift(entity, NO_FORCE_OPTIONAL, params, NO_DROP_SINGLE_LABEL, level);
	if( LISTget_length(params) > 1 ){
		raw("\n");
		indent_swift(level);
	}
	raw(") {\n");

	{	
		char buf[BUFSIZ];

		if( !LISTis_empty(params) ){
			raw("\n");
		}

		LISTdo(params, attr, Variable) {
			bool backing_store = false;//attribute_need_observer(attr);

			indent_swift(level2);
			raw("self.%s%s = ",
					backing_store ? BACKING_STORAGE_PREFIX : "",
					partialEntityAttribute_swiftName(attr, buf)
					);
			raw("%s\n",
					variable_swiftName(attr,buf)
					);
		}LISTod;

		if( !LISTis_empty(params) ){
			raw("\n");
		}
		indent_swift(level2);
		raw("super.init(asAbstractSuperclass:())\n");

	}
	
	indent_swift(level);
	raw("}\n");
}

static void copyConstructor( Entity entity, int level ) {

	raw("\n");
	indent_swift(level);
	raw("//PARTIAL ENTITY COPY CONSTRUCTOR\n");

	/*
	 public required convenience init(from original:SDAI.PartialEntity) {
	  let original = original as! Self
	 	self.init(attr0: original.attr0, attr1:original.attr1, ...)
	 }
	 */


	Linked_List params = ENTITYget_constructor_params(entity);

	indent_swift(level);
	raw("/// copy constructor\n");
	indent_swift(level);
	raw("public required convenience init(from original:SDAI.PartialEntity) {\n");
	{	int level2 = level+nestingIndent_swift;
		char buf[BUFSIZ];

		if( LISTget_length(params) > 0 ) {
			indent_swift(level2);
			raw("let original = original as! Self\n");
		}

		indent_swift(level2);
		raw("self.init(");

		char* sep = "";
		LISTdo(params, attr, Variable) {
			bool backing_store = false;

			raw("%s %s: ",
					sep,
					variable_swiftName(attr, buf)
					);
			raw("original.%s%s",
					backing_store ? BACKING_STORAGE_PREFIX : "",
					partialEntityAttribute_swiftName(attr, buf)
					);

			sep = ",";
		}LISTod;

		raw(" )\n");
	}

	indent_swift(level);
	raw("}\n");

}

static void p21Constructor( Entity entity, int level ) {
	
	raw("\n");
	indent_swift(level);
	raw("//p21 PARTIAL ENTITY CONSTRUCTOR\n");
	
	/*
	public required convenience init?(parameters: [P21Decode.ExchangeStructure.Parameter], exchangeStructure: P21Decode.ExchangeStructure) {
		let numParams = <num explicit attrs>
		guard parameters.count == numParams 
	  else { exchangeStructure.error = "number of p21 parameters(\(parameters.count)) are different from expected(\(numParams)) for entity(\(Self.entityName))"; return nil }
		
		guard case .success(let p0) = exchangeStructure.recoverRequiredParameter(as: <attrtype>.self, from: parameters[0])
		else { exchangeStructure.add(errorContext: "while recovering parameter #0 for entity(\(Self.entityName))"); return nil }
		...
		
		self.init(attr0: p0, attr1: p1, ...)
	}
	*/
	
	
	Linked_List params = ENTITYget_constructor_params(entity);
	
	indent_swift(level);
	raw("/// initialize from ISO 10303-21 exchange structure parameters\n");
	indent_swift(level);
	raw("public required convenience init?(");force_wrap();
	raw("parameters: [P21Decode.ExchangeStructure.Parameter],");force_wrap();
	raw("exchangeStructure: P21Decode.ExchangeStructure\n");
	indent_swift(level);
	raw(") {\n");
	{	int level2 = level+nestingIndent_swift;
		char buf[BUFSIZ];
		
		indent_swift(level2);
		raw("let numParams = %d\n",
				LISTget_length(params)
				);
		indent_swift(level2);
		raw("guard parameters.count == numParams\n"); 
		indent_swift(level2);
		raw("else { exchangeStructure.error = \"number of p21 parameters(\\(parameters.count)) are different from expected(\\(numParams)) for entity(\\(Self.entityName)) constructor\"; return nil }\n\n");
		
		int paramNo = 0;				
		LISTdo(params, attr, Variable) {
			bool omittable = VARis_dynamic(attr);
			bool optional = VARis_optional_by_large(attr);
			
			char* recoverFunc = "internalError";
			if( !optional && !omittable ) {
				recoverFunc = "recoverRequiredParameter";
			}
			else if( !optional && omittable ) {
				recoverFunc = "recoverOmittableParameter";
			}
			else if( optional && !omittable ) {
				recoverFunc = "recoverOptionalParameter";
			}
			else if( optional && omittable ) {
				recoverFunc = "recoverOmittableOptionalParameter";
			}
			
			indent_swift(level2);
			raw("guard case .success(let p%d) = exchangeStructure.%s(as: ",
					paramNo,
					recoverFunc
					);
			TYPE_head_swift(entity, attr->type, WO_COMMENT, LEAF_OWNED);
			raw(".self, from: parameters[%d])\n",
					paramNo
					);
			indent_swift(level2);
			raw("else { exchangeStructure.add(errorContext: \"while recovering parameter #%d for entity(\\(Self.entityName)) constructor\"); return nil }\n\n",
					paramNo
					);
			++paramNo;
		}LISTod;
				
		indent_swift(level2);
		raw("self.init(");
		
		paramNo = 0;				
		char* sep = "";
		LISTdo(params, attr, Variable) {
			if( TYPEis_logical(attr->type) ) {
				raw("%s %s: SDAI.LOGICAL(p%d)",
						sep,
						variable_swiftName(attr, buf),
						paramNo
						);
			}
			else {
				raw("%s %s: p%d",
						sep,
						variable_swiftName(attr, buf),
						paramNo
						);
			}
			sep = ",";
			++paramNo;			
		}LISTod;
		
		raw(" )\n");
	}
	
	indent_swift(level);
	raw("}\n");
}



//MARK: - partial entity swift definition

void partialEntityDefinition_swift
 ( Schema schema,
	Entity entity,
	int level,
	Linked_List attr_overrides,
	Linked_List dynamic_attrs ) 
{
	 int level2 = level  + nestingIndent_swift;
	 int level3 = level2 + nestingIndent_swift;
	 
	 raw("\n\n");
	 raw("//MARK: - Partial Entity\n");

	// markdown
	{	char buf[BUFSIZ];

		raw("\n/** Partial Entity\n");
		raw("- EXPRESS:\n");
		raw("```express\n");
		ENTITY_out(entity, level);
		raw("\n```\n");
		raw("- ENTITY reference:\n");
		raw("\t+\t``%s``\n",
				ENTITY_swiftName(entity, schema->superscope, DOCC_QUALIFIER, buf)
				);
		raw("*/\n");
	}

	 {	char buf[BUFSIZ];
		 
		 indent_swift(level);
		 raw("@_documentation(visibility:public)\n");
		 indent_swift(level);
		 wrap("public final class %s : SDAI.PartialEntity, @unchecked Sendable {\n",
					partialEntity_swiftName(entity, buf)
					);
	 }
	 
	 {
		 indent_swift(level2);
		 raw("public override class var entityReferenceType: SDAI.EntityReference.Type {\n");
		 {	char buf[BUFSIZ];
			 indent_swift(level2+nestingIndent_swift);
			 raw("%s.self\n",
					 ENTITY_swiftName(entity, NO_QUALIFICATION, SWIFT_QUALIFIER, buf)
					 );
		 }
		 indent_swift(level2);
		 raw("}\n");
		 
		 localAttributeDefinitions_swift(entity, level2, attr_overrides, dynamic_attrs);
		 fixupPartialComplexEntityAttributes_swift(entity, dynamic_attrs, level2);
		 
		 indent_swift(level2);
		 raw("public override var typeMembers: Set<SDAI.STRING> {\n");
		 {
			 indent_swift(level3);
			 raw("var members = Set<SDAI.STRING>()\n");

			 indent_swift(level3);
			 raw("members.insert(SDAI.STRING(Self.typeName))\n");
			 
			 TYPEinsert_select_type_members_swift(ENTITYget_type(entity), level3);
			 
			 indent_swift(level3);
			 raw("return members\n");
		 }
		 indent_swift(level2);
		 raw( "}\n\n" );
		 		 
		 valueComparisonSupports_swift(entity, level2);
		 TYPEwhereDefinitions_swift( entity, level2);
		 uniqueDefinitions_swift(entity, level2);

		 expressConstructor(entity, level2);
		 copyConstructor(entity, level2);
		 p21Constructor(entity, level2);

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
