//
//  swift_type.h
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#ifndef swift_type_h
#define swift_type_h

#include "../express/type.h"

const char* TYPE_swiftName( Type t );
char TYPE_swiftNameInitial( Type t );
const char* enumCase_swiftName( Expression expr );
const char* selectCase_swiftName( Type selection );

void TYPE_swift( Type t, int level );
void TYPE_head_swift( Type t, int level );
void TYPE_body_swift( Type t, int level );

const char* TYPEhead_string_swift( Type t, char buf[BUFSIZ]);

#endif /* swift_type_h */
