//
//  swift_algorithm.c
//  exp2swift
//
//  Created by Yoshida on 2020/07/28.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//


#include <express/scope.h>
#include <express/linklist.h>
#include <exppp/exppp.h>

#include "pp.h"

#include "swift_algorithm.h"
#include "swift_files.h"
#include "swift_symbol.h"
#include "swift_type.h"
#include "swift_expression.h"
#include "swift_scope_types.h"
#include "swift_scope_entities.h"
#include "swift_scope_algs.h"
#include "swift_scope_consts.h"
#include "swift_scope_locals.h"

void ALGget_generics( Scope s, Linked_List generics, Linked_List aggregates ) {
	assert(generics);
	assert(aggregates);
	
	DictionaryEntry dictEntry;
	Type tag;
	
	DICTdo_type_init( s->symbol_table, &dictEntry, OBJ_TAG );
	while( 0 != ( tag = ( Type )DICTdo( &dictEntry ) ) ) {
		TypeBody base = TYPEget_body(tag->u.type->head);
		if(!base) {	// hack!
			LISTadd_last(aggregates, tag);
			continue;
		}
		switch (base->type) {
			case generic_:
				LISTadd_last(generics, tag);
				break;
				
			case aggregate_:
				LISTadd_last(aggregates, tag);
				break;
				
			default:
				break;
		}
	}
}


void ALGargs_swift( Scope current, bool force_optional, Linked_List args, bool drop_single_label, int level ) {
	int indent2save = captureWrapIndent();
	
	bool single_param = drop_single_label && (LISTget_second(args) == NULL);
	char* sep = (single_param ? "_ " : "");
	
	LISTdo(args, formalp, Variable) {
		raw("%s", sep);
		positively_wrap();
		{
			char buf[BUFSIZ];
			wrap("%s: ", variable_swiftName(formalp,buf));
		}
		if( VARis_inout(formalp) ) {
			wrap("inout ");
		}
		variableType_swift(current, formalp, force_optional, NOT_IN_COMMENT);
		sep = ", ";
	}LISTod;
	
	restoreWrapIndent(indent2save);
}

void ALGscope_declarations_swift(Schema schema, Scope s, int level ) {
	SCOPEtypeList_swift(schema, s, level);
	SCOPEentityList_swift(s, level);
	SCOPEalgList_swift(schema, s, level);
	SCOPEconstList_swift(true, s, level);
	SCOPElocalList_swift(s, level);
}
