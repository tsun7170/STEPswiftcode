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

#define YES_IN_COMMENT	true
#define NOT_IN_COMMENT	false

#define NO_QUALIFICATION	NULL

int accumulate_qualification(Scope target, Scope current, char buf[BUFSIZ]);

const char* TYPE_swiftName( Type t, Scope current, char buf[BUFSIZ]  );
char TYPE_swiftNameInitial( Type t );
const char* enumCase_swiftName( Expression expr, char buf[BUFSIZ] );
const char* selectCase_swiftName( Type selection, char buf[BUFSIZ] );

void TYPEdefinition_swift( Type t, int level );

void TYPE_head_swift( Scope current, Type t, int level, bool in_comment );
void TYPE_body_swift( Scope current, Type t, int level, bool in_comment );

const char* TYPEhead_string_swift( Scope current, Type t, bool in_comment, char buf[BUFSIZ]);

#endif /* swift_type_h */
