//
//  swift_select_type.h
//  exp2swift
//
//  Created by Yoshida on 2020/09/03.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

#ifndef swift_select_type_h
#define swift_select_type_h

#include "../express/type.h"

extern void selectTypeDefinition_swift(Schema schema, Type select_type,  int level);
extern void selectTypeExtension_swift(Schema schema, Type select_type,  int level);

#endif /* swift_select_type_h */
