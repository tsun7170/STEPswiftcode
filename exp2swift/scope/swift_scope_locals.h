//
//  swift_scope_locals.h
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#ifndef swift_scope_locals_h
#define swift_scope_locals_h

#include <express/linklist.h>
#include <express/scope.h>

void SCOPElocalList_swift( Scope s, int level );

void SCOPElocals_order( Linked_List list, Variable v ); // in pretty_scope.c

#endif /* swift_scope_locals_h */
