

/**
** Module:  Entity \file entity.c
*
** This module is used to represent Express entity definitions.
**  An entity definition consists of a name, a set of attributes, a
**  collection of uniqueness sets, a specification of the entity's
**  position in a class hierarchy (i.e., sub- and supertypes), and
**  a list of constraints which must be satisfied by instances of
**  the entity.  A uniquess set is a set of one or more attributes
**  which, when taken together, uniquely identify a specific instance
**  of the entity.  Thus, the uniqueness set { name, ssn } indicates
**  that no two instances of the entity may share both the same
**  values for both name and ssn.
*
** Constants:
**  ENTITY_NULL - the null entity
**
************************************************************************/

/*
 * This software was developed by U.S. Government employees as part of
 * their official duties and is not subject to copyright.
 *
 * $Log: entity.c,v $
 * Revision 1.11  1997/01/21 19:19:51  dar
 * made C++ compatible
 *
 * Revision 1.10  1995/03/09  18:43:03  clark
 * various fixes for caddetc - before interface clause changes
 *
 * Revision 1.9  1994/11/10  19:20:03  clark
 * Update to IS
 *
 * Revision 1.8  1993/10/15  18:48:48  libes
 * CADDETC certified
 *
 * Revision 1.6  1993/02/16  03:14:47  libes
 * simplified find calls
 *
 * Revision 1.5  1993/01/19  22:45:07  libes
 * *** empty log message ***
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
 * Revision 1.7  1992/05/10  06:01:29  libes
 * cleaned up OBJget_symbol
 *
 * Revision 1.6  1992/05/05  19:51:47  libes
 * final alpha
 *
 * Revision 1.5  1992/02/19  15:48:18  libes
 * changed types to enums & flags
 *
 * Revision 1.4  1992/02/17  14:32:57  libes
 * lazy ref/use evaluation now working
 *
 * Revision 1.3  1992/02/12  07:05:23  libes
 * y
 * do sub/supertype
 *
 * Revision 1.2  1992/02/09  00:49:53  libes
 * does ref/use correctly
 *
 * Revision 1.1  1992/02/05  08:34:24  libes
 * Initial revision
 *
 * Revision 1.0.1.1  1992/01/22  02:47:57  libes
 * copied from ~pdes
 *
 * Revision 4.7  1991/09/20  06:20:43  libes
 * fixed bug that printed out entity->attributes incorrectly
 * assumed they were objects rather than strings
 *
 * Revision 4.6  1991/09/16  23:13:12  libes
 * added print functionsalgorithm.c
 *
 * Revision 4.5  1991/07/18  05:07:08  libes
 * added SCOPEget_use
 *
 * Revision 4.4  1991/07/13  05:04:17  libes
 * added ENTITYget_supertype and ..get_subtype
 *
 * Revision 4.3  1991/01/24  22:20:36  silver
 * merged changes from NIST and SCRA
 * SCRA changes are due to DEC ANSI C compiler tests.
 *
 * Revision 4.3  91/01/08  18:55:42  pdesadmn
 * Initial - Beta checkin at SCRA
 *
 * Revision 4.2  90/11/23  16:33:43  clark
 * Initial checkin at SCRA
 *
 * Revision 4.2  90/11/23  16:33:43  clark
 * initial checkin at SCRA
 *
 * Revision 4.2  90/11/23  16:33:43  clark
 * Fixes for better error handling on supertype lists
 *
 * Revision 4.1  90/09/13  15:13:08  clark
 * BPR 2.1 alpha
 *
 */

#include <assert.h>
#include <sc_memmgr.h>
#include "express/entity.h"
#include "express/express.h"
#include "express/object.h"

struct freelist_head ENTITY_fl;
int ENTITY_MARK = 0;

/** returns true if variable is declared (or redeclared) directly by entity */
bool ENTITYdeclares_variable( Entity e, Variable v ) {
    LISTdo( e->u.entity->attributes, attr, Variable )
    if( attr == v ) {
        return true;
    }
    LISTod;

    return false;
}

static Entity ENTITY_find_inherited_entity( Entity entity, char * name, int down ) {
    Entity result;

    /* avoid searching scopes that we've already searched */
    /* this can happen due to several things */
    /* if A ref's B which ref's C, and A ref's C.  Then C */
    /* can be searched twice by A.  Similar problem with */
    /* sub/super inheritance. */
		if( SCOPE_search_visited(entity) ) return NULL;

    LISTdo( entity->u.entity->supertypes, super, Entity )
    if( streq( super->symbol.name, name ) ) {
        return super;
    }
    LISTod

    LISTdo( entity->u.entity->supertypes, super, Entity )
    result = ENTITY_find_inherited_entity( super, name, down );
    if( result ) {
        return result;
    }
    LISTod;

    if( down ) {
        LISTdo( entity->u.entity->subtypes, sub, Entity )
        if( streq( sub->symbol.name, name ) ) {
            return sub;
        }
        LISTod;

        LISTdo( entity->u.entity->subtypes, sub, Entity )
        result = ENTITY_find_inherited_entity( sub, name, down );
        if( result ) {
            return result;
        }
        LISTod;
    }

    return 0;
}

struct Scope_ * ENTITYfind_inherited_entity( struct Scope_ *entity, char * name, int down, bool under_search ) {
	if( streq( name, entity->symbol.name ) ) {
		return( entity );
	}
	
	//    __SCOPE_search_id++;
	if(!under_search) SCOPE_begin_search();
	Entity result = ENTITY_find_inherited_entity( entity, name, down );
	if(!under_search) SCOPE_end_search();
	return result;
}

//*TY2020/07/18
int ENTITYget_attr_ambiguous_count( Entity entity, const char* attrName ) {
	Dictionary all_attrs = ENTITYget_all_attributes(entity);
	Linked_List attr_defs = DICTlookup(all_attrs, attrName);
	if( attr_defs == NULL )return 0;
	
	Entity defined_entity = NULL;
	bool defined_entity_is_subtype = false;
	LISTdo(attr_defs, attr, Variable) {		
		if( VARis_redeclaring(attr) ) attr = attr->original_attribute;
		if( defined_entity != NULL ){
			if( (defined_entity_is_subtype == (bool)LIST_aux) && 
				 	(attr->defined_in != defined_entity) ) return 2;
		}
		else{
			defined_entity = attr->defined_in;		
			defined_entity_is_subtype = LIST_aux;
		}
	}LISTod;
	
	assert(defined_entity != NULL);
	return 1;
}


/** find a (possibly inherited) attribute */
Variable ENTITY_find_inherited_attribute( Entity entity, char * name, int * down, struct Symbol_ ** where ) {
    Variable result;

    /* avoid searching scopes that we've already searched */
    /* this can happen due to several things */
    /* if A ref's B which ref's C, and A ref's C.  Then C */
    /* can be searched twice by A.  Similar problem with */
    /* sub/super inheritance. */
	if( SCOPE_search_visited(entity) ) return NULL;
	
    /* first look locally */
    result = ( Variable )DICTlookup( entity->symbol_table, name );
    if( result ) {
        if( down && *down && where ) {
            *where = &entity->symbol;
        }
        return result;
    }

    /* check supertypes */
    LISTdo( entity->u.entity->supertypes, super, Entity )
    result = ENTITY_find_inherited_attribute( super, name, down, where );
    if( result ) {
        return result;
    }
    LISTod;

    /* check subtypes, if requested */
    if( down ) {
        ++*down;
        LISTdo( entity->u.entity->subtypes, sub, Entity )
        result = ENTITY_find_inherited_attribute( sub, name, down, where );
        if( result ) {
            return result;
        }
        LISTod;
        --*down;
    }

    return 0;
}

Variable ENTITYfind_inherited_attribute( struct Scope_ *entity, char * name,
        struct Symbol_ ** down_where, bool under_search ) {
    int down_flag = 0;

	if( !under_search ) SCOPE_begin_search();
	Variable result;
	if( down_where ) {
        result = ENTITY_find_inherited_attribute( entity, name, &down_flag, down_where );
    } else {
        result = ENTITY_find_inherited_attribute( entity, name, NULL, NULL );
    }
	if( !under_search ) SCOPE_end_search();
	return result;
}

/** resolve a (possibly group-qualified) attribute ref.
 * report errors as appropriate
 */
Variable ENTITYresolve_attr_ref( Entity e, Symbol * grp_ref, Symbol * attr_ref ) {
	assert( e != NULL );	//*TY202/12/2
    extern Error ERROR_unknown_supertype;
    extern Error ERROR_unknown_attr_in_entity;
    Entity ref_entity;
    Variable attr;
    struct Symbol_ *where;

    if( grp_ref ) {
        /* use entity provided in group reference */
        ref_entity = ENTITYfind_inherited_entity( e, grp_ref->name, 0, false );
        if( !ref_entity ) {
            ERRORreport_with_symbol( ERROR_unknown_supertype, grp_ref,
                                     grp_ref->name, e->symbol.name );
            return 0;
        }
        attr = ( Variable )DICTlookup( ref_entity->symbol_table,
                                       attr_ref->name );
        if( !attr ) {
            ERRORreport_with_symbol( ERROR_unknown_attr_in_entity,
                                     attr_ref, attr_ref->name,
                                     ref_entity->symbol.name );
            /*      resolve_failed(e);*/
        }
    } else {
        /* no entity provided, look through supertype chain */
        where = NULL;
        attr = ENTITYfind_inherited_attribute( e, attr_ref->name, &where, false );
        if( !attr /* was ref_entity? */ ) {
            ERRORreport_with_symbol( ERROR_unknown_attr_in_entity,
                                     attr_ref, attr_ref->name,
                                     e->symbol.name );
        } else if( where != NULL ) {
            ERRORreport_with_symbol( ERROR_implicit_downcast, attr_ref,
                                     where->name );
        }
    }
    return attr;
}

/**
** low-level function for type Entity
**
** \note The attribute list of a new entity is defined as an
**  empty list; all other aspects of the entity are initially
**  undefined (i.e., have appropriate NULL values).
*/
Entity ENTITYcreate( Symbol * sym ) {
    Scope s = SCOPEcreate( OBJ_ENTITY );

    s->u.entity = ENTITY_new();
	s->u_tag = scope_is_entity;	//*TY2020/08/02
    s->u.entity->attributes = LISTcreate();
    s->u.entity->inheritance = ENTITY_INHERITANCE_UNINITIALIZED;

    /* it's so useful to have a type hanging around for each entity */
    s->u.entity->type = TYPEcreate_name( sym );
    s->u.entity->type->u.type->body = TYPEBODYcreate( entity_ );
    s->u.entity->type->u.type->body->entity = s;
    return( s );
}

/**
 * currently, this is only used by USEresolve
 * low-level function for type Entity
 */
Entity ENTITYcopy( Entity e ) {
    /* for now, do a totally shallow copy */
    Entity e2 = SCOPE_new();
    *e2 = *e;
    return e2;
}

/** Initialize the Entity module. */
void ENTITYinitialize() {
	//*TY2020/08/26
	current_filename = __FILE__;
	yylineno = __LINE__;

	MEMinitialize( &ENTITY_fl, sizeof( struct Entity_ ), 500, 100 );
    OBJcreate( OBJ_ENTITY, SCOPE_get_symbol, "entity",
               OBJ_ENTITY_BITS );
}

/**
** \param entity entity to modify
** \param attr attribute to add
** Add an attribute to an entity.
*/
void ENTITYadd_attribute( Entity entity, Variable attr ) {
	int rc;
	
	if( attr->name->type->u.type->body->type != op_ ) {
		/* simple id */
		rc = DICTdefine(	/*dict*/	entity->symbol_table, 
											/*name*/	attr->name->symbol.name,
											/*obj*/ 	( Generic )attr, 
											/*sym*/ 	&attr->name->symbol, 
											/*type*/ 	OBJ_VARIABLE );
	} else {
		/* SELF\ENTITY.SIMPLE_ID */
		rc = DICTdefine(	/*dict*/	entity->symbol_table, 
											/*name*/	attr->name->e.op2->symbol.name,
											/*obj*/		( Generic )attr, 
											/*sym*/		&attr->name->symbol, 
											/*type*/	OBJ_VARIABLE );
	}
	if( rc == DICTsuccess ) {
		LISTadd_last( entity->u.entity->attributes, ( Generic )attr );
		VARput_offset( attr, entity->u.entity->attribute_count );
		entity->u.entity->attribute_count++;
		
		//*TY2020/06/21 added
		attr->defined_in = entity;
	}
}

/**
** \param entity entity to modify
** \param instance new instance
** Add an item to the instance list of an entity.
*/
void ENTITYadd_instance( Entity entity, Generic instance ) {
    if( entity->u.entity->instances == LIST_NULL ) {
        entity->u.entity->instances = LISTcreate();
    }
    LISTadd_last( entity->u.entity->instances, instance );
}

//*TY2021/1/6
Entity ENTITYget_common_super( Entity e1, Entity e2) {
	if( e1 == e2 ) return e1;
	
	return NULL;
}


//*TY2020/07/11
bool ENTITYis_a( Entity entity, Entity sup ) {
	if( sup==NULL || entity==NULL ) return false;
	if( sup == entity ) return true;
	return ENTITYhas_supertype(entity, sup);
}


/**
** \param sub entity to check parentage of
** \param sup supertype to check for
** \return does child's superclass chain include sup ?
** Look for a certain entity in the supertype graph of an entity.
*/
bool ENTITYhas_supertype( Entity sub, Entity sup ) {
	LISTdo( sub->u.entity->supertypes, sup1, Entity ) {
		if( sup1 == sup ) {
			return true;
		}
		if( ENTITYhas_supertype( sup1, sup ) ) {
			return true;
		}
	}LISTod;
	return false;
}

/**
** \param child entity to check parentage of
** \param parent parent to check for
** \return is parent a direct supertype of child?
** Check whether an entity has a specific immediate superclass.
*/
bool ENTITYhas_immediate_supertype( Entity child, Entity parent ) {
    LISTdo( child->u.entity->supertypes, entity, Entity )
    if( entity == parent ) {
        return true;
    }
    LISTod;
    return false;
}

#if 0
/** called by ENTITYget_all_attributes(). \sa ENTITYget_all_attributes */
static void ENTITY_get_all_attributes( Entity entity, Linked_List result ) {
	//*TY2020/07/19
	if( entity->search_id == __SCOPE_search_id ) return;
	entity->search_id = __SCOPE_search_id;
	
    LISTdo( entity->u.entity->supertypes, super, Entity )
    /*  if (OBJis_kind_of(super, Class_Entity))*/
    ENTITY_get_all_attributes( super, result );
    LISTod;
    /* Gee, aren't they resolved by this time? */
    LISTdo( entity->u.entity->attributes, attr, Generic )
    LISTadd_last( result, attr );
    LISTod;
}

/**
 ** \param entity  - entity to examine
 ** \return all attributes of this entity
 **
 ** Retrieve the attribute list of an entity.
 ** \sa ENTITY_get_all_attributes()
 **
 ** \note   If an entity has neither defines nor inherits any
 **      attributes, this call returns an empty list.  Note
 **      that this is distinct from the constant LIST_NULL.
 */
Linked_List ENTITYget_all_attributes( Entity entity ) {
    Linked_List result = LISTcreate();
	//*TY2020/07/19
	__SCOPE_search_id++;

    ENTITY_get_all_attributes( entity, result );
    return result;
}
#endif
//*TY2020/07/19 alternative implementation; width-wise search
static void ENTITY_build_all_attributes( Linked_List queue, Dictionary result  ) {
	Entity entity;
	while ( (entity = LISTremove_first_if(queue)) != NULL ) {
		if( SCOPE_search_visited(entity) ) continue;
		
		LISTadd_all(queue, ENTITYget_supertypes(entity));

		LISTdo( ENTITYget_attributes(entity), attr, Generic ){
			char* attr_name = VARget_simple_name(attr);
			Linked_List attr_list = DICTlookup(result, attr_name);
			if( attr_list == NULL ) {
				attr_list = LISTcreate();
				DICTdefine(result, attr_name, attr_list, NULL, OBJ_UNKNOWN);
			}
			LISTadd_last(attr_list, attr);
		}LISTod;
	}
}
//*TY2021/01/10 added for subtype search
static void ENTITY_build_all_subtype_attributes( Linked_List queue, Dictionary result  ) {
	Entity entity;
	while ( (entity = LISTremove_first_if(queue)) != NULL ) {
		if( SCOPE_search_visited(entity) ) continue;
		
		LISTadd_all(queue, ENTITYget_subtypes(entity));

		LISTdo( ENTITYget_attributes(entity), attr, Generic ){
			char* attr_name = VARget_simple_name(attr);
			Linked_List attr_list = DICTlookup(result, attr_name);
			if( attr_list == NULL ) {
				attr_list = LISTcreate();
				DICTdefine(result, attr_name, attr_list, NULL, OBJ_UNKNOWN);
			}
			LISTadd_last_marking_aux(attr_list, attr, (Generic)true);
		}LISTod;
	}
}
Dictionary ENTITYget_all_attributes( Entity entity ) {
	assert(entity->u_tag==scope_is_entity);
	if( entity->u.entity->all_attributes ) return entity->u.entity->all_attributes;
	
	Dictionary result = DICTcreate(25);
	Linked_List queue = LISTcreate();
	LISTadd_last(queue, entity);
	
	SCOPE_begin_search();
	ENTITY_build_all_attributes(queue, result);
	LISTadd_all(queue, ENTITYget_subtypes(entity));
	ENTITY_build_all_subtype_attributes(queue, result);
	SCOPE_end_search();
	
	entity->u.entity->all_attributes = result;
	LISTfree(queue);
	return result;
}

//*TY2020/07/19
Variable ENTITYfind_attribute_effective_definition( Entity entity, const char* attr_name ) {
	if( ENTITYget_attr_ambiguous_count(entity, attr_name) > 1 ) return NULL;
		
	Dictionary all_attrs = ENTITYget_all_attributes(entity);
	Linked_List attr_defs = DICTlookup(all_attrs, attr_name);
	if( attr_defs == NULL ) return NULL;
	
	Variable effective_def = LISTget_first(attr_defs);
	return effective_def;	
}

//*TY2020/08/09
Linked_List ENTITYget_constructor_params( Entity entity ) {
	if( entity->u.entity->constructor_params ) return entity->u.entity->constructor_params;

	Linked_List params = entity->u.entity->constructor_params = LISTcreate();
	LISTdo( entity->u.entity->attributes, attr, Variable ) {
		if( VARis_redeclaring(attr) ) continue;
		if( VARis_derived(attr) ) continue;
		if( VARis_inverse(attr) ) continue;
		LISTadd_last(params, attr);
	}LISTod;

	return params;
}



//*TY2020/07/19
static void ENTITY_build_super_entitiy_list( Entity entity, Linked_List result ) {
	if( SCOPE_search_visited(entity) ) return;
	
	LISTdo( entity->u.entity->supertypes, super, Entity )
	ENTITY_build_super_entitiy_list(super, result);
	LISTod;
	
	LISTadd_last(result, entity);
}
Linked_List ENTITYget_super_entity_list( Entity entity ) {
	if( entity->u.entity->supertype_list ) return entity->u.entity->supertype_list;
	
	Linked_List result = LISTcreate();
	
	SCOPE_begin_search();
	ENTITY_build_super_entitiy_list( entity, result );
	SCOPE_end_search();
	
	entity->u.entity->supertype_list = result;
	return result;
}

//*TY2021/01/10
static void ENTITY_build_sub_entitiy_list( Entity entity, Linked_List result ) {
	if( SCOPE_search_visited(entity) ) return;
	
	LISTdo( entity->u.entity->subtypes, sub, Entity )
	ENTITY_build_sub_entitiy_list(sub, result);
	LISTod;
	
	LISTadd_last(result, entity);
}
Linked_List ENTITYget_sub_entity_list( Entity entity ) {
	if( entity->u.entity->subtype_list ) return entity->u.entity->subtype_list;
	
	Linked_List result = LISTcreate();
	
	SCOPE_begin_search();
	ENTITY_build_sub_entitiy_list( entity, result );
	SCOPE_end_search();
	
	entity->u.entity->subtype_list = result;
	return result;
}


/**
** \param entity  - entity to examine
** \param  name    - name of attribute to retrieve
** \return the named attribute of this entity
** Retrieve an entity attribute by name.
**
** \note   If the entity has no attribute with the given name,
**  VARIABLE_NULL is returned.
*/
Variable ENTITYget_named_attribute( Entity entity, char * name ) {
    Variable attribute;

    LISTdo( entity->u.entity->attributes, attr, Variable )
    if( streq( VARget_simple_name( attr ), name ) ) {
        return attr;
    }
    LISTod;

    LISTdo( entity->u.entity->supertypes, super, Entity )
    /*  if (OBJis_kind_of(super, Class_Entity) && */
    if( 0 != ( attribute = ENTITYget_named_attribute( super, name ) ) ) {
        return attribute;
    }
    LISTod;
    return 0;
}

/**
** \param entity entity to examine
** \param attribute attribute to retrieve offset for
** \return offset to given attribute
** Retrieve offset to an entity attribute.
**
** \note   If the entity does not include the attribute, -1
**  is returned.
*/
int ENTITYget_attribute_offset( Entity entity, Variable attribute ) {
    int offset;
    int value;

    LISTdo( entity->u.entity->attributes, attr, Variable )
    if( attr == attribute ) {
        return entity->u.entity->inheritance + VARget_offset( attribute );
    }
    LISTod;
    offset = 0;
    LISTdo( entity->u.entity->supertypes, super, Entity )
    /*  if (OBJis_kind_of(super, Class_Entity)) {*/
    if( ( value = ENTITYget_attribute_offset( super, attribute ) ) != -1 ) {
        return value + offset;
    }
    offset += ENTITYget_initial_offset( super );
    /*  }*/
    LISTod;
    return -1;
}

/**
** \param entity entity to examine
** \param name name of attribute to retrieve
** \return offset to named attribute of this entity
** Retrieve offset to an entity attribute by name.
**
** \note   If the entity has no attribute with the given name,
**      -1 is returned.
*/
int ENTITYget_named_attribute_offset( Entity entity, char * name ) {
    int offset;
    int value;

    LISTdo( entity->u.entity->attributes, attr, Variable )
    if( streq( VARget_simple_name( attr ), name ) )
        return entity->u.entity->inheritance +
               VARget_offset( ENTITY_find_inherited_attribute( entity, name, 0, 0 ) );
    LISTod;
    offset = 0;
    LISTdo( entity->u.entity->supertypes, super, Entity )
    /*  if (OBJis_kind_of(super, Class_Entity)) {*/
    if( ( value = ENTITYget_named_attribute_offset( super, name ) ) != -1 ) {
        return value + offset;
    }
    offset += ENTITYget_initial_offset( super );
    /*  }*/
    LISTod;
    return -1;
}

/**
** \param entity entity to examine
** \return number of inherited attributes
** Retrieve the initial offset to an entity's local frame.
*/
int ENTITYget_initial_offset( Entity entity ) {
    return entity->u.entity->inheritance;
}
