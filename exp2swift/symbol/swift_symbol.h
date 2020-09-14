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

#define YES_OPTIONAL_TYPE	true
#define NON_OPTIONAL_TYPE	false


const char* canonical_swiftName(const char* name, char buf[BUFSIZ]);

const char * variable_swiftName(Variable v, char buf[BUFSIZ]);
void variableType_swift(Scope current, Variable v, bool force_optional, bool in_comment);
void optionalType_swift(Scope current, Type type, bool optional, bool in_comment);

#endif /* swift_symbol_h */
