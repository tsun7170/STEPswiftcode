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
char ENTITY_swiftNameInitial( Entity e);
const char* attribute_swiftName( Variable attr );
const char* partialEntity_swiftName( Entity e, char buf[BUFSIZ] );
const char* partialEntityAttribute_swiftName( Variable attr, char buf[BUFSIZ] );
const char* whereLabel_swiftName( Where w, char buf[BUFSIZ] );

void ENTITY_swift( Entity e, int level );
//void ENTITYunique_swift( Linked_List u, int level );
//void ENTITYinverse_swift( Linked_List attrs, int level );
//void ENTITYattrs_swift( Linked_List attrs, int derived, int level );


#endif /* swift_entity_h */
