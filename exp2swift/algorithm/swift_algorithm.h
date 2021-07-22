//
//  swift_algorithm.h
//  exp2swift
//
//  Created by Yoshida on 2020/07/28.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

#ifndef swift_algorithm_h
#define swift_algorithm_h

#include <express/linklist.h>
#include <express/scope.h>

extern void ALGget_generics( Scope s, Linked_List generics, Linked_List aggregates );

#define YES_FORCE_OPTIONAL	true
#define NO_FORCE_OPTIONAL		false
#define YES_DROP_SINGLE_LABEL	true
#define NO_DROP_SINGLE_LABEL	false
extern void ALGargs_swift( Scope current, bool force_optional, Linked_List args, bool drop_single_label, int level );
extern void ALGvarnize_args_swift( Linked_List args, int level );
extern void ALGscope_declarations_swift(Schema schema, Scope s, int level );


#endif /* swift_algorithm_h */
