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

const char* canonical_swiftName_n(const char* name, char* buf, int maxlen) {
	char* to = buf;
	int remain = maxlen;
	for(const char* from = name; *from; ++from ) {
		if( islower(*from) ) {
			(*to) = toupper(*from);
		}
		else {
			(*to) = (*from);
		}
		++to;
		--remain;
		if( remain == 0 ) break;
	}
	(*to) = 0;
	return buf;
}

const char* canonical_swiftName(const char* name, char buf[BUFSIZ]) {
	return canonical_swiftName_n(name, buf, BUFSIZ-1);
//	char* to = buf;
//	int remain = BUFSIZ-1;
//	for(const char* from = name; *from; ++from ) {
//		if( islower(*from) ) {
//			(*to) = toupper(*from);
//		}
//		else {
//			(*to) = (*from);
//		}
//		++to;
//		--remain;
//		if( remain == 0 ) break;
//	}
//	(*to) = 0;
//	return buf;
}


const char * variable_swiftName(Variable v, char buf[BUFSIZ]) {
	return canonical_swiftName(v->name->symbol.name, buf);
}

void variableType_swift(Scope current, Variable v, bool force_optional, SwiftOutCommentOption in_comment) {
	bool optional = force_optional || VARis_optional(v);
	optionalType_swift(current, v->type, optional, in_comment);
}

void optionalType_swift(Scope current, Type type, bool optional, SwiftOutCommentOption in_comment) {
	bool simple_type = 	(type->symbol.name != NULL) || 
						!(TYPEhas_bounds(type) || TYPEget_unique(type));
	
	optional &= !TYPEis_optional(type);
	
	if( optional && !simple_type ) raw("(");
	TYPE_head_swift(current, type, in_comment);
	if( optional ) {
		if( !simple_type ) raw(")");
		raw("? ");
	} 
}
