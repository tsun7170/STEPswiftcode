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
#include "swift_scope_algs.h"
#include "swift_symbol.h"


const char* SCHEMA_swiftName( Schema schema, char buf[BUFSIZ]) {
	return canonical_swiftName(schema->symbol.name, buf);
}

const char* schema_nickname(Schema schema, char buf[BUFSIZ] ) {
	char* underline = strchr(schema->symbol.name, '_');
	
	if( underline != NULL) {
		int nicklen = (int)(underline - schema->symbol.name);
		snprintf(buf, BUFSIZ, "%.*s", nicklen, schema->symbol.name);
	}
	else {
		snprintf(buf, BUFSIZ, "%s", schema->symbol.name);
	}
	return buf;
}

void SCHEMA_swift( Schema schema ) {
	
	openSwiftFileForSchema(schema);
		
	raw("\n"
			"import SwiftSDAIcore\n");

	int level = 0;
	
	first_line = false;
	beginExpress_swift("SCHEMA DEFINITION");
	raw( "SCHEMA %s;\n", schema->symbol.name );
	
	if( schema->u.schema->usedict || schema->u.schema->use_schemas
		 || schema->u.schema->refdict || schema->u.schema->ref_schemas ) {
		raw( "\n" );
	}
	
	REFout( schema->u.schema->usedict, schema->u.schema->use_schemas, "USE", level + exppp_nesting_indent );
	REFout( schema->u.schema->refdict, schema->u.schema->ref_schemas, "REFERENCE", level + exppp_nesting_indent );
	SCOPEconsts_out( schema, level + exppp_nesting_indent );
	
	raw(	"\n"
			"END_SCHEMA;"	);
	tail_comment( schema->symbol );
	endExpress_swift();
	
	// swift code generation
	raw("/* SCHEMA */\n");
	{	char buf[BUFSIZ];
		raw("public enum %s {\n", SCHEMA_swiftName(schema, buf));
	}
	
	{	int level2 = level+nestingIndent_swift;
		SCOPEconstList_swift( false, schema, level2 );
	}

	raw("}\n");
	raw("/* END_SCHEMA */\n");

	SCHEMAtypeList_swift( schema );
	SCHEMAentityList_swift( schema );
	SCHEMAalgList_swift( schema );

	
	return;
}
