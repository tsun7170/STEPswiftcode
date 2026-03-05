#ifndef DICTIONARY_H
#define DICTIONARY_H

/** **********************************************************************
** Module:  Dictionary \file dict.h
** Description: This module implements the dictionary abstraction.  A
**  dictionary is a repository for a number of objects, all of which
**  can be named using the same function.  A dictionary is limited to
**  storing items which can be cast to Generic (void*); in addition,
**  the constant NULL is used to report errors, so this is one value
**  which it is probably not a good idea to store in a dictionary.
** Constants:
**  DICTIONARY_NULL - the null dictionary
**
************************************************************************/

/*
 * This work was supported by the United States Government, and is
 * not subject to copyright.
 *
 * $Log: dict.h,v $
 * Revision 1.4  1997/01/21 19:17:11  dar
 * made C++ compatible
 *
 * Revision 1.3  1994/11/10  19:20:03  clark
 * Update to IS
 *
 * Revision 1.2  1993/10/15  18:49:23  libes
 * CADDETC certified
 *
 * Revision 1.5  1993/01/19  22:45:07  libes
 * *** empty log message ***
 *
 * Revision 1.4  1992/08/18  17:15:40  libes
 * rm'd extraneous error messages
 *
 * Revision 1.3  1992/06/08  18:07:35  libes
 * prettied up interface to print_objects_when_running
 */

/*************/
/* constants */
/*************/

#define DICTIONARY_NULL (Dictionary)NULL

/*****************/
/* packages used */
/*****************/

#include "hash.h"
#include "error.h"

/************/
/* typedefs */
/************/

typedef struct Hash_Table_ * Dictionary;
typedef HashEntry       DictionaryEntry;

/****************/
/* modules used */
/****************/

#include "symbol.h"

/***************************/
/* hidden type definitions */
/***************************/

/********************/
/* global variables */
/********************/

extern SC_EXPRESS_EXPORT char DICT_type;  /**< set as a side-effect of DICT lookup routines to type of object found */
//*TY2020/09/05
extern const char* DICT_key;	/**< set as a side-effect of DICTdo routines to the key of current entry */

/*******************************/
/* macro function definitions */
/*******************************/

#define DICTcreate(estimated_max_size)  HASHcreate(estimated_max_size)
/** should really can DICTdo_init and rename do_type_init to do_init! */
#define DICTdo_init(dict,de)        HASHlistinit((dict),(de))
#define DICTdo_type_init(dict,de,t) HASHlistinit_by_type((dict),(de),(t))
#define DICTdo_end(hash_entry)      HASHlistend(hash_entry)

/** modify dictionary entry in-place */
#define DICTchange(e,obj,sym,typ)   { \
                    (e)->data = (obj); \
                    (e)->symbol = (sym); \
                    (e)->type = (typ); \
                    }
#define DICTchange_type(e,typ)      (e)->type = (typ)

/***********************/
/* function prototypes */
/***********************/

extern SC_EXPRESS_EXPORT void     DICTinitialize PROTO( ( void ) );
extern SC_EXPRESS_EXPORT void     DICTcleanup PROTO( ( void ) );

/// Defines an entry in the specified dictionary.
///
/// If an entry with the same name already exists:
/// - If both the new and existing entries are enumerations, the old entry is marked as ambiguous.
/// - If both are non-enumerations, an error is reported and the definition fails.
/// - If one is an enumeration and the other is not, both are allowed as per newer scoping rules.
///
/// - Parameters:
///   - dict: The dictionary in which to define the entry.
///   - name: The key for the entry.
///   - obj: The value (generic object) to store.
///   - sym: Associated symbol, for error reporting and metadata.
///   - type: The type code of the object being defined.
/// - Returns: `0` on success, `1` on failure (e.g., duplicate non-enum definition).
///
/// This function may set error state and cause side effects in global error tracking variables.
/// 
extern SC_EXPRESS_EXPORT int      DICTdefine PROTO( ( Dictionary, const char *, Generic, Symbol *, char ) );
extern SC_EXPRESS_EXPORT int      DICT_define PROTO( ( Dictionary, const char *, Generic, Symbol *, char ) );
extern SC_EXPRESS_EXPORT void     DICTundefine PROTO( ( Dictionary, const char * ) );
extern SC_EXPRESS_EXPORT Generic      DICTlookup PROTO( ( Dictionary, const char * ) );
extern SC_EXPRESS_EXPORT Generic      DICTlookup_symbol PROTO( ( Dictionary, const char *, Symbol ** ) );
extern SC_EXPRESS_EXPORT Generic      DICTdo PROTO( ( DictionaryEntry * ) );
extern SC_EXPRESS_EXPORT void     DICTprint PROTO( ( Dictionary ) );

//*TY2020/06/28 added
#define DICTsuccess	0
#define DICTfailure 1
extern const char* DICTdo_key(DictionaryEntry* dict_entry);
extern Element DICTdo_tuple(DictionaryEntry* dict_entry);
extern int DICTcount_type(Dictionary dict, char type);
#define DICTcount(dict)		DICTcount_type(dict,OBJ_ANY)

#endif /*DICTIONARY_H*/
