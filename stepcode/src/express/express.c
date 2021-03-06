

/**
** \file express.c
** Express package manager.
************************************************************************/

/*
 * This code was developed with the support of the United States Government,
 * and is not subject to copyright.
 *
 * $Log: express.c,v $
 * Revision 1.20  1997/01/21 19:53:20  dar
 * made C++ compatible
 *
 * Revision 1.18  1995/06/08 22:59:59  clark
 * bug fixes
 *
 * Revision 1.17  1995/05/17  14:25:56  libes
 * Fixed bug in EXPRESS_PATH handling.  Improved debugging diagnostics.
 *
 * Revision 1.16  1995/04/05  13:55:40  clark
 * CADDETC preval
 *
 * Revision 1.15  1995/03/09  18:42:21  clark
 * various fixes for caddetc - before interface clause changes
 *
 * Revision 1.14  1995/02/09  18:00:49  clark
 * update version string
 *
 * Revision 1.13  1994/11/10  19:20:03  clark
 * Update to IS
 *
 * Revision 1.12  1994/05/11  19:51:24  libes
 * numerous fixes
 *
 * Revision 1.11  1993/10/15  18:48:48  libes
 * CADDETC certified
 *
 * Revision 1.9  1993/02/22  21:45:06  libes
 * fix pass print bug
 *
 * Revision 1.8  1993/02/16  03:19:56  libes
 * added unwriteable error
 *
 * Revision 1.7  1993/01/19  22:44:17  libes
 * *** empty log message ***
 *
 * Revision 1.6  1992/09/16  18:19:14  libes
 * fixed bug in EXPRESS_PATH searching
 *
 * Revision 1.5  1992/08/27  23:38:35  libes
 * removed redundant call to initialize much of EXPRESS
 * rewrote schema-rename-connect to use fifo instead of dictionary
 * fixed smashing of file names by new schema files
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
 */

#include "sc_memmgr.h"
#include <ctype.h>
#include <stdlib.h>
#include <setjmp.h>
#include <errno.h>

#include "express/basic.h"
#include "express/express.h"
#include "express/resolve.h"
#include "stack.h"
#include "express/scope.h"
#include "token_type.h"
#include "expparse.h"
#include "expscan.h"
#include "parse_data.h"
#include "express/lexact.h"
#include "express/exp_kw.h"

#include "express/expr.h"
#include "builtin.h"

extern const char *const yyTokenName[];	//*TY2020/08/23

void * ParseAlloc( void * ( *mallocProc )( size_t ) );
void ParseFree( void * parser, void ( *freeProc )( void * ) );
void Parse( void * parser, int tokenID, YYSTYPE data, parse_data_t parseData );
void ParseTrace(FILE *TraceFILE, char *zTracePrompt);

Linked_List EXPRESS_path;
int EXPRESSpass;

void ( *EXPRESSinit_args ) PROTO( ( int, char ** ) )   = 0;
void ( *EXPRESSinit_parse ) PROTO( ( void ) )     = 0;
int ( *EXPRESSfail ) PROTO( ( Express ) )        = 0;
int ( *EXPRESSsucceed ) PROTO( ( Express ) )     = 0;
void ( *EXPRESSbackend ) PROTO( ( Express ) )     = 0;
char * EXPRESSprogram_name;
extern char   EXPRESSgetopt_options[];  /* initialized elsewhere */
int ( *EXPRESSgetopt ) PROTO( ( int, char * ) )   = 0;
bool    EXPRESSignore_duplicate_schemas      = false;

Dictionary EXPRESSbuiltins; /* procedures/functions */

Error ERROR_bail_out        = ERROR_none;
Error ERROR_syntax      = ERROR_none;
Error ERROR_unlabelled_param_type = ERROR_none;
Error ERROR_file_unreadable;
Error ERROR_file_unwriteable;
Error ERROR_warn_unsupported_lang_feat;
Error ERROR_warn_small_real;

struct Scope_ * FUNC_NVL;
struct Scope_ * FUNC_USEDIN;
extern Express yyexpresult;

static Error ERROR_ref_nonexistent;
static Error ERROR_tilde_expansion_failed;
static Error ERROR_schema_not_in_own_schema_file;

extern Linked_List PARSEnew_schemas;
void SCOPEinitialize( void );

static Express PARSERrun PROTO( ( char *, FILE * ) );

/** name specified on command line */
char * input_filename = 0;

int EXPRESS_fail( Express model ) {
    ERRORflush_messages();

    if( EXPRESSfail ) {
        return( ( *EXPRESSfail )( model ) );
    }

    fprintf( stderr, "Errors in input\n" );
    return 1;
}

int EXPRESS_succeed( Express model ) {
    if( EXPRESSsucceed ) {
        return( ( *EXPRESSsucceed )( model ) );
    }

    fprintf( stderr, "No errors in input\n" );
    return 0;
}

Symbol * EXPRESS_get_symbol( Generic e ) {
    return( &( ( Express )e )->symbol );
}

Express EXPRESScreate() {
    Express model = SCOPEcreate( OBJ_EXPRESS );
    model->u.express = ( struct Express_ * )sc_calloc( 1, sizeof( struct Express_ ) );
	model->u_tag = scope_is_express;	//*TY2020/08/02
    return model;
}

void EXPRESSdestroy( Express model ) {
    if( model->u.express->basename ) {
        sc_free( model->u.express->basename );
    }
    if( model->u.express->filename ) {
        sc_free( model->u.express->filename );
    }
    sc_free( model->u.express );
    SCOPEdestroy( model );
}

#define MAX_SCHEMA_FILENAME_SIZE    256
typedef struct Dir {
    char full[MAX_SCHEMA_FILENAME_SIZE];
    char * leaf;
} Dir;

static void EXPRESS_PATHinit() {
    char * p;
    Dir * dir;

    EXPRESS_path = LISTcreate();
    p = getenv( "EXPRESS_PATH" );
    if( !p ) {
        /* if no EXPRESS_PATH, search current directory anyway */
        dir = ( Dir * )sc_malloc( sizeof( Dir ) );
        dir->leaf = dir->full;
        LISTadd_last( EXPRESS_path, ( Generic )dir );
    } else {
        int done = 0;
        while( !done ) {
            char * start;   /* start of current dir */
            int length; /* length of dir */
            char * slash;   /* last slash in dir */
            char save;  /* place to character from where we */
            /* temporarily null terminate */

            /* get next directory */
            while( isspace( *p ) ) {
                p++;
            }
            if( *p == '\0' ) {
                break;
            }
            start = p;

            /* find the end of the directory */
            while( *p != '\0' && !isspace( *p ) ) {
                p++;
            }
            save = *p;
            if( *p == 0 ) {
                done = 1;
            } else {
                *p = '\0';
            }
            p++;    /* leave p after terminating null */

            dir = ( Dir * )sc_malloc( sizeof( Dir ) );

            /* if it's just ".", make it as if it was */
            /* just "" to make error messages cleaner */
            if( streq( ".", start ) ) {
                dir->leaf = dir->full;
                LISTadd_last( EXPRESS_path, ( Generic )dir );
                *( p - 1 ) = save; /* put char back where */
                /* temp null was */
                continue;
            }

            length = (int)(( p - 1 ) - start);

            /* if slash present at end, don't add another */
            slash = strrchr( start, '/' );
            if( slash && ( slash[1] == '\0' ) ) {
                strcpy( dir->full, start );
                dir->leaf = dir->full + length;
            } else {
                sprintf( dir->full, "%s/", start );
                dir->leaf = dir->full + length + 1;
            }
            LISTadd_last( EXPRESS_path, ( Generic )dir );

            *( p - 1 ) = save; /* put char back where temp null was */
        }
    }
}

static void EXPRESS_PATHfree( void ) {
    LISTdo( EXPRESS_path, dir, Dir * )
    sc_free( dir );
    LISTod
    LISTfree( EXPRESS_path );
}

/** inform object system about bit representation for handling pass diagnostics */
void PASSinitialize() {
    OBJcreate( OBJ_PASS, UNK_get_symbol, "pass", OBJ_PASS_BITS );
}

//*TY2020/09/20 made these extern
Function
func_abs,   func_acos,  func_asin,  func_atan,
func_blength,
func_cos,   func_exists,    func_exp,   func_format,
func_hibound,   func_hiindex,   func_length,    func_lobound,
func_log,   func_log10, func_log2,  func_loindex,
func_odd,   func_rolesof,   func_sin,   func_sizeof,
func_sqrt,  func_tan,   func_typeof,
func_value, func_value_in,  func_value_unique;
Procedure
proc_insert,    proc_remove;

/** Initialize the Express package. */
void EXPRESSinitialize( void ) {

    _MEMinitialize();
    ERRORinitialize();
    OBJinitialize();

    HASHinitialize();   /* comes first - used by just about everything else! */
    DICTinitialize();
    LISTinitialize();   /* ditto */
    ERRORinitialize_after_LIST();
    STACKinitialize();
    PASSinitialize();

    RESOLVEinitialize();

    SYMBOLinitialize();

    SCOPEinitialize();
    TYPEinitialize();   /* cannot come before SCOPEinitialize */
    VARinitialize();

    ALGinitialize();
    ENTITYinitialize();
    SCHEMAinitialize();

    CASE_ITinitialize();
    EXPinitialize();
    /*    LOOP_CTLinitialize();*/
    STMTinitialize();

    SCANinitialize();

    EXPRESSbuiltins = DICTcreate( 35 );
	//*TY2021/10/30
#define RETURN_DETERMINATE	true
#define RETURN_INDETERMINATE false
#define funcdef(x,y,c,r,opt) \
            x = ALGcreate(OBJ_FUNCTION);\
            x->symbol.name = y;\
            x->u.func->pcount = c; \
            x->u.func->parameters = LISTcreate(); \
            x->u.func->return_type = r; \
						x->u.func->return_determinate = opt; \
            x->u.func->builtin = true; \
            resolved_all(x); \
            DICTdefine(EXPRESSbuiltins,y,(Generic)x,0,OBJ_FUNCTION);
#define procdef(x,y,c)  x = ALGcreate(OBJ_PROCEDURE);\
            x->symbol.name = y;\
            x->u.proc->pcount = c; \
            x->u.proc->parameters = LISTcreate(); \
            x->u.proc->builtin = true; \
            resolved_all(x); \
            DICTdefine(EXPRESSbuiltins,y,(Generic)x,0,OBJ_PROCEDURE);
//*TY2020/08/02
#define YES_VAR		1
#define NO__VAR 	0
#define YES_OPT		1
#define NO__OPT 	0
#define funcdef_param(x,n,t,o,v)										\
					{																					\
						Variable p = VAR_new();									\
						p->type = t;														\
						p->name = EXPcreate_simple(t);					\
						p->name->symbol.name = n;								\
						p->flags.optional = o;									\
						p->flags.var = v;												\
						LISTadd_last(x->u.func->parameters, p);	\
					}
#define procdef_param(x,n,t,o,v)										\
					{																					\
						Variable p = VAR_new();									\
						p->type = t;														\
						p->name = EXPcreate_simple(t);					\
						p->name->symbol.name = n;								\
						p->flags.optional = o;									\
						p->flags.var = v;												\
						LISTadd_last(x->u.proc->parameters, p);	\
					}
	
    /* third arg is # of parameters */

    /* eventually everything should be data-driven, but for now */
    /* uppercase def's are global to allow passing necessary information */
    /* into resolver */
    procdef( proc_insert, KW_INSERT, 3 );
		procdef_param(proc_insert, "L", Type_List_Of_Generic, YES_OPT, YES_VAR);
		procdef_param(proc_insert, "E", Type_Generic, YES_OPT, NO__VAR);
		procdef_param(proc_insert, "P", Type_Integer_Generic, YES_OPT, NO__VAR);
	
    procdef( proc_remove, KW_REMOVE, 2 );
		procdef_param(proc_remove, "L", Type_List_Of_Generic, YES_OPT, YES_VAR);
		procdef_param(proc_remove, "P", Type_Integer_Generic, YES_OPT, NO__VAR);
	

    funcdef( func_abs,    KW_ABS,    1, Type_Number, RETURN_INDETERMINATE );
		funcdef_param(func_abs, "V", Type_Number_Generic, YES_OPT, NO__VAR);
	
    funcdef( func_acos,   KW_ACOS,   1, Type_Real, RETURN_INDETERMINATE );
		funcdef_param(func_acos, "V", Type_Number_Generic, YES_OPT, NO__VAR);

    funcdef( func_asin,   KW_ASIN,   1, Type_Real, RETURN_INDETERMINATE );
		funcdef_param(func_asin, "V", Type_Number_Generic, YES_OPT, NO__VAR);


    funcdef( func_atan,   KW_ATAN,   2, Type_Real, RETURN_INDETERMINATE );
		funcdef_param(func_atan, "V1", Type_Number_Generic, YES_OPT, NO__VAR);
		funcdef_param(func_atan, "V2", Type_Number_Generic, YES_OPT, NO__VAR);

    funcdef( func_blength, KW_BLENGTH, 1, Type_Integer, RETURN_INDETERMINATE );
		funcdef_param(func_blength, "V", Type_Binary_Generic, YES_OPT, NO__VAR);

    funcdef( func_cos,    KW_COS,    1, Type_Real, RETURN_INDETERMINATE );
		funcdef_param(func_cos, "V", Type_Number_Generic, YES_OPT, NO__VAR);

    funcdef( func_exists, KW_EXISTS, 1, Type_Boolean, RETURN_DETERMINATE );
		funcdef_param(func_exists, "V", Type_Generic, YES_OPT, NO__VAR);

    funcdef( func_exp,    KW_EXP,    1, Type_Real, RETURN_INDETERMINATE );
		funcdef_param(func_exp, "V", Type_Number_Generic, YES_OPT, NO__VAR);

    funcdef( func_format, KW_FORMAT, 2, Type_String, RETURN_INDETERMINATE );
		funcdef_param(func_format, "N", Type_Number_Generic, YES_OPT, NO__VAR);
		funcdef_param(func_format, "F", Type_String_Generic, YES_OPT, NO__VAR);

    funcdef( func_hibound, KW_HIBOUND, 1, Type_Integer, RETURN_INDETERMINATE );
		funcdef_param(func_hibound, "V", Type_Aggregate_Of_Generic, YES_OPT, NO__VAR);

    funcdef( func_hiindex, KW_HIINDEX, 1, Type_Integer, RETURN_INDETERMINATE );
		funcdef_param(func_hiindex, "V", Type_Aggregate_Of_Generic, YES_OPT, NO__VAR);

    funcdef( func_length, KW_LENGTH, 1, Type_Integer, RETURN_INDETERMINATE );
		funcdef_param(func_length, "V", Type_String_Generic, YES_OPT, NO__VAR);

    funcdef( func_lobound, KW_LOBOUND, 1, Type_Integer, RETURN_INDETERMINATE );
		funcdef_param(func_lobound, "V", Type_Aggregate_Of_Generic, YES_OPT, NO__VAR);

    funcdef( func_log,    KW_LOG,    1, Type_Real, RETURN_INDETERMINATE );
		funcdef_param(func_log, "V", Type_Number_Generic, YES_OPT, NO__VAR);

    funcdef( func_log10,  KW_LOG10,  1, Type_Real, RETURN_INDETERMINATE );
		funcdef_param(func_log10, "V", Type_Number_Generic, YES_OPT, NO__VAR);

    funcdef( func_log2,   KW_LOG2,   1, Type_Real, RETURN_INDETERMINATE );
		funcdef_param(func_log2, "V", Type_Number_Generic, YES_OPT, NO__VAR);

    funcdef( func_loindex, KW_LOINDEX, 1, Type_Integer, RETURN_INDETERMINATE );
		funcdef_param(func_loindex, "V", Type_Aggregate_Of_Generic, YES_OPT, NO__VAR);

    funcdef( FUNC_NVL,    KW_NVL,    2, Type_Generic, RETURN_DETERMINATE );
		funcdef_param(FUNC_NVL, "V", Type_Generic, YES_OPT, NO__VAR);
		funcdef_param(FUNC_NVL, "SUBSTITUTE", Type_Generic, YES_OPT, NO__VAR);

    funcdef( func_odd,    KW_ODD,    1, Type_Logical, RETURN_DETERMINATE );
		funcdef_param(func_odd, "V", Type_Integer_Generic, YES_OPT, NO__VAR);

    funcdef( func_rolesof, KW_ROLESOF, 1, Type_Set_Of_String, RETURN_DETERMINATE );
		funcdef_param(func_rolesof, "V", Type_Generic, YES_OPT, NO__VAR);

    funcdef( func_sin,    KW_SIN,    1, Type_Real, RETURN_INDETERMINATE );
		funcdef_param(func_sin, "V", Type_Number_Generic, YES_OPT, NO__VAR);

    funcdef( func_sizeof, KW_SIZEOF, 1, Type_Integer, RETURN_INDETERMINATE );
		funcdef_param(func_sizeof, "V", Type_Aggregate_Of_Generic, YES_OPT, NO__VAR);

    funcdef( func_sqrt,   KW_SQRT,   1, Type_Real, RETURN_INDETERMINATE );
		funcdef_param(func_sqrt, "V", Type_Number_Generic, YES_OPT, NO__VAR);

    funcdef( func_tan,    KW_TAN,    1, Type_Real, RETURN_INDETERMINATE );
		funcdef_param(func_tan, "V", Type_Number_Generic, YES_OPT, NO__VAR);

    funcdef( func_typeof, KW_TYPEOF, 1, Type_Set_Of_String, RETURN_DETERMINATE );
		funcdef_param(func_typeof, "V", Type_Generic, YES_OPT, NO__VAR);

    funcdef( FUNC_USEDIN, KW_USEDIN,  2, Type_Bag_Of_GenericEntity, RETURN_DETERMINATE );
		funcdef_param(FUNC_USEDIN, "T", Type_Generic, YES_OPT, NO__VAR);
		funcdef_param(FUNC_USEDIN, "R", Type_String_Generic, YES_OPT, NO__VAR);

    funcdef( func_value,  KW_VALUE,   1, Type_Number, RETURN_INDETERMINATE );
		funcdef_param(func_value, "V", Type_String_Generic, YES_OPT, NO__VAR);

    funcdef( func_value_in, KW_VALUE_IN,    2, Type_Logical, RETURN_DETERMINATE );
		funcdef_param(func_value_in, "C", Type_Aggregate_Of_Generic, YES_OPT, NO__VAR);
		funcdef_param(func_value_in, "V", Type_Generic, YES_OPT, NO__VAR);

    funcdef( func_value_unique, KW_VALUE_UNIQUE, 1, Type_Logical, RETURN_DETERMINATE );
		funcdef_param(func_value_unique, "V", Type_Aggregate_Of_Generic, YES_OPT, NO__VAR);

	
	
    ERROR_bail_out = ERRORcreate( "Aborting due to internal error(s)", SEVERITY_DUMP );
    ERROR_syntax = ERRORcreate( "%s in %s %s", SEVERITY_EXIT );
    /* i.e., "syntax error in procedure foo" */
    ERROR_ref_nonexistent = ERRORcreate( "USE/REF of non-existent object (%s in schema %s)", SEVERITY_ERROR );
    ERROR_tilde_expansion_failed = ERRORcreate(
            "Tilde expansion for %s failed in EXPRESS_PATH environment variable", SEVERITY_ERROR );
    ERROR_schema_not_in_own_schema_file = ERRORcreate(
            "Schema %s was not found in its own schema file (%s)", SEVERITY_ERROR );
    ERROR_unlabelled_param_type = ERRORcreate(
            "Return type or local variable requires type label in `%s'", SEVERITY_ERROR );
    ERROR_file_unreadable = ERRORcreate( "Could not read file %s: %s", SEVERITY_ERROR );
    ERROR_file_unwriteable = ERRORcreate( "Could not write file %s: %s", SEVERITY_ERROR );
    ERROR_warn_unsupported_lang_feat = ERRORcreate( "Unsupported language feature (%s) at %s:%d", SEVERITY_WARNING );
    ERROR_warn_small_real = ERRORcreate( "REALs with extremely small magnitude may be interpreted as zero by other EXPRESS parsers "
                                         "(IEEE 754 float denormals are sometimes rounded to zero) - fabs(%f) <= FLT_MIN.", SEVERITY_WARNING );

    OBJcreate( OBJ_EXPRESS, EXPRESS_get_symbol, "express file", OBJ_UNUSED_BITS );

/* I don't think this should be a mere warning; exppp crashes if this warning is suppressed.
 *     ERRORcreate_warning( "unknown_subtype", ERROR_unknown_subtype );
 */
    ERRORcreate_warning( "unsupported", ERROR_warn_unsupported_lang_feat );
    ERRORcreate_warning( "limits", ERROR_warn_small_real );

    EXPRESS_PATHinit(); /* note, must follow defn of errors it needs! */
}

/** Clean up the EXPRESS package. */
void EXPRESScleanup( void ) {
    EXPRESS_PATHfree();

    ERRORdestroy( ERROR_bail_out );
    ERRORdestroy( ERROR_syntax );
    ERRORdestroy( ERROR_ref_nonexistent );
    ERRORdestroy( ERROR_tilde_expansion_failed );
    ERRORdestroy( ERROR_schema_not_in_own_schema_file );
    ERRORdestroy( ERROR_unlabelled_param_type );
    ERRORdestroy( ERROR_file_unreadable );
    ERRORdestroy( ERROR_file_unwriteable );
    ERRORdestroy( ERROR_warn_unsupported_lang_feat );
    ERRORdestroy( ERROR_warn_small_real );

    DICTcleanup();
    OBJcleanup();
    ERRORcleanup();
    RESOLVEcleanup();
    TYPEcleanup();
    EXPcleanup();
    SCANcleanup();
    LISTcleanup();
}

/**
** \param filename  - Express source file to parse
** return resulting Working Form model
** Parse an Express source file into the Working Form.
*/
void EXPRESSparse( Express model, FILE * fp, char * filename ) {
    yyexpresult = model;

    if( !fp ) {
        fp = fopen( filename, "r" );
    }
    if( !fp ) {
        /* go down path looking for file */
        LISTdo( EXPRESS_path, dir, Dir * )
        sprintf( dir->leaf, "%s", filename );
			printf("searching: '%s' ...\n",dir->full);
        if( 0 != ( fp = fopen( dir->full, "r" ) ) ) {
            filename = dir->full;
            break;
        }
        LISTod
    }

    if( !fp ) {
        ERRORreport( ERROR_file_unreadable, filename, strerror( errno ) );
        return;
    }

    if( filename ) {
        char * dot   = strrchr( filename, '.' );
        char * slash = strrchr( filename, '/' );

        /* get beginning of basename */
        char * start = slash ? ( slash + 1 ) : filename;

        int length = (int)strlen( start );

        /* drop .exp suffix if present */
        if( dot && streq( dot, ".exp" ) ) {
            length -= 4;
        }

        model->u.express->basename = ( char * )sc_malloc( length + 1 );
        memcpy( model->u.express->basename, filename, length );
        model->u.express->basename[length] = '\0';

        /* get new copy of filename to avoid being smashed */
        /* by subsequent lookups on EXPRESS_path */
        model->u.express->filename = SCANstrdup( filename );
        filename = model->u.express->filename;
    }

    PARSEnew_schemas = LISTcreate();
    PARSERrun( filename, model->u.express->file = fp );
}

/* TODO LEMON ought to put this in expparse.h */
void parserInitState(void);

/** start parsing a new schema file */
static Express PARSERrun( char * filename, FILE * fp ) {
    extern void SCAN_lex_init PROTO( ( char *, FILE * ) );
    extern YYSTYPE yylval;
    extern int yyerrstatus;
    int tokenID;
    parse_data_t parseData;

    void * parser = ParseAlloc( malloc );
    perplex_t scanner = perplexFileScanner( fp );
    parseData.scanner = scanner;

    if( print_objects_while_running & OBJ_PASS_BITS ) {
        fprintf( stderr, "parse (pass %d)\n", EXPRESSpass );
    }

    if( print_objects_while_running & OBJ_SCHEMA_BITS ) {
        fprintf( stderr, "parse: %s (schema file)\n", filename );
    }

    SCAN_lex_init( filename, fp );
    parserInitState();

    yyerrstatus = 0;
	
	//*TY2020/09/20
	yylineno = 1;
	
    /* NOTE uncomment the next line to enable parser tracing */
    /* ParseTrace( stderr, "- expparse - " ); */
	int itoken = 0;
    while( ( tokenID = yylex( scanner ) ) > 0 ) {
			++itoken;
        Parse( parser, tokenID, yylval, parseData );
			//*TY2020/08/23
#if 0
			if(itoken >= 159000)
			{
				ParseTrace(stdout, "yy:");
				char token_text[BUFSIZ];
				snprintf(token_text, BUFSIZ, "%*s",(int)(parseData.scanner->cursor - parseData.scanner->tokenStart), parseData.scanner->tokenStart);
				char msg[BUFSIZ];
				snprintf(msg, BUFSIZ, "after Prase[%d] %s (%s)", itoken, yyTokenName[tokenID],token_text);
				check_aggregate_initializers(msg, true);
			}
#endif
    }
    Parse( parser, 0, yylval, parseData );

    /* want 0 on success, 1 on invalid input, 2 on memory exhaustion */
    if( yyerrstatus != 0 ) {
        fprintf( stderr, ">> Bailing! (yyerrstatus = %d)\n", yyerrstatus );
        ERRORreport( ERROR_bail_out );
        /* free model and model->u.express */
        return 0;
    }
    EXPRESSpass = 1;

    perplexFree( scanner );
    ParseFree( parser, free );

    return yyexpresult;
}

static void RENAMEresolve( Rename * r, Schema s );

/**
 * find the final object to which a rename points
 * i.e., follow chain of USEs or REFs
 * sets DICT_type
 *
 * Sept 2013 - remove unused param enum rename_type type (TODO should this be used)?
 */
static Generic SCOPEfind_for_rename( Scope schema, char * name ) {
    Generic result;
    Rename * rename;

    /* object can only appear in top level symbol table */
    /* OR in another rename clause */

    result = DICTlookup( schema->symbol_table, name );
    if( result ) {
        return result;
    }

    /* Occurs in a fully USE'd schema? */
    LISTdo( schema->u.schema->use_schemas, use_schema, Schema ) {
        /* follow chain'd USEs */
        result = SCOPEfind_for_rename( use_schema, name );
        if( result ) {
            return( result );
        }
    } LISTod;

    /* Occurs in a partially USE'd schema? */
    rename = ( Rename * )DICTlookup( schema->u.schema->usedict, name );
    if( rename ) {
        RENAMEresolve( rename, schema );
        DICT_type = rename->type;
        return( rename->object );
    }

    LISTdo( schema->u.schema->uselist, r, Rename * )
    if( streq( ( r->nnew ? r->nnew : r->old )->name, name ) ) {
        RENAMEresolve( r, schema );
        DICT_type = r->type;
        return( r->object );
    }
    LISTod;

    /* we are searching for an object to be interfaced from this schema. */
    /* therefore, we *never* want to look at things which are REFERENCE'd */
    /* *into* the current schema.  -snc */

    return 0;
}

static void RENAMEresolve( Rename * r, Schema s ) {
    Generic remote;

    /*   if (is_resolved_rename_raw(r->old)) return;*/
    if( r->object ) {
        return;
    }

    if( is_resolve_failed_raw( r->old ) ) {
        return;
    }

    if( is_resolve_in_progress_raw( r->old ) ) {
        ERRORreport_with_symbol( ERROR_circular_reference,
                                 r->old, r->old->name );
        resolve_failed_raw( r->old );
        return;
    }
    resolve_in_progress_raw( r->old );

    remote = SCOPEfind_for_rename( r->schema, r->old->name );
    if( remote == 0 ) {
        ERRORreport_with_symbol( ERROR_ref_nonexistent, r->old,
                                 r->old->name, r->schema->symbol.name );
        resolve_failed_raw( r->old );
    } else {
        r->object = remote;
        r->type = DICT_type;
        switch( r->rename_type ) {
            case use:
                SCHEMAdefine_use( s, r );
                break;
            case ref:
                SCHEMAdefine_reference( s, r );
                break;
        }
        /*  resolve_rename_raw(r->old);*/
    }
    resolve_not_in_progress_raw( r->old );
}

#ifdef using_enum_items_is_a_pain
static void RENAMEresolve_enum( Type t, Schema s ) {
    DictionaryEntry de;
    Expression      x;

    DICTdo_type_init( t->symbol_table, &de, OBJ_EXPRESSION );
    while( 0 != ( x = ( Expression )DICTdo( &de ) ) ) {
        /*      SCHEMAadd_use(s, v*/
        /*          raw(x->symbol.name);*/
    }
}
#endif

Schema EXPRESSfind_schema( Dictionary modeldict, char * name ) {
    Schema s;
    FILE * fp;
    char * src, *dest;
    char lower[MAX_SCHEMA_FILENAME_SIZE];   /* avoid lowerizing original */

    if( print_objects_while_running & OBJ_SCHEMA_BITS ) {
        fprintf( stderr, "pass %d: %s (schema reference)\n",
                 EXPRESSpass, name );
    }

    s = ( Schema )DICTlookup( modeldict, name );
    if( s ) {
        return s;
    }

    dest = lower;
    for( src = name; *src; src++ ) {
        *dest++ = tolower( *src );
    }
    *dest = '\0';

    /* go down path looking for file */
    LISTdo( EXPRESS_path, dir, Dir * )
    sprintf( dir->leaf, "%s.exp", lower );
    if( print_objects_while_running & OBJ_SCHEMA_BITS ) {
        fprintf( stderr, "pass %d: %s (schema file?)\n",
                 EXPRESSpass, dir->full );
    }
    fp = fopen( dir->full, "r" );
    if( fp ) {
        Express express;

        if( print_objects_while_running & OBJ_SCHEMA_BITS ) {
            fprintf( stderr, "pass %d: %s (schema file found)\n",
                     EXPRESSpass, dir->full );
        }

        express = PARSERrun( SCANstrdup( dir->full ), fp );
        if( express ) {
            s = ( Schema )DICTlookup( modeldict, name );
        }
        if( s ) {
            return s;
        }
        ERRORreport( ERROR_schema_not_in_own_schema_file,
                     name, dir->full );
        return 0;
    } else {
        if( print_objects_while_running & OBJ_SCHEMA_BITS ) {
            fprintf( stderr, "pass %d: %s (schema file not found), errno = %d\n", EXPRESSpass, dir->full, errno );
        }
    }
    LISTod
    return 0;
}


/**
 * make the initial connections from one schema to another
 * dictated by USE/REF clauses that use dictionaries, i.e.,
 * because of partial schema references
 * \sa connect_schema_lists()
 */
static void connect_lists( Dictionary modeldict, Schema schema, Linked_List list ) {
    Rename * r;

    /* translate symbols to schemas */
    LISTdo_links( list, l )
    r = ( Rename * )l->data;
    r->schema = EXPRESSfind_schema( modeldict, r->schema_sym->name );
    if( !r->schema ) {
        ERRORreport_with_symbol( ERROR_undefined_schema,
                                 r->schema_sym,
                                 r->schema_sym->name );
        resolve_failed_raw( r->old );
        resolve_failed( schema );
    }
    LISTod
}

/**
 * same as `connect_lists` except for full schemas
 * \sa connect_lists()
 */
static void connect_schema_lists( Dictionary modeldict, Schema schema, Linked_List schema_list ) {
    Symbol * sym;
    Schema ref_schema;

    /* translate symbols to schemas */
    LISTdo_links( schema_list, list )
    sym = ( Symbol * )list->data;
    ref_schema = EXPRESSfind_schema( modeldict, sym->name );
    if( !ref_schema ) {
        ERRORreport_with_symbol( ERROR_undefined_schema, sym, sym->name );
        resolve_failed( schema );
        list->data = 0;
    } else {
        list->data = ( Generic )ref_schema;
    }
    LISTod
}

/**
** \param model   - Working Form model to resolve
** Perform symbol resolution on a loosely-coupled WF.
*/
void EXPRESSresolve( Express model ) {
    /* resolve multiple schemas.  Schemas will be resolved here or when */
    /* they are first encountered by a use/reference clause, whichever */
    /* comes first - DEL */

    Schema schema;
    DictionaryEntry de;

    jmp_buf env;
    if( setjmp( env ) ) {
        return;
    }
    ERRORsafe( env );

    EXPRESSpass++;
    if( print_objects_while_running & OBJ_PASS_BITS ) {
        fprintf( stderr, "pass %d: resolving schema references\n", EXPRESSpass );
    }

    /* connect the real schemas to all the rename clauses */

    /* we may be adding new schemas (to dictionary) as we traverse it, */
    /* so to avoid confusing dictionary, use a list as a fifo.  I.e., */
    /* add news schemas to end, drop old ones off the front as we */
    /* process them. */

    LISTdo( PARSEnew_schemas, print_schema, Schema ) {
        if( print_objects_while_running & OBJ_SCHEMA_BITS ) {
            fprintf( stderr, "pass %d: %s (schema)\n",
                    EXPRESSpass, print_schema->symbol.name );
        }

        if( print_schema->u.schema->uselist )
            connect_lists( model->symbol_table,
                        print_schema, print_schema->u.schema->uselist );
        if( print_schema->u.schema->reflist )
            connect_lists( model->symbol_table,
                        print_schema, print_schema->u.schema->reflist );

        connect_schema_lists( model->symbol_table,
                            print_schema, print_schema->u.schema->use_schemas );
        connect_schema_lists( model->symbol_table,
                            print_schema, print_schema->u.schema->ref_schemas );
    } LISTod;

    LISTfree( PARSEnew_schemas );
    PARSEnew_schemas = 0;   /* just in case */

    EXPRESSpass++;
    if( print_objects_while_running & OBJ_PASS_BITS ) {
        fprintf( stderr, "pass %d: resolving objects references to other schemas\n", EXPRESSpass );
    }

    /* connect the object in each rename clause to the real object */
    DICTdo_type_init( model->symbol_table, &de, OBJ_SCHEMA );
    while( 0 != ( schema = ( Schema )DICTdo( &de ) ) ) {
        if( is_not_resolvable( schema ) ) {
            continue;
        }

        if( print_objects_while_running & OBJ_SCHEMA_BITS ) {
            fprintf( stderr, "pass %d: %s (schema)\n",
                     EXPRESSpass, schema->symbol.name );
        }

        /* do USE's first because they take precedence */
        if( schema->u.schema->uselist ) {
            LISTdo( schema->u.schema->uselist, r, Rename * )
            RENAMEresolve( r, schema );
            LISTod;
            LISTfree( schema->u.schema->uselist );
            schema->u.schema->uselist = 0;
        }
        if( schema->u.schema->reflist ) {
            LISTdo( schema->u.schema->reflist, r, Rename * )
            RENAMEresolve( r, schema );
            LISTod;
            LISTfree( schema->u.schema->reflist );
            schema->u.schema->reflist = 0;
        }
    }

    /* resolve sub- and supertype references.  also resolve all */
    /* defined types */
    EXPRESSpass++;
    if( print_objects_while_running & OBJ_PASS_BITS ) {
        fprintf( stderr, "pass %d: resolving sub and supertypes\n", EXPRESSpass );
    }

    DICTdo_type_init( model->symbol_table, &de, OBJ_SCHEMA );
    while( 0 != ( schema = ( Schema )DICTdo( &de ) ) ) {
        if( is_not_resolvable( schema ) ) {
            continue;
        }
        SCOPEresolve_subsupers( schema );
    }
    if( ERRORoccurred ) {
        ERRORunsafe();
    }

    /* resolve types */
    EXPRESSpass++;
    if( print_objects_while_running & OBJ_PASS_BITS ) {
        fprintf( stderr, "pass %d: resolving types\n", EXPRESSpass );
    }

    SCOPEresolve_types( model );
    if( ERRORoccurred ) {
        ERRORunsafe();
    }

#ifdef using_enum_items_is_a_pain
    /* resolve USE'd enumerations */
    /* doesn't really deserve its own pass, but needs to come after */
    /* type resolution */
    EXPRESSpass++;
    if( print_objects_while_running & OBJ_PASS_BITS ) {
        fprintf( stderr, "pass %d: resolving implied USE's\n", EXPRESSpass );
    }

    DICTdo_type_init( model->symbol_table, &de, OBJ_SCHEMA );
    while( 0 != ( schema = ( Schema )DICTdo( &de ) ) ) {
        if( is_not_resolvable( schema ) ) {
            continue;
        }

        if( print_objects_while_running & OBJ_SCHEMA_BITS ) {
            fprintf( stderr, "pass %d: %s (schema)\n",
                     EXPRESSpass, schema->symbol.name );
        }

        if( schema->u.schema->usedict ) {
            DICTdo_init( schema->u.schema->usedict, &fg )
            while( 0 != ( r = ( Rename )DICTdo( &fg ) ) ) {
                if( ( r->type = OBJ_TYPE ) && ( ( Type )r->object )->body &&
                        TYPEis_enumeration( ( Type )r->object ) ) {
                    RENAMEresolve_enum( ( Type )r->object, schema );
                }
            }
        }
    }
    if( ERRORoccurred ) {
        ERRORunsafe();
    }
#endif

    EXPRESSpass++;
    if( print_objects_while_running & OBJ_PASS_BITS ) {
        fprintf( stderr, "pass %d: resolving expressions and statements\n", EXPRESSpass );
    }

    SCOPEresolve_expressions_statements( model );
    if( ERRORoccurred ) {
        ERRORunsafe();
    }

    /* mark everything resolved if possible */
    DICTdo_init( model->symbol_table, &de );
    while( 0 != ( schema = ( Schema )DICTdo( &de ) ) ) {
        if( is_resolvable( schema ) ) {
            resolved_all( schema );
        }
    }
}
