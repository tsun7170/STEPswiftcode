/** \file pretty_case.c
 * split out of exppp.c 9/21/13
 */

#include <express/linklist.h>
#include <exppp/exppp.h>

#include "pp.h"
#include "pretty_expr.h"
#include "pretty_stmt.h"
#include "pretty_case.h"

void CASEout( struct Case_Statement_ * c, int level ) {
    int len = 0, max_indent = 0, old_curpos = 0;

    raw( "%*sCASE ", level, "" );
    EXPR_out( c->selector, 0 );
    wrap( " OF\n" );

    /* EXPRlength messes up curpos */
    old_curpos = curpos;
    /* pass 1: calculate length of longest label */
    LISTdo( c->cases, ci, Case_Item ) {
        if( ci->labels ) {
					len -= 2;
            LISTdo_n( ci->labels, label, Expression, b ) {
                len += EXPRlength( label ) +2;
            } LISTod
        } else {
            len = strlen( "OTHERWISE" );
        }
        if( len > max_indent ) {
            max_indent = len;
        }
    } LISTod
    curpos = old_curpos;

    level += exppp_nesting_indent;
    if( max_indent + level > exppp_linelength / 2 ) {
        max_indent = ( exppp_linelength / 3 ) - level;
    }

    /* pass 2: print them */
    LISTdo( c->cases, ci, Case_Item ) {
        if( ci->labels ) {
					indent2 = level + exppp_continuation_indent;
					raw( "%*s", level, "" );
					//*TY2020/08/26 fixed to produce condensed labels
					char* sep = "";
            LISTdo_n( ci->labels, label, Expression, b ) {
                /* print label(s) */
							raw(sep);
                EXPR_out( label, 0 );
							sep = ", ";
						} LISTod

					int spaces = level + max_indent - curpos - exppp_nesting_indent;
								raw( "%*s : ", ( ( spaces > 0 ) ? spaces : 0 ), "" );
                /* print action */
                STMT_out( ci->action, level + exppp_nesting_indent );
        } else {
            /* print OTHERWISE */
            indent2 = level + exppp_continuation_indent;
            raw( "%*s", level, "" );
            raw( "OTHERWISE" );
            raw( "%*s : ", level + max_indent - curpos - exppp_nesting_indent, "" );

            /* print action */
            STMT_out( ci->action, level + exppp_nesting_indent );
        }
    } LISTod
		level -= exppp_nesting_indent;
    raw( "%*sEND_CASE;\n", level, "" );
}
