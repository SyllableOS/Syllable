/****************************************************************************
** $Id$
**
** Parser and code generator for meta object compiler
**
** Created : 930417
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
** --------------------------------------------------------------------------
**
** This compiler reads a C++ header file with class definitions and ouputs
** C++ code to build a meta class. The meta data includes public methods
** (not constructors, destructors or operator functions), signals and slot
** definitions. The output file should be compiled and linked into the
** target application.
**
** C++ header files are assumed to have correct syntax, and we are therefore
** doing less strict checking than C++ compilers.
**
** The C++ grammar has been adopted from the "The Annotated C++ Reference
** Manual" (ARM), by Ellis and Stroustrup (Addison Wesley, 1992).
**
** Notice that this code is not possible to compile with GNU bison, instead
** use standard AT&T yacc or Berkeley yacc.
*****************************************************************************/

%{
void yyerror( const char *msg );

#include "qlist.h"
#include "qasciidict.h"
#include "qdict.h"
#include "qstrlist.h"
#include "qdatetime.h"
#include "qfile.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>



static QCString rmWS( const char * );

enum Access { Private, Protected, Public };


struct Argument					// single arg meta data
{
    Argument( char *left, char *right )
	{ leftType  = rmWS( left );
	  rightType = rmWS( right );
	  if ( leftType == "void" && rightType.isEmpty() )
	      leftType = "";
	}
    QCString leftType;
    QCString rightType;
};

class ArgList : public QList<Argument> {	// member function arg list
public:
    ArgList() { setAutoDelete( TRUE ); }
   ~ArgList() { clear(); }
};


struct Function					// member function meta data
{						//   used for signals and slots
    Access access;
    QCString    qualifier;			// const or volatile
    QCString    name;
    QCString    type;
    int	       lineNo;
    ArgList   *args;
    Function() { args=0; }
   ~Function() { delete args; }
    const char* accessAsString() {
	switch ( access ) {
	case Private: return "Private";
	case Protected: return "Protected";
	default: return "Public";
	}
    }
};

class FuncList : public QList<Function> {	// list of member functions
public:
    FuncList( bool autoDelete = FALSE ) { setAutoDelete( autoDelete ); }

    FuncList find( const char* name )
    {
	FuncList result;
	for ( QListIterator<Function> it( *this); it.current(); ++it ) {
	    if ( it.current()->name == name )
		result.append( it.current() );
	}
	return result;
    }
};

class Enum : public QStrList
{
public:
    QCString name;
    bool set;
};

class EnumList : public QList<Enum> {		// list of property enums
public:
    EnumList() { setAutoDelete(TRUE); }
};


struct Property
{
    Property( int l, const char* t, const char* n, const char* s, const char* g, const char* r,
	      const QCString& st, int d, bool ov )
	: lineNo(l), type(t), name(n), set(s), get(g), reset(r), setfunc(0), getfunc(0), resetfunc(0),
	  sspec(Unspecified), gspec(Unspecified), stored( st ),
	  designable( d ), override( ov ), oredEnum( -1 )
    {}

    int lineNo;
    QCString type;
    QCString name;
    QCString set;
    QCString get;
    QCString reset;
    QCString stored;
    int designable; // Allowed values are 1 (True), 0 (False), and -1 (Unset)
    bool override;
    int oredEnum; // If the enums item may be ored. That means the data type is int.
		  // Allowed values are 1 (True), 0 (False), and -1 (Unset)

    Function* setfunc;
    Function* getfunc;
    Function* resetfunc;

    enum Specification  { Unspecified, Class, Reference, Pointer, ConstCharStar };
    Specification sspec;
    Specification gspec;

    bool stdSet () {
	QCString s = "set";
	s += toupper( name[0] );
	s +=name.mid(1);
	return s ==set;
    }

    static const char* specToString( Specification s )
    {
	switch ( s ) {
	case Class:
	    return "Class";
	case Reference:
	    return "Reference";
	case Pointer:
	    return "Pointer";
	case ConstCharStar:
	    return "ConstCharStar";
	default:
	    return "Unspecified";
	}
    }
};

class PropList : public QList<Property> {	// list of properties
public:
    PropList() { setAutoDelete( TRUE ); }
};


struct ClassInfo
{
    ClassInfo( const char* n, const char* v )
	: name(n), value(v)
    {}
    QCString name;
    QCString value;
};

class ClassInfoList : public QList<ClassInfo> {	// list of class infos
public:
    ClassInfoList() { setAutoDelete( TRUE ); }
};


/*
  Attention!
  This table is copied from qvariant.cpp. If you change
  one, change both.
*/
static const int ntypes = 26;
static const char* const type_map[ntypes] =
{
    0,
    "QMap<QString,QVariant>",
    "QValueList<QVariant>",
    "QString",
    "QStringList",
    "QFont",
    "QPixmap",
    "QBrush",
    "QRect",
    "QSize",
    "QColor",
    "QPalette",
    "QColorGroup",
    "QIconSet",
    "QPoint",
    "QImage",
    "int",
    "uint",
    "bool",
    "double",
    "QCString",
    "QPointArray",
    "QRegion",
    "QBitmap",
    "QCursor",
    "QSizePolicy"
};

int qvariant_nameToType( const char* name )
{
    for ( int i = 0; i < ntypes; i++ ) {
	if ( !qstrcmp( type_map[i], name ) )
	    return i;
    }
    return 0;
}


ArgList *addArg( Argument * );			// add arg to tmpArgList

enum Member { SignalMember,
	      SlotMember,
	      PropertyCandidateMember
	    };

void	 addMember( Member );			// add tmpFunc to current class
void     addEnum();				// add tmpEnum to current class

char	*strnew( const char * );		// returns a new string (copy)
char	*stradd( const char *, const char * );	// add two strings
char	*stradd( const char *, const char *,	// add three strings
			       const char * );
char	*straddSpc( const char *, const char * );
char	*straddSpc( const char *, const char *,
			       const char * );
char	*straddSpc( const char *, const char *,
		    const char *, const char * );

extern int yydebug;
bool	   lexDebug	   = FALSE;
int	   lineNo;				// current line number
bool	   errorControl	   = FALSE;		// controlled errors
bool	   displayWarnings = TRUE;
bool	   skipClass;				// don't generate for class
bool	   skipFunc;				// don't generate for func
bool	   templateClass;			// class is a template
bool	   templateClassOld;			// previous class is a template

ArgList	  *tmpArgList;				// current argument list
Function  *tmpFunc;				// current member function
Enum      *tmpEnum;				// current enum
Access tmpAccess;			// current access permission
Access subClassPerm;			// current access permission

bool	   Q_OBJECTdetected;			// TRUE if current class
						// contains the Q_OBJECT macro
bool	   Q_PROPERTYdetected;			// TRUE if current class
						// contains at least one Q_PROPERTY,
						// Q_OVERRIDE, Q_SETS or Q_ENUMS macro
bool	   tmpPropOverride;			// current property override setting

// some temporary values
QCString   tmpExpression;			// Used to store the characters the lexer
						// is currently skipping (see addExpressionChar and friends)
int	   tmpYYStart;				// Used to store the lexers current mode
int	   tmpYYStart2;				// Used to store the lexers current mode
						// (if tmpYYStart is already used)

const int  formatRevision = 9;			// moc output format revision

%}


%union {
    char	char_val;
    int		int_val;
    double	double_val;
    char       *string;
    Access	access;
    Function   *function;
    ArgList    *arg_list;
    Argument   *arg;
}

%start declaration_seq

%token <char_val>	CHAR_VAL		/* value types */
%token <int_val>	INT_VAL
%token <double_val>	DOUBLE_VAL
%token <string>		STRING
%token <string>		IDENTIFIER		/* identifier string */

%token			FRIEND			/* declaration keywords */
%token			TYPEDEF
%token			AUTO
%token			REGISTER
%token			STATIC
%token			EXTERN
%token			INLINE
%token			VIRTUAL
%token			CONST
%token			VOLATILE
%token			CHAR
%token			SHORT
%token			INT
%token			LONG
%token			SIGNED
%token			UNSIGNED
%token			FLOAT
%token			DOUBLE
%token			VOID
%token			ENUM
%token			CLASS
%token			STRUCT
%token			UNION
%token			ASM
%token			PRIVATE
%token			PROTECTED
%token			PUBLIC
%token			OPERATOR
%token			DBL_COLON
%token			TRIPLE_DOT
%token			TEMPLATE
%token			NAMESPACE
%token			USING
%token			MUTABLE

%token			SIGNALS
%token			SLOTS
%token			Q_OBJECT
%token			Q_PROPERTY
%token			Q_OVERRIDE
%token			Q_CLASSINFO
%token			Q_ENUMS
%token			Q_SETS

%token			READ
%token			WRITE
%token			STORED
%token			DESIGNABLE
%token			RESET

%type  <string>		class_name
%type  <string>		template_class_name
%type  <string>		template_spec
%type  <string>		opt_base_spec

%type  <string>		base_spec
%type  <string>		base_list
%type  <string>		qt_macro_name
%type  <string>		base_specifier
%type  <access>		access_specifier
%type  <string>		fct_name
%type  <string>		type_name
%type  <string>		simple_type_names
%type  <string>		simple_type_name
%type  <string>		class_key
%type  <string>		complete_class_name
%type  <string>		qualified_class_name
%type  <string>		elaborated_type_specifier

%type  <arg_list>	argument_declaration_list
%type  <arg_list>	arg_declaration_list
%type  <arg_list>	arg_declaration_list_opt
%type  <string>		abstract_decl_opt
%type  <string>		abstract_decl
%type  <arg>		argument_declaration
%type  <string>		cv_qualifier_list_opt
%type  <string>		cv_qualifier_list
%type  <string>		cv_qualifier
%type  <string>		decl_specifiers
%type  <string>		decl_specifier
%type  <string>		decl_specs_opt
%type  <string>		decl_specs
%type  <string>		type_specifier
%type  <string>		declarator
%type  <string>		ptr_operator
%type  <string>		ptr_operators
%type  <string>		ptr_operators_opt

%%
declaration_seq:	  /* empty */
                        | declaration_seq declaration
			;

declaration:		  class_def
/* | template_declaration */
                        | namespace_def
                        | namespace_alias_def
                        | using_declaration
                        | using_directive
                        ;

namespace_def:            named_namespace_def
                        | unnamed_namespace_def
                        ;

named_namespace_def:      NAMESPACE
                          IDENTIFIER         { enterNameSpace($2); }
                          '{'                { BEGIN IN_NAMESPACE; }
                          namespace_body
                          '}'                { leaveNameSpace();
			                       selectOutsideClassState();
                                             }
                        ;

unnamed_namespace_def:    NAMESPACE          { enterNameSpace(); }
                          '{'                { BEGIN IN_NAMESPACE; }
                          namespace_body
                          '}'                { leaveNameSpace();
  			                       selectOutsideClassState();
			                     }
                        ;

namespace_body:           declaration_seq
                        ;

namespace_alias_def:      NAMESPACE IDENTIFIER '=' complete_class_name ';'
                                            { selectOutsideClassState(); }
                        ;


using_directive:          USING NAMESPACE   { selectOutsideClassState(); } /* Skip namespace */
                        ;

using_declaration:        USING IDENTIFIER  { selectOutsideClassState(); }
                        | USING DBL_COLON   { selectOutsideClassState(); }
                        ;

class_def:				      { initClass(); }
			  class_specifier ';' { generateClass();
			                        registerClassInNamespace();
						selectOutsideClassState(); }
			;


/***** r.17.1 (ARM p.387 ): Keywords	*****/

class_name:		  IDENTIFIER	      { $$ = $1; }
			| template_class_name { $$ = $1; }
			;

template_class_name:	  IDENTIFIER '<' template_args '>'
				   { $$ = stradd( $1, "<",
				     tmpExpression =
				     tmpExpression.stripWhiteSpace(), ">" ); }
			;

/*
   template_args skips all characters until it encounters a ">" (it
   handles and discards sublevels of parentheses).  Since the rule is
   empty it must be used with care!
*/

template_args:		  /* empty */		  { initExpression();
						    templLevel = 1;
						    BEGIN IN_TEMPL_ARGS; }
			;

/***** r.17.2 (ARM p.388): Expressions	*****/


/* const_expression skips all characters until it encounters either one
   of "]", ")" or "," (it handles and discards sublevels of parentheses).
   Since the rule is empty it must be used with care!
*/

const_expression:	   /* empty */		  { initExpression();
						    BEGIN IN_EXPR; }
			;

enumerator_expression:	   /* empty */		  { initExpression();
						    BEGIN IN_ENUM; }
			;

/***** r.17.3 (ARM p.391): Declarations *****/

decl_specifier:		  storage_class_specifier { $$ = ""; }
			| type_specifier	  { $$ = $1; }
			| fct_specifier		  { $$ = ""; }
			| FRIEND		  { skipFunc = TRUE; $$ = ""; }
			| TYPEDEF		  { skipFunc = TRUE; $$ = ""; }
			;

decl_specifiers:	  decl_specs_opt type_name decl_specs_opt
						  { $$ = straddSpc($1,$2,$3); }
			;

decl_specs_opt:			/* empty */	  { $$ = ""; }
			| decl_specs		  { $$ = $1; }
			;

decl_specs:		  decl_specifier	    { $$ = $1; }
			| decl_specs decl_specifier { $$ = straddSpc($1,$2); }
			;

storage_class_specifier:  AUTO
			| REGISTER
			| STATIC		{ skipFunc = TRUE; }
			| EXTERN
			;

fct_specifier:		  INLINE
			| VIRTUAL
			;

type_specifier:		  CONST			{ $$ = "const"; }
			| VOLATILE		{ $$ = "volatile"; }
			;

type_name:		  elaborated_type_specifier { $$ = $1; }
			| complete_class_name	    { $$ = $1; }
			| simple_type_names	    { $$ = $1; }
			;

simple_type_names:	  simple_type_names simple_type_name
						    { $$ = straddSpc($1,$2); }
			| simple_type_name	    { $$ = $1; }

simple_type_name:	  CHAR			    { $$ = "char"; }
			| SHORT			    { $$ = "short"; }
			| INT			    { $$ = "int"; }
			| LONG			    { $$ = "long"; }
			| SIGNED		    { $$ = "signed"; }
			| UNSIGNED		    { $$ = "unsigned"; }
			| FLOAT			    { $$ = "float"; }
			| DOUBLE		    { $$ = "double"; }
			| VOID			    { $$ = "void"; }
			;

template_spec:		  TEMPLATE '<' template_args '>'
				   { $$ = stradd( "template<",
				     tmpExpression =
				     tmpExpression.stripWhiteSpace(), ">" ); }
			;

opt_template_spec:	  /* empty */
			| template_spec		{ templateClassOld = templateClass;
						  templateClass = TRUE;
						}
			;


class_key:		  opt_template_spec CLASS	{ $$ = "class"; }
			| opt_template_spec STRUCT	{ $$ = "struct"; }
			;

complete_class_name:	  qualified_class_name	{ $$ = $1; }
			| DBL_COLON  qualified_class_name
						{ $$ = stradd( "::", $2 ); }
			;

qualified_class_name:	  qualified_class_name DBL_COLON class_name
						{ $$ = stradd( $1, "::", $3 );}
			| class_name		{ $$ = $1; }
			;

elaborated_type_specifier:
			  class_key IDENTIFIER	{ $$ = straddSpc($1,$2); }
			| ENUM IDENTIFIER	{ $$ = stradd("enum ",$2); }
			| UNION IDENTIFIER	{ $$ = stradd("union ",$2); }
			;

/***** r.17.4 (ARM p.393): Declarators	*****/

argument_declaration_list:  arg_declaration_list_opt triple_dot_opt { $$ = $1;}
			|   arg_declaration_list ',' TRIPLE_DOT	    { $$ = $1;
				       func_warn("Ellipsis not supported"
						 " in signals and slots.\n"
						 "Ellipsis argument ignored."); }
			;

arg_declaration_list_opt:	/* empty */	{ $$ = tmpArgList; }
			| arg_declaration_list	{ $$ = $1; }
			;

triple_dot_opt:			/* empty */
			| TRIPLE_DOT { func_warn("Ellipsis not supported"
						 " in signals and slots.\n"
						 "Ellipsis argument ignored."); }

			;

arg_declaration_list:	  arg_declaration_list
			  ','
			  argument_declaration	{ $$ = addArg($3); }
			| argument_declaration	{ $$ = addArg($1); }

argument_declaration:	  decl_specifiers abstract_decl_opt
				{ $$ = new Argument(straddSpc($1,$2),""); }
			| decl_specifiers abstract_decl_opt
			  '=' { expLevel = 1; }
			  const_expression
				{ $$ = new Argument(straddSpc($1,$2),""); }
			| decl_specifiers abstract_decl_opt dname
				abstract_decl_opt
				{ $$ = new Argument(straddSpc($1,$2),$4); }
			| decl_specifiers abstract_decl_opt dname
				abstract_decl_opt
			  '='	{ expLevel = 1; }
			  const_expression
				{ $$ = new Argument(straddSpc($1,$2),$4); }
			;


abstract_decl_opt:	  /* empty */		{ $$ = ""; }
			| abstract_decl		{ $$ = $1; }
			;

abstract_decl:		  abstract_decl ptr_operator
						{ $$ = straddSpc($1,$2); }
			| '['			{ expLevel = 1; }
			  const_expression ']'
				   { $$ = stradd( "[",
				     tmpExpression =
				     tmpExpression.stripWhiteSpace(), "]" ); }
			| abstract_decl '['	{ expLevel = 1; }
			  const_expression ']'
				   { $$ = stradd( $1,"[",
				     tmpExpression =
				     tmpExpression.stripWhiteSpace(),"]" ); }
			| ptr_operator		{ $$ = $1; }
			| '(' abstract_decl ')' { $$ = $2; }
			;

declarator:		  dname			{ $$ = ""; }
			| declarator ptr_operator
						{ $$ = straddSpc($1,$2);}
			| declarator '['	{ expLevel = 1; }
			  const_expression ']'
				   { $$ = stradd( $1,"[",
				     tmpExpression =
				     tmpExpression.stripWhiteSpace(),"]" ); }
			| '(' declarator ')'	{ $$ = $2; }
			;

dname:			  IDENTIFIER
			;

fct_decl:		  '('
			  argument_declaration_list
			  ')'
			  cv_qualifier_list_opt
			  ctor_initializer_opt
			  fct_body_or_semicolon
						{ tmpFunc->args	     = $2;
						  tmpFunc->qualifier = $4; }
			;

fct_name:		  IDENTIFIER		/* NOTE: simplified! */
			| IDENTIFIER array_decls
				{ func_warn("Variable as signal or slot."); }
			| IDENTIFIER '=' { expLevel=0; }
			  const_expression     /* probably const member */
						{ skipFunc = TRUE; }
			| IDENTIFIER array_decls '=' { expLevel=0; }
			  const_expression     /* probably const member */
						{ skipFunc = TRUE; }
			;


array_decls:		 '['			{ expLevel = 1; }
			  const_expression ']'
			| array_decls '['	 { expLevel = 1; }
			  const_expression ']'

			;

ptr_operators_opt:	   /* empty */		{ $$ = ""; }
			| ptr_operators		 { $$ = $1; }
			;

ptr_operators:		  ptr_operator		{ $$ = $1; }
			| ptr_operators ptr_operator { $$ = straddSpc($1,$2);}
			;

ptr_operator:		  '*' cv_qualifier_list_opt { $$ = straddSpc("*",$2);}
			| '&' cv_qualifier_list_opt { $$ = stradd("&",$2);}
/*!			| complete_class_name
			  DBL_COLON
			  '*'
			  cv_qualifier_list_opt { $$ = stradd($1,"::*",$4); }*/
			;

cv_qualifier_list_opt:		/* empty */	{ $$ = ""; }
			| cv_qualifier_list	{ $$ = $1; }
			;

cv_qualifier_list:	  cv_qualifier		{ $$ = $1; }
			| cv_qualifier_list cv_qualifier
						{ $$ = straddSpc($1,$2); }
			;

cv_qualifier:		  CONST			{ $$ = "const"; }
			| VOLATILE		{ $$ = "volatile"; }
			;

fct_body_or_semicolon:	  ';'
			| fct_body
			| '=' INT_VAL ';'   /* abstract func, INT_VAL = 0 */
			;

fct_body:		  '{' { BEGIN IN_FCT; fctLevel = 1;}
			  '}' { BEGIN QT_DEF; }
			;


/***** r.17.5 (ARM p.395): Class Declarations *****/

class_specifier:	  full_class_head
			  '{'			{ BEGIN IN_CLASS;
                                                  classPLevel = 1;
						}
			  opt_obj_member_list
			  '}'			{ BEGIN QT_DEF; } /*catch ';'*/
			| class_head		{ BEGIN QT_DEF;	  /* -- " -- */
						  skipClass = TRUE; }
			| class_head
			  '(' IDENTIFIER ')' /* Qt macro name */
						{ BEGIN QT_DEF; /* catch ';' */
						  skipClass = TRUE; }
			| template_spec whatever { skipClass = TRUE;
						  BEGIN GIMME_SEMICOLON; }
			;

whatever:		  IDENTIFIER
			| simple_type_name
			| type_specifier
			| storage_class_specifier
			| fct_specifier
			;


class_head:		  class_key
			  qualified_class_name	{ className = $2; }
			| class_key
			  IDENTIFIER		/* possible DLL EXPORT macro */
			  class_name		{ className = $3; }
			;

full_class_head:	  class_head
			  opt_base_spec		{ superclassName = $2; }
			;

nested_class_head:	  class_key
			  qualified_class_name
			  opt_base_spec		{ templateClass = templateClassOld; }

ctor_initializer_opt:		/* empty */
			| ctor_initializer
			;

ctor_initializer:	':' mem_initializer_list
			;

mem_initializer_list:	mem_initializer
			| mem_initializer ',' mem_initializer_list
			;

/* complete_class_name also represents IDENTIFIER */
mem_initializer:	complete_class_name '('	{ expLevel = 1; }
			const_expression ')'
			;


opt_base_spec:			/* empty */	{ $$ = 0; }
			| base_spec		{ $$ = $1; }
			;

opt_obj_member_list:		/* empty */
			| obj_member_list
			;

obj_member_list:	  obj_member_list obj_member_area
			| obj_member_area
			;


qt_access_specifier:	  access_specifier	{ tmpAccess = $1; }
			| SLOTS	      { moc_err( "Missing access specifier"
						   " before \"slots:\"." ); }
			;

obj_member_area:	  qt_access_specifier	{ BEGIN QT_DEF; }
			  slot_area
			| SIGNALS		{ BEGIN QT_DEF; }
			  ':'  opt_signal_declarations
			| Q_OBJECT		{
			      if ( tmpAccess )
				  moc_warn("Q_OBJECT is not in the private"
					   " section of the class.\n"
					   "Q_OBJECT is a macro that resets"
					   " access permission to \"private\".");
			      Q_OBJECTdetected = TRUE;
			  }
			| Q_PROPERTY { tmpYYStart = YY_START;
				       tmpPropOverride = FALSE;
				       BEGIN IN_PROPERTY; }
			  '(' property ')' {
						BEGIN tmpYYStart;
				   	   }
			  opt_property_candidates
			| Q_OVERRIDE { tmpYYStart = YY_START;
				       tmpPropOverride = TRUE;
				       BEGIN IN_PROPERTY; }
			  '(' property ')' {
						BEGIN tmpYYStart;
				   	   }
			  opt_property_candidates
			| Q_CLASSINFO { tmpYYStart = YY_START; BEGIN IN_CLASSINFO; }
			  '(' STRING ',' STRING ')'
				  {
				      infos.append( new ClassInfo( $4, $6 ) );
				      BEGIN tmpYYStart;
				  }
			  opt_property_candidates
			| Q_ENUMS { tmpYYStart = YY_START; BEGIN IN_PROPERTY; }
			  '(' qt_enums ')' {
			  			Q_PROPERTYdetected = TRUE;
						BEGIN tmpYYStart;
				   	   }
			  opt_property_candidates
			| Q_SETS { tmpYYStart = YY_START; BEGIN IN_PROPERTY; }
			  '(' qt_sets ')' {
			  			Q_PROPERTYdetected = TRUE;
						BEGIN tmpYYStart;
				   	   }
			  opt_property_candidates
			;

slot_area:		  SIGNALS ':'	{ moc_err( "Signals cannot "
						 "have access specifiers" ); }
			| SLOTS	  ':' opt_slot_declarations
			| ':'		{ if ( tmpAccess == Public && Q_PROPERTYdetected )
                                                  BEGIN QT_DEF;
                                              else
                                                  BEGIN IN_CLASS;
					  suppress_func_warn = TRUE;
                                        }
			  opt_property_candidates
					{
					  suppress_func_warn = FALSE;
					}
			| IDENTIFIER	{ BEGIN IN_CLASS;
					   if ( classPLevel != 1 )
					       moc_warn( "unexpected access"
							 "specifier" );
					}
			;

opt_property_candidates:	  /*empty*/
				| property_candidate_declarations
			;

property_candidate_declarations:	  property_candidate_declarations property_candidate_declaration
					| property_candidate_declaration
				;

property_candidate_declaration:	signal_or_slot { addMember( PropertyCandidateMember ); }
				;

opt_signal_declarations:	/* empty */
			| signal_declarations
			;

signal_declarations:	  signal_declarations signal_declaration
			| signal_declaration
			;


signal_declaration:	  signal_or_slot	{ addMember( SignalMember ); }
			;

opt_slot_declarations:		/* empty */
			| slot_declarations
			;

slot_declarations:	  slot_declarations slot_declaration
			| slot_declaration
			;

slot_declaration:	  signal_or_slot		{ addMember( SlotMember ); }
			;

opt_semicolons:			/* empty */
			| opt_semicolons ';'
			;

base_spec:		  ':' base_list		{ $$=$2; }
			;

base_list		: base_list ',' base_specifier
			| base_specifier
			;

qt_macro_name:		  IDENTIFIER '(' IDENTIFIER ')'
					   { $$ = stradd( $1, "(", $3, ")" ); }
			| IDENTIFIER '(' simple_type_name ')'
					   { $$ = stradd( $1, "(", $3, ")" ); }
			;

base_specifier:		  complete_class_name			       {$$=$1;}
			| VIRTUAL access_specifier complete_class_name {$$=$3;}
			| VIRTUAL complete_class_name		       {$$=$2;}
			| access_specifier VIRTUAL complete_class_name {$$=$3;}
			| access_specifier complete_class_name	       {$$=$2;}
			| qt_macro_name				       {$$=$1;}
			| VIRTUAL access_specifier qt_macro_name       {$$=$3;}
			| VIRTUAL qt_macro_name			       {$$=$2;}
			| access_specifier VIRTUAL qt_macro_name       {$$=$3;}
			| access_specifier qt_macro_name	       {$$=$2;}
			;

access_specifier:	  PRIVATE		{ $$=Private; }
			| PROTECTED		{ $$=Protected; }
			| PUBLIC		{ $$=Public; }
			;

operator_name:		  decl_specs_opt IDENTIFIER ptr_operators_opt
			| decl_specs_opt simple_type_name ptr_operators_opt
			| '+'
			| '-'
			| '*'
			| '/'
			| '%'
			| '^'
			| '&'
			| '|'
			| '~'
			| '!'
			| '='
			| '<'
			| '>'
			| '+' '='
			| '-' '='
			| '*' '='
			| '/' '='
			| '%' '='
			| '^' '='
			| '&' '='
			| '|' '='
			| '~' '='
			| '!' '='
			| '=' '='
			| '<' '='
			| '>' '='
			| '<' '<'
			| '>' '>'
			| '<' '<' '='
			| '>' '>' '='
			| '&' '&'
			| '|' '|'
			| '+' '+'
			| '-' '-'
			| ','
			| '-' '>' '*'
			| '-' '>'
			| '(' ')'
			| '[' ']'
			;


opt_virtual:		  /* empty */
			| VIRTUAL
			;

type_and_name:		  type_name fct_name
						{ tmpFunc->type = $1;
						  tmpFunc->name = $2; }
			| fct_name
						{ tmpFunc->type = "int";
						  tmpFunc->name = $1;
				  if ( tmpFunc->name == className )
				      func_warn( "Constructors cannot be"
						 " signals or slots.");
						}
			| opt_virtual '~' fct_name
						{ tmpFunc->type = "void";
						  tmpFunc->name = "~";
						  tmpFunc->name += $3;
				       func_warn( "Destructors cannot be"
						  " signals or slots.");
						}
			| decl_specs type_name decl_specs_opt
			  ptr_operators_opt fct_name
						{
						    char *tmp =
							straddSpc($1,$2,$3,$4);
						    tmpFunc->type = rmWS(tmp);
						    delete tmp;
						    tmpFunc->name = $5; }
			| decl_specs type_name /* probably friend decl */
						{ skipFunc = TRUE; }
			| type_name ptr_operators fct_name
						{ tmpFunc->type =
						      straddSpc($1,$2);
						  tmpFunc->name = $3; }
			| type_name decl_specs ptr_operators_opt
			  fct_name
						{ tmpFunc->type =
						      straddSpc($1,$2,$3);
						  tmpFunc->name = $4; }
			| type_name OPERATOR operator_name
						{ operatorError();    }
			| OPERATOR operator_name
						{ operatorError();    }
			| decl_specs type_name decl_specs_opt
			  ptr_operators_opt OPERATOR operator_name
						{ operatorError();    }
			| type_name ptr_operators OPERATOR  operator_name
						{ operatorError();    }
			| type_name decl_specs ptr_operators_opt
			  OPERATOR  operator_name
						{ operatorError();    }
			;


signal_or_slot:		type_and_name fct_decl opt_semicolons
			| type_and_name opt_bitfield ';' opt_semicolons
				{ func_warn("Variable as signal or slot."); }
			| type_and_name opt_bitfield ','member_declarator_list
			  ';' opt_semicolons
				{ func_warn("Variable as signal or slot."); }
			| enum_specifier opt_identifier ';' opt_semicolons
				{ func_warn("Enum declaration as signal or"
					    " slot."); }
                        | USING complete_class_name ';' opt_semicolons
                                { func_warn("Using declaration as signal or"
					    " slot."); }
			| USING NAMESPACE complete_class_name ';' opt_semicolons
                                { func_warn("Using declaration as signal or"
					    " slot."); }
			| NAMESPACE IDENTIFIER '{'
                                { classPLevel++;
				  moc_err("Namespace declaration as signal or"
					  " slot."); }
 			| nested_class_head ';' opt_semicolons
 				{ func_warn("Class declaration as signal or slot.");}
 			| nested_class_head
 			  '{'   { func_warn("Class declaration as signal or slot.");
				  BEGIN IN_FCT; fctLevel=1;
				}
                          '}'  { BEGIN QT_DEF; }
			  ';' opt_semicolons
			;


member_declarator_list:	  member_declarator
			| member_declarator_list ',' member_declarator
			;

member_declarator:	  declarator
			| IDENTIFIER ':'	 { expLevel = 0; }
			  const_expression
			| ':'			 { expLevel = 0; }
			  const_expression
			;

opt_bitfield:		  /* empty */
			| ':'			 { expLevel = 0; }
			  const_expression
			;


enum_specifier:		  ENUM enum_tail
			;

/* optional final comma at the end of an enum list. Not really C++ but
some people still use it */
opt_komma:		  /*empty*/
			| ','
			;

enum_tail:		  IDENTIFIER '{'   enum_list opt_komma
			  '}'   { BEGIN QT_DEF;
				  if ( tmpAccess == Public) {
				      tmpEnum->name = $1;
				      addEnum();
				  }
				}
			| '{'   enum_list opt_komma
			  '}'   { tmpEnum->clear();}
			;

opt_identifier:		  /* empty */
			| IDENTIFIER
			;

enum_list:		  /* empty */
			| enumerator
			| enum_list ',' enumerator
			;

enumerator:		  IDENTIFIER { if ( tmpAccess == Public) tmpEnum->append( $1 ); }
			| IDENTIFIER '=' { expLevel=0; }
			  enumerator_expression {  if ( tmpAccess == Public) tmpEnum->append( $1 );  }
			;

property:		IDENTIFIER IDENTIFIER
				{
				     propWrite = "";
				     propRead = "";
				     if ( tmpPropOverride )
				         propStored = "";
				     else
					 propStored = "true";
				     propReset = "";
				     propOverride = tmpPropOverride;
				     if ( tmpPropOverride )
				         propDesignable = -1;
				     else
				         propDesignable = 1;
				}
			prop_statements
				{
				    if ( propRead.isEmpty() && !propOverride)
					moc_err( "A property must at least feature a read method." );
				    checkIdentifier( $2 );
				    Q_PROPERTYdetected = TRUE;
				    // Avoid duplicates
				    for( QListIterator<Property> lit( props ); lit.current(); ++lit ) {
					if ( lit.current()->name == $2 ) {
					    if ( displayWarnings )
						moc_err( "Property '%s' defined twice.",
							 (const char*)lit.current()->name );
					}
				    }
				    props.append( new Property( lineNo, $1, $2,
								propWrite, propRead, propReset, propStored,
								propDesignable, propOverride ) );
				}
			;

prop_statements:	  /* empty */
			| READ IDENTIFIER prop_statements { propRead = $2; }
			| WRITE IDENTIFIER prop_statements { propWrite = $2; }
			| RESET IDENTIFIER prop_statements { propReset = $2; }
			| STORED IDENTIFIER prop_statements { propStored = $2; }
			| DESIGNABLE IDENTIFIER
				{
					if ( qstrcmp( $2, "true" ) == 0 )
						propDesignable = 1;
					else if ( qstrcmp( $2, "false" ) == 0 )
						propDesignable = 0;
					else
						moc_err( "DESIGNABLE may only be followed by 'true' or 'false'" );
				}
			  prop_statements
			;

qt_enums:		  /* empty */ { }
			| IDENTIFIER qt_enums { qtEnums.append( $1 ); }
			;

qt_sets:		  /* empty */ { }
			| IDENTIFIER qt_sets { qtSets.append( $1 ); }
			;

%%

#if defined(_OS_WIN32_)
#include <io.h>
#undef isatty
extern "C" int hack_isatty( int )
{
    return 0;
}
#define isatty hack_isatty
#else
#include <unistd.h>
#endif

#include "lex.yy.c"

void 	  init();				// initialize
void 	  initClass();				// prepare for new class
void 	  generateClass();			// generate C++ code for class
void 	  initExpression();			// prepare for new expression
QCString  combinePath( const char *, const char * );

QCString  fileName;				// file name
QCString  outputFile;				// output file name
QCString  includeFile;				// name of #include file
QCString  includePath;				// #include file path
QCString  qtPath;				// #include qt file path
bool	  noInclude     = FALSE;		// no #include <filename>
bool	  generatedCode = FALSE;		// no code generated
bool	  mocError = FALSE;			// moc parsing error occurred
QCString  className;				// name of parsed class
QCString  superclassName;			// name of super class
FuncList  signals;				// signal interface
FuncList  slots;				// slots interface
FuncList  propfuncs;				// all possible property access functions
FuncList  funcs( TRUE );			// all parsed functions, including signals
EnumList  enums;				// enums used in properties
PropList  props;				// list of all properties
ClassInfoList	infos;				// list of all class infos

// Used to store the values in the Q_PROPERTY macro
QCString propWrite;				// set function
QCString propRead;				// get function
QCString propReset;				// reset function
QCString propStored;				// "true", "false" or function or empty if not specified
bool propOverride;				// Wether OVERRIDE was detected
int propDesignable;				// Wether DESIGNABLE was TRUE (1) or FALSE (0) or not specified (-1)

QStrList qtEnums;				// Used to store the contents of Q_ENUMS
QStrList qtSets;				// Used to store the contents of Q_SETS

FILE  *out;					// output file

int yyparse();

void replace( char *s, char c1, char c2 );

int main( int argc, char **argv )
{
    bool autoInclude = TRUE;
    char *error	     = 0;
    qtPath = "";
    for ( int n=1; n<argc && error==0; n++ ) {
	QCString arg = argv[n];
	if ( arg[0] == '-' ) {			// option
	    QCString opt = &arg[1];
	    if ( opt[0] == 'o' ) {		// output redirection
		if ( opt[1] == '\0' ) {
		    if ( !(n < argc-1) ) {
			error = "Missing output-file name";
			break;
		    }
		    outputFile = argv[++n];
		} else
		    outputFile = &opt[1];
	    } else if ( opt == "i" ) {		// no #include statement
		noInclude   = TRUE;
		autoInclude = FALSE;
	    } else if ( opt[0] == 'f' ) {	// produce #include statement
		noInclude   = FALSE;
		autoInclude = FALSE;
		if ( opt[1] ) {			// -fsomething.h
		    includeFile = &opt[1];
		}
	    } else if ( opt[0] == 'p' ) {	// include file path
		if ( opt[1] == '\0' ) {
		    if ( !(n < argc-1) ) {
			error = "Missing path name for the -p option.";
			break;
		    }
		    includePath = argv[++n];
		} else {
		    includePath = &opt[1];
		}
	    } else if ( opt[0] == 'q' ) {	// qt include file path
		if ( opt[1] == '\0' ) {
		    if ( !(n < argc-1) ) {
			error = "Missing path name for the -q option.";
			break;
		    }
		    qtPath = argv[++n];
		} else {
		    qtPath = &opt[1];
		}
		replace(qtPath.data(),'\\','/');
		if ( qtPath.right(1) != "/" )
		    qtPath += '/';
	    } else if ( opt == "k" ) {		// don't stop on errors
		errorControl = TRUE;
	    } else if ( opt == "nw" ) {		// don't display warnings
		displayWarnings = FALSE;
	    } else if ( opt == "ldbg" ) {	// lex debug output
		lexDebug = TRUE;
	    } else if ( opt == "ydbg" ) {	// yacc debug output
		yydebug = TRUE;
	    } else {
		error = "Invalid argument";
	    }
	} else {
	    if ( !fileName.isNull() )		// can handle only one file
		error	 = "Too many input files specified";
	    else
		fileName = arg.copy();
	}
    }

    if ( autoInclude ) {
	int ppos = fileName.findRev('.');
	if ( ppos != -1 && tolower( fileName[ppos + 1] ) == 'h' )
	    noInclude = FALSE;
	else
	    noInclude = TRUE;
    }
    if ( !fileName.isEmpty() && !outputFile.isEmpty() &&
	 includeFile.isEmpty() && includePath.isEmpty() ) {
	includeFile = combinePath(fileName,outputFile);
    }
    if ( includeFile.isEmpty() )
	includeFile = fileName.copy();
    if ( !includePath.isEmpty() ) {
	if ( includePath.right(1) != "/" )
	    includePath += '/';
	includeFile = includePath + includeFile;
    }
    if ( fileName.isNull() && !error ) {
	fileName = "standard input";
	yyin	 = stdin;
    } else if ( argc < 2 || error ) {		// incomplete/wrong args
	fprintf( stderr, "Qt meta object compiler\n" );
	if ( error )
	    fprintf( stderr, "moc: %s\n", error );
	fprintf( stderr, "Usage:  moc [options] <header-file>\n"
		 "\t-o file  Write output to file rather than stdout\n"
		 "\t-i       Do not generate an #include statement\n"
		 "\t-f[file] Force #include, optional file name\n"
		 "\t-p path  Path prefix for included file\n"
		 "\t-k       Do not stop on errors\n"
		 "\t-nw      Do not display warnings\n"
		 );
	return 1;
    } else {
	yyin = fopen( (const char *)fileName, "r" );
	if ( !yyin ) {
	    fprintf( stderr, "moc: %s: No such file\n", (const char*)fileName);
	    return 1;
	}
    }
    if ( !outputFile.isEmpty() ) {		// output file specified
	out = fopen( (const char *)outputFile, "w" );	// create output file
	if ( !out ) {
	    fprintf( stderr, "moc: Cannot create %s\n",
		     (const char*)outputFile );
	    return 1;
	}
    } else {					// use stdout
	out = stdout;
    }
    init();
    yyparse();
    fclose( yyin );
    if ( !outputFile.isNull() )
	fclose( out );

    if ( !generatedCode && displayWarnings && !mocError ) {
        fprintf( stderr, "%s:%d: Warning: %s\n", fileName.data(), 0,
		 "No relevant classes found. No output generated." );
    }

    slots.clear();
    signals.clear();
    propfuncs.clear();
    funcs.clear();
    infos.clear();
    props.clear();
    infos.clear();

    return mocError ? 1 : 0;
}

void replace( char *s, char c1, char c2 )
{
    if ( !s )
	return;
    while ( *s ) {
	if ( *s == c1 )
	    *s = c2;
	s++;
    }
}

/*
  This function looks at two file names and returns the name of the
  infile, with a path relative to outfile. Examples:
    /tmp/abc	/tmp/bcd	->	abc
    xyz/a/bc	xyz/b/ac	->	../a/bc
    /tmp/abc	xyz/klm		-)	/tmp/abc
 */

// Code stolen from QDir::isRelativePath
bool isRelativePath( const QString &path )
{
  int len = path.length();
  if ( len == 0 )
    return TRUE;

  int i = 0;
#ifdef WIN32
  if ( path[0].isLetter() && path[1] == ':' )		// drive, e.g. a:
    i = 2;
#endif
  return path[i] != '/' && path[i] != '\\';
}

// Code stolen from QDir::cleanDirPath
QString cleanDirPath( const QCString &filePath )
{
  QString name = filePath;
  QString newPath;

  if ( name.isEmpty() )
    return name;

  // already done before calling this function
  // slashify( name );

  bool addedSeparator;
  if ( isRelativePath(name) ) {
    addedSeparator = TRUE;
    name.insert( 0, '/' );
  } else {
    addedSeparator = FALSE;
  }

  int ePos, pos, upLevel;

  pos = ePos = name.length();
  upLevel = 0;
  int len;

  while ( pos && (pos = name.findRev('/',--pos)) != -1 ) {
    len = ePos - pos - 1;
    if ( len == 2 && name.at(pos + 1) == '.'
      && name.at(pos + 2) == '.' ) {
      upLevel++;
    } else {
      if ( len != 0 && (len != 1 || name.at(pos + 1) != '.') ) {
        if ( !upLevel )
          newPath = QString::fromLatin1("/")
          + name.mid(pos + 1, len) + newPath;
        else
          upLevel--;
      }
    }
    ePos = pos;
  }
  if ( addedSeparator ) {
    while ( upLevel-- )
      newPath.insert( 0, QString::fromLatin1("/..") );
    if ( !newPath.isEmpty() )
      newPath.remove( 0, 1 );
    else
      newPath = QString::fromLatin1(".");
  } else {
    if ( newPath.isEmpty() )
      newPath = QString::fromLatin1("/");
#if defined(_OS_FATFS_) || defined(_OS_OS2EMX_)
    if ( name[0] == '/' ) {
      if ( name[1] == '/' )		// "\\machine\x\ ..."
        newPath.insert( 0, '/' );
    } else {
      newPath = name.left(2) + newPath;
    }
#endif
  }
  return newPath;
}

QCString combinePath( const char *infile, const char *outfile )
{
    QCString a = infile;  replace(a.data(),'\\','/');
    QCString b = outfile; replace(b.data(),'\\','/');
    a = a.stripWhiteSpace();
    b = b.stripWhiteSpace();
    QString aDir( cleanDirPath( a ) );
    a = aDir;
    QString bDir( cleanDirPath( b ) );
    b = bDir;
    QCString r;
    int i = 0;
    int ncommondirs = 0;
    while ( a[i] && a[i] == b[i] ) {
	if ( a[i] == '/' && i > 0 )
	    ncommondirs++;
	i++;
    }
    if ( ncommondirs > 0 ) {			// common base directory
	while ( i>=0 ) {
	    if ( a[i] == '/' && b[i] == '/' )
		break;
	    --i;
	}
	++i;
	a = &a[i];
	b = &b[i];
    } else {
	if ( (a[0] == '/') || (isalpha(a[0]) && a[1] == ':') )
	    return a;
	b = &b[i];
    }
    i = b.contains('/');
    while ( i-- > 0 )
	r += "../";
    r += a;
    return r;
}


#define getenv hack_getenv			// workaround for byacc
char *getenv()		     { return 0; }
char *getenv( const char * ) { return 0; }

void init()					// initialize
{
    BEGIN OUTSIDE;
    lineNo	 = 1;
    skipClass	 = FALSE;
    skipFunc	 = FALSE;
    tmpArgList	 = new ArgList;
    tmpFunc	 = new Function;
    tmpEnum	 = new Enum;
}

void initClass()				 // prepare for new class
{
    tmpAccess      = Private;
    subClassPerm       = Private;
    Q_OBJECTdetected   = FALSE;
    Q_PROPERTYdetected = FALSE;
    skipClass	       = FALSE;
    templateClass      = FALSE;
    slots.clear();
    signals.clear();
    propfuncs.clear();
    enums.clear();
    funcs.clear();
    props.clear();
    infos.clear();
    qtSets.clear();
    qtEnums.clear();
}

struct NamespaceInfo
{
    QCString name;
    int pLevelOnEntering; // Parenthesis level on entering the namespace
    QDict<char> definedClasses; // Classes defined in the namespace
};

QList<NamespaceInfo> namespaces;

void enterNameSpace( const char *name = 0 )   	 // prepare for new class
{
    static bool first = TRUE;
    if ( first ) {
	namespaces.setAutoDelete( TRUE );
	first = FALSE;
    }

    NamespaceInfo *tmp = new NamespaceInfo;
    if ( name )
	tmp->name = name;
    tmp->pLevelOnEntering = namespacePLevel;
    namespaces.append( tmp );
}

void leaveNameSpace()				 // prepare for new class
{
    NamespaceInfo *tmp = namespaces.last();
    namespacePLevel = tmp->pLevelOnEntering;
    namespaces.remove();
}

QCString nameQualifier()
{
    QListIterator<NamespaceInfo> iter( namespaces );
    NamespaceInfo *tmp;
    QCString qualifier = "";
    for( ; (tmp = iter.current()) ; ++iter ) {
	if ( !tmp->name.isNull() ) {  // If not unnamed namespace
	    qualifier += tmp->name;
	    qualifier += "::";
	}
    }
    return qualifier;
}

int openNameSpaceForMetaObject( FILE *out )
{
    int levels = 0;
    QListIterator<NamespaceInfo> iter( namespaces );
    NamespaceInfo *tmp;
    QCString indent = "";
    for( ; (tmp = iter.current()) ; ++iter ) {
	if ( !tmp->name.isNull() ) {  // If not unnamed namespace
	    fprintf( out, "%snamespace %s {\n", (const char *)indent,
		     (const char *) tmp->name );
	    indent += "    ";
	    levels++;
	}
    }
    QCString nm = className;
    int pos;
    while( (pos = nm.find( "::" )) != -1 ) {
	QCString spaceName = nm.left( pos );
	nm = nm.right( nm.length() - pos - 2 );
	if ( !spaceName.isEmpty() ) {
	    fprintf( out, "%snamespace %s {\n", (const char *)indent,
		     (const char *) spaceName );
	    indent += "    ";
	    levels++;
	}
    }
    return levels;
}

void closeNameSpaceForMetaObject( FILE *out, int levels )
{
    int i;
    for( i = 0 ; i < levels ; i++ )
	    fprintf( out, "}" );
    if ( levels )
	fprintf( out, "\n" );

}

void selectOutsideClassState()
{
    if ( namespaces.count() == 0 )
	BEGIN OUTSIDE;
    else
	BEGIN IN_NAMESPACE;
}

void registerClassInNamespace()
{
    if ( namespaces.count() == 0 )
	return;
    namespaces.last()->definedClasses.insert((const char *)className,(char*)1);
}

//
// Remove white space from SIGNAL and SLOT names.
// This function has been copied from qobject.cpp.
//

inline bool isIdentChar( char x )
{						// Avoid bug in isalnum
    return x == '_' || (x >= '0' && x <= '9') ||
	 (x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z');
}

inline bool isSpace( char x )
{
#if defined(_CC_BOR_)
  /*
    Borland C++ 4.5 has a weird isspace() bug.
    isspace() usually works, but not here.
    This implementation is sufficient for our internal use: rmWS()
  */
    return (uchar)x <= 32;
#else
    return isspace( x );
#endif
}

static QCString rmWS( const char *src )
{
    QCString result( qstrlen(src)+1 );
    char *d = result.data();
    char *s = (char *)src;
    char last = 0;
    while( *s && isSpace(*s) )			// skip leading space
	s++;
    while ( *s ) {
	while ( *s && !isSpace(*s) )
	    last = *d++ = *s++;
	while ( *s && isSpace(*s) )
	    s++;
	if ( *s && isIdentChar(*s) && isIdentChar(last) )
	    last = *d++ = ' ';
    }
    result.truncate( (int)(d - result.data()) );
    return result;
}


void initExpression()
{
    tmpExpression = "";
}

void addExpressionString( const char *s )
{
    tmpExpression += s;
}

void addExpressionChar( const char c )
{
    tmpExpression += c;
}

void yyerror( const char *msg )			// print yacc error message
{
    mocError = TRUE;
    fprintf( stderr, "%s:%d: Error: %s\n", fileName.data(), lineNo, msg );
}

void moc_err( const char *s )
{
    yyerror( s );
    if ( errorControl ) {
	exit( -1 );
    }
}

void moc_err( const char *s1, const char *s2 )
{
    static char tmp[1024];
    sprintf( tmp, s1, s2 );
    yyerror( tmp );
    if ( errorControl ) {
	exit( -1 );
    }
}

void moc_warn( const char *msg )
{
    if ( displayWarnings )
	fprintf( stderr, "%s:%d: Warning: %s\n", fileName.data(), lineNo, msg);
}

void moc_warn( char *s1, char *s2 )
{
    static char tmp[1024];
    sprintf( tmp, s1, s2 );
    if ( displayWarnings )
	fprintf( stderr, "%s:%d: Warning: %s\n", fileName.data(), lineNo, tmp);
}

static bool suppress_func_warn = FALSE;
void func_warn( const char *msg )
{
    if ( !suppress_func_warn )
	moc_warn( msg );
    skipFunc = TRUE;
}

void operatorError()
{
    if ( !suppress_func_warn )
	moc_warn("Operator functions cannot be signals or slots.");
    skipFunc = TRUE;
}

#ifndef yywrap
int yywrap()					// more files?
{
    return 1;					// end of file
}
#endif

char *stradd( const char *s1, const char *s2 )	// adds two strings
{
    char *n = new char[qstrlen(s1)+qstrlen(s2)+1];
    qstrcpy( n, s1 );
    strcat( n, s2 );
    return n;
}

char *stradd( const char *s1, const char *s2, const char *s3 )// adds 3 strings
{
    char *n = new char[qstrlen(s1)+qstrlen(s2)+qstrlen(s3)+1];
    qstrcpy( n, s1 );
    strcat( n, s2 );
    strcat( n, s3 );
    return n;
}

char *stradd( const char *s1, const char *s2,
	      const char *s3, const char *s4 )// adds 4 strings
{
    char *n = new char[qstrlen(s1)+qstrlen(s2)+qstrlen(s3)+qstrlen(s4)+1];
    qstrcpy( n, s1 );
    strcat( n, s2 );
    strcat( n, s3 );
    strcat( n, s4 );
    return n;
}


char *straddSpc( const char *s1, const char *s2 )
{
    char *n = new char[qstrlen(s1)+qstrlen(s2)+2];
    qstrcpy( n, s1 );
    strcat( n, " " );
    strcat( n, s2 );
    return n;
}

char *straddSpc( const char *s1, const char *s2, const char *s3 )
{
    char *n = new char[qstrlen(s1)+qstrlen(s2)+qstrlen(s3)+3];
    qstrcpy( n, s1 );
    strcat( n, " " );
    strcat( n, s2 );
    strcat( n, " " );
    strcat( n, s3 );
    return n;
}

char *straddSpc( const char *s1, const char *s2,
	      const char *s3, const char *s4 )
{
    char *n = new char[qstrlen(s1)+qstrlen(s2)+qstrlen(s3)+qstrlen(s4)+4];
    qstrcpy( n, s1 );
    strcat( n, " " );
    strcat( n, s2 );
    strcat( n, " " );
    strcat( n, s3 );
    strcat( n, " " );
    strcat( n, s4 );
    return n;
}

// Generate C++ code for building member function table

QCString pureClassName()
{
    QCString result;
    int pos = className.findRev( "::");
    if ( pos != -1 )
        result = className.right( className.length() - pos - 2 );
    else
	result = className;
    return result;
}

QCString qualifiedClassName()
{
    QCString tmp = nameQualifier();
    tmp += className;
    return tmp;
}

QCString qualifiedSuperclassName()
{
    if ( namespaces.count() == 0 )
	return superclassName;
    if ( namespaces.last()->definedClasses.find((const char *)superclassName)){
	QCString tmp = nameQualifier();
	tmp += superclassName;
	return tmp;
    } else {
	return superclassName;
    }
}

QCString specialQualifiedSuperclassName()
{
#if defined(_CC_MSVC_)
    // MSVC doesn't grok the code NS::A::func() in a virtual method of a class B that 
    // inherits A, but is defined outside the namespace NS.
    int i = superclassName.findRev( "::" );
    if ( i < 0 )
	return superclassName;
    return superclassName.right( superclassName.length() - i - 2 );
#else
    return superclassName;
#endif
}

const int Slot_Num   = 1;
const int Signal_Num = 2;
const int Prop_Num = 3;

void generateFuncs( FuncList *list, char *functype, int num )
{
    Function *f;
    for ( f=list->first(); f; f=list->next() ) {
	QCString typstr = "";
	int count = 0;
	Argument *a = f->args->first();
	while ( a ) {
	    if ( !a->leftType.isEmpty() || ! a->rightType.isEmpty() ) {
		if ( count++ )
		    typstr += ",";
		typstr += a->leftType;
		typstr += a->rightType;
	    }
	    a = f->args->next();
	}
	fprintf( out, "    typedef %s (%s::*m%d_t%d)(%s)%s;\n",
		 (const char*)f->type, (const char*)qualifiedClassName(),
		 num, list->at(),
		 (const char*)typstr,  (const char*)f->qualifier );
	fprintf( out, "    typedef %s (QObject::*om%d_t%d)(%s)%s;\n",
		 (const char*)f->type,
		 num, list->at(),
		 (const char*)typstr,  (const char*)f->qualifier );
	f->type = f->name;
	f->type += "(";
	f->type += typstr;
	f->type += ")";
    }
    for ( f=list->first(); f; f=list->next() ) {
	fprintf( out, "    m%d_t%d v%d_%d = &%s::%s;\n",
		 num, list->at(), num, list->at(),
		 (const char*)qualifiedClassName(), (const char*)f->name);
	fprintf( out, "    om%d_t%d ov%d_%d = (om%d_t%d)v%d_%d;\n",
		 num, list->at(), num, list->at(),
		 num, list->at(), num, list->at());
    }
    if ( list->count() ) {
	fprintf(out,"    QMetaData *%s_tbl = QMetaObject::new_metadata(%d);\n",
		functype, list->count() );
    }

    //### remove 3.0
    if ( qstrcmp( functype, "slot" )  == 0 ) {
	if ( list->count() )
	    fprintf(out,"    QMetaData::Access *%s_tbl_access = QMetaObject::new_metaaccess(%d);\n",
		    functype, list->count() );
	else
	    fprintf(out,"    QMetaData::Access *%s_tbl_access = 0;\n",
		    functype );
    }


    for ( f=list->first(); f; f=list->next() ) {
	fprintf( out, "    %s_tbl[%d].name = \"%s\";\n",
		 functype, list->at(), (const char*)f->type );
	fprintf( out, "    %s_tbl[%d].ptr = (QMember)ov%d_%d;\n",
		 functype, list->at(), num, list->at() );

/* ### do this in 3.0:
	fprintf( out, "    %s_tbl[%d].access = QMetaData::%s;\n",
		 functype, list->at(), f->accessAsString() );

### for now:
*/
	if ( qstrcmp( functype, "slot" ) == 0 )
	    fprintf( out, "    %s_tbl_access[%d] = QMetaData::%s;\n",
		     functype, list->at(), f->accessAsString() );
    }
}


void generateTypedef( Function* f, int num )
{
  QCString typstr = "";
  int count = 0;
  Argument *a = f->args->first();
  while ( a ) {
    if ( !a->leftType.isEmpty() || ! a->rightType.isEmpty() ) {
      if ( count++ )
	typstr += ",";
      typstr += a->leftType;
      typstr += a->rightType;
    }
    a = f->args->next();
  }

  fprintf( out, "    typedef %s (%s::*m%d_t%d)(%s)%s;\n",
	   (const char*)f->type, (const char*)className, Prop_Num, num,
	   (const char*)typstr,  (const char*)f->qualifier );
  fprintf( out, "    typedef %s (QObject::*om%d_t%d)(%s)%s;\n",
	   (const char*)f->type, Prop_Num, num,
	   (const char*)typstr,  (const char*)f->qualifier );
}


bool isPropertyType( const char* type )
{
    if ( qvariant_nameToType( type ) != 0 )
	return TRUE;

    for( QListIterator<Enum> lit( enums ); lit.current(); ++lit ) {
	if ( lit.current()->name == type )
	    return TRUE;
    }
    return FALSE;
}

/*!
  Returns TRUE if the type is not one of a QVariant types.
  So it is either a enum/set or an error.
*/
bool isEnumType( const char* type )
{
    if ( qvariant_nameToType( type ) != 0 )
	return FALSE;

    return TRUE;
}

void finishProps()
{
    int entry = 0;
    fprintf( out, "#ifndef QT_NO_PROPERTIES\n" );
    for( QListIterator<Property> it( props ); it.current(); ++it ) {
	if ( !isPropertyType( it.current()->type ) ||
	     it.current()->override )
	    fprintf( out, "    metaObj->resolveProperty( &props_tbl[%d] );\n", entry );
	++entry;
    }
    fprintf( out, "#endif // QT_NO_PROPERTIES\n" );
}

int generateEnums()
{
    if ( enums.count() == 0 )
	return 0;

    fprintf( out, "#ifndef QT_NO_PROPERTIES\n" );
    fprintf( out, "    QMetaEnum* enum_tbl = QMetaObject::new_metaenum( %i );\n", enums.count() );

    int i = 0;
    for ( QListIterator<Enum> it( enums ); it.current(); ++it, ++i ) {
	fprintf( out, "    enum_tbl[%i].name = \"%s\";\n", i, (const char*)it.current()->name );
	fprintf( out, "    enum_tbl[%i].count = %u;\n", i, it.current()->count() );
	fprintf( out, "    enum_tbl[%i].items = QMetaObject::new_metaenum_item( %u );\n",
		 i, it.current()->count() );

	if ( it.current()->set )
		fprintf( out, "    enum_tbl[%i].set = TRUE;\n", i );
	else
		fprintf( out, "    enum_tbl[%i].set = FALSE;\n", i );

	int k = 0;
	for( QStrListIterator eit( *it.current() ); eit.current(); ++eit, ++k ) {
	    fprintf( out, "    enum_tbl[%i].items[%i].key = \"%s\";\n", i, k, eit.current() );
	    fprintf( out, "    enum_tbl[%i].items[%i].value = (int) %s::%s;\n",
		     i, k, (const char*)className, eit.current() );
	}
    }
    fprintf( out, "#endif // QT_NO_PROPERTIES\n" );

    return enums.count();
}


int generateProps()
{
    if ( displayWarnings && !Q_OBJECTdetected )
	moc_err("The declaration of the class \"%s\" contains properties"
		" but no Q_OBJECT macro!", className.data());

    fprintf( out, "#ifndef QT_NO_PROPERTIES\n" );
    //
    // Resolve and verify property access functions
    //
    for( QListIterator<Property> it( props ); it.current(); ) {
	Property* p = it.current();
	++it;

	// verify get function
	if ( !p->get.isEmpty() ) {
	    FuncList candidates = propfuncs.find( p->get );
	    for ( Function* f = candidates.first(); f; f = candidates.next() ) {
		if ( f->qualifier != "const" ) // get functions must be const
		    continue;
		if ( f->args && !f->args->isEmpty() ) // and must not take any arguments
		    continue;
		QCString tmp = f->type;
		tmp = tmp.simplifyWhiteSpace();
		Property::Specification spec = Property::Unspecified;
		if ( p->type == "QCString" && (tmp == "const char*" || tmp == "const char *" ) ) {
		    tmp = "QCString";
		    spec = Property::ConstCharStar;
		} else if ( tmp.right(1) == "&" ) {
		    tmp = tmp.left( tmp.length() - 1 );
		    spec = Property::Reference;
		} else if ( tmp.right(1) == "*" ) {
		    tmp = tmp.left( tmp.length() - 1 );
		    spec = Property::Pointer;
		} else {
		    spec = Property::Class;
		}
		if ( tmp.left(6) == "const " )
		    tmp = tmp.mid( 6, tmp.length() - 6 );
		tmp = tmp.simplifyWhiteSpace();
		if ( p->type == tmp ) {
		    // If it is an enum then it may not be a set
		    bool ok = TRUE;
		    for( QListIterator<Enum> lit( enums ); lit.current(); ++lit )
			if ( lit.current()->name == p->type && lit.current()->set )
			    ok = FALSE;
		    if ( !ok ) continue;
		    p->gspec = spec;
		    p->getfunc = f;
		    p->oredEnum = 0;
		    break;
		}
		else if ( isEnumType( p->type ) ) {
		    if ( tmp == "int" || tmp == "uint" || tmp == "unsigned int" ) {
			// Test wether the enum is really a set (unfortunately we don't know enums of super classes)
			bool ok = TRUE;
			for( QListIterator<Enum> lit( enums ); lit.current(); ++lit )
			    if ( lit.current()->name == p->type && !lit.current()->set )
				ok = FALSE;
			if ( !ok ) continue;
		        p->gspec = spec;
		        p->getfunc = f;
			p->oredEnum = 1;
		    }
		}
	    }
	    if ( p->getfunc == 0 ) {
		if ( displayWarnings ) {

		    // Is the type a set, that means, mentioned in Q_SETS ?
		    bool set = FALSE;
		    for( QListIterator<Enum> lit( enums ); lit.current(); ++lit )
			if ( lit.current()->name == p->type && lit.current()->set )
			    set = TRUE;

		    fprintf( stderr, "%s:%d: Warning: Property '%s' not available.\n",
			     fileName.data(), p->lineNo, (const char*) p->name );
		    fprintf( stderr, "   Have been looking for public get functions \n");
		    if ( !set ) {
			fprintf( stderr,
			     "      %s %s() const\n"
			     "      %s& %s() const\n"
			     "      const %s& %s() const\n"
			     "      %s* %s() const\n",
			     (const char*) p->type, (const char*) p->get,
			     (const char*) p->type, (const char*) p->get,
			     (const char*) p->type, (const char*) p->get,
			     (const char*) p->type, (const char*) p->get );
		    }
		    if ( set || !isPropertyType( p->type ) ) {
			fprintf( stderr,
			     "      int %s() const\n"
			     "      uint %s() const\n"
			     "      unsigned int %s() const\n",
			     (const char*) p->get,
			     (const char*) p->get,
			     (const char*) p->get );
		    }
		    if ( p->type == "QCString" )
			fprintf( stderr, "      const char* %s() const\n",
				 (const char*)p->get );

		    if ( candidates.isEmpty() ) {
			fprintf( stderr, "   but found nothing.\n");
		    } else {
			fprintf( stderr, "   but only found the missmatching candidate(s)\n");
			for ( Function* f = candidates.first(); f; f = candidates.next() ) {
			    QCString typstr = "";
			    Argument *a = f->args->first();
			    int count = 0;
			    while ( a ) {
				if ( !a->leftType.isEmpty() || ! a->rightType.isEmpty() ) {
				    if ( count++ )
					typstr += ",";
				    typstr += a->leftType;
				    typstr += a->rightType;
				}
				a = f->args->next();
			    }
			    fprintf( stderr, "      %s:%d: %s %s(%s) %s\n", fileName.data(), f->lineNo,
				     (const char*) f->type,(const char*) f->name, (const char*) typstr,
				     f->qualifier.isNull()?"":(const char*) f->qualifier );
			}
		    }
		}
	    }
	}

	// verify set function
	if ( !p->set.isEmpty() ) {
	    FuncList candidates = propfuncs.find( p->set );
	    for ( Function* f = candidates.first(); f; f = candidates.next() ) {
		if ( !f->args || f->args->isEmpty() )
		    continue;
		QCString tmp = f->args->first()->leftType;
		tmp = tmp.simplifyWhiteSpace();
		Property::Specification spec = Property::Unspecified;
		if ( tmp.right(1) == "&" ) {
		    tmp = tmp.left( tmp.length() - 1 );
		    spec = Property::Reference;
		}
		else {
		    spec = Property::Class;
		}
		if ( p->type == "QCString" && (tmp == "const char*" || tmp == "const char *" ) ) {
		    tmp = "QCString";
		    spec = Property::ConstCharStar;
		}
		if ( tmp.left(6) == "const " )
		    tmp = tmp.mid( 6, tmp.length() - 6 );
		tmp = tmp.simplifyWhiteSpace();

		if ( p->type == tmp && f->args->count() == 1 ) {
		    // If it is an enum then it may not be a set
		    if ( p->oredEnum == 1 )
			continue;
		    bool ok = TRUE;
		    for( QListIterator<Enum> lit( enums ); lit.current(); ++lit )
			if ( lit.current()->name == p->type && lit.current()->set )
			    ok = FALSE;
		    if ( !ok ) continue;
		    p->sspec = spec;
		    p->setfunc = f;
		    p->oredEnum = 0;
		    break;
		}
		else if ( isEnumType( p->type ) && f->args->count() == 1 ) {
		    if ( tmp == "int" || tmp == "uint" || tmp == "unsigned int" ) {
		        if ( p->oredEnum == 0 )
			    continue;
			// Test wether the enum is really a set (unfortunately we don't know enums of super classes)
			bool ok = TRUE;
			for( QListIterator<Enum> lit( enums ); lit.current(); ++lit )
			    if ( lit.current()->name == p->type && !lit.current()->set )
				ok = FALSE;
			if ( !ok ) continue;
		        p->sspec = spec;
		        p->setfunc = f;
			p->oredEnum = 1;
		    }
		}
	    }
	    if ( p->setfunc == 0 ) {
		if ( displayWarnings ) {

		    // Is the type a set, that means, mentioned in Q_SETS ?
		    bool set = FALSE;
		    for( QListIterator<Enum> lit( enums ); lit.current(); ++lit )
			if ( lit.current()->name == p->type && lit.current()->set )
			    set = TRUE;

		    fprintf( stderr, "%s:%d: Warning: Property '%s' not writable.\n",
			     fileName.data(), p->lineNo, (const char*) p->name );
		    fprintf( stderr, "   Have been looking for public set functions \n");
		    if ( !set && p->oredEnum != 1 ) {
			fprintf( stderr,
			     "      void %s( %s )\n"
			     "      void %s( %s& )\n"
			     "      void %s( const %s& )\n",
			     (const char*) p->set, (const char*) p->type,
			     (const char*) p->set, (const char*) p->type,
			     (const char*) p->set, (const char*) p->type );
		    }
		    if ( set || ( !isPropertyType( p->type ) && p->oredEnum != 0 ) ) {
			fprintf( stderr,
			     "      void %s( int )\n"
			     "      void %s( uint )\n"
			     "      void %s( unsigned int )\n",
			     (const char*) p->set,
			     (const char*) p->set,
			     (const char*) p->set );
		    }

		    if ( p->type == "QCString" )
			fprintf( stderr, "      void %s( const char* ) const\n",
				 (const char*) p->set );

		    if ( !candidates.isEmpty() ) {
			fprintf( stderr, "   but only found the missmatching candidate(s)\n");
			for ( Function* f = candidates.first(); f; f = candidates.next() ) {
			    QCString typstr = "";
			    Argument *a = f->args->first();
			    int count = 0;
			    while ( a ) {
				if ( !a->leftType.isEmpty() || ! a->rightType.isEmpty() ) {
				    if ( count++ )
					typstr += ",";
				    typstr += a->leftType;
				    typstr += a->rightType;
				}
				a = f->args->next();
			    }
			    fprintf( stderr, "      %s:%d: %s %s(%s)\n", fileName.data(), f->lineNo,
				     (const char*) f->type,(const char*) f->name, (const char*) typstr );
			}
		    }
		}
	    }
	}

	// verify reset function
	if ( !p->reset.isEmpty() ) {
	    FuncList candidates = propfuncs.find( p->reset );
	    for ( Function* f = candidates.first(); f; f = candidates.next() ) {
		if ( f->qualifier == "const" ) // reset functions must not be const
		    continue;
		if ( f->args && !f->args->isEmpty() ) // and must not take any arguments
		    continue;
		QCString tmp = f->type;
		tmp = tmp.simplifyWhiteSpace();
		if ( tmp != "void" ) // Reset function must return void
		    continue;
		p->resetfunc = f;
	    }
	    if ( p->resetfunc == 0 ) {
		if ( displayWarnings ) {

		    fprintf( stderr, "%s:%d: Warning: Property '%s' not resetable.\n",
			     fileName.data(), p->lineNo, (const char*) p->name );
		    fprintf( stderr, "   Have been looking for public reset functions \n");
		    fprintf( stderr, "      void %s()\n", (const char*) p->reset );

		    if ( candidates.isEmpty() ) {
			fprintf( stderr, "   but found nothing.\n");
		    } else {
			fprintf( stderr, "   but only found the missmatching candidate(s)\n");
			for ( Function* f = candidates.first(); f; f = candidates.next() ) {
			    QCString typstr = "";
			    Argument *a = f->args->first();
			    int count = 0;
			    while ( a ) {
				if ( !a->leftType.isEmpty() || ! a->rightType.isEmpty() ) {
				    if ( count++ )
					typstr += ",";
				    typstr += a->leftType;
				    typstr += a->rightType;
				}
				a = f->args->next();
			    }
			    fprintf( stderr, "      %s:%d: %s %s(%s) %s\n", fileName.data(), f->lineNo,
				     (const char*) f->type,(const char*) f->name, (const char*) typstr,
				     f->qualifier.isNull()?"":(const char*) f->qualifier );
			}
		    }
		}
	    }
	}

	// Resolve and verify the STORED function (if any)
	if ( !p->stored.isEmpty() &&  p->stored != "true" && p->stored != "false" ) {
	    bool found = FALSE;
	    FuncList candidates = propfuncs.find( p->stored );
	    for ( Function* f = candidates.first(); f; f = candidates.next() ) {
		if ( f->qualifier != "const" ) // stored functions must be const
		    continue;
		if ( f->args && !f->args->isEmpty() ) // and must not take any arguments
		    continue;

		if ( f->type == "bool" )
		    found = TRUE;
	    }
	    if ( !found ) {
		if ( displayWarnings ) {
		    fprintf( stderr, "%s:%d: Warning: Property '%s' not stored.\n",
			     fileName.data(), p->lineNo, (const char*) p->name );
		    fprintf( stderr, "   Have been looking for public function \n"
			     "      bool %s() const\n",
			     (const char*) p->stored );

		    if ( candidates.isEmpty() ) {
			fprintf( stderr, "   but found nothing.\n");
		    } else {
			fprintf( stderr, "   but only found the missmatching candidate(s)\n");
			for ( Function* f = candidates.first(); f; f = candidates.next() ) {
			    QCString typstr = "";
			    Argument *a = f->args->first();
			    int count = 0;
			    while ( a ) {
				if ( !a->leftType.isEmpty() || ! a->rightType.isEmpty() ) {
				    if ( count++ )
					typstr += ",";
				    typstr += a->leftType;
				    typstr += a->rightType;
				}
				a = f->args->next();
			    }
			    fprintf( stderr, "      %s:%d: %s %s(%s) %s\n", fileName.data(), f->lineNo,
				     (const char*) f->type,(const char*) f->name, (const char*) typstr,
				     f->qualifier.isNull()?"":(const char*) f->qualifier );
			}
		    }
		}
	    }
	}

    }

    //
    // Generate all typedefs
    //
    {
	int count = 0;
	for( QListIterator<Property> it( props ); it.current(); ++it ) {
	    if ( it.current()->getfunc )
		generateTypedef( it.current()->getfunc, count );
	    ++count;
	    if ( it.current()->setfunc )
		generateTypedef( it.current()->setfunc, count );
	    ++count;
	    if ( it.current()->resetfunc )
		generateTypedef( it.current()->resetfunc, count );
	    ++count;
	}
    }

    {
	int count = 0;
	for( QListIterator<Property> it( props ); it.current(); ++it ) {
	    if ( it.current()->getfunc ) {
		fprintf( out, "    m%d_t%d v%d_%d = &%s::%s;\n",
			 Prop_Num, count, Prop_Num, count,
			 (const char*)className, (const char*)it.current()->getfunc->name );
		fprintf( out, "    om%d_t%d ov%d_%d = (om%d_t%d)v%d_%d;\n",
			 Prop_Num, count, Prop_Num, count, Prop_Num, count, Prop_Num, count );
	    }
	    ++count;
	    if ( it.current()->setfunc ) {
		fprintf( out, "    m%d_t%d v%d_%d = &%s::%s;\n",
			 Prop_Num, count, Prop_Num, count,
			 (const char*)className, (const char*)it.current()->setfunc->name );
		fprintf( out, "    om%d_t%d ov%d_%d = (om%d_t%d)v%d_%d;\n",
			 Prop_Num, count, Prop_Num, count, Prop_Num, count, Prop_Num, count );
	    }
	    ++count;
	    if ( it.current()->resetfunc ) {
		fprintf( out, "    m%d_t%d v%d_%d = &%s::%s;\n",
			 Prop_Num, count, Prop_Num, count,
			 (const char*)className, (const char*)it.current()->resetfunc->name );
		fprintf( out, "    om%d_t%d ov%d_%d = (om%d_t%d)v%d_%d;\n",
			 Prop_Num, count, Prop_Num, count, Prop_Num, count, Prop_Num, count );
	    }
	    ++count;
	}
    }

    //
    // Create meta data
    //
    if ( props.count() )
    {
	fprintf( out, "    QMetaProperty *props_tbl = QMetaObject::new_metaproperty( %d );\n", props.count() );
	int count = 0;
	int entry = 0;
	for( QListIterator<Property> it( props ); it.current(); ++it ){

	    fprintf( out, "    props_tbl[%d].t = \"%s\";\n", entry,
		     (const char*)it.current()->type );
	    fprintf( out, "    props_tbl[%d].n = \"%s\";\n",
		     entry, (const char*) it.current()->name );

	    if ( it.current()->getfunc )
		fprintf( out, "    props_tbl[%d].get = (QMember)ov%d_%d;\n",
			 entry, Prop_Num, count );
	    else
		fprintf( out, "    props_tbl[%d].get = 0;\n", entry );

	    if ( it.current()->setfunc )
		fprintf( out, "    props_tbl[%d].set = (QMember)ov%d_%d;\n",
			 entry, Prop_Num, count + 1 );
	    else
		fprintf( out, "    props_tbl[%d].set = 0;\n", entry );

	    if ( it.current()->resetfunc )
		fprintf( out, "    props_tbl[%d].reset = (QMember)ov%d_%d;\n",
			 entry, Prop_Num, count + 2 );
	    else
		fprintf( out, "    props_tbl[%d].reset = 0;\n", entry );

	    fprintf( out, "    props_tbl[%d].gspec = QMetaProperty::%s;\n",
		     entry, Property::specToString(it.current()->gspec ));

	    fprintf( out, "    props_tbl[%d].sspec = QMetaProperty::%s;\n",
		     entry, Property::specToString(it.current()->sspec ));

	    int enumpos = -1;
	    int k = 0;
	    for( QListIterator<Enum> eit( enums ); eit.current(); ++eit, ++k ){
		if ( eit.current()->name == it.current()->type )
		    enumpos = k;
	    }

	    QCString flags;
	
	    // Is it an enum of this class ?
	    if ( enumpos != -1 )
		fprintf( out, "    props_tbl[%d].enumData = &enum_tbl[%i];\n", entry, enumpos );
	    // Is it an unknown enum that needs to be resolved ?
	    else if (!isPropertyType( it.current()->type ) ) {
		if ( it.current()->oredEnum == 1 )
		    flags += "QMetaProperty::UnresolvedSet|";
		else if ( it.current()->oredEnum == 0 )
		    flags += "QMetaProperty::UnresolvedEnum|";
		else
		    flags +="QMetaProperty::UnresolvedEnumOrSet|";
	    }

	    // Handle STORED
	    if ( it.current()->stored == "false" )
		flags +="QMetaProperty::NotStored|";
	    else if ( !it.current()->stored.isEmpty() && it.current()->stored != "true" )
	    {
		fprintf( out, "    typedef bool (%s::*s3_t%d)()const;\n", (const char*)className, count );
		fprintf( out, "    typedef bool (QObject::*os3_t%d)()const;\n", count );
		fprintf( out, "    s3_t%d sv3_%d = &%s::%s;\n", count, count, (const char*)className,
			 (const char*)it.current()->stored );
		fprintf( out, "    os3_t%d osv3_%d = (os3_t%d)sv3_%d;\n", count, count, count, count );
		fprintf( out, "    props_tbl[%d].store = (QMember)osv3_%d;\n", entry, count );
	    }
	    // else { Default is TRUE -> do nothing }

	    // OVERRIDE but no STORED ?
	    if ( it.current()->override && it.current()->stored.isEmpty() )
		flags += "QMetaProperty::UnresolvedStored|";

	    // Handle DESIGNABLE
	    if ( it.current()->designable == 0 )
		flags += "QMetaProperty::NotDesignable|";
	    // else { Default is TRUE -> do nothing }

	    // OVERRIDE but no DESIGNABLE ?
	    if ( it.current()->override && it.current()->designable == -1 )
		flags += "QMetaProperty::UnresolvedDesignable|";
	
	    if ( it.current()->stdSet() )
		flags += "QMetaProperty::StdSet|";
	
	    if (!flags.isEmpty() ) {
		if ( flags[ (int) flags.length() - 1] == '|' )
		    flags.remove( flags.length()-1, 1);
		fprintf( out, "    props_tbl[%d].setFlags(%s);\n", entry, flags.data() );
	    }
	
	    ++entry;
	    count += 3;
	}
    }
    fprintf( out, "#endif // QT_NO_PROPERTIES\n" );

    return props.count();
}


int generateClassInfos()
{
    if ( infos.isEmpty() )
	return 0;

    if ( displayWarnings && !Q_OBJECTdetected )
	moc_err("The declaration of the class \"%s\" contains class infos"
		" but no Q_OBJECT macro!", className.data());

    fprintf( out, "    QClassInfo* classinfo_tbl = QMetaObject::new_classinfo( %i );\n", infos.count() );

    int i = 0;
    for( QListIterator<ClassInfo> it( infos ); it.current(); ++it, ++i ) {
	fprintf( out, "    classinfo_tbl[%i].name = \"%s\";\n", i, it.current()->name.data() );
	fprintf( out, "    classinfo_tbl[%i].value = \"%s\";\n", i, it.current()->value.data() );
    }

    return i;
}

void generateClass()		      // generate C++ source code for a class
{
    static int gen_count = 0;
    char *hdr1 = "/****************************************************************************\n"
		 "** %s meta object code from reading C++ file '%s'\n**\n";
    char *hdr2 = "** Created: %s\n"
		 "**      by: The Qt MOC ($Id$)\n**\n";
    char *hdr3 = "** WARNING! All changes made in this file will be lost!\n";
    char *hdr4 = "*****************************************************************************/\n\n";
    int   i;

    if ( skipClass )				// don't generate for class
	return;
    if ( !Q_OBJECTdetected ) {
	if ( signals.count() == 0 && slots.count() == 0 && props.count() == 0 && infos.count() == 0 )
	    return;
	generatedCode = TRUE;
	if ( displayWarnings && (signals.count()+slots.count()) != 0 )
	    moc_err("The declaration of the class \"%s\" contains slots "
		    "and/or signals\n\t but no Q_OBJECT macro!", className.data());
    } else {
	if ( superclassName.isEmpty() )
	    moc_err("The declaration of the class \"%s\" contains the\n"
		    "\tQ_OBJECT macro but does not inherit from any class!\n"
		    "\tInherit from QObject or one of its descendants"
		    " or remove Q_OBJECT. ", className.data() );
    }
    if ( templateClass ) {			// don't generate for class
	moc_err( "Sorry, Qt does not support templates that contain\n"
		 "signals, slots or Q_OBJECT. This will be supported soon." );
	return;
    }
    generatedCode = TRUE;
    if ( gen_count++ == 0 ) {			// first class to be generated
	QDateTime dt = QDateTime::currentDateTime();
	QCString dstr = dt.toString().ascii();
	QCString fn = fileName;
	i = fileName.length()-1;
	while ( i>0 && fileName[i-1] != '/' && fileName[i-1] != '\\' )
	    i--;				// skip path
	if ( i >= 0 )
	    fn = &fileName[i];
	fprintf( out, hdr1, (const char*)qualifiedClassName(),(const char*)fn);
	fprintf( out, hdr2, (const char*)dstr );
	fprintf( out, hdr3 );
	fprintf( out, hdr4 );
	fprintf( out, "#if !defined(Q_MOC_OUTPUT_REVISION)\n" );
	fprintf( out, "#define Q_MOC_OUTPUT_REVISION %d\n", formatRevision );
	fprintf( out, "#elif Q_MOC_OUTPUT_REVISION != %d\n", formatRevision );
	fprintf( out, "#error \"Moc format conflict - "
		 "please regenerate all moc files\"\n" );
	fprintf( out, "#endif\n\n" );
	if ( !noInclude )
	    fprintf( out, "#include \"%s\"\n", (const char*)includeFile );
	fprintf( out, "#include <%sqmetaobject.h>\n", (const char*)qtPath );
	fprintf( out, "#include <%sqapplication.h>\n", (const char*)qtPath );
	fprintf( out, "\n" );
	fprintf( out, "\n\n" );
    } else {
	fprintf( out, "\n\n" );
    }


//
// Generate virtual function className()
//
    fprintf( out, "const char *%s::className() const\n{\n    ",
	     (const char*)qualifiedClassName() );
    fprintf( out, "return \"%s\";\n}\n\n", (const char*)qualifiedClassName() );

//
// Generate static metaObj variable
//
    fprintf( out, "QMetaObject *%s::metaObj = 0;\n\n", (const char*)qualifiedClassName());

//
// Generate initMetaObject member function
//
    fprintf( out, "void %s::initMetaObject()\n{\n", (const char*)qualifiedClassName() );
    fprintf( out, "    if ( metaObj )\n\treturn;\n" );
    fprintf( out, "    if ( qstrcmp(%s::className(), \"%s\") != 0 )\n"
	          "\tbadSuperclassWarning(\"%s\",\"%s\");\n",
             (const char*)specialQualifiedSuperclassName(), (const char*)qualifiedSuperclassName(),
             (const char*)qualifiedClassName(), (const char*)qualifiedSuperclassName() );
    fprintf( out, "    (void) staticMetaObject();\n");
    fprintf( out, "}\n\n");

//
// Generate tr member function ### 3.0 one function
//
    fprintf( out, "#ifndef QT_NO_TRANSLATION\n\n" );
    fprintf( out, "QString %s::tr(const char* s)\n{\n",
	     (const char*)qualifiedClassName() );
    fprintf( out, "    return qApp->translate"
	     "( \"%s\", s, 0 );\n}\n\n", (const char*)qualifiedClassName() );
    fprintf( out, "QString %s::tr(const char* s, const char * c)\n{\n",
	     (const char*)qualifiedClassName() );
    fprintf( out, "    return qApp->translate"
	     "( \"%s\", s, c );\n}\n\n", (const char*)qualifiedClassName() );
    fprintf( out, "#endif // QT_NO_TRANSLATION\n\n" );

//
// Generate staticMetaObject member function
//
    fprintf( out, "QMetaObject* %s::staticMetaObject()\n{\n", (const char*)qualifiedClassName() );
    fprintf( out, "    if ( metaObj )\n\treturn metaObj;\n" );
    fprintf( out, "    (void) %s::staticMetaObject();\n", (const char*)superclassName );

//
// Build the enums array in staticMetaObject()
// Enums HAVE to be generated BEFORE the properties
//
   int n_enums = generateEnums();

//
// Build property array in staticMetaObject()
//
   int n_props = generateProps();

//
// Build class info  array in staticMetaObject()
//
   int n_infos = generateClassInfos();

//
// Build slots array in staticMetaObject()
//
    generateFuncs( &slots, "slot", Slot_Num );

//
// Build signals array in staticMetaObject()
//
    generateFuncs( &signals, "signal", Signal_Num );

//
// Finally code to create and return meta object
//
    fprintf( out, "    metaObj = QMetaObject::new_metaobject(\n"
		  "\t\"%s\", \"%s\",\n",
	     (const char*)qualifiedClassName(), (const char*)qualifiedSuperclassName() );

    if ( slots.count() )
	fprintf( out, "\tslot_tbl, %d,\n", slots.count() );
    else
	fprintf( out, "\t0, 0,\n" );

    if ( signals.count() )
	fprintf( out, "\tsignal_tbl, %d,\n", signals.count());
    else
	fprintf( out, "\t0, 0,\n" );

    fprintf( out, "#ifndef QT_NO_PROPERTIES\n" );
    if ( n_props )
	fprintf( out, "\tprops_tbl, %d,\n", n_props );
    else
	fprintf( out, "\t0, 0,\n" );

    if ( n_enums )
	fprintf( out, "\tenum_tbl, %d,\n", n_enums );
    else
	fprintf( out, "\t0, 0,\n" );
    fprintf( out, "#endif // QT_NO_PROPERTIES\n" );

    if ( n_infos )
	fprintf( out, "\tclassinfo_tbl, %d );\n", n_infos );
    else
	fprintf( out, "\t0, 0 );\n" );


    //### remove 3.0
    fprintf( out, "    metaObj->set_slot_access( slot_tbl_access );\n" );

//
// Finish property array in staticMetaObject()
//
    finishProps();

    fprintf( out, "    return metaObj;\n}\n" );

//
// End of function staticMetaObject()
//

//
// Generate internal signal functions
//
    Function *f;
    f = signals.first();			// make internal signal methods
    static bool included_list_stuff = FALSE;
    while ( f ) {
	QCString typstr = "";			// type string
	QCString valstr = "";			// value string
	QCString argstr = "";			// argument string (type+value)
	char	buf[12];
	Argument *a = f->args->first();
	QCString typvec[32], valvec[32], argvec[32];
	typvec[0] = "";
	valvec[0] = "";
	argvec[0] = "";

	i = 0;
	while ( a ) {				// argument list
	    if ( !a->leftType.isEmpty() || !a->rightType.isEmpty() ) {
		if ( i ) {
		    typstr += ",";
		    valstr += ", ";
		    argstr += ", ";
		}
		typstr += a->leftType;
		typstr += a->rightType;
		argstr += a->leftType;
		argstr += " ";
		sprintf( buf, "t%d", i );
		valstr += buf;
		argstr += buf;
		argstr += a->rightType;
		++i;
		typvec[i] = typstr.copy();
		valvec[i] = valstr.copy();
		argvec[i] = argstr.copy();
	    }
	    a = f->args->next();
	}

	const char *predef_call_func;
	if ( typstr == "bool" ) {
	    predef_call_func = "activate_signal_bool";
	} else if ( typstr == "QString" ) {
	    predef_call_func = "activate_signal_string";
	} else if ( typstr == "const QString&" ) {
	    predef_call_func = "activate_signal_strref";
	} else if ( typstr.isEmpty() || typstr == "short" ||
		    typstr == "int" ||  typstr == "long" ||
		    typstr == "char*" || typstr == "const char*" ) {
	    predef_call_func = "activate_signal";
	} else {
	    predef_call_func = 0;
	}
	if ( !predef_call_func && !included_list_stuff ) {
	    // yes we need it, because otherwise QT_VERSION may not be defined
	    fprintf( out, "\n#include <%sqobjectdefs.h>\n", (const char*)qtPath );
	    fprintf( out, "#include <%sqsignalslotimp.h>\n", (const char*)qtPath );
	    included_list_stuff = TRUE;
	}

	fprintf( out, "\n/" /* c++ */ "/ SIGNAL %s\n", (const char*)f->name );
	fprintf( out, "void %s::%s(", (const char*)qualifiedClassName(),
		 (const char*)f->name );

	if ( argstr.isEmpty() )
	    fprintf( out, ")\n{\n" );
	else
	    fprintf( out, " %s )\n{\n", (const char*)argstr );

	if ( predef_call_func ) {
	    fprintf( out, "    %s( \"%s(%s)\"", predef_call_func,
		     (const char*)f->name, (const char*)typstr );
	    if ( !valstr.isEmpty() )
		fprintf( out, ", %s", (const char*)valstr );
	    fprintf( out, " );\n}\n" );
	    f = signals.next();
	    continue;
	}


	fprintf( out,"    // No builtin function for signal parameter type %s\n",
		 (const char*)typstr );
	int nargs = f->args->count();
	fprintf( out, "    QConnectionList *clist = receivers(\"%s(%s)\");\n",
		 (const char*)f->name, (const char*)typstr );
	fprintf( out, "    if ( !clist || signalsBlocked() )\n\treturn;\n" );
	if ( nargs ) {
	    for ( i=0; i<=nargs; i++ ) {
		fprintf( out, "    typedef void (QObject::*RT%d)(%s);\n",
			 i, (const char*)typvec[i] );
	    }
	} else {
	    fprintf( out, "    typedef void (QObject::*RT)(%s);\n",
		     (const char*)typstr);
	}
	if ( nargs ) {
	    for ( i=0; i<=nargs; i++ )
		fprintf( out, "    RT%d r%d;\n", i, i );
	} else {
	    fprintf( out, "    RT r;\n" );
	}
	fprintf( out, "    QConnectionListIt it(*clist);\n" );
	fprintf( out, "    QConnection   *c;\n" );
	fprintf( out, "    QSenderObject *object;\n" );
	fprintf( out, "    while ( (c=it.current()) ) {\n" );
	fprintf( out, "\t++it;\n" );
	fprintf( out, "\tobject = (QSenderObject*)c->object();\n" );
	fprintf( out, "\tobject->setSender( this );\n" );
	if ( nargs ) {
	    fprintf( out, "\tswitch ( c->numArgs() ) {\n" );
	    for ( i=0; i<=nargs; i++ ) {
		fprintf( out, "\t    case %d:\n", i );
		fprintf( out, "#ifdef Q_FP_CCAST_BROKEN\n" );
		fprintf( out, "\t\tr%d = reinterpret_cast<RT%d>(*(c->member()));\n", i, i );
		fprintf( out, "#else\n" );
		fprintf( out, "\t\tr%d = (RT%d)*(c->member());\n", i, i );
		fprintf( out, "#endif\n" );
		fprintf( out, "\t\t(object->*r%d)(%s);\n",
			 i, (const char*)valvec[i] );
		fprintf( out, "\t\tbreak;\n" );
	    }
	    fprintf( out, "\t}\n" );
	} else {
	    fprintf( out, "#ifdef Q_FP_CCAST_BROKEN\n" );
	    fprintf( out, "\tr = reinterpret_cast<RT>(*(c->member()));\n" );
	    fprintf( out, "#else\n" );
	    fprintf( out, "\tr = (RT)*(c->member());\n" );
	    fprintf( out, "\t(object->*r)(%s);\n", (const char*)valstr );
	    fprintf( out, "#endif\n" );
	}
	fprintf( out, "    }\n}\n" );
	f = signals.next();
    }
}

ArgList *addArg( Argument *a )			// add argument to list
{
    tmpArgList->append( a );
    return tmpArgList;
}

void addEnum()
{
    // Avoid duplicates
    for( QListIterator<Enum> lit( enums ); lit.current(); ++lit ) {
	if ( lit.current()->name == tmpEnum->name )
        {
	    if ( displayWarnings )
		moc_err( "Enum %s defined twice.", (const char*)tmpEnum->name );
	}
    }

    // Only look at stuff in Q_ENUMS and Q_SETS
    if ( qtEnums.contains( tmpEnum->name ) || qtSets.contains( tmpEnum->name ) )
    {
	enums.append( tmpEnum );
	if ( qtSets.contains( tmpEnum->name ) )
	    tmpEnum->set = TRUE;
	else
	    tmpEnum->set = FALSE;
    }
    else
	delete tmpEnum;
    tmpEnum = new Enum;
}

void addMember( Member m )
{
    if ( skipFunc ) {
	tmpFunc->args = tmpArgList; // just to be sure
  	delete tmpFunc;
	tmpArgList  = new ArgList;   // ugly but works!
	tmpFunc	    = new Function;
	skipFunc    = FALSE;
	return;
    }

    tmpFunc->access = tmpAccess;
    tmpFunc->args	= tmpArgList;
    tmpFunc->lineNo	= lineNo;

    funcs.append( tmpFunc );

    switch( m ) {
    case SignalMember:
	signals.append( tmpFunc );
	break;
    case SlotMember:
	slots.append( tmpFunc );
	// fall trough
    case PropertyCandidateMember:
	if ( !tmpFunc->name.isEmpty() && tmpFunc->access == Public )
	    propfuncs.append( tmpFunc );
    }

 Failed:
    skipFunc = FALSE;
    tmpFunc  = new Function;
    tmpArgList = new ArgList;
}

/* Used to check property names. They must match the pattern
 * [A-Za-z][A-Za-z0-9_]*
 */
void checkIdentifier( const char* ident )
{
    const char* p = ident;
    if ( p == 0 || *p == 0 )
    {
	moc_err( "A property name must not be of zero length");
	return;
    }
    if ( !( *p >= 'A' && *p <= 'Z' ) && !( *p >= 'a' && *p <= 'z' )  )
    {
	moc_err( "'%s' is not a valid property name. It must match the pattern [A-Za-z][A-Za-z0-9_]*", (char*) ident );
	return;
    }

    while( *p )
    {
    	if ( !( *p >= 'A' && *p <= 'Z' ) && !( *p >= 'a' && *p <= 'z' ) && !( *p >= '0' && *p <= '9' ) && *p != '_' )
        {
	    moc_err( "'%s' is not a valid property name. It must match the pattern [A-Za-z][A-Za-z0-9_]*", (char*) ident );
            return;
        }
	++p;
    }
}
