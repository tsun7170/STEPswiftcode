

/** **********************************************************************
** Module:  Symbol \file symbol.c
** This module implements the Symbol abstraction.
** Constants:
**  SYMBOL_NULL - the null Symbol
**
************************************************************************/

/*
 * This code was developed with the support of the United States Government,
 * and is not subject to copyright.
 *
 * $Log: symbol.c,v $
 * Revision 1.6  1997/01/21 19:19:51  dar
 * made C++ compatible
 *
 * Revision 1.5  1994/05/11  19:51:24  libes
 * numerous fixes
 *
 * Revision 1.4  1993/10/15  18:48:48  libes
 * CADDETC certified
 *
 * Revision 1.3  1992/08/18  17:23:43  libes
 * no change
 *
 * Revision 1.2  1992/05/31  23:32:26  libes
 * implemented ALIAS resolution
 *
 * Revision 1.1  1992/05/28  03:55:04  libes
 * Initial revision
 */

#include <string.h>	//*TY2021/01/30
#include <sc_memmgr.h>
#include "express/symbol.h"


struct freelist_head SYMBOL_fl;

/** Initialize the Symbol module */
void SYMBOLinitialize( void ) {
    MEMinitialize( &SYMBOL_fl, sizeof( struct Symbol_ ), 100, 100 );
}

Symbol * SYMBOLcreate( char * name, int line, const char * filename ) {
    Symbol * sym = SYMBOL_new();
    sym->name = name;
    sym->line = line;
    sym->filename = filename; /* NOTE this used the global 'current_filename',
                               * instead of 'filename'. This func is only
                               * called in two places, and both calls use
                               * 'current_filename'. Changed this to avoid
                               * potential future headaches. (MAP, Jan 12)
                               */
    return sym;
}

//*TY2020/09/19
#if !INLINE_SYMBOLset
struct Obj_ {
	Symbol symbol;
};

void SYMBOLset( void* o ) {
	static int last_lineno = -1;
	static Symbol* last_symbol = NULL;
	
	struct Obj_* obj = o;
	obj->symbol.line = yylineno;
	obj->symbol.filename = current_filename;
	
	if( yylineno != last_lineno ){
		if( last_symbol != NULL && last_symbol->name != NULL ){
			printf("SYMBOL[%s]\t at line: %d\n",last_symbol->name, last_symbol->line);
		}
		last_symbol = &(obj->symbol);
		last_lineno = yylineno;
	}
}
#endif

bool names_are_equal( const char* name1, const char* name2){
	bool equal_symbol = name1 == name2;
	if( !equal_symbol ){
		if(name1!=NULL && name2!=NULL){
			equal_symbol = strcmp(name1, name2)==0;
		}
	}
	return equal_symbol;
}
