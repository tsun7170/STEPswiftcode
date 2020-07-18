//
//  swift_files.h
//  exp2swift
//
//  Created by Yoshida on 2020/06/13.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#ifndef swift_files_h
#define swift_files_h

#include <express/schema.h>

extern const int nestingIndent_swift;
void beginExpress_swift(void);
void endExpress_swift(void);
void indent_swift(int level);

void openSwiftFileForSchema(Schema s);
void openSwiftFileForType(Type type);
void openSwiftFileForEntity(Entity entity);
void closeSwiftFile(void);

#endif /* swift_files_h */
