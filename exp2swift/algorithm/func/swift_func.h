//
//  swift_func.h
//  exp2swift
//
//  Created by Yoshida on 2020/07/27.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

#ifndef swift_func_h
#define swift_func_h

#include "../express/alg.h"

char 				FUNC_swiftNameInitial( Function func );

const char* FUNC_swiftName( Function func, char buf[BUFSIZ] );
const char* FUNCcall_swiftName( Expression fcall, char buf[BUFSIZ] );

void FUNC_swift(Schema schema, bool nested, Function func, int level );

#endif /* swift_func_h */
