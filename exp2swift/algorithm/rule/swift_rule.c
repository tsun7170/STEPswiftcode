//
//  swift_rule.c
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
#include "pretty_rule.h"


#include "swift_rule.h"
#include "swift_files.h"

const char* RULE_swiftName( Rule rule ) {
	return rule->symbol.name;
}

char 				RULE_swiftNameInitial( Rule rule ) {
	return toupper( RULE_swiftName(rule)[0] );
}

//MARK: - main entry point

void RULE_swift( Rule rule, int level ) {
	// EXPRESS summary
	beginExpress_swift("RULE DEFINITION");
	RULE_out(rule, level);
	endExpress_swift();	
	
}
