//
//  swift_named_aggregate_type.h
//  exp2swift
//
//  Created by Yoshida on 2020/10/25.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

#ifndef swift_named_aggregate_type_h
#define swift_named_aggregate_type_h

#include "../express/type.h"

extern void namedAggregateTypeDefinition_swift( Schema schema, Type type, int level);
extern void namedAggregateTypeExtension_swift( Schema schema, Type type, int level);

#endif /* swift_named_aggregate_type_h */
