//
//  swift_proc.c
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
#include "pretty_proc.h"

#include "swift_proc.h"
#include "swift_files.h"
#include "swift_type.h"
#include "swift_algorithm.h"
#include "swift_statement.h"
#include "swift_symbol.h"


char 				PROC_swiftNameInitial( Procedure proc ) {
	return toupper( proc->symbol.name[0] );
}

const char* PROC_swiftName( Procedure proc, char buf[BUFSIZ] ) {
	return canonical_swiftName(proc->symbol.name, buf);	
}

const char* PROCcall_swiftName( Statement pcall, char buf[BUFSIZ] ) {
	return canonical_swiftName(pcall->symbol.name, buf);
}


//MARK: - main entry point

void PROC_swift( bool nested, Procedure proc, int level ) {
	if(!nested) {
		// EXPRESS summary
		beginExpress_swift("PROCEDURE DEFINITION");
		PROC_out(proc, level);
		endExpress_swift();	
	}
	
		// function head
		indent_swift(level);
		char* access = (nested ? "//NESTED PROCEDURE" : "public static ");
		raw("%s\n", access);
		indent_swift(level);
	{
		char buf[BUFSIZ];
		raw("func %s", PROC_swiftName(proc,buf));
	}

		Linked_List generics = LISTcreate();
		Linked_List aggregates = LISTcreate();
		ALGget_generics(proc, generics, aggregates);
		if(!LISTis_empty(generics) || !LISTis_empty(aggregates)) {
			//generic procedure
			char buf[BUFSIZ];
			char* sep = "";
			raw("<");
			LISTdo(generics, gtag, Type) {
				wrap("%s%s: SDAIGenericType",sep,TYPE_swiftName(gtag,NO_QUALIFICATION,buf));
				sep=", ";
			}LISTod;
			LISTdo(aggregates, atag, Type) {
				wrap("%s%s: SDAIAggregationType",sep,TYPE_swiftName(atag,NO_QUALIFICATION,buf));
				sep=", ";
			}LISTod;
			raw(">");
		}
		
		// parameters
		raw("(");
		ALGargs_swift( proc->superscope, YES_FORCE_OPTIONAL, proc->u.proc->parameters, YES_DROP_SINGLE_LABEL, level );
		raw(")");
				
		if(!LISTis_empty(aggregates)) {
			// constraint for aggregate element type
			raw(" ");
			char buf[BUFSIZ];
			char* sep = "where ";
			LISTdo(aggregates, atag, Type) {
				Type base = atag->u.type->head;	//hack!
				positively_wrap();
				wrap("%s%s.Element == ",sep,TYPE_swiftName(atag,NULL,buf));
				TYPE_head_swift(proc->superscope, base, level, NOT_IN_COMMENT);
				sep = ", ";
			}LISTod;
		}
		wrap(" {\n");
		
		// proc body
		{	int level2 = level+nestingIndent_swift;
			
			ALGscope_swift(proc, level2);
			STMTlist_swift(proc->u.proc->body, level2);
		}
		
		indent_swift(level);
		if( nested ) {
			char buf[BUFSIZ];
			raw("} //END PROCEDURE %s\n\n", PROC_swiftName(proc,buf));
		}
		else {
			raw("}\n\n");
		}
		
		LISTfree(generics);
		LISTfree(aggregates);

}
