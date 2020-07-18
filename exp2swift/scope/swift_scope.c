//
//  swift_scope.c
//  exp2swift
//
//  Created by Yoshida on 2020/06/13.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

#include "swift_scope.h"

#if 0
/** ****************************************************************
 ** Procedure:  SCOPEPrint
 ** Parameters: const Scope scope   - scope to print
 **     FILE*       file    - file on which to print
 ** Returns:
 ** Description:  cycles through the scopes of the input Express schema
 ** Side Effects:  calls functions for processing entities and types
 ** Status:  working 1/15/91
 ** --  it's still not clear how include files should be organized
 ** and what the relationship is between this organization and the
 ** organization of the schemas in the input Express
 ******************************************************************/
void SCOPEPrint( Scope scope, FILES * files, Schema schema, ComplexCollect * complexCollect, int suffixIndex ) {
    Linked_List entityList = SCOPEget_entities_superclass_order( scope );
    DictionaryEntry dictEntry;
    Type i;
    int redefinedEnums = 0;

    if( suffixIndex <= 1 ) {
        /* This will be the case if this is the first time we are generating a
        ** file for this schema.  (cnt = the file suffix.  If it = 1, it's the
        ** first of multiple; if it = 0, it's the only one.)  Basically, this
        ** if generates all the files which are not divided (into _1, _2 ...)
        ** during multiple passes.  This includes Sdaiclasses.h, SdaiAll.cc,
        ** and the Sdaixxx.init.cc files. */

        fprintf( files -> lib, 
								"\n"
								"Schema * %s::schema = 0;\n", SCHEMAget_name( schema ) );

        /* Do \'new\'s for types descriptors (in SdaiAll.cc (files->create)),
           and the externs typedefs, and incomplete descriptors (in Sdai-
           classes.h (files->classes)). */
        fprintf( files->create, 
								"\n"
								"  //  *****  Initialize the Types\n" );
        fprintf( files->classes, 
								"\n"
								"// Types:\n" );
			
        SCOPEdo_types( scope, type, dictEntry ) {
            //TYPEprint_new moved to TYPEPrint_cc and TYPEprint_descriptions in classes_type.c
            TYPEprint_typedefs( type, files->classes );
					
            //print in namespace. Some logic copied from TypeDescriptorName()
            fprintf( files->names, 
										"    extern SC_SCHEMA_EXPORT %s * %s%s;\n", 
										GetTypeDescriptorName( type ), TYPEprefix( type ), TYPEget_name( type ) );
        } SCOPEod

        fprintf( files->classes, 
								"\n"
								"// Entity class typedefs:" );
        LISTdo( entityList, e, Entity ) {
            ENTITYprint_classes( e, files->classes );
        } LISTod
    }

    /* fill in the values for the type descriptors and print the enumerations */
    fprintf( files -> inc, 
						"\n"
						"/*    **************  TYPES      */\n" );
    fprintf( files -> lib, 
						"\n"
						"/*    **************  TYPES      */\n" );
	
    /* The following was `SCOPEdo_types( scope, t, de ) ... SCOPEod;`
     * Modified Jan 2012 by MAP - moving enums to own dictionary */
    if( scope->enum_table ) {
        HASHlistinit_by_type( scope->enum_table, &dictEntry, OBJ_TYPE );
        Type type;
        while( 0 != ( type = ( Type ) DICTdo( &dictEntry ) ) ) {
            // First check for one exception:  Say enumeration type B is defined
            // to be a rename of enum A.  If A is in this schema but has not been
            // processed yet, we must wait till it's processed first.  The reason
            // is because B will basically be defined with a couple of typedefs to
            // the classes which represent A.  (To simplify, we wait even if A is
            // in another schema, so long as it's been processed.)
            if( ( type->search_id == CANPROCESS )
                    && ( TYPEis_enumeration( type ) )
                    && ( ( i = TYPEget_ancestor( type ) ) != NULL )
                    && ( i->search_id >= CANPROCESS ) ) {
                redefinedEnums = 1;
            }
        }
    }

    SCOPEdo_types( scope, type, dictEntry ) {
        /* NOTE the following comment seems to contradict the logic below it (... && !( TYPEis_enumeration( t ) && ...)
        // Do the non-redefined enumerations:*/
        if( ( type->search_id == CANPROCESS )
                && !( TYPEis_enumeration( type ) 
								&& TYPEget_head( type ) ) ) {
            TYPEprint_descriptions( type, files, schema );
            if( !TYPEis_select( type ) ) {
                // Selects have a lot more processing and are done below.
                type->search_id = PROCESSED;
            }
        }
    } SCOPEod

    if( redefinedEnums ) {
        // Here we process redefined enumerations.  See note, 2 loops ago.
        fprintf( files->inc, 
								"//    ***** Redefined Enumerations:\n" );
			
        /* The following was `SCOPEdo_types( scope, t, de ) ... SCOPEod;`
        * Modified Jan 2012 by MAP - moving enums to own dictionary */
        HASHlistinit_by_type( scope->enum_table, &dictEntry, OBJ_TYPE );
        Type type;
        while( 0 != ( type = ( Type ) DICTdo( &dictEntry ) ) ) {
            if( type->search_id == CANPROCESS && TYPEis_enumeration( type ) ) {
                TYPEprint_descriptions( type, files, schema );
                type->search_id = PROCESSED;
            }
        }
    }

    /*  do the select definitions next, since they depend on the others  */
    fprintf( files->inc, 
						"\n"
						"//        ***** Build the SELECT Types          \n" );
	
    // Note - say we have sel B, rename of sel A (as above by enum's).  Here
    // we don't have to worry about printing B before A.  This is checked in
    // TYPEselect_print().
    SCOPEdo_types( scope, type, dictEntry ) {
        if( type->search_id == CANPROCESS ) {
            // Only selects haven't been processed yet and may still be set to
            // CANPROCESS.
            if( TYPEis_select( type ) ) {
                TYPEselect_print( type, files, schema );
            }
            if( TYPEis_enumeration( type ) ) {
                TYPEprint_descriptions( type, files, schema );
            }

            type->search_id = PROCESSED;
        }
    } SCOPEod

    fprintf( files -> inc, 
						"\n"
						"/*        **************  ENTITIES          */\n" );
    fprintf( files -> lib, 
						"\n"
						"/*        **************  ENTITIES          */\n" );

    fprintf( files->inc, 
						"\n"
						"//        ***** Print Entity Classes          \n" );
    LISTdo( entityList, e, Entity ) {
        if( e->search_id == CANPROCESS ) {
            ENTITYPrint( e, files, schema, complexCollect->externMapping( ENTITYget_name( e ) ) );
            e->search_id = PROCESSED;
        }
    } LISTod

    if( suffixIndex <= 1 ) {
        int index = 0;

        // Do the model stuff:
        fprintf( files->inc, 
								"\n"
								"//        ***** generate Model related pieces\n" );
        fprintf( files->inc, 
								"\n"
								"class SdaiModel_contents_%s : public SDAI_Model_contents {\n", SCHEMAget_name( schema ) );
        fprintf( files -> inc, 
								"\n"
								"  public:\n" );
        fprintf( files -> inc, 
								"    SdaiModel_contents_%s();\n", SCHEMAget_name( schema ) );
        LISTdo( entityList, e, Entity ) {
            MODELprint_new( e, files );
        } LISTod

        fprintf( files->inc, 
								"\n"
								"};\n\n" );

        fprintf( files->inc, 
								"typedef       SdaiModel_contents_%s *     SdaiModel_contents_%s_ptr;\n", 
								SCHEMAget_name( schema ), SCHEMAget_name( schema ) );
        fprintf( files->inc, 
								"typedef const SdaiModel_contents_%s *     SdaiModel_contents_%s_ptr_c;\n", 
								SCHEMAget_name( schema ), SCHEMAget_name( schema ) );
        fprintf( files->inc, 
								"typedef       SdaiModel_contents_%s_ptr   SdaiModel_contents_%s_var;\n", 
								SCHEMAget_name( schema ), SCHEMAget_name( schema ) );
        fprintf( files->inc, 
								"SDAI_Model_contents_ptr create_SdaiModel_contents_%s();\n", 
								SCHEMAget_name( schema ) );

        fprintf( files->lib, 
								"\n"
								"SDAI_Model_contents_ptr create_SdaiModel_contents_%s() {\n", 
								SCHEMAget_name( schema ) );
        fprintf( files->lib, 
								"    return new SdaiModel_contents_%s;\n"
								"}\n", 
								SCHEMAget_name( schema ) );

        fprintf( files->lib, 
								"\n"
								"SdaiModel_contents_%s::SdaiModel_contents_%s() {\n", 
								SCHEMAget_name( schema ), SCHEMAget_name( schema ) );
        fprintf( files->lib, 
								"    SDAI_Entity_extent_ptr eep = (SDAI_Entity_extent_ptr)0;\n\n" );
			
        LISTdo( entityList, entity, Entity ) {
            MODELPrintConstructorBody( entity, files, schema );
        } LISTod
			
        fprintf( files -> lib, 
								"}\n" );
			
        index = 0;
        LISTdo( entityList, entity, Entity ) {
            MODELPrint( entity, files, schema, index );
            index++;
        } LISTod
    }

    LISTfree( entityList );
}
#endif
