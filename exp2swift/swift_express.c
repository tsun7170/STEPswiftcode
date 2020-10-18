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


