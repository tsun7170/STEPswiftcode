//
//  swift_expression.h
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#ifndef swift_expression_h
#define swift_expression_h

#include "../express/expbasic.h"
#include "../express/express.h"

//*TY2020/07/11
#define YES_PAREN	true
#define NO_PAREN	false

extern void EXPRbounds_swift( Scope SELF, TypeBody tb, bool in_comment );
extern void EXPR_swift( Scope SELF, Expression e, bool paren);
extern void EXPRassignment_rhs_swift( Scope SELF, Expression rhs, Type lhsType);

#endif /* swift_expression_h */
