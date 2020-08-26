//
//  swift_rule.h
//  exp2swift
//
//  Created by Yoshida on 2020/07/27.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#ifndef swift_rule_h
#define swift_rule_h

#include "../express/alg.h"

const char* RULE_swiftName( Rule rule, char buf[BUFSIZ] );
char 				RULE_swiftNameInitial( Rule rule );

void RULE_swift( Rule rule, int level );

#endif /* swift_rule_h */
