//
//  swift_files.h
//  exp2swift
//
//  Created by Yoshida on 2020/06/13.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

#ifndef swift_files_h
#define swift_files_h

#include <express/schema.h>

extern const int nestingIndent_swift;
void beginExpress_swift(char* description);
void endExpress_swift(void);
void indent_swift(int level);
void indent_with_char(int level, char c);

void openSwiftFileForSchema(Schema s);
void openSwiftFileForType(Schema s, Type type);
void openSwiftFileForEntity(Schema s, Entity entity);
void openSwiftFileForRule(Schema s, Rule rule);
void openSwiftFileForFunction(Schema s, Function func);
void openSwiftFileForProcedure(Schema s, Procedure proc);
void closeSwiftFile(void);

#endif /* swift_files_h */
