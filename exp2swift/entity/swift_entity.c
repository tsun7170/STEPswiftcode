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
#include "swift_symbol.h"


const char* ENTITY_swiftName( Entity e, const char* prefix, const char* postfix, Scope current, char buf[BUFSIZ] ) {
	int qual_len = accumulate_qualification(e->superscope, current, buf);
	char buf2[BUFSIZ];
	snprintf(&buf[qual_len], BUFSIZ-qual_len, "%s%s%s", 
					 prefix, canonical_swiftName(e->symbol.name, buf2), postfix);
	return buf;
}

char ENTITY_swiftNameInitial( Entity e) {
	return toupper( e->symbol.name[0] );
}

const char* ENTITY_canonicalName( Entity e, char buf[BUFSIZ] ) {
	return canonical_swiftName(e->symbol.name, buf);
//	char* to = buf;
//	int remain = BUFSIZ-1;
//	for(char* from = e->symbol.name; *from; ++from ) {
//		if( islower(*from) ) {
//			(*to) = toupper(*from);
//		}
//		else {
//			(*to) = (*from);
//		}
//		++to;
//		--remain;
//		if( remain == 0 ) break;
//	}
//	(*to) = 0;
//	return buf;
}

const char* ENTITY_swiftProtocolName( Entity e, char buf[BUFSIZ]) {
	return ENTITY_swiftName(e, "", "_protocol", NO_QUALIFICATION, buf);
}

const char* partialEntity_swiftName( Entity e, char buf[BUFSIZ] ) {
	snprintf(buf, BUFSIZ, "_%s", e->symbol.name);
	return buf;
}

const char* attribute_swiftName( Variable attr, char buf[BUFSIZ] ) {
	return canonical_swiftName(ATTRget_name_string(attr), buf);
}

const char* partialEntityAttribute_swiftName( Variable attr, char buf[BUFSIZ] ) {
	snprintf(buf, BUFSIZ, "_%s", ATTRget_name_string(attr));
	return buf;
}

const char* dynamicAttribute_swiftProtocolName( Variable original, char buf[BUFSIZ] ) {
	char buf2[BUFSIZ];
	char buf3[BUFSIZ];
	snprintf(buf, BUFSIZ, "%s__%s__provider", 
					 ENTITY_swiftName(original->defined_in, "","",NO_QUALIFICATION,buf2), 
					 attribute_swiftName(original,buf3));
	return buf;
}

const char* whereRuleLabel_swiftName( Where w, char buf[BUFSIZ] ) {
	if( w-> label ) {
		snprintf(buf, BUFSIZ, "WHERE_%s", w->label->name );
	}
	else {
		snprintf(buf, BUFSIZ, "WHERE_%d", w->serial );
	}
	return buf;
}

const char* uniqueRuleLabel_swiftName( int serial, Linked_List unique, char buf[BUFSIZ] ) {
	Symbol* label = LISTget_first(unique);
	if( label ) {
		snprintf(buf, BUFSIZ, "UNIQUE_%s", label->name );
	}
	else {
		snprintf(buf, BUFSIZ, "UNIQUE_%d", serial );
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
	if( VARis_optional( v ) ) {
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
				}LISTod
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
				if(VARis_optional(v)) {
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
	++entity_no;
	indent_swift(level);
	if( entity == leaf ) {
		raw("ENTITY(SELF)\t");
	}
	else {
		raw("ENTITY(%d)\t", entity_no);
	}
	raw("%s\n", entity->symbol.name);

	{	int level2 = level+nestingIndent_swift;
		
		if( LISTempty(entity->u.entity->attributes) ) {
			indent_swift(level2);
			raw("(no local attributes)\n");
		}
		LISTdo( entity->u.entity->attributes, attr, Variable ) {
			attribute_out(leaf, attr, level2);
		}LISTod;
	}
	
	raw("\n");
}

static void listAllAttributes( Entity leaf, Entity entity, int level ) {
	Linked_List supertypes = ENTITYget_super_entity_list(leaf);
	
	LISTdo(supertypes, super, Entity ) {
		listLocalAttributes(leaf, super, level);
	} LISTod;
}





//MARK: - main entry point

void ENTITY_swift( Entity entity, int level, 
									Linked_List dynamic_attrs, Linked_List attr_overrides ) {
	// EXPRESS summary
	beginExpress_swift("ENTITY DEFINITION");
	ENTITY_out(entity, level);
	endExpress_swift();	
	
	raw("\n//MARK: - ALL DEFINED ATTRIBUTES\n/*\n");
	entity_no = 0;
	listAllAttributes(entity, entity, level);
	raw("*/\n");
	
	// partial entity definition
	partialEntityDefinition_swift(entity, level, attr_overrides, dynamic_attrs);
	
	// entity reference definition
	entityReferenceDefinition_swift(entity, level);
}


void ENTITY_swiftProtocol(Schema schema, Entity entity, int level, Linked_List dynamic_attrs ) {
	if( LISTempty(dynamic_attrs) ) return;
	
	raw("\n");
	raw("//MARK: - Entity Dynamic Attribute Protocols\n");

	LISTdo(dynamic_attrs, attr, Variable) {
		explicitDynamicAttributeProtocolDefinition_swift(schema, entity, attr, level);
	}LISTod;
}


