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

void SCOPEconstList_swift( bool nested, Scope s, Linked_List consts, int level );


#endif /* swift_scope_const_h */
