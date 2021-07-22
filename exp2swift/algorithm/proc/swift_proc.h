//
//  swift_proc.h
//  exp2swift
//
//  Created by Yoshida on 2020/07/27.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

#ifndef swift_proc_h
#define swift_proc_h

#include "../express/alg.h"

char 				PROC_swiftNameInitial( Procedure proc );

const char* PROC_swiftName( Procedure proc, char buf[BUFSIZ] );
const char* PROCcall_swiftName( Statement pcall, char buf[BUFSIZ] );


void PROC_swift(Schema schema, bool nested, Procedure proc, int level );

#endif /* swift_proc_h */
