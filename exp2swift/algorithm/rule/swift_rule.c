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
#include "swift_symbol.h"
#include "swift_entity.h"
#include "swift_algorithm.h"
#include "swift_statement.h"
#include "swift_expression.h"

const char* RULE_swiftName( Rule rule, char buf[BUFSIZ] ) {
	return canonical_swiftName(rule->symbol.name,buf);
}

char 				RULE_swiftNameInitial( Rule rule ) {
	return toupper( rule->symbol.name[0] );
}

//MARK: - main entry point

void RULE_swift(Schema schema, Rule rule, int level ) {
	// EXPRESS summary
	beginExpress_swift("RULE DEFINITION");
	RULE_out(rule, level);
	endExpress_swift();	
	
	//rule head
	indent_swift(level);
	raw("public static\n");
	indent_swift(level);
	{
		char buf[BUFSIZ];
		raw("func %s( ", RULE_swiftName(rule,buf));
	}
	wrap("allComplexEntities: Set<SDAI.ComplexEntity> ) -> SDAI.LOGICAL {\n");
	
	{	int level2 = level+nestingIndent_swift;
		
		//entity refs
		raw("\n");
		indent_swift(level2);
		raw("//ENTITY REFERENCES\n");
		LISTdo(rule->u.rule->parameters, entity_ref, Variable) {
			assert(TYPEis_entity(TYPEget_base_type(entity_ref->type)));
			Entity ent = TYPEget_body(TYPEget_base_type(entity_ref->type))->entity;
			indent_swift(level2);

			char buf[BUFSIZ];
			raw("let %s = ", variable_swiftName(entity_ref,buf));
			wrap("SDAI.POPULATION(OF: %s.self, FROM: allComplexEntities)\n", ENTITY_swiftName(ent, NO_QUALIFICATION, buf) );
		}LISTod;
		raw("\n");
		
		//rule body
		ALGscope_declarations_swift(schema, rule, level2);
		STMTlist_swift(rule, rule->u.rule->body, level2);
		
		//where rules
		raw("\n");
		indent_swift(level2);
		raw("//WHERE\n");
		Linked_List where_rules = RULEget_where(rule);
		LISTdo(where_rules, where, Where){
			indent_swift(level2);
			
			char buf[BUFSIZ];
			raw("let %s = ",whereRuleLabel_swiftName(where, buf));
//			EXPR_swift(schema,where->expr,Type_Logical,YES_PAREN);
			EXPRassignment_rhs_swift(NO_RESOLVING_GENERIC, schema, where->expr, Type_Logical, NO_PAREN, OP_UNKNOWN, YES_WRAP);
			raw("\n\n");
		}LISTod;
		
		//final result
		raw("\n");
		indent_swift(level2);
		raw("return");
		char* sep = " ";
		LISTdo(where_rules, where, Where){
			raw("%s",sep);
			
			char buf[BUFSIZ];
			wrap("%s",whereRuleLabel_swiftName(where, buf));
			sep = " && ";
		}LISTod;
		raw("\n");
	}
	
	indent_swift(level);
	raw("}\n\n");
}
