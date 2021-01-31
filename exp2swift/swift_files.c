//
//  swift_files.c
//  exp2swift
//
//  Created by Yoshida on 2020/06/13.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#include <stdlib.h>
#include <errno.h>
#include <sc_mkdir.h>


#include <sc_version_string.h>
#include <express/express.h>

#include <exppp/exppp.h>
#include "pp.h"


#include "swift_files.h"
#include "swift_schema.h"
#include "swift_type.h"
#include "swift_entity.h"
#include "swift_rule.h"
#include "swift_func.h"
#include "swift_proc.h"

const int nestingIndent_swift = 2;       /* default nesting indent */

void beginExpress_swift(char* description) {
	raw( "//MARK: -%s in EXPRESS\n/*\n", description );
}

void endExpress_swift() {
	raw( "\n*/\n\n" );
}

void indent_swift(int level) {
	indent2 = level+nestingIndent_swift;
	if( level <= 0 ) return;
	raw( "%*s", level, "");
}
void indent_with_char(int level, char c) {
	for(int i=1; i<=level; ++i) {
		raw("%c",c);
	}
}


//char * exppp_output_filename = ( char * )0; /* if this is set, override default output filename */
static char swift_filename_buffer[1000];           /* output file name */

/* Only the first line is compared to an existing file, so putting a
 * version number in here won't cause problems. The actual version must
 * be inserted later - this can't be initialized with non-constant.
 */
static char * expheader[] = {
    "/* This file was generated by the EXPRESS to Swift translator \"exp2swift\",",
    "derived from STEPcode (formerly NIST's SCL).\n",
		"exp2swift version:","",/*version string to be inserted here*/"", "\n",
    "WARNING: You probably don't want to edit it since your modifications",
		"will be lost if exp2swift is used to regenerate it.\n",
		"*/\n\n"                     ,
    0
};


void closeSwiftFile(void) {
	if( exppp_fp != NULL ) {
		fclose( exppp_fp );
	}
	exppp_fp = NULL;
}


static void printOutHeaderComment() {
	char ** hp;
	error_sym.line = 1;
	/* print our header - generated by exppp, don't edit, etc */
	for( hp = expheader; *hp; hp++ ) {
		if( ( **hp == '\0' ) && ( **( hp + 1 ) == '\0' ) ) {
			/* if this and the next lines are blank, put version string on this line */
			wrap( "%s ", sc_version );
		} else {
			wrap( "%s ", *hp );
		}
	}
}

static bool described = false;

//MARK: - swift file setup
void openSwiftFileForSchema(Schema schema) {
	closeSwiftFile();
	exppp_output_filename = 0;
	
	exppp_output_filename = swift_filename_buffer;
	exppp_output_filename_reset = true;
	char buf[BUFSIZ];
	sprintf( swift_filename_buffer, "%s.swift", SCHEMA_swiftName(schema, buf) );
	
	error_sym.filename = swift_filename_buffer;
	
	if( !described && !exppp_terse ) {
		fprintf( stdout, "%s: writing schema file %s\n", EXPRESSprogram_name, swift_filename_buffer );
	}
	if( !( exppp_fp = fopen( swift_filename_buffer, "w" ) ) ) {
		ERRORreport( ERROR_file_unwriteable, swift_filename_buffer, strerror( errno ) );
		abort();
	}
	
	printOutHeaderComment();
}


static void openSwiftFile(Schema s, const char* category, char initial, const char* filename) {
	closeSwiftFile();
	exppp_output_filename = swift_filename_buffer;
	exppp_output_filename_reset = true;
	
	char dir1[ BUFSIZ ];
	{
		char buf[BUFSIZ];
		snprintf(dir1, BUFSIZ-1, "%s-%s", category,schema_nickname(s, buf));
		if( mkDirIfNone( dir1 ) == -1 ) {
			fprintf( stderr, "At %s:%d - mkdir() failed with error ", __FILE__, __LINE__);
			perror( 0 );
			abort();
		}
	}
	
	char dir2[ BUFSIZ ];
	snprintf( dir2, BUFSIZ-1, "%s/%c", dir1, initial );
	if( mkDirIfNone( dir2 ) == -1 ) {
		fprintf( stderr, "At %s:%d - mkdir() failed with error ", __FILE__, __LINE__);
		perror( 0 );
		abort();
	}	
	
	sprintf( swift_filename_buffer, "%s/%s.swift", dir2, filename );
	error_sym.filename = swift_filename_buffer;
	
	if( !described && !exppp_terse ) {
		fprintf( stdout, "%s: writing schema translation file %s\n", EXPRESSprogram_name, swift_filename_buffer );
	}
	if( !( exppp_fp = fopen( swift_filename_buffer, "w" ) ) ) {
		ERRORreport( ERROR_file_unwriteable, swift_filename_buffer, strerror( errno ) );
		abort();
	}
	
	printOutHeaderComment();
}


void openSwiftFileForType(Schema s, Type type) {
	openSwiftFile(s, "type", TYPE_swiftNameInitial(type), type->symbol.name);
}

void openSwiftFileForEntity(Schema s, Entity entity) {
	openSwiftFile(s, "entity", ENTITY_swiftNameInitial(entity), entity->symbol.name);
//	if( strcmp(entity->symbol.name, "a3m_equivalence_criterion_of_detailed_assembly_data_content")==0 ){
//		bool debug = true;
//	}
}

void openSwiftFileForRule(Schema s, Rule rule) {
	openSwiftFile(s, "rule", RULE_swiftNameInitial(rule), rule->symbol.name);
}


void openSwiftFileForFunction(Schema s, Function func) {
	openSwiftFile(s, "func", FUNC_swiftNameInitial(func), func->symbol.name);
//	if( strcmp(func->symbol.name, "list_to_array")==0 ){
//		bool debug = true;
//	}
}

void openSwiftFileForProcedure(Schema s, Procedure proc) {
	openSwiftFile(s, "proc", PROC_swiftNameInitial(proc), proc->symbol.name);
}


