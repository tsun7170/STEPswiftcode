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
 (Entity entity, char* access, char* prefix, Variable attr, bool isDynamic, int level, SwiftOutCommentOption in_comment, char buf[BUFSIZ]) {
	 if(isDynamic) {
		 indent_swift(level);
		 raw("/// - see the corresponding attribute accesser in the entity reference definition for the attribute value dynamic lookup logic\n");
	 }

	indent_swift(level);
	raw("%s var %s%s: ",access, prefix, partialEntityAttribute_swiftName(attr, buf));
	
	bool optional = variableType_swift(entity, attr, (isDynamic ? YES_FORCE_OPTIONAL : NO_FORCE_OPTIONAL), in_comment); // to handle p21 asterisk encoded attribute value for the explicit attributes redeclared as derived.
	 return optional;
}

//MARK: - attribute observers

static void emitObserveLeavingReferenceOwner(Variable attr, int level) {
	/* 
	 //OBSERVING ENTITY: <observer>
	 referencedComplex.partialEntityInstance(<observer>.self)?
		 .<observer attr>__observe(leavingReferencerOwner: referencerOwner)
	 */
	
	char buf[BUFSIZ];
	LISTdo( VARget_observers(attr), observingAttr, Variable) {
		Entity observingEntity = observingAttr->defined_in;
		
		indent_swift(level);
		raw("//OBSERVING ENTITY: %s\n", ENTITY_swiftName(observingEntity, NO_QUALIFICATION, buf));
		
		indent_swift(level);
		raw("referencedComplex.partialEntityInstance(%s.self)?", partialEntity_swiftName(observingEntity, buf));
		wrap(".%s__observe(leavingReferencerOwner: referencerOwner)\n", partialEntityAttribute_swiftName(observingAttr, buf));
	} LISTod
}

static void emitObserveRemovedReference(Variable attr, int level) {
	/* 
	 //OBSERVING ENTITY: <observer>
	 referencedComplex.partialEntityInstance(<observer>.self)?
		 .<observer attr>__observeRemovedReference(in: referencerOwner)
	 */
	
	char buf[BUFSIZ];
	LISTdo( VARget_observers(attr), observingAttr, Variable) {
		Entity observingEntity = observingAttr->defined_in;
		
		indent_swift(level);
		raw("//OBSERVING ENTITY: %s\n", ENTITY_swiftName(observingEntity, NO_QUALIFICATION, buf));
		
		indent_swift(level);
		raw("referencedComplex.partialEntityInstance(%s.self)?", partialEntity_swiftName(observingEntity, buf));
		wrap(".%s__observeRemovedReference(in: referencerOwner)\n", partialEntityAttribute_swiftName(observingAttr, buf));
	} LISTod
}

static void emitObserveAddedReference(Variable attr, int level) {
	/* 
	 //OBSERVING ENTITY: <observer>
	 referencedComplex.partialEntityInstance(<observer>.self)?
		 .<observer attr>__observeAddedReference(in: referencerOwner)
	 */
	
	char buf[BUFSIZ];
	LISTdo( VARget_observers(attr), observingAttr, Variable) {
		Entity observingEntity = observingAttr->defined_in;
		
		indent_swift(level);
		raw("//OBSERVING ENTITY: %s\n", ENTITY_swiftName(observingEntity, NO_QUALIFICATION, buf));
		
		indent_swift(level);
		raw("referencedComplex.partialEntityInstance(%s.self)?", partialEntity_swiftName(observingEntity, buf));
		wrap(".%s__observeAddedReference(in: referencerOwner)\n", partialEntityAttribute_swiftName(observingAttr, buf));
	} LISTod
}

static void attribureObservers_swift(Entity entity, Variable attr, int level) {
	 /*
		internal class <attr>__observer: SDAI.EntityReferenceObserver.ObserverCode {
		
			final override class func observe<RemovingEntities: Sequence, AddingEntities: Sequence>(
			referencer: SDAI.PartialEntity, 
				removing: RemovingEntities, adding: AddingEntities )
			where RemovingEntities.Element: SDAI.EntityReference, 
						AddingEntities.Element: SDAI.EntityReference
			{
				for referencedComplex in removing.map({ $0.complexEntity }) {
					for referencerOwner in referencer.owners {
						//OBSERVING ENTITY: <observer>
						referencedComplex.partialEntityInstance(<observer>.self)?
							.<observer attr>__observeRemovedReference(in: referencerOwner)
			
						...
						...
					}
				}
				
				for referencedComplex in adding.map({ $0.complexEntity }) {
					for referencerOwner in referencer.owners {
						//OBSERVING ENTITY: <observer>
						referencedComplex.partialEntityInstance(<observer>.self)?
							.<observer attr>__observeAddedReference(in: referencerOwner)
						
						...
						...		
					}
				}
			}
			
			final override class func observe(newReferencerOwner referencerOwner: SDAI.ComplexEntity) {
				guard let attrValue = referencerOwner.partialEntityInstance(<partial_entity>.self)?.<attr> else { return }
				for referencedComplex in attrValue.entityReferences.map({ $0.complexEntity }) {
					//OBSERVING ENTITY: <observer>
					referencedComplex.partialEntityInstance(<observer>.self)?
						.<observer attr>__observeAddedReference(in: referencerOwner)
					
					...
					...		
				}
			}
					
			final override class func observe(leavingReferencerOwner referencerOwner: SDAI.ComplexEntity) {
				guard let attrValue = referencerOwner.partialEntityInstance(<partial_entity>.self)?.<attr> else { return }
				for referencedComplex in attrValue.entityReferences.map({ $0.complexEntity }) {
					//OBSERVING ENTITY: <observer>
					referencedComplex.partialEntityInstance(<observer>.self)?
						.<observer attr>__observe(leavingReferencerOwner: referencerOwner)
					
					...
					...		
				}
			}
		}
		*/
	 
	int level2 = level + nestingIndent_swift;
	int level3 = level2 + nestingIndent_swift;
	int level4 = level3 + nestingIndent_swift;
	int level5 = level4 + nestingIndent_swift;

	char partialEntityName[BUFSIZ];
	partialEntity_swiftName(entity, partialEntityName);
	
	char partialAttributeName[BUFSIZ];
	partialEntityAttribute_swiftName(attr, partialAttributeName);

//	bool optional = VARis_dynamic(attr)||VARis_optional_by_large(attr);

	indent_swift(level);
	raw("/// attribute observer code\n");
	indent_swift(level);
	raw("internal class %s__observer: SDAI.EntityReferenceObserver.ObserverCode {\n",
			 partialAttributeName);
	{
		// attribute value change
		indent_swift(level2);
		raw("final override class func observe<RemovingEntities: Sequence, AddingEntities: Sequence>(\n");
		{
			indent_swift(level3);
			raw("referencer: SDAI.PartialEntity, removing: RemovingEntities, adding: AddingEntities )\n");
			indent_swift(level3);
			raw("where RemovingEntities.Element: SDAI.EntityReference, AddingEntities.Element: SDAI.EntityReference\n");
			indent_swift(level2);
			raw("{\n");
			indent_swift(level3);
			raw("for referencedComplex in removing.map({ $0.complexEntity }) {\n");
			indent_swift(level4);
			raw("for referencerOwner in referencer.owners {\n");
			
			if( VARis_observed(attr)){
				emitObserveRemovedReference(attr, level5);
			}
			
			if( VARis_overriden(attr)) {
				DictionaryEntry de;
				Variable overrider;
				DICTdo_init(attr->overriders, &de);
				while( 0 != ( overrider = DICTdo( &de ) ) ) {
					if( VARis_observed(overrider)){
						emitObserveRemovedReference(overrider, level5);
					}
				}
			}
			
			indent_swift(level4);
			raw("}\n");
			indent_swift(level3);
			raw("}\n\n");
			
			indent_swift(level3);
			raw("for referencedComplex in adding.map({ $0.complexEntity }) {\n");
			indent_swift(level4);
			raw("for referencerOwner in referencer.owners {\n");
			
			if( VARis_observed(attr)){
				emitObserveAddedReference(attr,  level5);
			}
			
			if( VARis_overriden(attr)) {
				DictionaryEntry de;
				Variable overrider;
				DICTdo_init(attr->overriders, &de);
				while( 0 != ( overrider = DICTdo( &de ) ) ) {
					if( VARis_observed(overrider)){
						emitObserveAddedReference(overrider, level5);
					}
				}
			}			
			
			indent_swift(level4);
			raw("}\n");
			indent_swift(level3);
			raw("}\n");
		}
		indent_swift(level2);
		raw("}\n\n");

		// incoming owner
		indent_swift(level2);
		raw("final override class func observe(newReferencerOwner referencerOwner: SDAI.ComplexEntity) {\n");
		{
			indent_swift(level3);
			raw("guard let attrValue = referencerOwner.partialEntityInstance(%s.self)?.%s else { return }\n",
					partialEntityName, partialAttributeName);

			indent_swift(level3);
			raw("for referencedComplex in attrValue.entityReferences.map({ $0.complexEntity }) {\n");

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

			indent_swift(level3);
			raw("}\n");
			
		}
		indent_swift(level2);
		raw("}\n\n");
		
		// leaving owner
		indent_swift(level2);
		raw("final override class func observe(leavingReferencerOwner referencerOwner: SDAI.ComplexEntity) {\n");
		{
			indent_swift(level3);
			raw("guard let attrValue = referencerOwner.partialEntityInstance(%s.self)?.%s else { return }\n",
					partialEntityName, partialAttributeName);

			indent_swift(level3);
			raw("for referencedComplex in attrValue.entityReferences.map({ $0.complexEntity }) {\n");

			if( VARis_observed(attr)){
				emitObserveLeavingReferenceOwner(attr, level3);
			}
			
			if( VARis_overriden(attr)) {
				DictionaryEntry de;
				Variable overrider;
				DICTdo_init(attr->overriders, &de);
				while( 0 != ( overrider = DICTdo( &de ) ) ) {
					if( VARis_observed(overrider)){
						emitObserveLeavingReferenceOwner(overrider, level3);
					}
				}
			}

			indent_swift(level3);
			raw("}\n");
			
		}
		indent_swift(level2);
		raw("}\n\n");
		
		
	}
	indent_swift(level);
	raw("}\n");
}


static void observerBroadcasting(Entity entity, int level) {
	/*
	 final public override func broadcast(addedToComplex complex: SDAI.ComplexEntity) {
		 super.broadcast(addedToComplex: complex)

		 <attr>__observer.observe(newReferencerOwner: complex)

		 ...
		 ...
	 }

	 final public override func broadcast(removedFromComplex complex: SDAI.ComplexEntity) {
		 super.broadcast(removedFromComplex: compex)
		 
		 <attr>__observer.observe(leavingReferencerOwner: complex)

		 ...
		 ...	 
	 }
	 */
	
	int level2 = level + nestingIndent_swift;
	

	Linked_List local_attributes = entity->u.entity->attributes;
	bool emitted = false;
	
//	char entityname[BUFSIZ];
//	ENTITY_swiftName(entity, NO_QUALIFICATION, entityname);
//
	LISTdo(local_attributes, attr, Variable) {
		if( attribute_need_observer(attr) && !VARis_redeclaring(attr) ){
			if( !emitted ){
				emitted = true;
				raw("\n");
				indent_swift(level);
				raw("/// broadcasting a new complex entity becoming an owner of the partial entity\n");
				indent_swift(level);
				raw("final public override func broadcast(addedToComplex complex: SDAI.ComplexEntity) {\n");
			}
			
			char partialAttrName[BUFSIZ];
			partialEntityAttribute_swiftName(attr, partialAttrName);
			
			indent_swift(level2);
			raw("%s__observer.observe(newReferencerOwner: complex)\n", partialAttrName);
		}
	}LISTod;

	if( !emitted ) return;
	indent_swift(level);
	raw("}\n\n");
	
	indent_swift(level);
	raw("/// broadcasting a complex entity withdrawing an owner of the partial entity\n");
	indent_swift(level);
	raw("final public override func broadcast(removedFromComplex complex: SDAI.ComplexEntity) {\n");
		
	LISTdo(local_attributes, attr, Variable) {
		if( attribute_need_observer(attr) && !VARis_redeclaring(attr) ){			
			char partialAttrName[BUFSIZ];
			partialEntityAttribute_swiftName(attr, partialAttrName);
			
			indent_swift(level2);
			raw("%s__observer.observe(leavingReferencerOwner: complex)\n", partialAttrName);
		}
	}LISTod;
		
	indent_swift(level);
	raw("}\n");
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
		 var newValue = newValue
		 let observer = SDAI.EntityReferenceObserver(referencer: self, observerCode: <attr>__observer.self)
		 newValue?.configure(with: observer)
		 observer.observe(removing: SDAI.UNWRAP(seq:_<attr>?.entityReferences), 
			 adding: SDAI.UNWRAP(seq: newValue?.entityReferences) )
		}
	 }
	 private var _<attr>: <attr_type>

	 */
	
	
	int level2 = level + nestingIndent_swift;
	int level3 = level2 + nestingIndent_swift;
	
	char buf[BUFSIZ];
	
	char partialAttrName[BUFSIZ];
	partialEntityAttribute_swiftName(attr,partialAttrName);
	
//	bool optional = VARis_dynamic(attr)||VARis_optional_by_large(attr);

	indent_swift(level);
	raw("/// EXPLICIT ATTRIBUTE %s\n", 
			VARis_observed(attr) ? "(OBSERVED)" : (attribute_need_observer(attr) ? "(SUBTYPE ATTR OBSERVED)" : ""));
	bool optional = attributeHead_swift(entity, "public internal(set)", 
																			NO_PREFIX, attr, VARis_dynamic(attr), 
																			level, NOT_IN_COMMENT, buf);
	
	if( attribute_need_observer(attr) ) {
		raw("{ // OBSERVED EXPLICIT ATTRIBUTE\n");
		{
			indent_swift(level2);
			raw("get {\n");
			{
				indent_swift(level3);
				raw("return %s%s%s.copy()\n",
						BACKING_STORAGE_PREFIX, partialAttrName, optional ? "?" : "");					
			}
			indent_swift(level2);
			raw("} // getter\n");
			indent_swift(level2);				
			raw("set {\n");
			{
				indent_swift(level3);
				raw("%s newValue = newValue\n", 
						TYPEis_entity(attr->type) ? "let" : "var");
				indent_swift(level3);
				raw("let observer = SDAI.EntityReferenceObserver(referencer: self, observerCode: %s__observer.self)\n",
						partialAttrName);
				indent_swift(level3);
				raw("newValue%s.configure(with: observer)\n", optional ? "?" : "");
				indent_swift(level3);
				raw("observer.observe(removing: SDAI.UNWRAP(seq:%s%s%s.entityReferences),",
						BACKING_STORAGE_PREFIX, partialAttrName, optional ? "?" : ""); 
				wrap("adding: SDAI.UNWRAP(seq: newValue%s.entityReferences) )\n", 
						 optional ? "?" : "");
			}
			indent_swift(level2);
			raw("} // setter\n");
		}
		indent_swift(level);
		raw("}\n\n");
		
		indent_swift(level);
		raw("/// backing storage for observed attribute\n");
		attributeHead_swift(entity, "private", 
												BACKING_STORAGE_PREFIX, attr, VARis_dynamic(attr), 
												level, NOT_IN_COMMENT, buf);
		raw("\n\n");
		
		attribureObservers_swift(entity, attr, level);
	}
	else {
		raw(" // PLAIN EXPLICIT ATTRIBUTE\n");
	}
}

static void explicitAttributeRedefinition_swift(Entity entity, Variable attr, int level) {
	char buf[BUFSIZ];
	
	attributeHead_swift(entity, "/* override", NO_PREFIX, attr, NO_DYNAMIC, level, YES_IN_COMMENT, buf);
	raw("\t//EXPLICIT REDEFINITION(%s)", 
			ENTITY_swiftName(attr->original_attribute->defined_in, NO_QUALIFICATION, buf));
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
			partialEntityAttribute_swiftName(attr, buf) );
	raw("%s? {\n",
			dynamicAttribute_swiftProtocolName(attr, buf) );
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
			raw("%s",sep);
			wrap("%s.typeIdentity",  partialEntity_swiftName(overrider->defined_in, buf));
			sep = ", ";
		}
		wrap("])\n");
		
		indent_swift(level2);
		raw("return resolved as? %s\n", dynamicAttribute_swiftProtocolName(attr, buf) );
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
				partialEntity_swiftName(entity, buf));
		
		LISTdo(dynamic_attrs, attr, Variable){
			if( VARis_optional(attr) )continue;
			
			raw("\n");
			indent_swift(level2);
			raw("if pe.%s == nil, ", partialEntityAttribute_swiftName(attr, buf));
			wrap("self.%s__provider(complex: partialComplex) == nil, ", buf);
			positively_wrap();
			wrap("let base_%s__provider = self.%s__provider(complex: baseComplex) {\n", 
					 buf, buf);
			
			indent_swift(level3);
			raw("pe.%s = base_%s__provider.%s__getter(complex: baseComplex)\n",
					buf, buf, buf);

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
	raw("internal func %s__getter(SELF: %s) -> ", 
			attrName, ENTITY_swiftName(entity, NO_QUALIFICATION, buf) );
	
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
			 ENTITY_swiftName(entity, NO_QUALIFICATION, buf));
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
		Expression simplified = EXPR_decompose(VARget_initializer(attr), VARget_type(attr), &tempvar_id, &tempvars);
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
	ENTITY_swiftName(original_attr->defined_in, NO_QUALIFICATION, originEntityName);
	
	char partialAttrName[BUFSIZ];
	partialEntityAttribute_swiftName(attr, partialAttrName);
	
	// provider protocol conformance
	indent_swift(level);
	raw("/// DERIVE ATTRIBUTE REDEFINITION(origin: %s)\n", originEntityName);
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
		Expression simplified = EXPR_decompose(VARget_initializer(attr), VARget_type(attr), &tempvar_id, &tempvars);
		EXPR_tempvars_swift(entity, tempvars, level2);
		
		indent_swift(level2);
		raw("let value = ");
		EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, entity, simplified, VARget_type(attr), NO_PAREN, OP_UNKNOWN, YES_WRAP);
		raw("\n");
		
//		if( attribute_need_observer(original_attr) ) {
//			char originalAttrName[BUFSIZ];
//			variable_swiftName(original_attr,originalAttrName);
//
//			indent_swift(level2);
//			raw("SELF.%s = ", originalAttrName );
//			if( VARis_optional_by_large(original_attr) ){
//				TYPE_head_swift(entity, original_attr->type, WO_COMMENT, LEAF_OWNED);
//				raw("(value)\n");
//			}
//			else{
//				if( TYPEis_logical(VARget_type(original_attr)) ){
//					wrap("SDAI.LOGICAL( ");
//				}
//				else{
//					wrap("SDAI.UNWRAP( ");
//				}
//				TYPE_head_swift(entity, original_attr->type, WO_COMMENT, LEAF_OWNED);
//				raw("(value) )\n");
//			}
//		}
		
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
	
	int level2 = level+nestingIndent_swift;
	
	char partialAttrName[BUFSIZ];
	partialEntityAttribute_swiftName(attr, partialAttrName);
	
	char attrTypeName[BUFSIZ];
	TYPEhead_string_swift(entity, attr->type, NOT_IN_COMMENT, LEAF_OWNED, attrTypeName);
	
	indent_swift(level);
	raw("/// INVERSE SIMPLE ATTRIBUTE\n");
	Variable observing_attr = VARget_inverse(attr);
	indent_swift(level);
	raw("/// observing %s", 
			partialEntity_swiftName(observing_attr->defined_in, buf) );
	raw(" .%s\n",
			partialEntityAttribute_swiftName(observing_attr, buf));

	indent_swift(level);
	raw("internal private(set) weak var %s: %s?\n",
			partialAttrName, attrTypeName);
	
	// addedReference observer func
	indent_swift(level);
	raw("/// INVERSE SIMPLE ATTR SUPPORT(ADDER)\n");
	indent_swift(level);
	raw("internal func %s__observeAddedReference(in complex: SDAI.ComplexEntity) {\n", partialAttrName);
	{	
		indent_swift(level2);
		raw("guard let newSource = complex.entityReference(%s.self) else { return }\n",
				attrTypeName);

		indent_swift(level2);
		raw("self.%s = newSource\n", partialAttrName );
	}
	indent_swift(level);
	raw("}\n");

	// removedReference observer func
	indent_swift(level);
	raw("/// INVERSE SIMPLE ATTR SUPPORT(REMOVER)\n");
	indent_swift(level);
	raw("internal func %s__observeRemovedReference(in complex: SDAI.ComplexEntity) {\n", partialAttrName);
	{
		indent_swift(level2);
		raw("guard let _ = complex.entityReference(%s.self) else { return }\n",
				attrTypeName);
		
		indent_swift(level2);
		raw("self.%s = nil\n", partialAttrName );
	}
	indent_swift(level);
	raw("}\n");

	// leavingReferenceOwner observer func
	indent_swift(level);
	raw("/// INVERSE SIMPLE ATTR SUPPORT(LEAVING REFERENCE OWNER)\n");
	indent_swift(level);
	raw("internal func %s__observe(leavingReferencerOwner complex: SDAI.ComplexEntity) {\n", partialAttrName);
	{
		indent_swift(level2);
		raw("guard let _ = complex.entityReference(%s.self) else { return }\n",
				attrTypeName);
		
		indent_swift(level2);
		raw("self.%s = nil\n", partialAttrName );
	}
	indent_swift(level);
	raw("}\n");
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
	
	int level2 = level+nestingIndent_swift;
	
	char partialAttrName[BUFSIZ];
	partialEntityAttribute_swiftName(attr, partialAttrName);
	
	char attrBaseTypeName[BUFSIZ];
	TYPEhead_string_swift(entity, ATTRget_base_type(attr), NOT_IN_COMMENT, LEAF_OWNED, attrBaseTypeName);

	indent_swift(level);
	raw("/// INVERSE AGGREGATE ATTRIBUTE\n");
	Variable observing_attr = VARget_inverse(attr);
	indent_swift(level);
	raw("/// observing %s", 
			partialEntity_swiftName(observing_attr->defined_in, buf) );
	raw(" .%s\n",
			partialEntityAttribute_swiftName(observing_attr, buf));

	indent_swift(level);
	raw("internal var %s: ",partialAttrName);
	TYPE_head_swift(entity, attr->type, WO_COMMENT, LEAF_OWNED);
	raw(" {\n");
	{
		indent_swift(level2);
		raw("return _%s.map{ $0.reference }\n", partialAttrName);
	}
	indent_swift(level);
	raw("}\n");
	
	// backing store
	indent_swift(level);
	raw("private var _%s = ",partialAttrName);
//	TYPE_head_swift(entity, attr->type, WO_COMMENT, LEAF_UNOWNED);
	EXPRaggregate_init_swift(NO_RESOLVING_GENERIC, entity, EXPRempty_aggregate_initializer(), attr->type, LEAF_UNOWNED);
	raw(" // unowned backing store\n");
	
	// addedReference observer func
	indent_swift(level);
	raw("/// INVERSE AGGREGATE ATTR SUPPORT(ADDER)\n");
	indent_swift(level);
	raw("internal func %s__observeAddedReference(in complex: SDAI.ComplexEntity) {\n", partialAttrName);
	{
		indent_swift(level2);
		raw("guard let newSource = complex.entityReference(%s.self) else { return }\n",
				attrBaseTypeName);

		indent_swift(level2);
			raw("self._%s.add(member: SDAI.UnownedWrap(newSource) )\n", partialAttrName );
	}
	indent_swift(level);
	raw("}\n");

	// removedReference observer func
	indent_swift(level);
	raw("/// INVERSE AGGREGATE ATTR SUPPORT(REMOVER)\n");
	indent_swift(level);
	raw("internal func %s__observeRemovedReference(in complex: SDAI.ComplexEntity) {\n", partialAttrName);
	{
		indent_swift(level2);
		raw("guard let oldSource = complex.entityReference(%s.self) else { return }\n",
				attrBaseTypeName);
		
		indent_swift(level2);
		raw("let success = self._%s.remove(member: SDAI.UnownedWrap(oldSource) )\n", partialAttrName );

		indent_swift(level2);
		raw("assert(success, \"failed to remove inverse attribute element\")\n");
	}
	indent_swift(level);
	raw("}\n");

	// leaving reference owner observer func
	indent_swift(level);
	raw("/// INVERSE AGGREGATE ATTR SUPPORT(LEAVING REFERENCE OWNER)\n");
	indent_swift(level);
	raw("internal func %s__observe(leavingReferencerOwner complex: SDAI.ComplexEntity) {\n", partialAttrName);
	{
		indent_swift(level2);
		raw("guard let oldSource = complex.entityReference(%s.self) else { return }\n",
				attrBaseTypeName);
		
		indent_swift(level2);
		raw("let success = self._%s.removeAll(member: SDAI.UnownedWrap(oldSource) )\n", partialAttrName );

		indent_swift(level2);
		raw("assert(success, \"failed to remove inverse attribute elements\")\n");
	}
	indent_swift(level);
	raw("}\n");
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
					(VARis_optional_by_large(attr)||(VARis_dynamic(attr) && !TYPEis_logical(attr->type))) ? "?" : "" );
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
					buf,optional, buf,optional );
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

//MARK: - where rule
//static void whereDefinitions_swift( Entity entity, int level ) {
//	Linked_List where_rules = TYPEget_where(entity);
//	if( LISTis_empty(where_rules) ) return;
//	
//	indent_swift(level);
//	raw("//WHERE RULES\n");
//
//	char whereLabel[BUFSIZ];
//	char buf[BUFSIZ];
//	LISTdo( where_rules, where, Where ){
//		indent_swift(level);
//		wrap("public func %s(SELF: %s) -> SDAI.LOGICAL {\n", 
//				 whereRuleLabel_swiftName(where, whereLabel), 
//				 ENTITY_swiftName(entity, NO_QUALIFICATION, buf) );
//		
//		{	int level2 = level+nestingIndent_swift;
//			int tempvar_id = 1;
//			Linked_List tempvars;
//			Expression simplified = EXPR_decompose(where->expr, &tempvar_id, &tempvars);
//			EXPR_tempvars_swift(entity, tempvars, level2);
//			
//			indent_swift(level2);
//			raw("return ");
////			EXPR_swift(entity, where->expr, Type_Logical, NO_PAREN);			
////			EXPR_swift(entity, simplified, Type_Logical, NO_PAREN);			
//			if( EXPRresult_is_optional(simplified, CHECK_DEEP) != no_optional ){
//				TYPE_head_swift(entity, Type_Logical, WO_COMMENT);	// wrap with explicit type cast
//				raw("(");
//				EXPR_swift(entity, simplified, Type_Logical, NO_PAREN);			
//				raw(")");
//			}
//			else {
//				EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, entity, simplified, Type_Logical, NO_PAREN, OP_UNKNOWN, YES_WRAP);
//			}
//			raw("\n");
//			EXPR_delete_tempvar_definitions(tempvars);
//		}
//		
//		indent_swift(level);
//		raw("}\n");
//	}LISTod;
//	raw("\n");
//}

//MARK: - unique rule
//static Variable unique_attribute( Expression expr ) {
//	assert(expr->u_tag == expr_is_variable);
//	return expr->u.variable;
//}

static void simpleUniquenessRule_swift( Entity entity, Linked_List unique, int level) {
	indent_swift(level);
	raw("//SIMPLE UNIQUE RULE\n\n");
	
	Expression attr_expr = LISTget_second(unique);
	
	switch (EXPRresult_is_optional(attr_expr, CHECK_DEEP)) {
		case no_optional:
			indent_swift(level);
			raw("let attr = ");
			EXPR_swift(entity, attr_expr,attr_expr->return_type, NO_PAREN);
			raw("\n");
			break;
			
		case yes_optional:
			indent_swift(level);
			raw("guard let attr = ");
			EXPR_swift(entity, attr_expr,attr_expr->return_type, NO_PAREN);
			wrap(" else { return nil }\n");
			break;
			
		case unknown:
			indent_swift(level);
			raw("guard let attr = SDAI.FORCE_OPTIONAL( ");
			EXPR_swift(entity, attr_expr,attr_expr->return_type, NO_PAREN);
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
	wrap("var attributes: Array<AnyHashable> = []\n\n");
	
	int attr_no = -1;
	LISTdo(unique, attr_expr, Expression) {
		if( ++attr_no == 0 ) continue;	// skip label
				
		switch (EXPRresult_is_optional(attr_expr, CHECK_DEEP)) {
			case no_optional:
				indent_swift(level);
				wrap("let attr%d = ", attr_no);
				EXPR_swift(entity, attr_expr,attr_expr->return_type, NO_PAREN);
				raw("\n");
				break;
				
			case yes_optional:
				indent_swift(level);
				wrap("guard let attr%d = ", attr_no);
				EXPR_swift(entity, attr_expr,attr_expr->return_type, NO_PAREN);
				wrap(" else { return nil }\n");
				break;
				
			case unknown:
				indent_swift(level);
				wrap("guard let attr%d = SDAI.FORCE_OPTIONAL( ", attr_no);
				EXPR_swift(entity, attr_expr,attr_expr->return_type, NO_PAREN);
				wrap(") else { return nil }\n");
				break;
		}
		
		indent_swift(level);
		wrap("attributes.append( AnyHashable(attr%d) )\n", attr_no);		
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
				uniqueRuleLabel_swiftName(serial, unique, buf) );
	}

	{	int level2 = level+nestingIndent_swift;
		{
			char buf[BUFSIZ];
			indent_swift(level2);
			raw("guard let SELF = SELF as? %s else { return nil }\n",
					ENTITY_swiftName(entity, NO_QUALIFICATION, buf) );
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
	raw("public init(");
	ALGargs_swift(entity, NO_FORCE_OPTIONAL, params, NO_DROP_SINGLE_LABEL, level);
	raw(") {\n");

	{	
		char buf[BUFSIZ];
		LISTdo(params, attr, Variable) {
			bool backing_store = attribute_need_observer(attr);
			
			indent_swift(level2);
			raw("self.%s%s = ",
					backing_store ? BACKING_STORAGE_PREFIX : "",
					partialEntityAttribute_swiftName(attr, buf));
			raw("%s\n", variable_swiftName(attr,buf));
		}LISTod;
		
		indent_swift(level2);
		raw("super.init(asAbstructSuperclass:())\n\n");
		
		LISTdo(params, attr, Variable) {
			if( attribute_need_observer(attr) ){
				bool optional = VARis_dynamic(attr)||VARis_optional_by_large(attr);
				
				indent_swift(level2);
				raw("self.%s%s.configure(with: SDAI.EntityReferenceObserver(referencer: self, observerCode: %s__observer.self))\n",
						partialEntityAttribute_swiftName(attr, buf),
						optional ? "?" : "",
						buf);
			}			
		}LISTod;
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
	raw("public required convenience init?(parameters: [P21Decode.ExchangeStructure.Parameter], exchangeStructure: P21Decode.ExchangeStructure) {\n");
	{	int level2 = level+nestingIndent_swift;
		char buf[BUFSIZ];
		
		indent_swift(level2);
		raw("let numParams = %d\n", LISTget_length(params));
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
			raw("guard case .success(let p%d) = exchangeStructure.%s(as: ",paramNo, recoverFunc );
			TYPE_head_swift(entity, attr->type, WO_COMMENT, LEAF_OWNED);
			raw(".self, from: parameters[%d])\n",paramNo );
			indent_swift(level2);
			raw("else { exchangeStructure.add(errorContext: \"while recovering parameter #%d for entity(\\(Self.entityName)) constructor\"); return nil }\n\n", paramNo);
			++paramNo;			
		}LISTod;
				
		indent_swift(level2);
		raw("self.init(");
		
		paramNo = 0;				
		char* sep = "";
		LISTdo(params, attr, Variable) {
			if( TYPEis_logical(attr->type) ) {
				raw("%s %s: SDAI.LOGICAL(p%d)", sep, variable_swiftName(attr, buf), paramNo);
			}
			else {
				raw("%s %s: p%d", sep, variable_swiftName(attr, buf), paramNo);
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
 ( Entity entity, int level, Linked_List attr_overrides, Linked_List dynamic_attrs ) 
{
	 int level2 = level  + nestingIndent_swift;
	 int level3 = level2 + nestingIndent_swift;
	 
	 raw("\n\n");
	 raw("//MARK: - Partial Entity\n");
	 
	 {	char buf[BUFSIZ];
		 
		 indent_swift(level);
		 wrap("public final class %s : SDAI.PartialEntity {\n", partialEntity_swiftName(entity, buf));
	 }
	 
	 {
		 indent_swift(level2);
		 raw("public override class var entityReferenceType: SDAI.EntityReference.Type {\n");
		 {	char buf[BUFSIZ];
			 indent_swift(level2+nestingIndent_swift);
			 raw("%s.self\n", ENTITY_swiftName(entity, NO_QUALIFICATION, buf));
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
		 p21Constructor(entity, level2);
		 observerBroadcasting(entity, level2);
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
