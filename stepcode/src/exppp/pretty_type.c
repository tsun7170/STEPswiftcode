/** \file pretty_type.c
 * split out of exppp.c 9/21/13
 */

#include <stdlib.h>

#include <express/error.h>
#include <sc_memmgr.h>

#include "exppp.h"
#include "pp.h"
#include "pretty_where.h"
#include "pretty_expr.h"
#include "pretty_type.h"

/** print a type definition.  I.e., a TYPE statement */
void TYPE_out( Type t, int level ) {
//    first_newline();
    exppp_ref_info( &t->symbol );

    raw( "%*sTYPE %s =", level, "", t->symbol.name );
    if( TYPEget_head( t ) ) {
			//*TY2020/09/20
//        wrap( " %s", TYPEget_name( TYPEget_head( t ) ) );
			TYPE_head_out(TYPEget_head( t ), level + exppp_nesting_indent);
    } else {
        TYPE_body_out( t, level + exppp_nesting_indent );
    }

    raw( ";\n" );

    WHERE_out( t->where, level );

    raw( "%*sEND_TYPE;", level, "" );
    tail_comment( t->symbol );
}

/** prints type description (preceded by a space).
 * I.e., the type of an attribute or other object
 */
void TYPE_head_out( Type t, int level ) {
	if( t->symbol.name ) {
		int old_indent = indent2;
		if( indent2 + ( int ) strlen( t->symbol.name ) > exppp_linelength ) {
			indent2 = ( indent2 + level ) / 2;
		}
		wrap( " %s", t->symbol.name );
		
		//*TY2020/09/19
		if( exppp_annotate_extensively ){
			Type fundamental = TYPEget_fundamental_type(t);
			raw("(*%s*) ", TYPEget_kind(fundamental));
		}
		
		indent2 = old_indent;
	} else {
		TYPE_body_out( t, level );
	}
}

void TYPEunique_or_optional_out( TypeBody tb ) {
    if( tb->flags.unique ) {
        wrap( " UNIQUE" );
    }
    if( tb->flags.optional ) {
        wrap( " OPTIONAL" );
    }
}

void TYPE_body_out( Type t, int level ) {
    bool first_time = true;

    Expression expr;
    DictionaryEntry de;

    TypeBody tb = TYPEget_body( t );

    switch( tb->type ) {
			case indeterminate_:	//*TY2020/12/31
				wrap( " (*INDETERMINATE*)" );
				break;
				
        case integer_:
            wrap( " INTEGER" );
            break;
        case real_:
            wrap( " REAL" );
            break;
        case string_:
            wrap( " STRING" );
            break;
        case binary_:
            wrap( " BINARY" );
            break;
        case boolean_:
            wrap( " BOOLEAN" );
            break;
        case logical_:
            wrap( " LOGICAL" );
            break;
        case number_:
            wrap( " NUMBER" );
            break;
        case entity_:
				if( tb->entity ){
					wrap( " %s (*ENTITY*)", tb->entity->symbol.name );
				}
				else {
					wrap( " <complex>(*ENTITY*)" );
				}
            break;
        case aggregate_:
        case array_:
        case bag_:
        case set_:
        case list_:
            switch( tb->type ) {
                /* ignore the aggregate bounds for now */
                case aggregate_:
                    wrap( " AGGREGATE" );
								//*TY2020/08/08
                    if( TYPEhas_tag(tb) ) {
                        wrap( ":%s", tb->tag->symbol.name );
                    }
                    wrap( " OF" );
                    break;

                case array_:
                    wrap( " ARRAY" );
                    EXPRbounds_out( tb );
                    wrap( " OF" );
                    TYPEunique_or_optional_out( tb );
                    break;

                case bag_:
                    wrap( " BAG" );
                    EXPRbounds_out( tb );
                    wrap( " OF" );
                    break;

                case set_:
                    wrap( " SET" );
                    EXPRbounds_out( tb );
                    wrap( " OF" );
                    break;

                case list_:
                    wrap( " LIST" );
                    EXPRbounds_out( tb );
                    wrap( " OF" );
                    TYPEunique_or_optional_out( tb );
                    break;
                default:
                    fprintf( stderr, "exppp: Reached default case, %s:%d", __FILE__, __LINE__ );
                    abort();
            }

            TYPE_head_out( tb->base, level );
            break;
        case enumeration_: {
            int i, count = 0;
            char ** names;

            /*
             * write names out in original order by first bucket sorting
             * to a temporary array.  This is trivial since all buckets
             * will get filled with one and only one object.
             */
            DICTdo_type_init( t->symbol_table, &de, OBJ_EXPRESSION );
            while( 0 != DICTdo( &de )  ) {
                count++;
            }
						assert( count > 0 );
            names = ( char ** )sc_calloc( count, sizeof( char * ) );
            DICTdo_type_init( t->symbol_table, &de, OBJ_EXPRESSION );
            while( 0 != ( expr = ( Expression )DICTdo( &de ) ) ) {
                names[expr->u.integer - 1] = expr->symbol.name;
            }

            wrap( " ENUMERATION OF\n" );

            for( i = 0; i < count; i++ ) {
                /* finish line from previous enum item */
                if( !first_time ) {
                    raw( ",\n" );
                }

                /* start new enum item */
                if( first_time ) {
                    raw( "%*s( ", level, "" );
                    first_time = false;
                } else {
                    raw( "%*s ", level, "" );
                }
                raw( names[i] );
            }
            raw( " )" );
            sc_free( ( char * )names );
        }
        break;
        case select_:
            wrap( " SELECT\n" );
            LISTdo( tb->list, type, Type )
            /* finish line from previous entity */
            if( !first_time ) {
                raw( ",\n" );
            }

            /* start new entity */
            if( first_time ) {
                raw( "%*s( ", level, "" );
                first_time = false;
            } else {
                raw( "%*s ", level, "" );
            }
            raw( type->symbol.name );
				
						//*TY2020/07/28
						if( TYPEis_entity(type)) {
							raw(" (*ENTITY*)");
						}
						else if( TYPEis_select(type) ) {
							raw(" (*SELECT*)");
						}
						else if( TYPEis_enumeration(type) ) {
							raw(" (*ENUM*)");
						}
						else {
							raw( " (*TYPE*)" );
						}

						LISTod

            /* if empty, force a left paren */
            if( first_time ) {
                ERRORreport_with_symbol( ERROR_select_empty, &error_sym, t->symbol.name );
                raw( "%*s( ", level, "" );
            }
            raw( " )" );
            break;
        case generic_:
            wrap( " GENERIC" );
            if( TYPEhas_tag(tb) ) {
                wrap( ":%s", tb->tag->symbol.name );
            }
            break;
				
				//*TY2020/09/19
			case attribute_:
				wrap("<attribute>");
				break;
			case identifier_:
				wrap("<identifier>");
				break;
			case runtime_:
				wrap("<runtime>");
				break;
			case unknown_:
				wrap("<unknown>");
				break;
				
        default:
            wrap( " (* unknown type %d *)", tb->type );
    }

    if( tb->precision ) {
        wrap( " ( " );
        EXPR_out( tb->precision, 0 );
        raw( " )" );
    }
    if( tb->flags.fixed ) {
        wrap( " FIXED" );
    }
}

char * TYPEto_string( Type t ) {
    if( prep_string() ) {
        return placeholder;
    }
    TYPE_out( t, 0 );
    return ( finish_string() );
}

/** return length of buffer used */
int TYPEto_buffer( Type t, char * buffer, int length ) {
    if( prep_buffer( buffer, length ) ) {
        return -1;
    }
    TYPE_out( t, 0 );
    return( finish_buffer() );
}

void TYPEout( Type t ) {
    prep_file();
    TYPE_out( t, 0 );
    finish_file();
}

char * TYPEhead_to_string( Type t ) {
    if( prep_string() ) {
        return placeholder;
    }
    TYPE_head_out( t, 0 );
    return ( finish_string() );
}

/** return length of buffer used */
int TYPEhead_to_buffer( Type t, char * buffer, int length ) {
    if( prep_buffer( buffer, length ) ) {
        return -1;
    }
    TYPE_out( t, 0 );
    return( finish_buffer() );
}

void TYPEhead_out( Type t ) {
    prep_file();
    TYPE_head_out( t, 0 );
    finish_file();
}

char * TYPEbody_to_string( Type t ) {
    if( prep_string() ) {
        return placeholder;
    }
    TYPE_body_out( t, 0 );
    return ( finish_string() );
}

/** return length of buffer used */
int TYPEbody_to_buffer( Type t, char * buffer, int length ) {
    if( prep_buffer( buffer, length ) ) {
        return -1;
    }
    TYPE_body_out( t, 0 );
    return( finish_buffer() );
}

void TYPEbody_out( Type t ) {
    prep_file();
    TYPE_body_out( t, 0 );
    finish_file();
}
