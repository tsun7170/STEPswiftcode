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

extern char 				FUNC_swiftNameInitial( Function func );

extern const char* FUNC_swiftName( Function func, char buf[BUFSIZ] );
extern const char* FUNCcall_swiftName( Expression fcall, char buf[BUFSIZ] );
extern const char* FUNC_cache_swiftName( Function func, char buf[BUFSIZ] );

extern void FUNC_swift(Schema schema, bool nested, Function func, int level );
extern void FUNC_result_cache_var_swift( Schema schema, Function func, int level );

#endif /* swift_func_h */
