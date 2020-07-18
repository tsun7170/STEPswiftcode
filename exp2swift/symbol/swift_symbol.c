//
//  swift_symbol.c
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#include "swift_symbol.h"

const char * variable_swiftName(Variable v) {
	return v->name->symbol.name;
}
