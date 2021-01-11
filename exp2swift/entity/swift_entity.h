//
//  swift_entity.h
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#ifndef swift_entity_h
#define swift_entity_h

#include "../express/entity.h"
#include "../express/expbasic.h"
#include "../express/express.h"

extern const char* ENTITY_swiftName( Entity e, Scope current, char buf[BUFSIZ] );
extern const char* as_entitySwiftName_n( const char* symbol_name, char* buf, int bufsize );
extern char 				ENTITY_swiftNameInitial( Entity e);
extern const char* ENTITY_canonicalName( Entity e, char buf[BUFSIZ] );
extern const char* ENTITY_swiftProtocolName( Entity e, char buf[BUFSIZ]);
extern const char* partialEntity_swiftName( Entity e, char buf[BUFSIZ] );
extern const char* superEntity_swiftPrefix;
extern const char* subEntity_swiftPrefix;

extern const char* attribute_swiftName( Variable attr, char[BUFSIZ] );
extern const char* as_attributeSwiftName_n( const char* symbol_name, char* buf, int bufsize );
extern const char* partialEntityAttribute_swiftName( Variable attr, char buf[BUFSIZ] );
extern const char* dynamicAttribute_swiftProtocolName( Variable original, char buf[BUFSIZ] );
extern bool attribute_need_observer( Variable attr ); 

extern const char* whereRuleLabel_swiftName( Where w, char buf[BUFSIZ] );
extern const char* uniqueRuleLabel_swiftName( int serial, Linked_List unique, char buf[BUFSIZ] );


extern void ENTITY_swift( Entity e, int level, Linked_List dynamic_attrs, Linked_List attr_overrides );
extern void ENTITY_swiftProtocol(Schema schema, Entity e, int level, Linked_List dynamic_attrs );


// swift_entity_partial.c
extern void partialEntityDefinition_swift
 ( Entity entity, int level, Linked_List attr_overrides, Linked_List dynamic_attrs );

extern void explicitDynamicAttributeProtocolDefinition_swift
 (Schema schema, Entity entity, Variable attr, int level);

extern void partialEntityAttrOverrideProtocolConformance_swift
 ( Schema schema, Entity entity, int level, Linked_List attr_overrides );


// swift_entity_reference.c
extern void entityReferenceDefinition_swift( Entity entity, int level );



#endif /* swift_entity_h */
