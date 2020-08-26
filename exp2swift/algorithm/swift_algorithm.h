//
//  swift_algorithm.h
//  exp2swift
//
//  Created by Yoshida on 2020/07/28.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#ifndef swift_algorithm_h
#define swift_algorithm_h

#include <express/linklist.h>
#include <express/scope.h>

void ALGget_generics( Scope s, Linked_List generics, Linked_List aggregates );

#define YES_FORCE_OPTIONAL	true
#define NO_FORCE_OPTIONAL		false
#define YES_DROP_SINGLE_LABEL	true
#define NO_DROP_SINGLE_LABEL	false
void ALGargs_swift( Scope current, bool force_optional, Linked_List args, bool drop_single_label, int level );
void ALGscope_swift( Scope s, int level );


#endif /* swift_algorithm_h */
