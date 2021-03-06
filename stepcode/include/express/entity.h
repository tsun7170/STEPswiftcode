#ifndef ENTITY_H
#define ENTITY_H

/** **********************************************************************
** Module:  Entity \file entity.h
** Description: This module represents Express entity definitions.  An
**  entity definition consists of a name, a list of attributes, a
**  collection of uniqueness sets, a specification of the entity's
**  position in a class hierarchy (i.e., sub- and supertypes), and
**  a list of constraints which must be satisfied by instances of
**  the entity.  A uniquess set is a set of one or more attributes
**  which, when taken together, uniquely identify a specific instance
**  of the entity.  Thus, the uniqueness set { name, ssn } indicates
**  that no two instances of the entity may share both the same
**  values for both name and ssn.
** Constants:
**  ENTITY_NULL - the null entity
**
************************************************************************/

/*
 * This software was developed by U.S. Government employees as part of
 * their official duties and is not subject to copyright.
 *
 * $Log: entity.h,v $
 * Revision 1.12  1997/01/21 19:17:11  dar
 * made C++ compatible
 *
 * Revision 1.11  1994/11/10  19:20:03  clark
 * Update to IS
 *
 * Revision 1.10  1994/05/11  19:51:05  libes
 * numerous fixes
 *
 * Revision 1.9  1993/10/15  18:48:24  libes
 * CADDETC certified
 *
 * Revision 1.7  1993/02/16  03:14:47  libes
 * simplified find calls
 *
 * Revision 1.6  1993/01/19  22:45:07  libes
 * *** empty log message ***
 *
 * Revision 1.5  1992/08/18  17:12:41  libes
 * rm'd extraneous error messages
 *
 * Revision 1.4  1992/06/08  18:06:24  libes
 * prettied up interface to print_objects_when_running
 *
 */

/*************/
/* constants */
/*************/

#define ENTITY_NULL             (Entity)0
#define ENTITY_INHERITANCE_UNINITIALIZED    -1

/*****************/
/* packages used */
/*****************/

#include <sc_export.h>
#include "expbasic.h"   /* get basic definitions */
#include "symbol.h"
#include "scope.h"

/************/
/* typedefs */
/************/

typedef struct Scope_ * Entity;

/****************/
/* modules used */
/****************/

#include "expr.h"
#include "variable.h"
#include "schema.h"

/***************************/
/* hidden type definitions */
/***************************/

struct Entity_ {
    Linked_List supertype_symbols;  /**< linked list of original symbols as generated by parser */
    Linked_List supertypes;         /**< linked list of immediate supertypes (as entities) */
    Linked_List subtypes;           /**< simple list of subtypes useful for simple lookups */
    Expression  subtype_expression; /**< DAG of subtypes, with complete information including, OR, AND, and ONEOF */
    Linked_List attributes;         /**< all local attributes (explicit,derive,inverse,redeclaration) */
    int         inheritance;        /**< total number of attributes inherited from supertypes */
    int         attribute_count;
    Linked_List unique;             /**< list of identifiers that are unique */
    Linked_List instances;          /**< hook for applications */
		int         mark;               /**< usual hack - prevent traversing sub/super graph twice */
    bool        abstract;           /**< is this an abstract supertype? */
    Type        type;               /**< type pointing back to ourself, useful to have when evaluating expressions involving entities */
	//*TY2020/07/19 added
	Linked_List supertype_list;	// list of all supertypes consistent with P21 spec (including SELF entity at the end).
	Dictionary all_attributes;	// dict of (linked_list of attr) keyed by attr simple name. linked_list is sorted from most sub-entity to super-entity (first item is the effective attr definition, if it is not an ambiguous definition), including possible subtype attributes. linked list of attr contains aux flag of true indicating the origin is subtype, while aux flag of false indicating the origin is self/supertype.
	Linked_List constructor_params;	// parameters for the implicit constructor
	//*TY2021/01/10 added
	Linked_List subtype_list; // list of all possible subtypes (excluding SELF)
//	Dictionary all_subtype_attributes; // dict of (linked list of attr) keyed by attr simple name, for all possible subtypes, sorted from nearest to the subject entity to the farthest subtype inheritances.
};

/********************/
/* global variables */
/********************/

extern SC_EXPRESS_EXPORT struct freelist_head ENTITY_fl;
extern SC_EXPRESS_EXPORT int ENTITY_MARK;

/******************************/
/* macro function definitions */
/******************************/

#define ENTITY_new()        (struct Entity_ *)MEM_new(&ENTITY_fl)
#define ENTITY_destroy(x)   MEM_destroy(&ENTITY_fl,(Freelist *)(char *)x)

#define ENTITYget_symbol(e) SCOPEget_symbol(e)
/* returns a function (i.e., which can be passed to other functions) */
#define ENTITY_get_symbol   SCOPE_get_symbol

#define ENTITYget_attributes(e) ((e)->u.entity->attributes)
#define ENTITYget_subtypes(e)   ((e)->u.entity->subtypes)
#define ENTITYget_supertypes(e) ((e)->u.entity->supertypes)
#define ENTITYget_name(e)   ((e)->symbol.name)
#define ENTITYget_uniqueness_list(e) ((e)->u.entity->unique)
#define ENTITYget_where(e)  ((e)->where)
#define ENTITYput_where(e,w)    ((e)->where = (w))

#define ENTITYget_abstract(e)   ((e)->u.entity->abstract)

#define ENTITYget_mark(e)   ((e)->u.entity->mark)
#define ENTITYput_mark(e,m) ((e)->u.entity->mark = (m))

#define ENTITYput_inheritance_count(e,c) ((e)->u.entity->inheritance = (c))
#define ENTITYget_inheritance_count(e)  ((e)->u.entity->inheritance)
#define ENTITYget_size(e)   ((e)->u.entity->attribute_count + (e)->u.entity->inheritance)
#define ENTITYget_attribute_count(e)    ((e)->u.entity->attribute_count)
#define ENTITYput_attribute_count(e,c)  ((e)->u.entity->attribute_count = (c))

#define ENTITYget_clientData(e)     ((e)->clientData)
#define ENTITYput_clientData(e,d)       ((e)->clientData = (d))

//*TY2021/03/01
#define ENTITYget_type(e)		((e)->u.entity->type)

/***********************/
/* function prototypes */
/***********************/

extern SC_EXPRESS_EXPORT struct Scope_  * ENTITYcreate PROTO( ( struct Symbol_ * ) );
extern SC_EXPRESS_EXPORT void     ENTITYinitialize PROTO( ( void ) );
extern SC_EXPRESS_EXPORT void     ENTITYadd_attribute PROTO( ( struct Scope_ *, struct Variable_ * ) );
extern SC_EXPRESS_EXPORT struct Scope_  * ENTITYcopy PROTO( ( struct Scope_ * ) );
extern SC_EXPRESS_EXPORT Variable     ENTITYresolve_attr_ref PROTO( ( Entity, Symbol *, Symbol * ) );

// entity related queries
extern SC_EXPRESS_EXPORT Entity       ENTITYfind_inherited_entity PROTO( ( struct Scope_ *, char *, int, bool ) );
extern SC_EXPRESS_EXPORT bool      ENTITYhas_immediate_supertype PROTO( ( Entity, Entity ) );
extern SC_EXPRESS_EXPORT bool      ENTITYhas_supertype PROTO( ( Entity sub, Entity sup) );
//*TY2020/07/11
extern bool ENTITYis_a( Entity entity , Entity sup );
extern Linked_List ENTITYget_super_entity_list( Entity entity ); // including starting entity
//*TY2021/1/6
extern Entity ENTITYget_common_super( Entity e1, Entity e2);
//*TY2021/01/10
extern Linked_List ENTITYget_sub_entity_list( Entity entity ); // excluding starting entity


// attribute related queries
extern SC_EXPRESS_EXPORT Variable     ENTITYfind_inherited_attribute PROTO( ( struct Scope_ *, char *, struct Symbol_ **, bool ) );
//*TY2020/07/19
extern Variable ENTITYfind_attribute_effective_definition( Entity entity, const char* attr_name );
extern SC_EXPRESS_EXPORT Variable     ENTITYget_named_attribute PROTO( ( Entity, char * ) );
extern SC_EXPRESS_EXPORT bool      ENTITYdeclares_variable PROTO( ( Entity, struct Variable_ * ) );
//*TY2020/07/19 changed the function signature
extern SC_EXPRESS_EXPORT Dictionary  ENTITYget_all_attributes PROTO( ( Entity ) ); // returns dict of (linked_list of attr) keyed by attr simple name
//*TY2020/07/11
extern int ENTITYget_attr_ambiguous_count( Entity entity, const char* attrName );	// 0: not defined anywhere, 1: unique defintion, >1: ambiguous definition
//*TY2020/08/09
extern Linked_List ENTITYget_constructor_params( Entity entity );

// misc.
extern SC_EXPRESS_EXPORT void     ENTITYadd_instance PROTO( ( Entity, Generic ) );
extern SC_EXPRESS_EXPORT int      ENTITYget_initial_offset PROTO( ( Entity ) );

#endif    /*  ENTITY_H  */
