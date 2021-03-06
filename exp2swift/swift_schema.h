//
//  swift_schema.h
//  exp2swift
//
//  Created by Yoshida on 2020/06/13.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

#ifndef swift_schema_h
#define swift_schema_h

#include <express/schema.h>

void SCHEMA_swift( Schema s );

const char* SCHEMA_swiftName( Schema schema, char buf[BUFSIZ]);
extern const char* as_schemaSwiftName_n( const char* symbol_name, char* buf, int bufsize );

const char* schema_nickname(Schema schema, char buf[BUFSIZ] );

#endif /* swift_schema_h */
