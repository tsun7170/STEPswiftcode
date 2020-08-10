//
//  swift_symbol.h
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#ifndef swift_symbol_h
#define swift_symbol_h

#include <express/scope.h>

const char * variable_swiftName(Variable v);
void variableType_swift(Variable v, bool force_optional, int level);

#endif /* swift_symbol_h */
