//
//  swift_express.c
//  exp2swift
//
//  Created by Yoshida on 2020/06/13.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#include <express/express.h>
#include <exppp/exppp.h>

#include "pp.h"
#include "pretty_schema.h"
#include "pretty_express.h"

#include "swift_express.h"
#include "swift_schema.h"
#include "swift_files.h"

void EXPRESS_swift( Express e ) {
    Schema s;
    DictionaryEntry de;

    exppp_init();

    DICTdo_init( e->symbol_table, &de );
    while( 0 != ( s = ( Schema )DICTdo( &de ) ) ) {
        SCHEMA_swift( s );
    }
	
	closeSwiftFile();
}





#if 0
/******************************************************************
 ** Procedure:  EXPRESSPrint
 ** Parameters:
       Express express -- in memory representation of an express model
       FILES* files  -- set of output files to print to
 ** Returns:
 ** Description:  drives functions to generate code for all the schemas
 ** in an Express model into one set of files  -- works with
 **     print_schemas_combined
 ** Side Effects:  generates code
 ** Status:  24-Feb-1992 new -kcm
 ******************************************************************/
void EXPRESSPrint( Express express, ComplexCollect & complexCollect, FILES * files ) {
    char fileName [MAX_LEN], *np;
    const char  * expressFileName;  /* schnm is really "express name" */
    FILE * schemaLibFile;
    FILE * schemaIncFile;
    FILE * allSchema_hFile = files -> incall;
    FILE * allSchema_ccFile = files -> initall;
    FILE * schemaInitFile;
    /* new */
    Schema schema;
    DictionaryEntry dictEntry;


    /**********  create files based on name of schema   ***********/
    /*  return if failure           */
    /*  1.  header file             */
    sprintf( fileName, "%s.h", expressFileName = ClassName( EXPRESSget_basename( express ) ) );
    if( !( schemaIncFile = ( files -> inc ) = FILEcreate( fileName ) ) ) {
        return;
    }
    fprintf( schemaIncFile, 
						"\n// in the exp2cxx source code, this file is generally referred to as files->inc or incfile\n" );

    fprintf( schemaIncFile, 
						"#include <sdai.h> \n" );

    np = fileName + strlen( fileName ) - 1; /*  point to end of constant part of string  */
//    /*  1.9 init unity files (large translation units, faster compilation) */
//    initUnityFiles( expressFileName, files );
    /*  2.  class source file            */
    sprintf( np, "cc" );
    if( !( schemaLibFile = ( files -> lib ) = FILEcreate( fileName ) ) ) {
        return;
    }
    fprintf( schemaLibFile, 
						"\n// in the exp2cxx source code, this file is generally referred to as files->lib or libfile\n" );

    fprintf( schemaLibFile, 
						"#include \"%s.h\" n", expressFileName );

    // 3. header for namespace to contain all formerly-global variables
    sprintf( fileName, "%sNames.h", expressFileName );
    if( !( files->names = FILEcreate( fileName ) ) ) {
        return;
    }
    fprintf( schemaLibFile, 
						"#include \"%sNames.h\"\n", expressFileName );
    fprintf( files->names, 
						"\n// In the exp2cxx source code, this file is referred to as files->names.\n"
						"// This line printed at %s:%d (one of two possible locations).\n\n", __FILE__, __LINE__ );
    fprintf( files->names, 
						"//this file contains a namespace for all formerly-global variables\n\n" );
    //the next line in this file depends on the schema name, so printing continues in the while loop ~25 lines below

    /*  4.  source code to initialize entity registry   */
    /*  prints header of file for input function    */

    sprintf( np, "init.cc" );
    if( !( schemaInitFile = ( files -> init ) = FILEcreate( fileName ) ) ) {
        return;
    }
    fprintf( schemaInitFile, 
						"\n// in the exp2cxx source code, this file is generally referred to as files->init or initfile\n" );
    fprintf( schemaInitFile, 
						"#include \"%s.h\"\n\n", expressFileName );
    fprintf( schemaInitFile, 
						"void \n"
						"%sInit (Registry& reg)\n"
						"{\n", expressFileName );

    /**********  record in files relating to entire input   ***********/

    /*  add to schema's include and initialization file */
    fprintf( allSchema_hFile, 
						"#include \"%sNames.h\"\n", expressFileName );
    fprintf( allSchema_hFile, 
						"#include \"%s.h\"\n\n", expressFileName );
    fprintf( allSchema_hFile, 
						"extern void %sInit (Registry & r);\n", expressFileName );
    fprintf( allSchema_ccFile, 
						"         extern void %sInit (Registry & r);\n", expressFileName );
    fprintf( allSchema_ccFile, 
						"         %sInit (reg);\n", expressFileName );

    /**********  do all schemas ***********/
    DICTdo_type_init( express->symbol_table, &dictEntry, OBJ_SCHEMA );
    while( ( schema = ( Scope )DICTdo( &dictEntry ) ) != 0 ) {
        numberAttributes( schema );
    }

    DICTdo_init( express->symbol_table, &dictEntry );
    bool first = true;
    while( 0 != ( schema = ( Scope )DICTdo( &dictEntry ) ) ) {
        if( !first ) {
            fprintf( files->names, 
										"} //namespace %s\n", SCHEMAget_name( schema ) );
        }
        first = false;
        fprintf( files->names, 
								"namespace %s {\n\n", SCHEMAget_name( schema ) );
        fprintf( files->names, 
								"    extern Schema * schema;\n\n" );

        SCOPEPrint( schema, files, schema, &complexCollect, 0 );
    }

    /**********  close the files    ***********/
//    closeUnityFiles( files );
    FILEclose( schemaLibFile );
    FILEclose( schemaIncFile );
    INITFileFinish( schemaInitFile, schema );
}

#endif
