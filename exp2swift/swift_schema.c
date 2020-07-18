//
//  swift_schema.c
//  exp2swift
//
//  Created by Yoshida on 2020/06/13.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#include <stdlib.h>
#include <errno.h>

#include <sc_version_string.h>
#include <express/express.h>

#include <exppp/exppp.h>
#include "pp.h"
#include "pretty_ref.h"
#include "pretty_scope.h"
#include "pretty_schema.h"

//#if defined( _WIN32 ) || defined ( __WIN32__ )
//#  define unlink _unlink
//#else
//#  include <unistd.h> /* for unlink */
//#endif

#include "swift_schema.h"
#include "swift_files.h"
#include "swift_scope_consts.h"
#include "swift_scope_types.h"
#include "swift_scope_entities.h"


const char* SCHEMA_swiftName( Schema schema) {
	return schema->symbol.name;
}


void SCHEMA_swift( Schema schema ) {
	
	openSwiftFileForSchema(schema);
	
	int level = 0;
	
	first_line = false;
	beginExpress_swift();
	raw( "SCHEMA %s;\n", schema->symbol.name );
	
	if( schema->u.schema->usedict || schema->u.schema->use_schemas
		 || schema->u.schema->refdict || schema->u.schema->ref_schemas ) {
		raw( "\n" );
	}
	
	REFout( schema->u.schema->usedict, schema->u.schema->use_schemas, "USE", level + exppp_nesting_indent );
	REFout( schema->u.schema->refdict, schema->u.schema->ref_schemas, "REFERENCE", level + exppp_nesting_indent );
	
	/* output order for DIS & IS schemas:
	 * CONSTANT
	 * TYPE
	 * ENTITY
	 * RULE
	 * FUNCTION
	 * PROCEDURE
	 *
	 * Within each of those groups, declarations must be sorted alphabetically.
	 */
	
	SCOPEconsts_out( schema, level + exppp_nesting_indent );
	//    SCOPEtypes_out( s, level + exppp_nesting_indent );
	//    SCOPEentities_out( s, level + exppp_nesting_indent );
	//    SCOPEalgs_out( s, level + exppp_nesting_indent );
	
	raw(	"\n"
			"END_SCHEMA;"	);
	tail_comment( schema->symbol.name );
	endExpress_swift();
	
	// swift code generation
	raw("/* SCHEMA */\n");
	raw("public enum %s: NameSpace {\n", SCHEMA_swiftName(schema));
	SCOPEconstList_swift( schema, level + nestingIndent_swift );

	raw("}\n");
	raw("/* END_SCHEMA */\n");

	SCHEMAtypeList_swift( schema );
	SCHEMAentityList_swift( schema );

	
	return;
}
