//
//  swift_type.h
//  exp2swift
//
//  Created by Yoshida on 2020/06/14.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#ifndef swift_type_h
#define swift_type_h

#include "../express/type.h"

typedef enum {
	YES_IN_COMMENT,
	NOT_IN_COMMENT,
	WO_COMMENT
} SwiftOutCommentOption;

//#define YES_IN_COMMENT	true
//#define NOT_IN_COMMENT	false

#define NO_QUALIFICATION	NULL

extern int accumulate_qualification(Scope target, Scope current, char buf[BUFSIZ]);

extern const char* TYPE_canonicalName( Type t, Scope current, char buf[BUFSIZ] );
extern const char* TYPE_swiftName( Type t, Scope current, char buf[BUFSIZ]  );
extern char TYPE_swiftNameInitial( Type t );
extern const char* enumCase_swiftName( Expression expr, char buf[BUFSIZ] );
extern const char* selectCase_swiftName( Type selection, char buf[BUFSIZ] );

extern void TYPEdefinition_swift(Schema schema, Type t, int level );
extern void TYPEextension_swift( Schema schema, Type t, int level );

extern void TYPE_head_swift( Scope current, Type t, SwiftOutCommentOption in_comment );
extern void TYPE_body_swift( Scope current, Type t, SwiftOutCommentOption in_comment );
extern const char* builtinTYPE_body_swiftname( Type t );

extern const char* TYPEhead_string_swift( Scope current, Type t, SwiftOutCommentOption in_comment, char buf[BUFSIZ]);

extern bool TYPEis_swiftAssignable(Type lhstype, Type rhstype);

extern const char* whereRuleLabel_swiftName( Where w, char buf[BUFSIZ] );
extern void TYPEwhereDefinitions_swift( Scope scope, int level );
extern void TYPEwhereRuleValidation_swift( Type type, int level );

#endif /* swift_type_h */
