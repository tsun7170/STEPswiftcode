//
//  swift_func.c
//  exp2swift
//
//  Created by Yoshida on 2020/07/27.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#include <assert.h>
#include <stdlib.h>

#include "exppp.h"
#include "pp.h"
#include "pretty_expr.h"
#include "pretty_func.h"


#include "swift_func.h"
#include "swift_files.h"
#include "swift_type.h"
#include "swift_algorithm.h"
#include "swift_statement.h"


const char* FUNC_swiftName( Function func ) {
	return func->symbol.name;
}

char 				FUNC_swiftNameInitial( Function func ) {
	return toupper( FUNC_swiftName(func)[0] );
}

const char* FUNCcall_swiftName( Expression fcall ) {
	return fcall->symbol.name;
}

//MARK: - main entry point

void FUNC_swift( bool nested, Function func, int level ) {
	if(!nested) {
		// EXPRESS summary
		beginExpress_swift("FUNCTION DEFINITION");
		FUNC_out(func, level);
		endExpress_swift();	
	}
	
	// function head
	indent_swift(level);
	char* access = (nested ? "//NESTED " : "public static ");
	raw("%s\n", access);
	indent_swift(level);
	raw("func %s", FUNC_swiftName(func));

	Linked_List generics = LISTcreate();
	Linked_List aggregates = LISTcreate();
	ALGget_generics(func, generics, aggregates);
	if(!LISTempty(generics) || !LISTempty(aggregates)) {
		//generic function
		char* sep = "";
		raw("<");
		LISTdo(generics, gtag, Type) {
			wrap("%s%s: SDAIGenericType",sep,TYPE_swiftName(gtag));
			sep=", ";
		}LISTod;
		LISTdo(aggregates, atag, Type) {
			wrap("%s%s: SDAIAggregationType",sep,TYPE_swiftName(atag));
			sep=", ";
		}LISTod;
		raw(">");
	}
	
	// parameters
	raw("(");
	ALGargs_swift( YES_FORCE_OPTIONAL, func->u.func->parameters, YES_DROP_SINGLE_LABEL, level );
	raw(") -> ");
	
	// return type
//	positively_wrap();
	TYPE_head_swift(func->u.func->return_type, level);raw("?");
	
	if(!LISTempty(aggregates)) {
		// constraint for aggregate element type
		raw(" ");
		char* sep = "where ";
		LISTdo(aggregates, atag, Type) {
			Type base = atag->u.type->head;	//hack!
			positively_wrap();
			wrap("%s%s.Element == ",sep,TYPE_swiftName(atag));
			TYPE_head_swift(base,level);
			sep = ", ";
		}LISTod;
	}
	wrap(" {\n");
	
	// function body
	{	int level2 = level+nestingIndent_swift;
		
		ALGscope_swift(func, level2);
		STMTlist_swift(func->u.proc->body, level2);
	}
	
	indent_swift(level);
	if( nested ) {
		raw("} //END FUNCTION %s\n\n", FUNC_swiftName(func));
	}
	else {
		raw("}\n\n");
	}
	
	LISTfree(generics);
	LISTfree(aggregates);
}
