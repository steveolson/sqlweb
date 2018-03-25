#ifndef YYERRCODE
#define YYERRCODE 256
#endif

#define IF 257
#define AND 258
#define OR 259
#define NOT 260
#define BETWEEN 261
#define LIKE 262
#define IS 263
#define NULLX 264
#define TRUEX 265
#define FALSEX 266
#define IN 267
#define ASSIGN 268
#define CAT 269
#define COMPARISON 270
#define LT 271
#define GT 272
#define LE 273
#define GE 274
#define EQ 275
#define NEQ 276
#define STRING 277
#define INTNUM 278
#define APPROXNUM 279
#define FUNCNAME 280
#define LITERAL 281
#define TO_SCALAR 282
#define UMINUS 283
typedef union {
    eBoolean_t bVal;	/* Boolean Value eTrue or eFalse */
    Scalar_t *pScalar;	/* Scalar Variable */
} YYSTYPE;
extern YYSTYPE yylval;
