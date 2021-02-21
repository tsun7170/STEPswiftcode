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
	
//	printf("\n");
//	DICTprint(s->symbol_table);	//*TY2020/12/31 for debug
//	printf("\n");
	
	DictionaryEntry dictEntry;
	Type tag;
	
	DICTdo_type_init( s->symbol_table, &dictEntry, OBJ_TAG );
	while( NULL != (tag = DICTdo(&dictEntry)) ) {
//		printf("KEY:<%s> \tTYPE:<%c> \t\n",DICT_key,DICT_type);
		if( DICT_type != OBJ_TAG ) continue;
		
		TypeBody base = TYPEget_body(tag->u.type->head);
		assert(base != NULL);
		switch (base->type) {
			case generic_:
				LISTadd_last(generics, tag);
				break;
				
			case aggregate_:
				LISTadd_last(aggregates, tag);
				break;
				
			default:
				assert(!"unexpected case");
				LISTadd_last(aggregates, tag);
				break;
		}
	}
}


void ALGargs_swift( Scope current, bool force_optional, Linked_List args, bool drop_single_label, int level ) {
	aggressively_wrap();
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

void ALGvarnize_args_swift( Linked_List args, int level ) {
	bool generated = false;
	LISTdo(args, formalp, Variable) {
		if( VARis_inout(formalp) ) continue;
		indent_swift(level);
		char buf[BUFSIZ];
		raw("var %s = ", variable_swiftName(formalp,buf));
		raw("%s; SDAI.TOUCH(var: &%s)\n",buf,buf);
		generated = true;
	}LISTod;
	if( generated )raw("\n");
}

void ALGscope_declarations_swift(Schema schema, Scope s, int level ) {
	SCOPEtypeList_swift(schema, s, level);
	SCOPEentityList_swift(s, level);
	SCOPEalgList_swift(schema, s, level);
	SCOPEconstList_swift(true, s, level);
	SCOPElocalList_swift(s, level);
}
