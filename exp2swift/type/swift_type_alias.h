//
//  swift_type_alias.h
//  exp2swift
//
//  Created by Yoshida on 2020/10/25.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#ifndef swift_type_alias_h
#define swift_type_alias_h

#include "../express/type.h"

extern void typeAliasDefinition_swift( Schema schema, Type type, Type original, int level);
extern void typeAliasExtension_swift( Schema schema, Type type, Type original, int level);

#endif /* swift_type_alias_h */
