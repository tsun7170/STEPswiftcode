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
#define YES_PAD	true
#define NO_PAD	false
#define YES_WRAP	true
#define NO_WRAP	false

#define EXPR_swift(SELF,e,p) EXPR__swift(SELF,e,p,OP_UNKNOWN, YES_WRAP)
#define EXPRop2_swift(SELF,oe,string,paren,pad) EXPRop2__swift(SELF,oe,string,paren,pad,OP_UNKNOWN, YES_WRAP)
#define EXPRop_swift(SELF,oe,paren) EXPRop__swift(SELF,oe,paren,OP_UNKNOWN, YES_WRAP)

void breakLongStr_swift( const char * in );


void EXPRbounds_swift( Scope SELF, TypeBody tb );
void EXPR__swift( Scope SELF, Expression e, bool paren, unsigned int previous_op, bool can_wrap );
void EXPRop__swift( Scope SELF, struct Op_Subexpression * oe, bool paren, unsigned int previous_op, bool can_wrap );
void EXPRop2__swift( Scope SELF, struct Op_Subexpression * eo, char * opcode, bool paren, bool pad, unsigned int previous_op, bool can_wrap );
void EXPRop1_swift( Scope SELF, struct Op_Subexpression * eo, char * opcode, bool paren, bool can_wrap );
void EXPRopGroup_swift( Scope SELF, struct Op_Subexpression * eo, bool can_wrap );
void EXPRopIn_swift( Scope SELF, struct Op_Subexpression * eo, bool can_wrap );


//void EXPRstring_swift( Scope SELF, char * buffer, Expression e );
//void EXPRop_string_swift( Scope SELF, char * buffer, struct Op_Subexpression * oe );

#endif /* swift_expression_h */
