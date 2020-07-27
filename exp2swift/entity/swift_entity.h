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

const char* ENTITY_swiftName( Entity e );
char 				ENTITY_swiftNameInitial( Entity e);
const char* ENTITY_canonicalName( Entity e, char buf[BUFSIZ] );
const char* ENTITY_swiftProtocolName( Entity e, char buf[BUFSIZ]);
const char* partialEntity_swiftName( Entity e, char buf[BUFSIZ] );

const char* attribute_swiftName( Variable attr );
const char* partialEntityAttribute_swiftName( Variable attr, char buf[BUFSIZ] );
const char* dynamicAttribute_swiftProtocolName( Variable original, char buf[BUFSIZ] );

const char* whereRuleLabel_swiftName( Where w, char buf[BUFSIZ] );
const char* uniqueRuleLabel_swiftName( int serial, Linked_List unique, char buf[BUFSIZ] );


void ENTITY_swift( Entity e, int level, Linked_List dynamic_attrs );
void ENTITY_swiftProtocol(Entity e, int level, Linked_List dynamic_attrs );


// swift_entity_partial.c
void partialEntityDefinition_swift
 ( Entity entity, int level, Linked_List attr_overrides, Linked_List dynamic_attrs );
void partialEntityAttrOverrideProtocolConformance_swift
 ( Entity entity, int level, Linked_List attr_overrides );
void explicitDynamicAttributeProtocolDefinition_swift(Entity entity, Variable attr, int level);


// swift_entity_reference.c
void entityReferenceDefinition_swift( Entity entity, int level );



#endif /* swift_entity_h */
