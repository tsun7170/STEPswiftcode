//
//  swift_symbol.c
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#include "pp.h"

#include "swift_symbol.h"
#include "swift_files.h"
#include "swift_type.h"

const char * variable_swiftName(Variable v) {
	return v->name->symbol.name;
}

void variableType_swift(Variable v, bool force_optional, int level) {
	bool optional = force_optional || VARis_optional(v);
	
	//	if( optional ) raw("(");
	TYPE_head_swift(v->type, level+nestingIndent_swift);
	if( optional ) raw("?");
}
