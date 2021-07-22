//
//  swift_symbol.h
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

#ifndef swift_symbol_h
#define swift_symbol_h

#include <express/scope.h>
#include "swift_type.h"

#define YES_OPTIONAL_TYPE	true
#define NON_OPTIONAL_TYPE	false

extern const char* canonical_swiftName_n(const char* name, char* buf, int maxlen);
extern const char* canonical_swiftName(const char* name, char buf[BUFSIZ]);

extern const char* canonical_dictName_n(const char* name, char* buf, int maxlen);
extern const char* canonical_dictName(const char* name, char buf[BUFSIZ]);


extern const char * variable_swiftName(Variable v, char buf[BUFSIZ]);
extern const char * asVariable_swiftName_n(const char* symbol_name, char* buf, int maxlen);

/// <#Description#>
/// @param current <#current description#>
/// @param v <#v description#>
/// @param force_optional <#force_optional description#>
/// @param in_comment <#in_comment description#>
/// @return optionality of variable type
extern bool variableType_swift(Scope current, Variable v, bool force_optional, SwiftOutCommentOption in_comment);

/// <#Description#>
/// @param current <#current description#>
/// @param type <#type description#>
/// @param optional <#optional description#>
/// @param in_comment <#in_comment description#>
/// @return optionality of variable type
extern bool optionalType_swift(Scope current, Type type, bool optional, SwiftOutCommentOption in_comment);

#endif /* swift_symbol_h */
