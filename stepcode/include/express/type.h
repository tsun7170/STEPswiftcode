#ifndef TYPE_H
#define TYPE_H

/** **********************************************************************
** Module:  Type \file type.h
** Description: This module implements the type abstraction.  It is
**  rather unpleasant, since this abstraction is quite well suited
**  to an object-oriented environment with inheritance.
**
************************************************************************/

/*
 * This software was developed by U.S. Government employees as part of
 * their official duties and is not subject to copyright.
 *
 * $Log: type.h,v $
 * Revision 1.14  1997/09/19 18:25:10  dar
 * small fix - removed multiple include
 *
 * Revision 1.13  1997/01/21 19:17:11  dar
 * made C++ compatible
 *
 * Revision 1.12  1994/11/10  19:20:03  clark
 * Update to IS
 *
 * Revision 1.11  1994/05/11  19:51:05  libes
 * numerous fixes
 *
 * Revision 1.10  1993/10/15  18:48:24  libes
 * CADDETC certified
 *
 * Revision 1.8  1993/03/19  20:43:45  libes
 * add is_enum macro
 *
 * Revision 1.7  1993/01/21  19:48:25  libes
 * fix bug in TYPEget_base_type
 *
 * Revision 1.6  1993/01/19  22:16:09  libes
 * *** empty log message ***
 *
 * Revision 1.5  1992/09/16  18:23:45  libes
 * added some functions for easier access to base types
 *
 * Revision 1.4  1992/08/18  17:12:41  libes
 * rm'd extraneous error messages
 *
 * Revision 1.3  1992/06/08  18:06:24  libes
 * prettied up interface to print_objects_when_running
 */

/*************/
/* constants */
/*************/

#define TYPE_NULL       (Type)0

/** since we can't evaluate true size of many aggregates,
 * prepare to grow them by chunks of this size */
#define AGGR_CHUNK_SIZE 30

/** these are all orthogonal types */
enum type_enum {
    unknown_ = 0,   /**< 0 catches uninit. errors */
    special_,   /**< placeholder, given meaning by it's owner, such as Type_Dont_Care, Type_Bad, Type_User_Def */
    runtime_,   /**< cannot be further determined until runtime */
    integer_,
    real_,
    string_,
    binary_,
    boolean_,
    logical_,

    /* formals-only */
    number_,
    generic_,

    /* aggregates */
    aggregate_, /**< as a formal */
    array_,
    bag_,
    set_,
    list_,
    last_aggregate_,/**< not real, just for easier computation */

    oneof_,

    /** while they are really used for different purposes, it might be worth considering collapsing entity_ and entity_list_ */
    entity_,    /**< single entity */
    entity_list_,   /**< linked list of entities */
    enumeration_,
    select_,
    reference_, /**< something only defined by a base type, i.e., a type reference */
    query_,
    op_,        /**< something with an operand */
    inverse_,   /**< is? an inverse */

    identifier_,    /**< simple identifier in an expression */
    attribute_, /**< attribute reference (i.e., expr->u.variable) */
    derived_,   /**< ?*/
    funcall_,   /**< a function call and actual parameters */

    self_,
	
		indeterminate_	//*ty2020/12/31 added
};

//*TY2021/06/05 added
typedef enum {
	select_attribute_unknown = 0,
	resolving_select_attribute,
	yes_yield_entity_reference,
	no_yield_entity_reference,
	infinite_looping_select_attribute
} Select_type_attribute;


/*****************/
/* packages used */
/*****************/

#include <sc_export.h>
#include "expbasic.h"   /* get basic definitions */
#include "symbol.h"
#include "object.h"

#include "dict.h"
#include "variable.h"

/************/
/* typedefs */
/************/

typedef struct TypeHead_ * TypeHead;
typedef struct TypeBody_ * TypeBody;
typedef enum type_enum  TypeType;

/* provide a replacement for Class */
typedef enum type_enum  Class_Of_Type;
typedef enum type_enum  Class;
#define OBJget_class(typ)   ((typ)->u.type->body->type)
#define CLASSinherits_from  TYPEinherits_from

/****************/
/* modules used */
/****************/

#include "dict.h"
#include "entity.h"
#include "expr.h"
#include "scope.h"

/***************************/
/* hidden type definitions */
/***************************/

struct TypeHead_ {
    Type head;          /**< if we are a defined type this is who we point to */
    struct TypeBody_ * body;    /**< true type, ignoring defined types */
#if 0
    /* if we are concerned about memory (over time) uncomment this and */
    /* other references to refcount in parser and TYPEresolve.  It is */
    /* explained in TYPEresolve. */
    int refcount;
#endif
};

struct TypeBody_ {
#if 1
    struct TypeHead_ * head;    /**< for debugging only */
#endif
    enum type_enum type;        /**< bits describing this type, int, real, etc */
    struct {
        unsigned unique     : 1;
        unsigned optional   : 1;
        unsigned fixed      : 1;
        unsigned shared     : 1; /**< type is shared */
        unsigned repeat     : 1; /**< Expression is the number of repetitions of the previous Expression
                                   * 10303-11:2004 production #203
                                   *   element = expression [ ':' repetition ] .
                                   * TODO exp2cxx and exp2py do not use this! Are all use cases handled by libexppp?
                                   */
        unsigned var        : 1; /** denotes variable marked 'VAR' - i.e. one where changes are propagated back to the caller */
        unsigned encoded    : 1; /**< encoded string */
			unsigned partial : 1; //*TY2021/03/21 to distinguish partial entity from fully established entity
    } flags;
    Type base;      /**< underlying base type if any can also contain true type if this type is a type reference */
    Type tag;       /**< optional tag */
    /* a lot of the stuff below can be unionized */
    Expression precision;
    Linked_List list;   /**< used by select_types and composed types, such as for a list of entities in an instance */
    Expression upper;
    Expression lower;
    struct Scope_ * entity;     /**< only used by entity types */
	//*TY2020/08/30
	Dictionary all_select_attributes;	// dict of (linked_list of attr for selection list entries, with aux pointing to the originating selection) keyed by attr simple name.
	Dictionary all_supertypes; // dict of (linked_list of originating selection) keyed by super-entity name
	//*TY2021/06/05
	Select_type_attribute select_type_attribute;
	//*TY2021/07/31 for TYPEOF() support
	Linked_List referenced_by_select;
};

/********************/
/* global variables */
/********************/

/* Very commonly-used read-only types */
/* non-constant versions probably aren't necessary? */
extern SC_EXPRESS_EXPORT Type Type_Bad;
extern SC_EXPRESS_EXPORT Type Type_Unknown;
extern SC_EXPRESS_EXPORT Type Type_Dont_Care;
extern SC_EXPRESS_EXPORT Type Type_Runtime;   /**< indicates that this object can't be
                                                    calculated now but must be deferred
                                                    until (the mythical) runtime */
extern SC_EXPRESS_EXPORT Type Type_Binary;
extern SC_EXPRESS_EXPORT Type Type_Boolean;
extern SC_EXPRESS_EXPORT Type Type_Enumeration;
extern SC_EXPRESS_EXPORT Type Type_Expression;
extern SC_EXPRESS_EXPORT Type Type_Aggregate;
extern SC_EXPRESS_EXPORT Type Type_Repeat;
extern SC_EXPRESS_EXPORT Type Type_Integer;
extern SC_EXPRESS_EXPORT Type Type_Number;
extern SC_EXPRESS_EXPORT Type Type_Real;
extern SC_EXPRESS_EXPORT Type Type_String;
extern SC_EXPRESS_EXPORT Type Type_String_Encoded;
extern SC_EXPRESS_EXPORT Type Type_Logical;
extern SC_EXPRESS_EXPORT Type Type_Set;
extern SC_EXPRESS_EXPORT Type Type_Attribute;
extern SC_EXPRESS_EXPORT Type Type_Entity;
extern SC_EXPRESS_EXPORT Type Type_Funcall;
extern SC_EXPRESS_EXPORT Type Type_Generic;
extern SC_EXPRESS_EXPORT Type Type_Identifier;
extern SC_EXPRESS_EXPORT Type Type_Oneof;
extern SC_EXPRESS_EXPORT Type Type_Query;
extern SC_EXPRESS_EXPORT Type Type_Self;
extern SC_EXPRESS_EXPORT Type Type_Set_Of_String;
extern SC_EXPRESS_EXPORT Type Type_Set_Of_Generic;
extern SC_EXPRESS_EXPORT Type Type_Bag_Of_Generic;
//*TY2020/08/02
extern SC_EXPRESS_EXPORT Type Type_Indeterminate;
extern SC_EXPRESS_EXPORT Type Type_List_Of_Generic;
extern SC_EXPRESS_EXPORT Type Type_Aggregate_Of_Generic;
extern SC_EXPRESS_EXPORT Type Type_Set_Of_GenericEntity;
extern SC_EXPRESS_EXPORT Type Type_Bag_Of_GenericEntity;
extern SC_EXPRESS_EXPORT Type Type_List_Of_GenericEntity;
extern SC_EXPRESS_EXPORT Type Type_Aggregate_Of_GenericEntity;


//*TY2021/1/1 - to suppress type casting code on assignment
#define Type_Binary_Generic		Type_Generic
#define Type_Boolean_Generic	Type_Generic
#define Type_Integer_Generic	Type_Generic
#define Type_Number_Generic		Type_Generic
#define Type_Real_Generic			Type_Generic
#define Type_String_Generic		Type_Generic


extern SC_EXPRESS_EXPORT struct freelist_head TYPEHEAD_fl;
extern SC_EXPRESS_EXPORT struct freelist_head TYPEBODY_fl;

extern SC_EXPRESS_EXPORT Error ERROR_corrupted_type;

/******************************/
/* macro function definitions */
/******************************/

#define TYPEHEAD_new()      (struct TypeHead_ *)MEM_new(&TYPEHEAD_fl)
#define TYPEHEAD_destroy(x) MEM_destroy(&TYPEHEAD_fl,(Freelist *)(Generic)x)
#define TYPEBODY_new()      (struct TypeBody_ *)MEM_new(&TYPEBODY_fl)
#define TYPEBODY_destroy(x) MEM_destroy(&TYPEBODY_fl,(Freelist *)(Generic)x)


#define TYPEget_symbol(t)   (&(t)->symbol)

#define TYPEget_head(t)     ((t)->u.type->head)
#define TYPEput_head(t,h)   ((t)->u.type->head = (h))

#define TYPEget_body(t)     ((t)->u.type->body)
#define TYPEput_body(t,h)   ((t)->u.type->body = (h))

#define TYPEget_type(t)     (TYPEget_body(t)->type)
#define TYPEget_base_type(t)    (TYPEget_body(t)->base)


#define TYPEis(t)       TYPEget_type(t)
#define TYPEis_identifier(t)    (TYPEget_type(t) == identifier_)
#define TYPEis_logical(t)   (TYPEget_type(t) == logical_)
#define TYPEis_boolean(t)   (TYPEget_type(t) == boolean_)
#define TYPEis_real(t)   (TYPEget_type(t) == real_)
#define TYPEis_integer(t)   (TYPEget_type(t) == integer_)
#define TYPEis_number(t)   (TYPEget_type(t) == number_)	//*TY2021/01/19
#define TYPEis_string(t)    (TYPEget_type(t) == string_)
#define TYPEis_expression(t)    (TYPEget_type(t) == op_)
#define TYPEis_oneof(t)     (TYPEget_type(t) == oneof_)
#define TYPEis_entity(t)    (TYPEget_type(t) == entity_)
#define TYPEis_partial_entity(t)	(TYPEis_entity(t) && TYPEget_body(t)->flags.partial)
#define TYPEis_enumeration(t)   (TYPEget_type(t) == enumeration_)
#define TYPEis_aggregation_data_type(t) (TYPEget_base_type(t) != NULL)	//*TY2020/11/19 renamed
#define TYPEis_AGGREGATE(t) (TYPEget_type(t) == aggregate_)	//*TY2020/11/19 renamed
#define TYPEis_generic(t)		(TYPEget_type(t) == generic_)	//*TY2020/11/19 added
#define TYPEis_free_generic(t)	(TYPEis_generic(t) && (TYPEget_tag(t)==NULL)) //*TY2021/02/06 added
#define TYPEis_array(t)     (TYPEget_type(t) == array_)
#define TYPEis_bag(t)     (TYPEget_type(t) == bag_)	//*TY2020/12/05 added
#define TYPEis_set(t)     (TYPEget_type(t) == set_)	//*TY2020/12/05 added
#define TYPEis_list(t)     (TYPEget_type(t) == list_)	//*TY2020/12/05 added
#define TYPEis_select(t)    (TYPEget_type(t) == select_)
#define TYPEis_reference(t) (TYPEget_type(t) == reference_)
#define TYPEis_unknown(t)   (TYPEget_type(t) == unknown_)
#define TYPEis_runtime(t)   (TYPEget_type(t) == runtime_)
#define TYPEis_shared(t)    (TYPEget_body(t)->flags.shared)
#define TYPEis_optional(t)  (TYPEget_body(t)->flags.optional)
#define TYPEis_encoded(t)   (TYPEget_body(t)->flags.encoded)
#define TYPEis_generic_entity(t) (TYPEis_entity(t) && TYPEget_body(t)->entity == NULL)	//*TY2020/12/2
#define TYPEis_literal(t)	()

//*TY2020/12/05
#define TYPEcanbe_array(t)	(TYPEis_array(t) || TYPEis_AGGREGATE(t))
#define TYPEcanbe_bag(t)	(TYPEis_bag(t) || TYPEis_AGGREGATE(t))
#define TYPEcanbe_set(t)	(TYPEis_set(t) || TYPEis_AGGREGATE(t))
#define TYPEcanbe_list(t)	(TYPEis_list(t) || TYPEis_AGGREGATE(t))

#define TYPEput_name(type,n)    ((type)->symbol.name = (n))
#define TYPEget_name(type)  ((type)->symbol.name)

#define TYPEget_where(t)    ((t)->where)
#define TYPEput_where(t,w)  (((t)->where) = w)

#define ENT_TYPEget_entity(t)       (TYPEget_body(t)->entity)
#define ENT_TYPEput_entity(t,ent)   (TYPEget_body(t)->entity = ent)

#define COMP_TYPEget_items(t)       (TYPEget_body(t)->list)
#define COMP_TYPEput_items(t,lis)   (TYPEget_body(t)->list = (lis))
#define COMP_TYPEadd_items(t,lis)   LISTadd_all(TYPEget_body(t)->list, (lis));

#define ENUM_TYPEget_items(t)       ((t)->symbol_table)
#define TYPEget_optional(t)     (TYPEget_body(t)->flags.optional)
#define TYPEget_unique(t)       (TYPEget_body(t)->flags.unique)

#define SEL_TYPEput_items(type,list)    COMP_TYPEput_items(type, list)
#define SEL_TYPEget_items(type)     COMP_TYPEget_items(type)

#define AGGR_TYPEget_upper_limit(t) (TYPEget_body(t)->upper ? TYPEget_body(t)->upper : LITERAL_INFINITY)
#define AGGR_TYPEget_lower_limit(t) (TYPEget_body(t)->lower ? TYPEget_body(t)->lower : LITERAL_ZERO)

#define TYPEget_enum_tags(t)        ((t)->symbol_table)

#define TYPEget_clientData(t)       ((t)->clientData)
#define TYPEput_clientData(t,d)     ((t)->clientData = (d))

//*TY2020/08/08
#define TYPEget_tag(t)							( TYPEget_body(t)->tag )
#define	TYPEhas_tag(t)							( (TYPEget_tag(t)!=NULL) && (TYPEget_tag(t)->symbol.name[0]!='_') )
#define TYPEhas_bounds(t)						( TYPEget_body(t)->upper != NULL )

/***********************/
/* function prototypes */
/***********************/

extern SC_EXPRESS_EXPORT Type TYPEcreate_partial PROTO( ( struct Symbol_ *, Scope ) );

extern SC_EXPRESS_EXPORT Type TYPEcreate PROTO( ( enum type_enum ) );
extern SC_EXPRESS_EXPORT Type TYPEcreate_from_body_anonymously PROTO( ( TypeBody ) );
extern SC_EXPRESS_EXPORT Type TYPEcreate_name PROTO( ( struct Symbol_ * ) );
extern SC_EXPRESS_EXPORT Type TYPEcreate_nostab PROTO( ( struct Symbol_ *, Scope, char ) );
extern SC_EXPRESS_EXPORT TypeBody TYPEBODYcreate PROTO( ( enum type_enum ) );
extern SC_EXPRESS_EXPORT void TYPEinitialize PROTO( ( void ) );
extern SC_EXPRESS_EXPORT void TYPEcleanup PROTO( ( void ) );
extern Type TYPEcreate_aggregate( enum type_enum aggr_type, Type base_type, Expression bound1, Expression bound2, bool unique, bool optional); //*TY2021/01/19
extern Type TYPEcopy( Type t ); //*TY2021/03/21

extern SC_EXPRESS_EXPORT bool TYPEinherits_from PROTO( ( Type, enum type_enum ) );
extern SC_EXPRESS_EXPORT Type TYPEget_nonaggregate_base_type PROTO( ( Type ) );

extern SC_EXPRESS_EXPORT Type TYPEcreate_user_defined_type PROTO( ( Type, Scope, struct Symbol_ * ) );
extern SC_EXPRESS_EXPORT Type TYPEcreate_user_defined_tag PROTO( ( Type, Scope, struct Symbol_ * ) );

//*TY2020/08/06
extern Symbol* SYMBOLcreate_implicit_tag( int tag_no, bool is_aggregate, int line, const char * filename );

//*TY2020/08/30
extern Dictionary SELECTget_all_attributes ( Type select_type ); // returns dict of (linked_list of attr) keyed by attr simple name
extern int SELECTget_attr_ambiguous_count( Type select_type, const char* attrName );	// 0: not defined anywhere, 1: unique defintion, >1: ambiguous definition
extern Variable SELECTfind_attribute_effective_definition( Type select_type, const char* attr_name );
extern bool SELECTattribute_is_unique( Type select_type, const char* attr_name );

extern Dictionary SELECTget_super_entity_list( Type select_type );	// returns dict of (linked list of selections) keyed by entity name

//*TY2021/1/5
extern Type SELECTfind_common_type(Linked_List attr_defs);
extern Type TYPEget_common(Type t, Type tref);

//*TY2020/09/14
extern bool TYPEs_are_equal(Type t1, Type t2);

//*TY2020/09/19
extern Type TYPEget_fundamental_type(Type t);
extern const char* TYPEget_kind(Type t);

//*TY2020/11/19
extern bool TYPEcontains_generic(Type t);

//*TY2021/06/02
extern bool TYPE_may_yield_entity_reference(Type t);

#endif    /*  TYPE_H  */
