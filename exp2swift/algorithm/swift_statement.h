//
//  swift_statement.h
//  exp2swift
//
//  Created by Yoshida on 2020/07/28.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#ifndef swift_statement_h
#define swift_statement_h

#include <express/stmt.h>
#include <express/linklist.h>

//const char * alias_swiftName(Statement s);


extern void STMT_swift( Scope algo, Statement stmt, int* tempvar_id, int level );
extern void STMTlist_swift( Scope algo, Linked_List stmts, int* tempvar_id, int level );


#endif /* swift_statement_h */
