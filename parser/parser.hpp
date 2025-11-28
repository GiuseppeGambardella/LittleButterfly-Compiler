/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_PARSER_HPP_INCLUDED
# define YY_YY_PARSER_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    TOKEN_EOF = 0,                 /* TOKEN_EOF  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    TOKEN_SCONOSCIUTO = 258,       /* TOKEN_SCONOSCIUTO  */
    TOKEN_INT = 259,               /* TOKEN_INT  */
    TOKEN_DOUBLE = 260,            /* TOKEN_DOUBLE  */
    TOKEN_CHAR = 261,              /* TOKEN_CHAR  */
    TOKEN_STRING = 262,            /* TOKEN_STRING  */
    TOKEN_VOID = 263,              /* TOKEN_VOID  */
    TOKEN_BOOL = 264,              /* TOKEN_BOOL  */
    TOKEN_CONST_INTEGER = 265,     /* TOKEN_CONST_INTEGER  */
    TOKEN_CONST_DOUBLE = 266,      /* TOKEN_CONST_DOUBLE  */
    TOKEN_CONST_CHAR = 267,        /* TOKEN_CONST_CHAR  */
    TOKEN_CONST_STRING = 268,      /* TOKEN_CONST_STRING  */
    TOKEN_TRUE = 269,              /* TOKEN_TRUE  */
    TOKEN_FALSE = 270,             /* TOKEN_FALSE  */
    TOKEN_ID = 271,                /* TOKEN_ID  */
    TOKEN_IF = 272,                /* TOKEN_IF  */
    TOKEN_THEN = 273,              /* TOKEN_THEN  */
    TOKEN_ELSE = 274,              /* TOKEN_ELSE  */
    TOKEN_LOOP = 275,              /* TOKEN_LOOP  */
    TOKEN_MAIN = 276,              /* TOKEN_MAIN  */
    TOKEN_FUNC = 277,              /* TOKEN_FUNC  */
    TOKEN_PRINT = 278,             /* TOKEN_PRINT  */
    TOKEN_SCAN = 279,              /* TOKEN_SCAN  */
    TOKEN_RETURN = 280,            /* TOKEN_RETURN  */
    TOKEN_EQ = 281,                /* TOKEN_EQ  */
    TOKEN_NEQ = 282,               /* TOKEN_NEQ  */
    TOKEN_LE = 283,                /* TOKEN_LE  */
    TOKEN_GE = 284,                /* TOKEN_GE  */
    TOKEN_AND = 285,               /* TOKEN_AND  */
    TOKEN_OR = 286                 /* TOKEN_OR  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_PARSER_HPP_INCLUDED  */
