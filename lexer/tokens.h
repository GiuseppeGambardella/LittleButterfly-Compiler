#ifndef TOKENS_H
#define TOKENS_H

// Usiamo numeri positivi (0 è riservato per EOF - fine file)
enum TokenType {
    //Token numerici/operzioni
    TOKEN_CONST_INTEGER=258,
    TOKEN_CONST_STRING=284,
    TOKEN_CONST_CHAR=285,
    TOKEN_CONST_DOUBLE=286,
    TOKEN_ID=259,
    TOKEN_TRUE=260,
    TOKEN_FALSE=261,
    TOKEN_AND=262,
    TOKEN_OR=263,

    //Token parole chiave
    TOKEN_IF=264,
    TOKEN_THEN=265,
    TOKEN_ELSE=266,
    TOKEN_LOOP=267,
    TOKEN_MAIN=269,
    TOKEN_INT=268,
    TOKEN_DOUBLE=274,
    TOKEN_BOOL=275,
    TOKEN_VOID=276,
    TOKEN_CHAR=277,
    TOKEN_STRING=278,


    //operatori relazionali
    TOKEN_EQ=280
    TOKEN_NEQ=281,
    TOKEN_LE=282,
    TOKEN_GE=283,

    //funzioni
    TOKEN_FUNC=279,
    TOKEN_PRINT=270,
    TOKEN_SCAN=271,
    TOKEN_RETURN=272,

    //Token speciali
    TOKEN_EOF =273,



};

#endif