

/*
 * This work was supported by the United States Government, and is
 * not subject to copyright.
 *
 * $Log: linklist.c,v $
 * Revision 1.3  1997/01/21 19:19:51  dar
 * made C++ compatible
 *
 * Revision 1.2  1993/10/15  18:49:55  libes
 * CADDETC certified
 *
 * Revision 1.4  1993/01/19  22:17:27  libes
 * *** empty log message ***
 *
 * Revision 1.3  1992/08/18  17:16:22  libes
 * rm'd extraneous error messages
 *
 * Revision 1.2  1992/06/08  18:08:05  libes
 * prettied up interface to print_objects_when_running
 */

#include <assert.h>
#include <sc_memmgr.h>
#include "express/linklist.h"


Error ERROR_empty_list = ERROR_none;
struct freelist_head LINK_fl;
struct freelist_head LIST_fl;

void LISTinitialize( void ) {
    MEMinitialize( &LINK_fl, sizeof( struct Link_ ), 500, 100 );
    MEMinitialize( &LIST_fl, sizeof( struct Linked_List_ ), 100, 50 );
    ERROR_empty_list = ERRORcreate( "Empty list in %s.", SEVERITY_ERROR );
}

void LISTcleanup( void ) {
    ERRORdestroy( ERROR_empty_list );
}

Linked_List LISTcreate() {
    Linked_List list = LIST_new();
    list->mark = LINK_new();
    list->mark->next = list->mark->prev = list->mark;
    return( list );
}

//*TY2020/09/04
void LISTadd_all(Linked_List list, Linked_List items) {
	int n = LISTget_length(list);
	
	LISTdo_links(items, srcnode) {
		Link node = LISTLINKadd_last(list);
		node->data = srcnode->data;
		node->aux = srcnode->aux;
	}LISTod;
	
	assert(LISTget_length(list) == n+LISTget_length(items));
}
void LISTadd_all_marking_aux(Linked_List list, Linked_List items, Generic aux) {
	LISTdo_links(items, srcnode) {
		Link node = LISTLINKadd_last(list);
		node->data = srcnode->data;
		node->aux = aux;
	}LISTod;
}
Linked_List LISTcopy( Linked_List src ) {
    Linked_List dst = LISTcreate();
#if 0
    LISTdo( src, x, Generic )
    LISTadd_last( dst, x );
    LISTod
#else
	//*TY2020/09/04 for aux support
	LISTadd_all(dst, src);
	assert(LISTget_length(dst) == LISTget_length(src));
#endif
    return dst;
}


void LISTfree( Linked_List list ) {
    Link p, q = list->mark->next;

    for( p = q->next; p != list->mark; q = p, p = p->next ) {
        LINK_destroy( q );
    }
    if( q != list->mark ) {
        LINK_destroy( q );
    }
    LINK_destroy( list->mark );
    LIST_destroy( list );
}

void LISTsort( Linked_List list, int (*comp)(void*, void*)) {
    unsigned int moved;
    Link node, prev;

    if (LISTis_empty(list))
        return;

    while (true) {
        for ( node = list->mark->next, moved = 0; node != list->mark; node = node->next ) {
            prev = node->prev;
            if (prev != list->mark && comp(prev->data, node->data) < 0) {
                LISTswap(prev, node);
                moved++;
            }
        }
        if (moved == 0)
            break;
    }
}

void LISTswap( Link p, Link q ) {
    Generic     tmp;

    if( p == LINK_NULL || q == LINK_NULL || p == q )
        return;

    tmp = p->data;
    p->data = q->data;
    q->data = tmp;
}

//*TY2020/09/04
Link LINKadd_after( Link node ) {
	Link newnode = LINK_new();
	Link next = node->next;

	newnode->next = next;
	newnode->prev = node;
	node->next = newnode;
	next->prev = newnode;
	
	return newnode;
}


Generic LISTadd_first( Linked_List list, Generic item ) {
#if 0
    Link        node;

    node = LINK_new();
    node->data = item;
    ( node->next = list->mark->next )->prev = node;
    ( list->mark->next = node )->prev = list->mark;
    return item;
#else
	//*TY2020/09/04
	int n = LISTget_length(list);

	Link node = LISTLINKadd_first(list);
	node->data = item;
	
	assert(LISTget_length(list) == n+1);
	return item;
#endif
}

Generic LISTadd_last( Linked_List list, Generic item ) {
	return LISTadd_last_marking_aux(list, item, NULL);
}
Generic LISTadd_last_marking_aux ( Linked_List list, Generic item, Generic aux ) {
	//*TY2020/09/04
	int n = LISTget_length(list);

	Link node = LISTLINKadd_last(list);
	node->data = item;
	node->aux = aux;
	
	assert(LISTget_length(list) == n+1);
	return item;
}

Generic LISTadd_after( Linked_List list, Link link, Generic item ) {
#if 0
    Link node;

    if( link == LINK_NULL ) {
        LISTadd_first( list, item );
    } else {
        node = LINK_new();
        node->data = item;
        ( node->next = link->next )->prev = node;
        ( link->next = node )->prev = link;
    }
    return item;
#else
	//*TY2020/09/04
	if( link == LINK_NULL ) return LISTadd_first(list, item);
	
	Link node = LINKadd_after(link);
	node->data = item;
	return item;
#endif
}

Generic LISTadd_before( Linked_List list, Link link, Generic item ) {
#if 0
    Link node;

    if( link == LINK_NULL ) {
        LISTadd_last( list, item );
    } else {
        node = LINK_new();
        node->data = item;

        link->prev->next = node;    /* fix up previous link */
        node->prev = link->prev;
        node->next = link;
        link->prev = node;      /* fix up next link */
    }
    return item;
#else
	//*TY2020/09/04
	if( link == LINK_NULL ) return LISTadd_last(list, item);
	
	Link node = LINKadd_before(link);
	node->data = item;
	return item;
#endif
}


Generic LISTremove_first( Linked_List list ) {
    Link        node;
    Generic     item;

    node = list->mark->next;
    if( node == list->mark ) {
        ERRORreport( ERROR_empty_list, "LISTremove_first" );
        return NULL;
    }
    item = node->data;
#if 0
    ( list->mark->next = node->next )->prev = list->mark;
    LINK_destroy( node );
#else
	//*TY2020/09/04
	LINKremove(node);
#endif
    return item;
}

//*TY2020/07/19
Generic LISTremove_first_if( Linked_List list ) {
	Link        node;
	Generic     item;
	
	node = list->mark->next;
	if( node == list->mark ) {
		return NULL;
	}
	item = node->data;
	LINKremove(node);
	return item;
}
void LINKremove( Link node ) {
	Link prev = node->prev;
	Link next = node->next;
	prev->next = next;
	next->prev = prev;
	LINK_destroy(node);
}


Generic LISTget_first( Linked_List list ) {
    Link node;
    Generic item;

    node = list->mark->next;
    if( node == list->mark ) {
        return NULL;
    }
    item = node->data;
    return item;
}
//*TY2021/01/10
Generic LISTget_first_aux( Linked_List list ) {
		Link node;
		Generic item;

		node = list->mark->next;
		if( node == list->mark ) {
				return NULL;
		}
		item = node->aux;
		return item;
}

Generic LISTget_second( Linked_List list ) {
    Link        node;
    Generic     item;

    node = list->mark->next;
    if( node == list->mark ) {
        return NULL;
    }
    node = node->next;
    if( node == list->mark ) {
        return NULL;
    }
    item = node->data;
    return item;
}

/** first is 1, not 0 */
Generic LISTget_nth( Linked_List list, int n ) {
    int count = 1;
    Link node;

    for( node = list->mark->next; node != list->mark; node = node->next ) {
        if( n == count++ ) {
            return( node->data );
        }
    }
    return( 0 );
}

int LISTget_length( Linked_List list ) {
    Link node;
    int count = 0;

    if( !list ) {
        return 0;
    }

    for( node = list->mark->next; node != list->mark; node = node->next ) {
			assert(node->data!=NULL);
        count++;
    }
    return count;
}

bool LISTis_empty( Linked_List list ) {
    if( !list ) {
        return true;
    }
    if( list->mark->next == list->mark ) {
        return true;
    }
    return false;
}
