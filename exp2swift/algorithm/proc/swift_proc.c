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


const char* PROC_swiftName( Procedure proc ) {
	return proc->symbol.name;	
}

const char* PROCcall_swiftName( Statement pcall ) {
	return pcall->symbol.name;
}


char 				PROC_swiftNameInitial( Procedure proc ) {
	return toupper( PROC_swiftName(proc)[0] );
}

//MARK: - main entry point

void PROC_swift( bool nested, Procedure proc, int level ) {
	if(!nested) {
		// EXPRESS summary
		beginExpress_swift("PROCEDURE DEFINITION");
		PROC_out(proc, level);
		endExpress_swift();	
	}
	
}
