//
//  swift_scope_const.h
//  exp2swift
//
//  Created by Yoshida on 2020/06/13.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#ifndef swift_scope_const_h
#define swift_scope_const_h

#include <express/linklist.h>
#include <express/scope.h>

//#include "pp.h"

void SCOPEconstList_swift( Scope s, int level );
void SCOPEaddvars_inorder( Linked_List list, Variable v );	// from pretty_scope.c


#endif /* swift_scope_const_h */
