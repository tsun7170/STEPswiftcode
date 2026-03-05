//
//  swift_entity_reference.c
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
#include "swift_symbol.h"
#include "swift_algorithm.h"

#define YES_OPTIONAL_ATTR  true
#define NO_OPTIONAL_ATTR  false

//MARK: - entity reference
static void ENTITY_supersub_out(Schema schema, Entity entity) {
	int entity_index = 0;

	raw("\n");
	raw("- SUPERTYPES:\n");

	Linked_List supertypes = ENTITYget_super_entity_list(entity);

	char buf1[BUFSIZ];
	LISTdo( supertypes, super, Entity ) {
		if( super == entity )continue;
		++entity_index;

		raw("\t%d.\t``%s``\n",
				entity_index,
				ENTITY_swiftName(super, schema->superscope, DOCC_QUALIFIER, buf1)
				);
	}LISTod;

	raw("\n");
	raw("- SUBTYPES:\n");
	entity_index = 0;
	Linked_List subtypes = ENTITYget_sub_entity_list(entity);

	LISTdo( subtypes, sub, Entity ) {
		if( sub == entity )continue;
		++entity_index;

		raw("\t%d.\t``%s``\n",
				entity_index,
				ENTITY_swiftName(sub, schema->superscope, DOCC_QUALIFIER, buf1)
				);

	}LISTod;
}

static void supertypeReferenceDefinition_swift(Entity entity, int level ) {
	int entity_index = 0;
	int level2 = level+nestingIndent_swift;

	/*
	 public let super_<SUPERENTITY>: <SUPERENTITY> 	// [n]	 
	 */	
	raw("\n");
	indent_swift(level);
	raw("//MARK: SUPERTYPES\n");
	
	Linked_List supertypes = ENTITYget_super_entity_list(entity);
	
	char buf1[BUFSIZ];
	char buf2[BUFSIZ];
	LISTdo( supertypes, super, Entity ) {
		++entity_index;
		if( super == entity ){
			indent_swift(level);
			wrap("public var %s%s: %s { return self } \t// [%d]\n", 
					 superEntity_swiftPrefix,
					 ENTITY_swiftName(super, NO_QUALIFICATION, SWIFT_QUALIFIER, buf1),
					 ENTITY_swiftName(super, entity, SWIFT_QUALIFIER, buf2),
					 entity_index
					 );
		}
		else {
			indent_swift(level);
			wrap("public let %s%s: %s \t// [%d]\n", 
					 superEntity_swiftPrefix,
					 ENTITY_swiftName(super, NO_QUALIFICATION, SWIFT_QUALIFIER, buf1),
					 ENTITY_swiftName(super, entity, SWIFT_QUALIFIER, buf2),
					 entity_index
					 );
		}
	}LISTod;

	/*
	 public let sub_<SUPERENTITY>: <SUPERENTITY>? 	// [n]	 
	 */	
	raw("\n");
	indent_swift(level);
	raw("//MARK: SUBTYPES\n");
	
	Linked_List subtypes = ENTITYget_sub_entity_list(entity);
	
	LISTdo( subtypes, sub, Entity ) {
		if( sub == entity )continue;
		++entity_index;
		indent_swift(level);
		wrap("public var %s%s: %s? {\t// [%d]\n", 
				 subEntity_swiftPrefix,
				 ENTITY_swiftName(sub, NO_QUALIFICATION, SWIFT_QUALIFIER, buf1),
				 ENTITY_swiftName(sub, entity, SWIFT_QUALIFIER, buf2),
				 entity_index
				 );

		indent_swift(level2);
		wrap("return self.complexEntity.entityReference(%s.self)\n", 
				 buf2
				 );

		indent_swift(level);
		raw("}\n\n");
	}LISTod;
}

static bool ENTITY_is_initializable_by_void(Entity entity) {
  Linked_List supertypes = ENTITYget_super_entity_list(entity);

  LISTdo( supertypes, super, Entity ) {
    Linked_List params = ENTITYget_constructor_params(super);
    if( LISTget_length(params) > 0 ) return false;
  }LISTod;

  return true;
}

static void ENTITY_partial_out(Schema schema, Entity entity) {
	raw("\n");
	raw("- PARTIAL ENTITY:\n");

	char partial[BUFSIZ]; partialEntity_swiftName(entity, schema->superscope, DOCC_QUALIFIER, partial);

	raw("\t+\t``%s``\n",
			partial
			);
}

static void partialEntityReferenceDefinition_swift( Entity entity, int level ) {
	/*
	 public let partialEntity: _<entity>
	 */
	
	raw("\n");
	indent_swift(level);
	raw("//MARK: PARTIAL ENTITY\n");

	char partial[BUFSIZ]; partialEntity_swiftName(entity, entity->superscope, SWIFT_QUALIFIER, partial);

	indent_swift(level);
	raw("public override class var partialEntityType: SDAI.PartialEntity.Type {\n");
	{
		indent_swift(level+nestingIndent_swift);
		raw("%s.self\n",
				partial
				);
	}
	indent_swift(level);
	raw("}\n");

	indent_swift(level);
	raw("/// partial entity reference\n");
	indent_swift(level);
	raw("public let partialEntity: %s\n",
			partial
			);
}


static void superEntityReference_swift(Entity entity, Entity supertype, bool is_subtype_attr, bool can_wrap) {
	if( supertype == entity ) {
		wrap_if(can_wrap,"self");
	}
	else {
		char buf[BUFSIZ];
		wrap_if(can_wrap,"%s%s",
						(is_subtype_attr ? subEntity_swiftPrefix : superEntity_swiftPrefix),
						ENTITY_swiftName(supertype, NO_QUALIFICATION, SWIFT_QUALIFIER, buf)
						);
	}
}

static void partialEntityReference_swift(Entity entity, Entity partial, bool is_subtype_attr, bool can_wrap) {
	superEntityReference_swift(entity, partial, is_subtype_attr, can_wrap);
	if( is_subtype_attr )raw("?");
	wrap(".partialEntity");
}

//MARK: - Attribute References

/// <#Description#>
/// @param entity <#entity description#>
/// @param access <#access description#>
/// @param attr <#attr description#>
/// @param is_subtype_attr <#is_subtype_attr description#>
/// @param level <#level description#>
/// @param label <#label description#>
/// @return optionality of variable type
static bool attributeRefHead_swift(Schema schema, Entity entity, char* access, Variable attr, bool is_subtype_attr, int level, char* label) {

	 { char buf[BUFSIZ];
		 
		 //markdown
		 indent_swift(level);
		 raw("/// __%s__ attribute\n", 
				 label
				 );
		 if( VARis_inverse(attr) ){
			 Variable observing_attr = VARget_inverse(attr);
			 indent_swift(level);
			 raw("/// observing ``%s",
					 ENTITY_swiftName(observing_attr->defined_in, schema->superscope, DOCC_QUALIFIER, buf)
					 );
			 raw("%s%s``\n",
					 DOCC_QUALIFIER,
					 attribute_swiftName(observing_attr, buf)
					 );
		 }
		 indent_swift(level);
		 raw("/// - origin: %s( ``%s`` )\n", 
				 (entity==attr->defined_in ? "SELF" : (is_subtype_attr ? "SUB" : "SUPER")), 
				 ENTITY_swiftName(attr->defined_in, schema->superscope, DOCC_QUALIFIER, buf)
				 );
		 
		 indent_swift(level);
		 raw("%s var %s: ",
				 access,
				 attribute_swiftName(attr,buf)
				 );
	 }
	 
	bool optional = variableType_swift(entity, attr,
																		 (is_subtype_attr ? YES_FORCE_OPTIONAL : NO_FORCE_OPTIONAL),
																		 NOT_IN_COMMENT);

	 raw(" {\n");
	return optional;
 }


//MARK: - explicit static attribute access

static void explicitStaticSetterCore
(
 Entity entity,
 Entity original_partial,
 Variable attr,
 bool is_subtype_attr,
 char *partialAttrName,
 bool check_dynamic_provider,
 char *originalPartialEntityName,
 int level
 )
{
  int level2 = level+nestingIndent_swift;

  Variable target_attr = VARget_original_attr(attr);
  bool target_optional = VARis_optional_by_large(target_attr);

  TypeBody target_typebody = TYPEget_body(target_attr->type);

  indent_swift(level);
  raw("set(newValue) {\n");
  {
    if( check_dynamic_provider ){
      indent_swift(level2);
      raw("if let _ = %s.%s__provider(complex: self.complexEntity) { return }\n\n",
          originalPartialEntityName,
          partialAttrName);
    }

    indent_swift(level2);
    if( is_subtype_attr ){
      raw("guard let partial = ");
      force_wrap();
      if( VARis_redeclaring(attr) ){
        superEntityReference_swift(entity, attr->defined_in, is_subtype_attr, YES_WRAP);
        raw("?");
        force_wrap();
        wrap(".");
        partialEntityReference_swift(entity, original_partial, false, NO_WRAP);
     }
      else {
        partialEntityReference_swift(entity, original_partial, is_subtype_attr, NO_WRAP);
      }
      force_wrap();
      wrap(" else { return }\n");
    }
    else{
      raw("let partial = ");
      partialEntityReference_swift(entity, original_partial, is_subtype_attr, NO_WRAP);
      raw("\n");
    }

    if( target_typebody->upper || target_typebody->precision) {
      indent_swift(level2);
      if( is_subtype_attr ){
        raw("guard ");
      }
      raw("let SELF = ");

      if( VARis_redeclaring(attr) ){
        if( is_subtype_attr ){
          superEntityReference_swift(entity, attr->defined_in, is_subtype_attr, YES_WRAP);
          raw("?");
          force_wrap();
          raw(".");
        }
        superEntityReference_swift(entity, original_partial, false, YES_WRAP);
      }
      else {
        superEntityReference_swift(entity, original_partial, is_subtype_attr, YES_WRAP);
      }

      if( is_subtype_attr ){
        force_wrap();
        raw(" else { return }");
      }

      raw("; SDAI.TOUCH(let:SELF)\n");
    }

    indent_swift(level2);
    wrap("partial.%s = ",
         partialAttrName
         );

    if( TYPEis_bounded(target_attr->type) ) {
      if( !target_optional ){
        raw("SDAI.UNWRAP(");
      }

      emit_typeReference_swift(entity, target_attr->type, WO_COMMENT);
      raw("(");

      force_wrap();
      if( TYPEis_aggregation_data_type(target_attr->type) ){
        emit_aggregateBoundSpec(target_attr->type, original_partial, EMIT_SELF, ", ");
      }
      else {// TYPEis_string(target_attr->type) || TYPEis_binary(target_attr->type)
        emit_widthSpec_asRequired("", target_attr->type, entity, NOT_FOR_UNDERLYING, EMIT_SELF, ", ");
      }

      force_wrap();
      raw("newValue");

      raw( ")" );

      if( !target_optional ){
        raw(")");
      }
      raw("\n");
    }
    else {
      if( !target_optional ){
        if( TYPEis_logical(VARget_type(target_attr)) ){
          raw("SDAI.LOGICAL(");
        }
        else {
          raw("SDAI.UNWRAP(");
        }
      }

      if( VARis_redeclaring(attr) ){
        emit_typeReference_swift(entity, target_attr->type, WO_COMMENT);
        raw("(fromGeneric: newValue)" );
      }
      else {
        raw("newValue");
      }

      if( !target_optional ){
        raw(")");
      }
      raw("\n");
    }
  }
  indent_swift(level);
  raw("}\n");
}

static void explicitStaticSetter_swift
 (Variable attr, bool is_subtype_attr, Entity entity, int level, Entity original_partial, char *partialAttr)
{
	/*
	 set(newValue) {
	   super_<PARTIAL>.partialEntity.<partialAttr> = <ORIGINAL TYPE>(newValue)
	 }
	 
	 */

  char partialAttrName[BUFSIZ];
  partialEntityAttribute_swiftName(attr, partialAttrName);

  bool check_dynamic_provider = false;

  explicitStaticSetterCore(entity, original_partial,
                           attr, is_subtype_attr, partialAttrName,
                           check_dynamic_provider, "NA",
                           level
                           );

}

static void explicitStaticGetterCore
(
 Entity entity,
 Entity original_partial,
 Variable attr,
 TypeBody attr_typebody,
 bool attr_optional,
 bool is_subtype_attr,
 Variable original_attr,
 bool original_attr_optional,
 const char *partialAttrName,
 int level
 )
{
  indent_swift(level);
  raw("let originalValue = " );

  if( VARis_redeclaring(attr) ){
    if( is_subtype_attr ){
      superEntityReference_swift(entity, attr->defined_in, is_subtype_attr, YES_WRAP);
      raw("?");
      force_wrap();
      raw(".");
    }
    partialEntityReference_swift(entity, original_partial, false, YES_WRAP);
  }
  else {
    partialEntityReference_swift(entity, original_partial, is_subtype_attr, YES_WRAP);
  }
  wrap(".%s\n",
       partialAttrName
       );

  if( VARis_redeclaring(attr) ){
    if( TYPEis_bounded(attr->type) ) {
      if( attr_typebody->upper || attr_typebody->precision ) {
        indent_swift(level);
        if( is_subtype_attr ){
          raw("guard ");
        }
        raw("let SELF = ");

        if( VARis_redeclaring(attr) ){
          if( is_subtype_attr ){
            superEntityReference_swift(entity, attr->defined_in, is_subtype_attr, YES_WRAP);
            raw("?");
            force_wrap();
            raw(".");
          }
          superEntityReference_swift(entity, original_partial, false, YES_WRAP);
        }
        else {
          superEntityReference_swift(entity, original_partial, is_subtype_attr, YES_WRAP);
        }

        if( is_subtype_attr ){
          force_wrap();
          if( !attr_optional ){
            wrap("else { return SDAI.UNWRAP(nil) }");
          }
          else {
            raw("else { return nil }");
          }
        }

        raw("; SDAI.TOUCH(let:SELF)\n");
      }

      indent_swift(level);
      wrap("let value = " );

      emit_typeReference_swift(entity, attr->type, WO_COMMENT);
      raw("(");

      if( TYPEis_aggregation_data_type(attr->type) ){
        emit_aggregateBoundSpec(attr->type, original_partial, EMIT_SELF, ", ");
      }
      else {// TYPEis_string(attr->type) || TYPEis_binary(attr->type)
        emit_widthSpec_asRequired("", attr->type, entity, NOT_FOR_UNDERLYING, EMIT_SELF, ", ");
      }

      force_wrap();
      raw("originalValue");

      raw( ")\n" );

      indent_swift(level);
      if( !attr_optional ){
        wrap("return SDAI.UNWRAP(value)\n");
      }
      else {
        raw("return value\n");
      }
      return;
    }
  }
  indent_swift(level);
  wrap("let value = " );
  if( !attr_optional ){
    if( TYPEis_logical(VARget_type(attr)) ){
      raw("SDAI.LOGICAL( ");
    }
    else {
      wrap("SDAI.UNWRAP( ");
    }
  }

  emit_typeReference_swift(entity, attr->type, WO_COMMENT);
  raw("( originalValue )" );

  if( !attr_optional ){
    raw(" )");
  }
  raw("\n");

  indent_swift(level);
  raw("return value\n");
}

static void explicitStaticGetter_swift
 (Variable attr, bool is_subtype_attr, Entity entity, int level, Entity original_partial, char *partialAttr)
{
	/*
	 get {
		 return self.partialEntity.<partialAttr>
	 }
	// or //	 
	 get {
		 return <ATTR TYPE>(super_<PARTIAL>.partialEntity.<partialAttr>)
	 }
	 */

  char partialAttrName[BUFSIZ];
  partialEntityAttribute_swiftName(attr, partialAttrName);

  TypeBody attr_typebody = TYPEget_body(attr->type);
  bool attr_optional = ( is_subtype_attr || VARis_optional_by_large(attr) );

	indent_swift(level);
	raw("get {\n");
	{	int level2 = level+nestingIndent_swift;

    explicitStaticGetterCore(entity, original_partial,
                             attr, attr_typebody, attr_optional, is_subtype_attr,
                             attr, attr_optional, partialAttrName,
                             level2
                             );

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
	explicitStaticSetter_swift(attr, is_subtype_attr, entity, level, partial, partialAttr);
}



//MARK: - explicit dynamic attribute access
static void explicitDynamicGetterSetter_swift
 (Variable attr,
  bool is_subtype_attr,
  Entity entity,
  int level,
  Entity original_partial,
  Variable original_attr)
{
	/*
	 get {
		 if let resolved = <original>.<attr>__provider(complex: self.complexEntity) {
       let originalValue = resolved._<original>__getter(complex: self.complexEntity)

			 let value = <ATTR TYPE>(originalValue)
	 // or //
	     let value = originalValue

	 		 return value
		 }
		 else {
       let originalValue = self.partialEntity._<attr>

       let value = <ATTR TYPE>(originalValue)
       // or //
       let value = originalValue

			 return value
		 }
	 }
	 set(newValue) {
	  if let _ = <original>.<attr>__provider(complex: self.complexEntity) { return }
	 
	 	// same as explicit static attribute setter //
	 }
	 
	 */
	
	int level2 = level+nestingIndent_swift;
  int level3 = level2+nestingIndent_swift;

	char partialAttrName[BUFSIZ];
	partialEntityAttribute_swiftName(attr, partialAttrName);
	
	char originalPartialEntityName[BUFSIZ];
	partialEntity_swiftName(original_attr->defined_in, entity->superscope, SWIFT_QUALIFIER, originalPartialEntityName);

  TypeBody attr_typebody = TYPEget_body(attr->type);
  bool attr_optional = ( is_subtype_attr || VARis_optional_by_large(attr) );

  // getter
  indent_swift(level);
  raw("get {\n");
  {
		indent_swift(level2);
		raw("if let resolved = %s.%s__provider(complex: self.complexEntity) {\n",
				originalPartialEntityName,
				partialAttrName);

		{
      indent_swift(level3);
      wrap("let originalValue = resolved" );
      wrap(".%s__getter(",
           partialAttrName
           );
      wrap("complex: self.complexEntity)\n" );


      if( VARis_redeclaring(attr) ){
        if( TYPEis_bounded(attr->type) ) {

          if( attr_typebody->upper || attr_typebody->precision ) {
            indent_swift(level3);
            if( is_subtype_attr ){
              raw("guard ");
            }
            raw("let SELF = ");

            if( VARis_redeclaring(attr) ){
              if( is_subtype_attr ){
                superEntityReference_swift(entity, attr->defined_in, is_subtype_attr, YES_WRAP);
                raw("?");
                force_wrap();
                raw(".");
              }
              superEntityReference_swift(entity, original_partial, false, YES_WRAP);
            }
            else {
              superEntityReference_swift(entity, original_partial, is_subtype_attr, YES_WRAP);
            }

            if( is_subtype_attr ){
              force_wrap();
              if( !attr_optional ){
                wrap("else { return SDAI.UNWRAP(nil) }");
              }
              else {
                raw("else { return nil }");
              }
            }

            raw("; SDAI.TOUCH(let:SELF)\n");
          }

          indent_swift(level3);
          wrap("let value = " );

          emit_typeReference_swift(entity, attr->type, WO_COMMENT);
          raw("(");

          if( TYPEis_aggregation_data_type(attr->type) ){
            emit_aggregateBoundSpec(attr->type, original_partial, EMIT_SELF, ", ");
          }
          else {// TYPEis_string(attr->type) || TYPEis_binary(attr->type)
            emit_widthSpec_asRequired("", attr->type, entity, NOT_FOR_UNDERLYING, EMIT_SELF, ", ");
          }

          force_wrap();
          raw("originalValue");

          raw( ")\n" );

          indent_swift(level3);
          if( !attr_optional ){
            raw("return SDAI.UNWRAP(value)\n");
          }
          else {
            raw("return value\n");
          }
        }
        else {
          indent_swift(level3);
          wrap("let value = " );
          if( !attr_optional ){
            if( TYPEis_logical(VARget_type(attr)) ){
              raw("SDAI.LOGICAL( ");
            }
            else {
              wrap("SDAI.UNWRAP( ");
            }
          }

          emit_typeReference_swift(entity, attr->type, WO_COMMENT);
          raw("( originalValue )");

          if( !attr_optional ){
            raw(" )");
          }
          raw("\n");
          
          indent_swift(level3);
          raw("return value\n");
        }
			}
      else {
        indent_swift(level3);
        wrap("return originalValue\n");
      }
		}
		indent_swift(level2);
		wrap("}\n");


		indent_swift(level2);
		wrap("else {\n");
    explicitStaticGetterCore(entity, original_partial,
              attr, attr_typebody, attr_optional, is_subtype_attr,
              original_attr, YES_OPTIONAL_ATTR, partialAttrName,
              level3
              );
		indent_swift(level2);
		wrap("}\n");
	}
	indent_swift(level);
	raw("}\n");
	
	// setter
  bool check_dynamic_provider = true;

  explicitStaticSetterCore(entity, original_partial,
            attr, is_subtype_attr, partialAttrName,
            check_dynamic_provider, originalPartialEntityName,
            level
            );

}


static void explicitAttributeReference_swift(Schema schema, Entity entity, Variable attr, bool is_subtype_attr, int level) {
	Entity partial = attr->defined_in;
	
	if( VARis_dynamic(attr) ) {
		attributeRefHead_swift( schema, entity, "public", attr, is_subtype_attr, level,
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
		attributeRefHead_swift( schema, entity, "public", attr, is_subtype_attr, level,
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
	 if let cached = cachedValue(derivedAttributeName:"<attr>") {
	  return cached.value as! <attr_type>
	 }
		 let value = self.partialEntity._<attr>__getter(SELF: self.pRef)
	   updateCache(derivedAttributeName:"<attr>", value:value)
		 return value
	 }
	 */
	
	char partialAttr[BUFSIZ];
	
	// getter
	indent_swift(level);
	raw("get {\n");
	{	int level2 = level+nestingIndent_swift;
		int level3 = level2+nestingIndent_swift;
		
		char attrname[BUFSIZ];
		canonical_swiftName(ATTRget_name_string(attr), attrname);
		
		// check cashed value
		indent_swift(level2);
		raw("if let cached = cachedValue(derivedAttributeName:\"%s\") {\n",
				attrname
				);
		indent_swift(level3);
		raw("return cached.value as! ");
		variableType_swift(entity, attr, (is_subtype_attr ? YES_FORCE_OPTIONAL : NO_FORCE_OPTIONAL), NOT_IN_COMMENT);
		raw("\n");
		indent_swift(level2);
		raw("}\n");
		
		// get derived value
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
		raw("let value = ");

		if( !is_subtype_attr && !VARis_optional_by_large(attr) ){
      if( TYPEis_logical(VARget_type(attr)) ){
        raw("SDAI.LOGICAL( ");
      }
      else {
        raw("SDAI.UNWRAP( ");
      }
    }

		if( entity == partial ){
			wrap("origin.partialEntity.%s__getter(SELF: origin.pRef",
					 partialEntityAttribute_swiftName(attr, partialAttr)
					 );
			raw(")");
		}
		else {
			emit_typeReference_swift(entity, attr->type, WO_COMMENT);
			raw("(");
			wrap("origin.partialEntity.%s__getter(SELF: origin.pRef",
					 partialEntityAttribute_swiftName(attr, partialAttr)
					 );
			raw("))");
		}

		if( !is_subtype_attr && !VARis_optional_by_large(attr) ){
			raw(" )");
		}

		raw("\n");
		
		// save as cached value
		indent_swift(level2);
		raw("updateCache(derivedAttributeName:\"%s\", value:value)\n",
				attrname
				);

		indent_swift(level2);
		raw("return value\n");
	}
	indent_swift(level);
	raw("}\n");
}

//MARK: explicit attribute redef (type cast)
static void explicitAttributeOverrideReference_swift(Schema schema, Entity entity, Variable attr, bool is_subtype_attr, int level) {
	Variable original_attr = VARget_original_attr(attr);
	Entity original_partial = original_attr->defined_in;

	if( VARis_dynamic(original_attr) ) {
		attributeRefHead_swift( schema, entity, "public", attr, is_subtype_attr, level,
													 (VARis_observed(attr)
														? "EXPLICIT REDEF(DYNAMIC)(OBSERVED)" 
														: "EXPLICIT REDEF(DYNAMIC)") );
		{	int level2 = level+nestingIndent_swift;
			
			explicitDynamicGetterSetter_swift(attr, is_subtype_attr, entity, level2, original_partial, original_attr);
		}
		indent_swift(level);
		raw("}\n");
	}
	else {
		attributeRefHead_swift( schema, entity, "public", attr, is_subtype_attr, level,
													 (VARis_observed(attr)
														? "EXPLICIT REDEF(OBSERVED)" 
														: "EXPLICIT REDEF") );
		{	int level2 = level+nestingIndent_swift;
			
			explicitStaticGetterSetter_swift(attr, is_subtype_attr, entity, level2, original_partial);		
		}
		indent_swift(level);
		raw("}\n");
	}
	
}

//MARK: derived attribute
static void derivedAttributeReference_swift(Schema schema, Entity entity, Variable attr, bool is_subtype_attr, int level) {
	Entity partial = attr->defined_in;
	
	attributeRefHead_swift( schema, entity, "public", attr, is_subtype_attr, level, "DERIVE" );
	{	int level2 = level+nestingIndent_swift;
		
		derivedGetter_swift(attr, is_subtype_attr, entity, level2, partial);
	}
	indent_swift(level);
	raw("}\n");
}

//MARK: derived attribute redef
static void derivedAttributeOverrideReference_swift(Schema schema, Entity entity, Variable attr, bool is_subtype_attr, int level) {
	Entity partial = attr->defined_in;
	
	assert(VARis_dynamic(attr));
	attributeRefHead_swift( schema, entity, "public", attr, is_subtype_attr, level, "EXPLICIT REDEF(DERIVE)" );
	{	int level2 = level+nestingIndent_swift;

		derivedGetter_swift(attr, is_subtype_attr, entity, level2, partial);
	}
	indent_swift(level);
	raw("}\n");
}

//MARK: - inverse attribute access
static void inverseAggregateAttributeGetter_swift(Entity entity, Variable attr, bool is_subtype_attr, int level) {
	/*
	 get {
		 if let cached = cachedValue(derivedAttributeName:"<attr>") {
			 return cached.value as! <attr_type>
		 }
     let keypath = \<source>.<attr>
     let entities = self.referencingEntities(for: keypath)
		 let swiftValue = <attr_type>.SwiftType(entities)
		 let value = <attr_type>(from: swiftValue, bound1: <I1>, bound2: <I2?>)
		 updateCache(derivedAttributeName:"<attr>", value:value)
		 return value
	 }
	 */

//	char partialAttr[BUFSIZ];

	// getter
	indent_swift(level);
	raw("get {\n");
	{	int level2 = level+nestingIndent_swift;
		int level3 = level2+nestingIndent_swift;

		char attrname[BUFSIZ];
		canonical_swiftName(ATTRget_name_string(attr), attrname);

		// check cashed value
		/*
		 if let cached = cachedValue(derivedAttributeName:"<attr>") {
		 return cached.value as! <attr_type>
		 }
		 */
		indent_swift(level2);
		raw("if let cached = cachedValue(derivedAttributeName:\"%s\") {\n",
				attrname
				);
		indent_swift(level3);
		raw("return cached.value as! ");
		variableType_swift(entity, attr, (is_subtype_attr ? YES_FORCE_OPTIONAL : NO_FORCE_OPTIONAL), NOT_IN_COMMENT);
		raw("\n");
		indent_swift(level2);
		raw("}\n");

		// get inverse reference value
		/*
     let keypath = \<source>.<attr>
     let entities = self.referencingEntities(for: keypath)
     let swiftValue = <attr_type>.SwiftType(entities)
		 */
		char buf[BUFSIZ];
		Variable observing_attr = VARget_inverse(attr);
		Type attr_element_type = TYPEget_body(attr->type)->base;
		Entity source = TYPEget_body(attr_element_type)->entity; //observing_attr->defined_in;

    indent_swift(level2);
    raw("let keypath = ");
     raw("\\%s",
        ENTITY_swiftName(source, NO_QUALIFICATION, SWIFT_QUALIFIER, buf)
        );
    raw(".%s\n",
        attribute_swiftName(observing_attr, buf)
        );

    indent_swift(level2);
    raw("let entities = ");
    raw("self.referencingEntities(for: keypath)\n");

		indent_swift(level2);
		raw("let swiftValue = ");
		emit_typeReference_swift(entity, attr->type, NOT_IN_COMMENT);
		raw(".SwiftType( entities )\n");

		/*
		 let value = <attr_type>(from: swiftValue, bound1: <I1>, bound2: <I2?>)
		 */

		indent_swift(level2);
		raw("let value = ");
		emit_typeReference_swift(entity, attr->type, NOT_IN_COMMENT);
    raw("(from: swiftValue, ");
    emit_aggregateBoundSpec(attr->type, entity, EMIT_SELF, "");
		raw(")\n");


		// save as cached value
		/*
		 updateCache(derivedAttributeName:"<attr>", value:value)
		 */
		indent_swift(level2);
		raw("updateCache(derivedAttributeName:\"%s\", value:value)\n",
				attrname
				);

		indent_swift(level2);
		raw("return value\n");
	}
	indent_swift(level);
	raw("}\n");


}

static void inverseSimpleAttributeGetter_swift(Entity entity, Variable attr, bool is_subtype_attr, int level) {
	/*
	 get {
		 if let cached = cachedValue(derivedAttributeName:"<attr>") {
			 return cached.value as! <attr_type>
		 }
		 let value = self.referencingEntity(for: \<source>.<attr>)
		 updateCache(derivedAttributeName:"<attr>", value:value)
		 return value
	 }
	 */

//	char partialAttr[BUFSIZ];

	// getter
	indent_swift(level);
	raw("get {\n");
	{	int level2 = level+nestingIndent_swift;
		int level3 = level2+nestingIndent_swift;

		char attrname[BUFSIZ];
		canonical_swiftName(ATTRget_name_string(attr), attrname);

		// check cashed value
		/*
		 if let cached = cachedValue(derivedAttributeName:"<attr>") {
		 return cached.value as! <attr_type>
		 }
		 */
		indent_swift(level2);
		raw("if let cached = cachedValue(derivedAttributeName:\"%s\") {\n",
				attrname
				);
		indent_swift(level3);
		raw("return cached.value as! ");
		variableType_swift(entity, attr, (is_subtype_attr ? YES_FORCE_OPTIONAL : NO_FORCE_OPTIONAL), NOT_IN_COMMENT);
		raw("\n");
		indent_swift(level2);
		raw("}\n");

		// get inverse reference value
		/*
		 let value = self.referencingEntity(for: \<source>.<attr>)
		 */
		char buf[BUFSIZ];
		Variable observing_attr = VARget_inverse(attr);
		Entity source = observing_attr->defined_in;

		indent_swift(level2);
		raw("let value = self.referencingEntity(for: \\%s",
				ENTITY_swiftName(source, NO_QUALIFICATION, SWIFT_QUALIFIER, buf)
				);
		raw(".%s)\n",
				attribute_swiftName(observing_attr, buf)
				);

		// save as cached value
		/*
		 updateCache(derivedAttributeName:"<attr>", value:value)
		 */
		indent_swift(level2);
		raw("updateCache(derivedAttributeName:\"%s\", value:value)\n",
				attrname
				);

		indent_swift(level2);
		raw("return value\n");
	}
	indent_swift(level);
	raw("}\n");

}


static void inverseAttributeReference_swift(Schema schema, Entity entity, Variable attr, bool is_subtype_attr, int level) {

	attributeRefHead_swift( schema, entity, "public", attr, is_subtype_attr, level, "INVERSE" );

	{
		// getter
    Entity definedIn = attr->defined_in;
    if( entity != definedIn ) {
      /*
       get {
       let value = <SUPER>?.<attr>
       return value
       }
       */

      indent_swift(level);
      raw("get {\n");
      { int level2 = level+nestingIndent_swift;

        char buf[BUFSIZ];
        indent_swift(level2);
        raw("let value = ");
        superEntityReference_swift(entity, definedIn, is_subtype_attr, YES_WRAP);
        if(is_subtype_attr) raw("?");
        wrap(".");
        raw("%s\n",
            attribute_swiftName(attr, buf)
            );
        indent_swift(level2);
        raw("return value\n");
      }
      indent_swift(level);
      raw("}\n");
    }
    else {
      if( TYPEis_aggregation_data_type(attr->type) ){
        inverseAggregateAttributeGetter_swift(entity, attr, is_subtype_attr, level);
      }
      else{
        inverseSimpleAttributeGetter_swift(entity, attr, is_subtype_attr, level);
      }
    }
	}
	
	indent_swift(level);
	raw("}\n");
}

//MARK: - attribute access main entry point
static void entityAttributeReferences_swift( Schema schema, Entity entity, Linked_List effective_attrs, int level ){
	raw("\n");
	indent_swift(level);
	raw("//MARK: ATTRIBUTES\n");
	
	Dictionary all_attrs = ENTITYget_all_attributes(entity);
	DictionaryEntry de;
	Linked_List attr_defs;

	DICTdo_init( all_attrs, &de );
	while( 0 != ( attr_defs = ( Linked_List )DICTdo( &de ) ) ) {
		Variable attr = LISTget_first(attr_defs);
		char* attrName = VARget_simple_name(attr);
		bool is_subtype_attr = LISTget_first_aux(attr_defs);
		
		if( ENTITYget_attr_ambiguous_count(entity, attrName) > 1 ) {
			if( attr->defined_in != entity ){
				indent_swift(level);
				char buf[BUFSIZ];
				wrap("// %s: (%d AMBIGUOUS REFs)\n\n",
						 as_attributeSwiftName_n(attrName, buf, BUFSIZ),
						 LISTget_length(attr_defs)
						 );
				continue;
			}
		}
		
		LISTadd_last_marking_aux(effective_attrs, attr, (Generic)is_subtype_attr);
	}
	
	LISTdo(effective_attrs, attr, Variable) {
		bool is_subtype_attr = LIST_aux;
		if( VARis_redeclaring(attr) ) {
			if( VARis_derived(attr) ) {
				derivedAttributeOverrideReference_swift(schema, entity, attr, is_subtype_attr, level);
			}
			else {
				explicitAttributeOverrideReference_swift(schema, entity, attr, is_subtype_attr, level);
			}
		}
		else {
			if( VARis_derived(attr) ) {
				derivedAttributeReference_swift(schema, entity, attr, is_subtype_attr, level);
			}
			else if( VARis_inverse(attr) ){
				inverseAttributeReference_swift(schema, entity, attr, is_subtype_attr, level);
			}
			else {	// explicit
				explicitAttributeReference_swift(schema, entity, attr, is_subtype_attr, level);
			}
		}
		raw("\n");
	}LISTod;
}

//MARK: - initializer
static void entityReferenceInitializers_swift( Entity entity, int level ){
	raw("\n");
	indent_swift(level);
	raw("//MARK: INITIALIZERS\n");
	
	/*
	 public convenience init?(_ entityRef: SDAI.EntityReference?) {
		 let complex = entityRef?.complexEntity
		 self.init(complex: complex)
	 }
	 */
	indent_swift(level);
	raw("/// initialize from entity reference (type conversion form)\n");
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
	raw("/// initialize from complex entity\n");
	indent_swift(level);
	raw( "public required init?(complex complexEntity: SDAI.ComplexEntity?) {\n");
	{	int level2=level+nestingIndent_swift;
		char buf[BUFSIZ];
		
		indent_swift(level2);
		raw( "guard let partial = complexEntity?.partialEntityInstance(%s.self) else { return nil }\n", 
				partialEntity_swiftName(entity, entity->superscope, SWIFT_QUALIFIER, buf)
				);
		indent_swift(level2);
		raw( "self.partialEntity = partial\n\n");
		
		// supertype references
		int entity_index = 0;
		LISTdo( ENTITYget_super_entity_list(entity), super, Entity ) {
			if( super == entity )continue;
			++entity_index;
			indent_swift(level2);
			wrap("guard let super%d = complexEntity?.entityReference(%s.self) else { return nil }\n", 
					 entity_index, ENTITY_swiftName(super, entity, SWIFT_QUALIFIER, buf)
					 );

			indent_swift(level2);
			wrap("self.%s%s = super%d\n\n", 
					 superEntity_swiftPrefix,
					 ENTITY_swiftName(super, NO_QUALIFICATION, SWIFT_QUALIFIER, buf),
					 entity_index
					 );
		}LISTod;


		indent_swift(level2);
		raw( "super.init(complex: complexEntity)\n");
	}
	indent_swift(level);
	raw( "}\n\n");
	
	/*
	 public required convenience init?<G: SDAI.GenericType>(fromGeneric generic: G?) {
		 guard let entityRef = generic?.entityReference else { return nil }
		 self.init(complex: entityRef.complexEntity)
	 }
	 */
	indent_swift(level);
	raw("/// initialize from SDAI generic type\n");
	indent_swift(level);
	raw( "public required convenience init?<G: SDAI.GenericType>(fromGeneric generic: G?) {\n");
	{	int level2=level+nestingIndent_swift;
		
		indent_swift(level2);
		raw( "guard let entityRef = generic?.entityReference else { return nil }\n" );
		indent_swift(level2);
		raw( "self.init(complex: entityRef.complexEntity)\n");
	}
	indent_swift(level);
	raw( "}\n\n");
	
	/*
	 public convenience init?<S: SDAI.SelectType>(_ select: S?) { self.init(possiblyFrom: select) }
	 public convenience init?(_ complex: SDAI.ComplexEntity?) { self.init(complex: complex) }
	 */
	indent_swift(level);
	raw("/// initialize from SDAI select type (type conversion form)\n");
	indent_swift(level);
	raw( "public convenience init?<S: SDAI.SelectType>(_ select: S?) { self.init(possiblyFrom: select) }\n");


	indent_swift(level);
	raw("/// initialize from complex entity (type conversion form)\n");
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
		raw("/// initialize simple entity reference from single partial entity\n");
		indent_swift(level);
		raw("public convenience init?(_ partial:%s) {\n",
				partialEntity_swiftName(entity,entity, SWIFT_QUALIFIER, buf)
				);

		{	int level2 = level+nestingIndent_swift;
			indent_swift(level2);
			raw("let complex = SDAI.ComplexEntity(entities:[partial])\n");

			indent_swift(level2);
			raw("self.init(complex: complex)\n");
		}
		
		indent_swift(level);
		raw("}\n\n");	
	}

  if( ENTITY_is_initializable_by_void(entity) ) {
  /*
   /// Initializable.ByVoid conformance
   public convenience init() {
    let partials = [
      <partial#0>(),
      <partial#1>(),
      <partial#2>(),
      ...
    ]
    let complex = SDAI.ComplexEntity(entities: partials)
    self.init(complex: complex)!
   }
   */

    indent_swift(level);
    raw("/// Initializable.ByVoid conformance\n");
    indent_swift(level);
    raw("public convenience init() {\n");

    {  int level2 = level+nestingIndent_swift;
      indent_swift(level2);
      raw("let partials = [\n");

      Linked_List supertypes = ENTITYget_super_entity_list(entity);

      LISTdo( supertypes, super, Entity ) {
        int level3 = level2+nestingIndent_swift;

        char partial[BUFSIZ]; partialEntity_swiftName(super,super->superscope, SWIFT_QUALIFIER, partial);
        indent_swift(level3);
        raw("%s(),\n",
            partial
            );
      }LISTod;

      indent_swift(level2);
      raw("]\n");


      indent_swift(level2);
      raw("let complex = SDAI.ComplexEntity(entities: partials)\n");

      indent_swift(level2);
      raw("self.init(complex: complex)!\n");
    }

    indent_swift(level);
    raw("}\n\n");
  }


}

//MARK: - SDAI Dictionary Schema support
static int partialEntityExplicitAttributeCount( Entity entity) {
	Linked_List local_attributes = entity->u.entity->attributes;
	int count = 0;
	LISTdo( local_attributes, attr, Variable ){
		if( VARis_redeclaring(attr) ) continue;
		if( VARis_derived(attr) ) continue;
		if( VARis_inverse(attr) ) continue;
		++count;
	}LISTod;
	return count;
}

static void dictEntityDefinition_swift( Entity entity, Linked_List effective_attrs, int level ) {
	raw("\n");
	indent_swift(level);
	raw("//MARK: DICTIONARY DEFINITION\n");

	indent_swift(level);
	raw("public class override var entityDefinition: SDAIDictionarySchema.EntityDefinition { _entityDefinition }\n");

	indent_swift(level);
	raw("private static let _entityDefinition: SDAIDictionarySchema.EntityDefinition = createEntityDefinition()\n\n");

	indent_swift(level);
	raw("private static func createEntityDefinition() -> SDAIDictionarySchema.EntityDefinition {\n");
	
	{	int level2 = level+nestingIndent_swift;
		int level3 = level2+nestingIndent_swift;
		char buf[BUFSIZ];
		indent_swift(level2);
		raw("let entityDef = SDAIDictionarySchema.EntityDefinition.Prototype( \n");
		indent_swift(level3);
		raw("name: \"%s\", \n",
				ENTITY_canonicalName(entity, buf)
				);
		indent_swift(level3);
		raw("type: self, explicitAttributeCount: %d)\n",
				partialEntityExplicitAttributeCount(entity)
				);

		
		raw("\n");
		indent_swift(level2);
		raw("//MARK: SUPERTYPE REGISTRATIONS\n");		
		LISTdo(ENTITYget_super_entity_list(entity), supertype, Entity) {
			indent_swift(level2);
			raw("entityDef.add(supertype: %s.self)\n", 
					ENTITY_swiftName(supertype, NO_QUALIFICATION, SWIFT_QUALIFIER, buf)
					);
		}LISTod;
		
		
		if( !LISTis_empty(effective_attrs) ) {
			raw("\n");
			indent_swift(level2);
			raw("//MARK: ATTRIBUTE REGISTRATIONS\n");		
			LISTdo(effective_attrs, attr, Variable) {
				bool is_thistype_attr = attr->defined_in == entity;
				bool is_subtype_attr = LIST_aux;
				bool is_redeclaring = VARis_redeclaring(attr);
				bool is_derived = VARis_derived(attr);
				bool is_inverse = VARis_inverse(attr);
				bool is_optional = VARis_optional(attr);
				bool may_yield_entity_ref = TYPE_may_yield_entity_reference(VARget_type(attr));
				
				indent_swift(level2);
				raw("entityDef.addAttribute(name: \"%s\", \n",
						canonical_swiftName(ATTRget_name_string(attr), buf)
						);

				indent_swift(level3);
				raw("keyPath: \\%s",
						ENTITY_swiftName(entity, NO_QUALIFICATION, SWIFT_QUALIFIER, buf)
						);
				raw(".%s, \n",
						attribute_swiftName(attr, buf)
						);

				indent_swift(level3);
				raw("kind: .");
				if( is_inverse ){
					raw("inverse");
				}
				else if( is_derived ){
					raw("derived");
					if( is_redeclaring ) raw("Redeclaring");
				}
				else{
					raw("explicit");
					if( is_optional ) raw("Optional");
					if( is_redeclaring ) raw("Redeclaring");
				}
				raw(", ");
				
				raw("source: .");
				if( is_thistype_attr ){
					raw("thisEntity");
				}
				else if( is_subtype_attr ){
					raw("subEntity");
				}
				else {
					raw("superEntity");
				}
				raw(", ");

				raw("mayYieldEntityReference: ");
				if( may_yield_entity_ref ){
					raw("true");
				}
				else {
					raw("false");
				}
				raw(")\n");
			}LISTod;
		}

		
		Linked_List unique_rules = ENTITYget_uniqueness_list(entity);
		if( !LISTis_empty(unique_rules)) {
			raw("\n");
			indent_swift(level2);
			raw("//MARK: UNIQUENESS RULE REGISTRATIONS\n");
			int serial = 0;
			char partial_name[BUFSIZ];partialEntity_swiftName(entity, entity->superscope, SWIFT_QUALIFIER, partial_name);
			LISTdo(unique_rules, unique, Linked_List) {
				++serial;
				indent_swift(level2);
				raw("entityDef.addUniquenessRule(label:\"%s\", rule: %s.%s)\n",
						uniqueRuleLabel_swiftName(serial, unique, buf),
						partial_name,
						buf
						);
			}LISTod;
		}
		
		raw("\n");
		indent_swift(level2);
		raw("return entityDef.freeze()\n");
	}
	
	indent_swift(level);
	raw("}\n\n");
} 

//MARK: - where rule validation support
static void entityWhereRuleValidation_swift( Entity entity, int level ) {
	Linked_List where_rules = TYPEget_where(entity);
	if( LISTis_empty(where_rules) ) return;

	raw("\n");
	indent_swift(level);
	raw("//MARK: WHERE RULE VALIDATION (ENTITY)\n");

	indent_swift(level);
	raw("public override class func validateWhereRules(instance:SDAI.EntityReference?, prefix:SDAI.WhereLabel) -> SDAI.WhereRuleValidationRecords {\n");

	{	int level2 = level+nestingIndent_swift;

		indent_swift(level2);
		raw("guard let instance = instance as? Self else { return [:] }\n\n");
				
		indent_swift(level2);
		raw("var result = super.validateWhereRules(instance:instance, prefix:prefix)\n");
		
		char partial[BUFSIZ];partialEntity_swiftName(entity, entity->superscope, SWIFT_QUALIFIER, partial);
		LISTdo( where_rules, where, Where ){
			char whereLabel[BUFSIZ];
			indent_swift(level2);
			raw("result[prefix + \" .%s\"] = %s.%s(SELF: instance.pRef)\n",
					whereRuleLabel_swiftName(where, whereLabel),
					partial,
					whereLabel
					);
		}LISTod;

		indent_swift(level2);
		raw("return result\n");		
	}
	
	indent_swift(level);
	raw("}\n\n");
}


//MARK: - entity reference definition main entry point
void entityReferenceDefinition_swift
 ( Schema schema,
  Entity entity,
  int level,
  Linked_List attr_overrides,
  Linked_List dynamic_attrs )
{
	raw("\n\n");
	raw("//MARK: - Entity Reference\n");
	
	// markdown
	raw("\n/** ENTITY reference\n");

	raw("- EXPRESS source code:\n");
	raw("```express\n");
	ENTITY_out(entity, level);
	raw("\n```\n");

	ENTITY_supersub_out(schema, entity);
	ENTITY_partial_out(schema, entity);

	raw("*/\n");
	
	indent_swift(level);
	{
		char buf[BUFSIZ];
		wrap("public final class %s : SDAI.EntityReference, SDAI.DualModeReference, ",
				 ENTITY_swiftName(entity, NO_QUALIFICATION, SWIFT_QUALIFIER, buf)
				 );
		if( LISTget_length(ENTITYget_super_entity_list(entity)) == 1 ) {
			wrap("SDAI.SimpleEntityType, ");
		}
    if( ENTITY_is_initializable_by_void(entity) ) {
      wrap("SDAI.Initializable.ByVoid, ");
    }
		wrap("@unchecked Sendable {\n");
	}
	
	{	int level2 = level+nestingIndent_swift;

		{
			char buf[BUFSIZ];
			indent_swift(level2);
			raw("/// persistent entity reference type\n");
			indent_swift(level2);
			wrap("public typealias PRef = SDAI.PersistentEntityReference<%s>\n\n",
					 ENTITY_swiftName(entity, NO_QUALIFICATION, SWIFT_QUALIFIER, buf)
					 );

			indent_swift(level2);
			raw("/// persistent entity reference\n");
			indent_swift(level2);
			wrap("public var pRef: PRef { PRef(self) }\n\n");
		}

		partialEntityReferenceDefinition_swift(entity, level2);
		supertypeReferenceDefinition_swift(entity, level2);
		
		Linked_List effective_attrs = LISTcreate();
		entityAttributeReferences_swift(schema, entity, effective_attrs, level2);
		entityReferenceInitializers_swift(entity, level2);
		entityWhereRuleValidation_swift(entity, level2);

		dictEntityDefinition_swift(entity, effective_attrs, level2);
		LISTfree(effective_attrs);

    // partial entity definition
    partialEntityDefinition_swift(schema, entity, level2, attr_overrides, dynamic_attrs);
	}
	
	indent_swift(level);
	raw("}\n");
}

