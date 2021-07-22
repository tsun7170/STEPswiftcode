//
//  swift_expression.h
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

#ifndef swift_expression_h
#define swift_expression_h

#include "../express/expbasic.h"
#include "../express/express.h"
#include "swift_type.h"

#define YES_PAREN	true
#define NO_PAREN	false
#define YES_WRAP	true
#define NO_WRAP	false

#define CHECK_DEEP	true
#define CHECK_SHALLOW	false

#define YES_RESOLVING_GENERIC	true
#define NO_RESOLVING_GENERIC	false

typedef enum  {
	no_optional, yes_optional, unknown
} type_optionality;

extern type_optionality EXPRresult_is_optional(Expression e, bool deep);
extern void EXPRbounds_swift( Scope SELF, TypeBody tb, SwiftOutCommentOption in_comment );
extern void EXPR_swift( Scope SELF, Expression e, Type target_type, bool paren);
extern void EXPRassignment_rhs_swift(bool resolve_generic, Scope SELF, Expression rhs, Type lhsType, bool paren, unsigned int previous_op, bool can_wrap);

#endif /* swift_expression_h */
