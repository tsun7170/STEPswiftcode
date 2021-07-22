//
//  decompose_expression.h
//  exp2swift
//
//  Created by Yoshida on 2021/01/16.
//  Copyright © 2021 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

#ifndef decompose_expression_h
#define decompose_expression_h

#include "../express/expbasic.h"
#include "../express/express.h"

extern Expression EXPR_decompose( Expression original, Type target_type, int*/*inout*/ tempvar_id, Linked_List*/*out*/ tempvar_definitions );
extern Expression EXPR_decompose_lhs( Expression e, Type target_type, int*/*inout*/ tempvar_id, Linked_List tempvars );
extern void EXPR_delete_tempvar_definitions( Linked_List tempvar_definitions );
extern int EXPR_tempvars_swift( Scope s, Linked_List tempvar_definitions, int level );

#endif /* decompose_expression_h */
