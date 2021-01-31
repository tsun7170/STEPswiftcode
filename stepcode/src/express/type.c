

/** **********************************************************************
** Module:  Type \file type.c
This module implements the type abstraction.  It is
**  rather unpleasant, since this abstraction is quite well suited
**  to an object-oriented environment with inheritance.
** Constants:
**  TYPE_AGGREGATE      - generic general aggregate
**  TYPE_BINARY         - binary type
**  TYPE_BOOLEAN        - boolean type
**  TYPE_GENERIC        - generic type
**  TYPE_INTEGER        - integer type with default precision
**  TYPE_LOGICAL        - logical type
**  TYPE_NULL           - the null type
**  TYPE_NUMBER         - number type
**  TYPE_REAL           - real type with default precision
**  TYPE_SET_OF_GENERIC - type for unconstrained set of generic items
**  TYPE_STRING         - string type with default precision
**
************************************************************************/

/*
 * This code was developed with the support of the United States Government,
 * and is not subject to copyright.
 *
 * $Log: type.c,v $
 * Revision 1.12  1997/01/21 19:19:51  dar
 * made C++ compatible
 *
 * Revision 1.11  1995/03/09  18:44:45  clark
 * various fixes for caddetc - before interface clause changes
 *
 * Revision 1.10  1994/11/10  19:20:03  clark
 * Update to IS
 *
 * Revision 1.9  1994/05/11  19:51:24  libes
 * numerous fixes
 *
 * Revision 1.8  1993/10/15  18:48:48  libes
 * CADDETC certified
 *
 * Revision 1.6  1993/01/19  22:16:09  libes
 * *** empty log message ***
 *
 * Revision 1.5  1992/09/16  18:23:45  libes
 * added some functions for easier access to base types
 *
 * Revision 1.4  1992/08/18  17:13:43  libes
 * rm'd extraneous error messages
 *
 * Revision 1.3  1992/06/08  18:06:57  libes
 * prettied up interface to print_objects_when_running
 *
 * Revision 1.2  1992/05/31  08:35:51  libes
 * multiple files
 *
 * Revision 1.1  1992/05/28  03:55:04  libes
 * Initial revision
 *
 * Revision 1.9  1992/05/14  10:14:06  libes
 * don't remember
 *
 * Revision 1.8  1992/05/10  06:03:18  libes
 * cleaned up OBJget_symbol
 *
 * Revision 1.7  1992/05/10  01:42:27  libes
 * does enums and repeat properly
 *
 * Revision 1.6  1992/05/05  19:52:11  libes
 * final alpha
 *
 * Revision 1.5  1992/02/19  15:48:46  libes
 * changed types to enums & flags
 *
 * Revision 1.4  1992/02/17  14:33:41  libes
 * lazy ref/use evaluation now working
 *
 * Revision 1.3  1992/02/12  07:05:45  libes
 * do sub/supertype
 *
 * Revision 1.2  1992/02/09  00:50:20  libes
 * does ref/use correctly
 *
 * Revision 1.1  1992/02/05  08:34:24  libes
 * Initial revision
 *
 * Revision 1.0.1.1  1992/01/22  02:47:57  libes
 * copied from ~pdes
 *
 * Revision 4.8  1991/11/14  07:37:50  libes
 * added TYPEget/put_original_type
 *
 * Revision 4.7  1991/09/16  23:13:12  libes
 * added print functions
 *
 * Revision 4.6  1991/06/14  20:48:16  libes
 * added binary type
 *
 * Revision 4.5  1991/01/24  22:20:36  silver
 * merged changes from NIST and SCRA
 * SCRA changes are due to DEC ANSI C compiler tests.
 *
 * Revision 4.4  91/01/08  18:56:05  pdesadmn
 * Initial - Beta checkin at SCRA
 *
 * Revision 4.3  90/09/14  16:02:28  clark
 * Initial checkin at SCRA
 *
 * Revision 4.3  90/09/14  16:02:28  clark
 * initial checkin at SCRA
 *
 * Revision 4.3  90/09/14  16:02:28  clark
 * Reintroduce ENT_TYPEget_entity
 *
 * Revision 4.2  90/09/14  09:33:20  clark
 * Add Class_{Boolean,Generic,Logical,Number}_Type
 * Fix TYPE_equal
 *
 * Revision 4.1  90/09/13  15:13:21  clark
 * BPR 2.1 alpha
 *
 */

#include <assert.h>
#include <sc_memmgr.h>
#include "express/type.h"
#include "express/symbol.h"	//*TY2021/01/30

/* Very commonly-used read-only types */
/* non-constant versions probably aren't necessary? */
Type Type_Bad;
Type Type_Unknown;
Type Type_Dont_Care;
Type Type_Runtime; /* indicates that this object can't be */
/* calculated now but must be deferred */
/* til (the mythical) runtime */
Type Type_Binary;
Type Type_Boolean;
Type Type_Enumeration;
Type Type_Expression;
Type Type_Aggregate;
Type Type_Repeat;
Type Type_Integer;
Type Type_Number;
Type Type_Real;
Type Type_String;
Type Type_String_Encoded;
Type Type_Logical;
Type Type_Set;
Type Type_Attribute;
Type Type_Entity;
Type Type_Funcall;
Type Type_Generic;
Type Type_Identifier;
Type Type_Oneof;
Type Type_Query;
Type Type_Self;
Type Type_Set_Of_String;
Type Type_Set_Of_Generic;
Type Type_Bag_Of_Generic;

//*TY2020/08/02
Type Type_Indeterminate;
Type Type_List_Of_Generic;
Type Type_Aggregate_Of_Generic;
Type Type_Set_Of_GenericEntity;
Type Type_Bag_Of_GenericEntity;
Type Type_List_Of_GenericEntity;
Type Type_Aggregate_Of_GenericEntity;


struct freelist_head TYPEHEAD_fl;
struct freelist_head TYPEBODY_fl;

Error ERROR_corrupted_type = ERROR_none;

static Error ERROR_undefined_tag;
/**
 * create a type with no symbol table
 */
Type TYPEcreate_nostab( struct Symbol_ *symbol, Scope scope, char objtype ) {
    Type t = SCOPEcreate_nostab( OBJ_TYPE );
    TypeHead th = TYPEHEAD_new();

    t->u.type = th;
	t->u_tag = scope_is_type;	//*TY2020/08/02
    t->symbol = *symbol;
    DICTdefine( scope->symbol_table, symbol->name, ( Generic )t, &t->symbol, objtype );

    return t;
}

/**
 * create a type but this is just a shell, either to be completed later
 * such as enumerations (which have a symbol table added later)
 * or to be used as a type reference
 */
Type TYPEcreate_name( Symbol * symbol ) {
    Scope s = SCOPEcreate_nostab( OBJ_TYPE );
    TypeHead t = TYPEHEAD_new();

    s->u.type = t;
	s->u_tag = scope_is_type;	//*TY2020/08/02
    s->symbol = *symbol;
    return s;
}

Symbol* SYMBOLcreate_implicit_tag( int tag_no, int line, const char * filename ) {
#define TAG_NAME_SIZE 16
	char* tag_name = (char*)sc_malloc(TAG_NAME_SIZE);
	snprintf(tag_name, TAG_NAME_SIZE, "_T%d", tag_no);
	Symbol* tag_sym = SYMBOLcreate(tag_name, line, filename);
	return tag_sym;
#undef TAG_NAME_SIZE
}

Type TYPEcreate_user_defined_tag( Type base, Scope scope, struct Symbol_ *symbol ) {
    Type t;
    extern int tag_count;

    t = ( Type )DICTlookup( scope->symbol_table, symbol->name );
    if( t ) {
        if( DICT_type == OBJ_TAG ) {
            return( t );
        } else {
            /* easiest to just generate the error this way!
             * following call WILL fail intentionally
             */
            DICTdefine( scope->symbol_table, symbol->name, 0, symbol, OBJ_TAG );
            return( 0 );
        }
    }

    /* tag is undefined
     * if we are outside a formal parameter list (hack, hack)
     * then we can only refer to existing tags, so produce an error
     */
    if( tag_count < 0 ) {
        ERRORreport_with_symbol( ERROR_undefined_tag, symbol,
                                 symbol->name );
        return( 0 );
    }

    /* otherwise, we're in a formal parameter list,
     * so it's ok to define it
     */
    t = TYPEcreate_nostab( symbol, scope, OBJ_TAG );
    t->u.type->head = base;

    /* count unique type tags inside PROC and FUNC headers */
    tag_count++;

    return( t );
}

Type TYPEcreate( enum type_enum type ) {
    TypeBody tb = TYPEBODYcreate( type );
    Type t = TYPEcreate_from_body_anonymously( tb );
    return( t );
}

Type TYPEcreate_from_body_anonymously( TypeBody tb ) {
    Type t = SCOPEcreate_nostab( OBJ_TYPE );
    TypeHead th = TYPEHEAD_new();

    t->u.type = th;
	t->u_tag = scope_is_type;	//*TY2020/08/02
    t->u.type->body = tb;
    t->symbol.name = 0;
    SYMBOLset( t );
    return t;
}

TypeBody TYPEBODYcreate( enum type_enum type ) {
    TypeBody tb = TYPEBODY_new();
    tb->type = type;
    return tb;
}

//*TY2021/01/19
Type TYPEcreate_aggregate( enum type_enum aggr_type, Type base_type, Expression bound1, Expression bound2, bool unique, bool optional){
	Type aggr = TYPEcreate(aggr_type);
	aggr->symbol.filename = "_AGGREGATE_";
	aggr->symbol.line = 0;
	TypeBody tb = TYPEget_body(aggr);
	tb->base = base_type;
	tb->flags.unique = unique;
	tb->flags.optional = optional;
	tb->lower = bound1;
	tb->upper = bound2;
	return aggr;	
}



/**
 * return true if "type t" inherits from "enum type_enum"
 * may need to be implemented for more types
 */
#define TYPE_inherits_from(t,e) ((t) && TYPEinherits_from((t),(e)))

bool TYPEinherits_from( Type t, enum type_enum e ) {
    TypeBody tb = t->u.type->body;
    assert( ( t->type == OBJ_TYPE ) && ( tb ) && "Not a Type!" );
    switch( e ) {
        case aggregate_:
            if( tb->type == aggregate_ ||
                    tb->type == array_ ||
                    tb->type == bag_ ||
                    tb->type == set_ ||
                    tb->type == list_ ) {
                return true;
            } else {
                return( TYPE_inherits_from( tb->base, e ) );
            }
        case array_:
            return( ( tb->type == array_ ) ? true : TYPE_inherits_from( tb->base, e ) );
        case bag_:
            return( ( tb->type == bag_ ||
                      tb->type == set_ ) ? true : TYPE_inherits_from( tb->base, e ) );
        case set_:
            return( ( tb->type == set_ ) ? true : TYPE_inherits_from( tb->base, e ) );
        case list_:
            return( ( tb->type == list_ ) ? true : TYPE_inherits_from( tb->base, e ) );
        default:
            break;
    }
    return ( tb->type == e );
}

#if 0
case binary_:
return( ( t->type == binary_ ) ? true : TYPEinherits_from( t->base, e ) );
case integer_:
return( ( t->type == integer_ ) ? true : TYPEinherits_from( t->base, e ) );
case real_:
return( ( t->type == real_ ) ? true : TYPEinherits_from( t->base, e ) );
case string_:
return( ( t->type == string_ ) ? true : TYPEinherits_from( t->base, e ) );
case logical_:
return( ( t->type == logical_ ) ? true : TYPEinherits_from( t->base, e ) );
case boolean_:
return( ( t->type == boolean_ ) ? true : TYPEinherits_from( t->base, e ) );
default:
return( false );
}
}
#endif

Symbol * TYPE_get_symbol( Generic t ) {
    return( &( ( Type )t )->symbol );
}


/** Initialize the Type module */
void TYPEinitialize() {
	//*TY2020/08/26
	current_filename = __FILE__;
	yylineno = __LINE__;

		MEMinitialize( &TYPEHEAD_fl, sizeof( struct TypeHead_ ), 500, 100 );
    MEMinitialize( &TYPEBODY_fl, sizeof( struct TypeBody_ ), 200, 100 );
    OBJcreate( OBJ_TYPE, TYPE_get_symbol, "type", OBJ_TYPE_BITS );
    /*  OBJcreate(OBJ_TYPE,TYPE_get_symbol,"(headless) type", OBJ_UNFINDABLE_BITS);*/
    OBJcreate( OBJ_TAG, TYPE_get_symbol, "tag", OBJ_TYPE_BITS );

    /* Very commonly-used read-only types */
    Type_Unknown = TYPEcreate( unknown_ );
    Type_Dont_Care = TYPEcreate( special_ );
    Type_Bad = TYPEcreate( special_ );
    Type_Runtime = TYPEcreate( runtime_ );

    Type_Enumeration = TYPEcreate( enumeration_ );
    Type_Enumeration->u.type->body->flags.shared = 1;
    resolved_all( Type_Enumeration );

    Type_Expression = TYPEcreate( op_ );
    Type_Expression->u.type->body->flags.shared = 1;

    Type_Aggregate = TYPEcreate( aggregate_ );
    Type_Aggregate->u.type->body->flags.shared = 1;
    Type_Aggregate->u.type->body->base = Type_Runtime;

    Type_Integer = TYPEcreate( integer_ );
    Type_Integer->u.type->body->flags.shared = 1;
    resolved_all( Type_Integer );

    Type_Real = TYPEcreate( real_ );
    Type_Real->u.type->body->flags.shared = 1;
    resolved_all( Type_Real );

    Type_Number = TYPEcreate( number_ );
    Type_Number->u.type->body->flags.shared = 1;
    resolved_all( Type_Number );

    Type_String = TYPEcreate( string_ );
    Type_String->u.type->body->flags.shared = 1;
    resolved_all( Type_String );

    Type_String_Encoded = TYPEcreate( string_ );
    Type_String_Encoded->u.type->body->flags.shared = 1;
    Type_String_Encoded->u.type->body->flags.encoded = 1;
    resolved_all( Type_String );

    Type_Logical = TYPEcreate( logical_ );
    Type_Logical->u.type->body->flags.shared = 1;
    resolved_all( Type_Logical );

    Type_Binary = TYPEcreate( binary_ );
    Type_Binary->u.type->body->flags.shared = 1;
    resolved_all( Type_Binary );

    Type_Number = TYPEcreate( number_ );
    Type_Number->u.type->body->flags.shared = 1;
    resolved_all( Type_Number );

    Type_Boolean = TYPEcreate( boolean_ );
    Type_Boolean->u.type->body->flags.shared = 1;
    resolved_all( Type_Boolean );

	Type_Attribute = TYPEcreate( attribute_ );
	Type_Attribute->u.type->body->flags.shared = 1;
	resolved_all( Type_Attribute );

	Type_Entity = TYPEcreate( entity_ );
	Type_Entity->u.type->body->flags.shared = 1;
	resolved_all( Type_Entity );

	Type_Funcall = TYPEcreate( funcall_ );
	Type_Funcall->u.type->body->flags.shared = 1;
	resolved_all( Type_Funcall );

    Type_Generic = TYPEcreate( generic_ );
    Type_Generic->u.type->body->flags.shared = 1;
	resolved_all( Type_Generic );

	Type_Identifier = TYPEcreate( identifier_ );
	Type_Identifier->u.type->body->flags.shared = 1;
	resolved_all( Type_Identifier );

	Type_Repeat = TYPEcreate( integer_ );
	Type_Repeat->u.type->body->flags.shared = 1;
	Type_Repeat->u.type->body->flags.repeat = 1;
	resolved_all( Type_Repeat );

	Type_Oneof = TYPEcreate( oneof_ );
	Type_Oneof->u.type->body->flags.shared = 1;
	resolved_all( Type_Oneof );

	Type_Query = TYPEcreate( query_ );
	Type_Query->u.type->body->flags.shared = 1;
	resolved_all( Type_Query );

	Type_Self = TYPEcreate( self_ );
	Type_Self->u.type->body->flags.shared = 1;
	resolved_all( Type_Self );
	
	//*TY2020/12/31
	Type_Indeterminate = TYPEcreate( indeterminate_ );
	Type_Indeterminate->u.type->body->flags.shared = 1;
	resolved_all( Type_Indeterminate );
	
	
    Type_Set_Of_String = TYPEcreate( set_ );
    Type_Set_Of_String->u.type->body->flags.shared = 1;
    Type_Set_Of_String->u.type->body->base = Type_String;
	resolved_all( Type_Set_Of_String );

    Type_Set_Of_Generic = TYPEcreate( set_ );
    Type_Set_Of_Generic->u.type->body->flags.shared = 1;
    Type_Set_Of_Generic->u.type->body->base = Type_Generic;
	resolved_all( Type_Set_Of_Generic );

    Type_Bag_Of_Generic = TYPEcreate( bag_ );
    Type_Bag_Of_Generic->u.type->body->flags.shared = 1;
    Type_Bag_Of_Generic->u.type->body->base = Type_Generic;
	resolved_all( Type_Bag_Of_Generic );

	//*TY2020/08/02
	Type_List_Of_Generic = TYPEcreate( list_ );
	Type_List_Of_Generic->u.type->body->flags.shared = 1;
	Type_List_Of_Generic->u.type->body->base = Type_Generic;
	resolved_all( Type_List_Of_Generic );
//
	Type_Aggregate_Of_Generic = TYPEcreate( aggregate_ );
	Type_Aggregate_Of_Generic->u.type->body->flags.shared = 1;
	Type_Aggregate_Of_Generic->u.type->body->base = Type_Generic;
	resolved_all( Type_Aggregate_Of_Generic );
//
	Type_Set_Of_GenericEntity = TYPEcreate( set_ );
	Type_Set_Of_GenericEntity->u.type->body->flags.shared = 1;
	Type_Set_Of_GenericEntity->u.type->body->base = Type_Entity;
	resolved_all( Type_Set_Of_GenericEntity );
//
	Type_Bag_Of_GenericEntity = TYPEcreate( bag_ );
	Type_Bag_Of_GenericEntity->u.type->body->flags.shared = 1;
	Type_Bag_Of_GenericEntity->u.type->body->base = Type_Entity;
	resolved_all( Type_Bag_Of_GenericEntity );
//
	Type_List_Of_GenericEntity = TYPEcreate( list_ );
	Type_List_Of_GenericEntity->u.type->body->flags.shared = 1;
	Type_List_Of_GenericEntity->u.type->body->base = Type_Entity;
	resolved_all( Type_List_Of_GenericEntity );
//
	Type_Aggregate_Of_GenericEntity = TYPEcreate( aggregate_ );
	Type_Aggregate_Of_GenericEntity->u.type->body->flags.shared = 1;
	Type_Aggregate_Of_GenericEntity->u.type->body->base = Type_Entity;
	resolved_all( Type_Aggregate_Of_GenericEntity );
	
	

    ERROR_corrupted_type =
        ERRORcreate( "Corrupted type in %s", SEVERITY_DUMP );

    ERROR_undefined_tag =
        ERRORcreate( "Undefined type tag %s", SEVERITY_ERROR );
}

/** Clean up the Type module */
void TYPEcleanup( void ) {
    ERRORdestroy( ERROR_corrupted_type );
    ERRORdestroy( ERROR_undefined_tag );
}

/**
 * \param t type to examine
 * \return the base type of the aggregate type
 * Retrieve the base type of an aggregate.
 */
Type TYPEget_nonaggregate_base_type( Type t ) {
    while( TYPEis_aggregation_data_type( t ) ) {
        t = t->u.type->body->base;
    }
    return t;
}

//MARK: - select type related
//static void put_origin(Linked_List list, Scope origin) {
//	LISTdo_links(list, link) {
//		link->aux = origin;
//	}LISTod;
//}
//static void filter_out_shadowed_attr_definitions(Linked_List attr_list) {
//	Link pivot = LISTLINKfirst(attr_list);
//	
//	while( pivot->next != attr_list->mark ){
//		Variable pivot_attr = (Variable)(pivot->data);
//		assert(pivot_attr!=NULL);
//		if( VARis_redeclaring(pivot_attr) ) pivot_attr = pivot_attr->original_attribute;
//		Entity defined_in = pivot_attr->defined_in;
//		
//		Link next;
//		for( Link node = pivot->next; ((void)(next=node->next), node!=attr_list->mark); node = next ){
//			Variable attr = (Variable)node->data;
//			if( VARis_redeclaring(attr) ) attr = attr->original_attribute;
//			if( attr->defined_in != defined_in ) continue;
//			
//			LINKremove(node);
//		}
//		pivot = pivot->next;
//		if(pivot == attr_list->mark) break;
//	}
//}
static void add_attributes_from_entity(Type selection_case, Dictionary result) {
	Entity case_entity = TYPEget_body(selection_case)->entity;
	assert(case_entity->u_tag==scope_is_entity);
	Dictionary all_attrs_from_entity = ENTITYget_all_attributes(case_entity);
	DictionaryEntry de;
	const char* attr_name;

	DICTdo_init(all_attrs_from_entity, &de);
	while( 0 != (attr_name = DICTdo_key(&de))) {
		if( ENTITYget_attr_ambiguous_count(case_entity, attr_name) > 1 )continue;
		
		Linked_List defined_attr_list = DICTlookup(result, attr_name);
		if( defined_attr_list == NULL ) {
			defined_attr_list = LISTcreate();
			DICTdefine(result, attr_name, defined_attr_list, NULL, OBJ_UNKNOWN);
		}
		Link added = LISTLINKadd_last(defined_attr_list);
		added->data = ENTITYfind_attribute_effective_definition(case_entity, attr_name);
		added->aux = selection_case;
		assert(added->data != NULL);
	}
	
	
//	Linked_List attr_defs_from_entity;
//	DICTdo_init( all_attrs_from_entity, &de );
//	while( 0 != ( attr_defs_from_entity = DICTdo(&de) ) ) {
//		const char* attr_name = DICT_key;
//		
//		Linked_List effective_attrs = LISTcopy(attr_defs_from_entity);
//		filter_out_shadowed_attr_definitions(effective_attrs);
//		put_origin(effective_attrs, selection_case);
//		
//		Linked_List select_attr_list = DICTlookup(result, attr_name);
//		if( select_attr_list == NULL ) {
//			select_attr_list = LISTcreate();
//			DICTdefine(result, attr_name, select_attr_list, NULL, OBJ_UNKNOWN);
//		}
//		LISTadd_all(select_attr_list, effective_attrs);
//	}
}

static void add_attributes_from_select(Type selection_case, Dictionary result) {
	Dictionary all_attrs_from_selection = SELECTget_all_attributes(selection_case);
	DictionaryEntry de;
	const char* attr_name;
	
	DICTdo_init(all_attrs_from_selection, &de);
	while( 0 != (attr_name = DICTdo_key(&de))) {
		if( SELECTget_attr_ambiguous_count(selection_case, attr_name) > 1 )continue;
		
		Linked_List defined_attr_list = DICTlookup(result, attr_name);
		if( defined_attr_list == NULL ) {
			defined_attr_list = LISTcreate();
			DICTdefine(result, attr_name, defined_attr_list, NULL, OBJ_UNKNOWN);
		}
		Link added = LISTLINKadd_last(defined_attr_list);
		added->data = SELECTfind_attribute_effective_definition(selection_case, attr_name);
		added->aux = selection_case;
		assert(added->data != NULL);
	}
}

Dictionary SELECTget_all_attributes ( Type select_type ) {
	assert(TYPEis_select(select_type));
	TypeBody typebody = TYPEget_body(select_type);
	if( typebody->all_select_attributes != NULL ) return typebody->all_select_attributes;
	
	Dictionary result = DICTcreate(25);

	LISTdo(typebody->list, selection_case, Type) {
		if( TYPEis_entity(selection_case) ) {
			add_attributes_from_entity(selection_case, result);
		}
		else if( TYPEis_select(selection_case) ) {
			add_attributes_from_select(selection_case, result);
		}
	}LISTod;
	
	typebody->all_select_attributes = result;
	return result;
}


// returns 0: not defined anywhere, 1: unique (common type) defintion, >1: ambiguous definition with different attr type
int SELECTget_attr_ambiguous_count( Type select_type, const char* attrName ) {
	assert( TYPEis_select(select_type) );
	Dictionary all_attrs = SELECTget_all_attributes(select_type);
	Linked_List attr_defs = DICTlookup(all_attrs, attrName);
	if( attr_defs == NULL )return 0;
	
	Entity defined_entity = NULL;
	Type attr_type = NULL;
	LISTdo(attr_defs, attr, Variable) {
		if( VARis_redeclaring(attr) ) attr = attr->original_attribute;
		if( attr_type == NULL ) attr_type = VARget_type(attr);
		
		//		if( defined_entity != NULL && attr->defined_in != defined_entity ) return 2;
		if( defined_entity != NULL ){
			if ( attr->defined_in != defined_entity ){
				if( TYPEget_common(attr_type, VARget_type(attr)) == NULL ) return 2;
				//			if( !TYPEs_are_equal(attr_type, VARget_type(attr)) && 
				//				 !TYPEs_are_equal(VARget_type(attr), attr_type) &&
				//				 !(TYPEis_entity(attr_type) && TYPEis_entity(VARget_type(attr))) ) return 2;
			}
		}
		else {
			defined_entity = attr->defined_in;		
		}
	}LISTod;
	
	assert(defined_entity != NULL);
	return 1;
}

Variable SELECTfind_attribute_effective_definition( Type select_type, const char* attr_name ) {
	assert(TYPEis_select(select_type));
	if( SELECTget_attr_ambiguous_count(select_type, attr_name) > 1 ) return NULL;
	
	Dictionary all_attrs = SELECTget_all_attributes(select_type);
	Linked_List attr_defs = DICTlookup(all_attrs, attr_name);
	if( attr_defs == NULL ) return NULL;
	
	Variable attr = LISTget_first(attr_defs);
	assert(attr != NULL);
	if( LISTis_single(attr_defs) ) return attr;
	// if there are multiple attribute declarations, return the common super-entity definition
	if( VARis_redeclaring(attr) ) attr = attr->original_attribute;
	return attr;
}

bool SELECTattribute_is_unique( Type select_type, const char* attr_name ){
	assert(TYPEis_select(select_type));
	if( SELECTget_attr_ambiguous_count(select_type, attr_name) > 1 ) return false;
	
	Dictionary all_attrs = SELECTget_all_attributes(select_type);
	Linked_List attr_defs = DICTlookup(all_attrs, attr_name);
	if( attr_defs == NULL ) return false;
	
	Variable pivot = LISTget_first(attr_defs);
	LISTdo(attr_defs, attr, Variable) {
		if( attr != pivot ) return false;
	}LISTod;
	return true;
}

static void add_superentities_from_entity(Type selection, Dictionary result) {
	Entity entity = TYPEget_body(selection)->entity;
	assert(entity->u_tag==scope_is_entity);
	
	Linked_List all_superentities = ENTITYget_super_entity_list(entity);
	LISTdo(all_superentities, super, Entity) {
		const char* super_name = super->symbol.name;
		
		Linked_List origins = DICTlookup(result, super_name);
		if( origins == NULL ) {
			origins = LISTcreate();
			DICTdefine(result, super_name, origins, NULL, OBJ_UNKNOWN);
		}
		LISTadd_last(origins, selection);
	}LISTod;
}
static void add_entities_from_select(Type selection, Dictionary result) {
	Dictionary all_entities = SELECTget_super_entity_list(selection);
	DictionaryEntry de;
	const char* entity_name;
	
	DICTdo_init(all_entities, &de);
	while( 0 != (entity_name = DICTdo_key(&de))) {		
		Linked_List origins = DICTlookup(result, entity_name);
		if( origins == NULL ) {
			origins = LISTcreate();
			DICTdefine(result, entity_name, origins, NULL, OBJ_UNKNOWN);
		}
		LISTadd_last(origins, selection);
	}
}
// returns dict of (linked list of selections) keyed by entity name
Dictionary SELECTget_super_entity_list( Type select_type ) {
	assert(TYPEis_select(select_type));
	TypeBody typebody = TYPEget_body(select_type);
	if( typebody->all_supertypes != NULL ) return typebody->all_supertypes;
	
	Dictionary result = DICTcreate(25);

	LISTdo(typebody->list, selection, Type) {
		if( TYPEis_entity(selection) ) {
			add_superentities_from_entity(selection, result);
		}
		else if( TYPEis_select(selection) ) {
			add_entities_from_select(selection, result);
		}
	}LISTod;

	typebody->all_supertypes = result;
	return result;
}	

// find common type among list of attribute definitions
Type SELECTfind_common_type(Linked_List attr_defs) {
	Type common = NULL;
	LISTdo(attr_defs, attr, Variable) {
		common = TYPEget_common(VARget_type(attr), common);
	}LISTod;
	
	// tentative handling; not needed if TYPEget_common get smarter
	if( common == NULL || common == Type_Entity ){
		common = NULL;
		LISTdo(attr_defs, attr, Variable) {
			if( VARis_redeclaring(attr) ) attr = attr->original_attribute;
			common = TYPEget_common(VARget_type(attr), common);
		}LISTod;
	}
	return common;
}


//*TY2021/1/6
static Expression get_common_bound(Expression boundA, Expression boundB, bool upper_bound){
	if( EXPs_are_equal(boundA, boundB) )return boundA;
	if( boundA == NULL )return boundB;
	if( boundB == NULL )return boundA;
	
	if( EXP_is_indeterminate(boundA) )return boundA;
	if( EXP_is_indeterminate(boundB) )return boundB;

	assert(TYPEis_integer(boundA->return_type));
	assert(TYPEis_integer(boundB->return_type));
	
	if(EXP_is_literal(boundA) && EXP_is_literal(boundB)){
		int boundAval = INT_LITget_value(boundA);
		int boundBval = INT_LITget_value(boundB);
		Expression resolved;
		if( upper_bound ){
			resolved = boundAval > boundBval ? boundA : boundB;
		}
		else{
			resolved = boundAval < boundBval ? boundA : boundB;
		}
		return resolved;
	}
	
	// bound expression is complicated; may need more analysis
	return NULL;	
}

Type TYPEget_common(Type t, Type tref) {
	if( tref == NULL ) return t;
	if( t == tref ) return tref;
	
	tref = TYPEget_fundamental_type(tref);
	t = TYPEget_fundamental_type(t);
	if( t == tref ) return tref;
	
	if( TYPEis_entity(tref) && TYPEis_entity(t) ) {
		Entity common_super = ENTITYget_common_super(ENT_TYPEget_entity(tref), ENT_TYPEget_entity(t));
		if(common_super == NULL) return Type_Entity;
		return common_super->u.entity->type;
	}
	
	if( (TYPEis(t) == TYPEis(tref)) && (!TYPEis_aggregation_data_type(t)) ){
		if( t->symbol.name == NULL ) return t;
		if( tref->symbol.name == NULL )return tref;
		switch( TYPEis(t) ){
			case indeterminate_: return( Type_Indeterminate );
			case number_:		return( Type_Number );
			case real_:			return( Type_Real );
			case integer_:	return( Type_Integer );
			case string_:		return( Type_String );
			case binary_:		return( Type_Binary );
			case logical_:	return( Type_Logical );
			case boolean_:	return( Type_Boolean );
				
			case bag_:
			case set_:
			case array_:
			case list_:
			default:
				break;
		}
	}
	
	if( TYPEis_integer(t) && TYPEis_real(tref) ) return Type_Real;
	if( TYPEis_integer(tref) && TYPEis_real(t) ) return Type_Real;
	
	if( TYPEis_integer(t) && TYPEis_number(tref) ) return Type_Number;
	if( TYPEis_integer(tref) && TYPEis_number(t) ) return Type_Number;

	if( TYPEis_real(t) && TYPEis_number(tref) ) return Type_Number;
	if( TYPEis_real(tref) && TYPEis_number(t) ) return Type_Number;
	
	if( TYPEis_boolean(t) && TYPEis_logical(tref) ) return Type_Logical;
	if( TYPEis_boolean(tref) && TYPEis_logical(t) ) return Type_Logical;

	if( TYPEis_aggregation_data_type(t) && TYPEis_aggregation_data_type(tref) ){
		Type common_base = TYPEget_common(TYPEget_nonaggregate_base_type(t), TYPEget_nonaggregate_base_type(tref));
		if( common_base != NULL ){
			Type common_aggregate = NULL;
			if( TYPEis(t) == TYPEis(tref) ) common_aggregate = tref;
			else if( TYPEis_set(t) && TYPEis_bag(tref) ) common_aggregate = tref;
			else if( TYPEis_set(tref) && TYPEis_bag(t) ) common_aggregate = t;
			
			if( common_aggregate != NULL ){
				bool optional = TYPEget_optional(t) || TYPEget_optional(tref);
				bool unique = TYPEget_unique(t) && TYPEget_unique(tref);
				Expression bound1 = get_common_bound(TYPEget_body(t)->lower, TYPEget_body(tref)->lower, false);
				Expression bound2 = get_common_bound(TYPEget_body(t)->upper, TYPEget_body(tref)->upper, true);
				Type resolved = TYPEcreate_aggregate(TYPEis(common_aggregate), common_base, bound1, bound2, unique, optional);
				return resolved;
			}
		}
	}
	
//	return Type_Generic;
	return NULL;
}


//*TY2020/09/14
bool TYPEs_are_equal(Type lhstype, Type rhstype) {
	if( lhstype == rhstype ) return true;
	if( (lhstype == NULL)||(rhstype == NULL) )return false;
	TypeBody lhstb = TYPEget_body(lhstype);
	TypeBody rhstb = TYPEget_body(rhstype);
	
	bool equal_symbol = names_are_equal( lhstype->symbol.name, rhstype->symbol.name );
	
	if( lhstb==NULL && rhstb==NULL ){
		return equal_symbol;
	}
	if( lhstb==NULL || rhstb==NULL ){
		return false;
	}
	
	switch (lhstb->type) {
		case indeterminate_:
			break;
		case integer_:
		case real_:
		case string_:
		case binary_:
		case boolean_:
		case number_:
		case logical_:
			if(lhstb->type != rhstb->type)return false;
			return  equal_symbol;
			
		case entity_:
			return lhstb->entity == rhstb->entity;
			
		case generic_:
		case aggregate_:
			return (lhstb->type == rhstb->type) && TYPEs_are_equal(lhstb->tag, rhstb->tag);

		case array_:
		case bag_:
		case set_:
		case list_:
			if( lhstb->type != rhstb->type )return false;
			if( lhstb->flags.unique != rhstb->flags.unique )return false;
			if( lhstb->flags.optional != rhstb->flags.optional )return false;
			if( !EXPs_are_equal(lhstb->upper, rhstb->upper) )return false;
			if( !EXPs_are_equal(lhstb->lower, rhstb->lower) )return false;
			return TYPEs_are_equal(lhstb->base, rhstb->base);
			
		case enumeration_: 
			break;
		case select_:
			break;
		default:
			break;
	}
	return false;
}

Type TYPEget_fundamental_type(Type t) {
	Type underlying;
	Type fundamental = t;
	while ( (underlying = TYPEget_head(fundamental)) ) {
		fundamental = underlying;
	}
	return fundamental;
}

const char* TYPEget_kind(Type t) {
	TypeBody tb = t->u.type->body;
	if( tb != NULL ){
		switch (TYPEis(t)) {
			case indeterminate_: return "INDETERMINATE";
			case integer_:	return "INTEGER";
			case real_:			return "REAL";
			case string_:		return "STRING";
			case binary_:		return "BINARY";
			case boolean_:	return "BOOLEAN";
			case logical_:	return "LOGICAL";
			case number_:		return "NUMBER";
				
			case entity_:		return "ENTITY";
				
			case generic_:	return "GENERIC";
			case aggregate_:	return "AGGREGATE";
				
			case array_:	return "ARRAY";
			case bag_:		return "BAG";
			case set_:		return "SET";
			case list_:		return "LIST";
				
			case enumeration_: 	return "ENUMERATION";
			case select_:	return "SELECT";
				
			default:	return "<unknown>";
		}
	}
	else {
		if( t->symbol.resolved != RESOLVED ) return "<unresolved>";
		return "<nullbody>";
	}

}

//*TY2002/11/19
bool TYPEcontains_generic(Type t) {
	if( TYPEis_generic(t) ) return true;
	if( TYPEis_AGGREGATE(t) ) return true;
	if( TYPEis_aggregation_data_type(t) ) return TYPEcontains_generic(TYPEget_base_type(t));
	return false;
}
