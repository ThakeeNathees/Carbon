#ifndef TKSCANNER_H
#define TKSCANNER_H

#include "carbon_conf.h"

#define TOKEN_NAME_SIZE 100

// token types
#define FOREACH_ENUM(func) \
	func(UNKNOWN)		\
	func(SYMBOL) 		\
	func(BRACKET) 		\
	func(DTYPE)			\
	func(OPERATOR)		\
	func(KEYWORD)		\
	func(BUILTIN)		\
	func(NUMBER)		\
	func(STRING) 		\
	func(IDENTIFIER) // variable, function, class name

#define GENERATE_ENUM(enum_name) enum_name,
#define GENERATE_STRING(enum_name) #enum_name,

/**************** CLASSES **********************/
enum TokenType
{
	FOREACH_ENUM(GENERATE_ENUM)
};

struct Token
{
	char* 			token_name;
	int 			_name_len;
	int 			_name_ptr;
	enum TokenType 	token_type;
};

struct TokenScanner
{
	char* src;
	struct Token current_token;
};

/****************** PUBLIC API ************************************/
const char* enumTokenType_toString(enum TokenType self);

// token
void structToken_init(struct Token* self);
int  structToken_toString(struct Token* self, char* buffer);
void structToken_print(struct Token* self);

// token scanner
void  structTokenScanner_init(struct TokenScanner* self, char* src);
bool structTokenScanner_scaneToken(struct TokenScanner* self, int* pos); // return true if eof

/****************************************************************/


// symbols
#define SYM_DOT 		"."
#define SYM_COMMA 		","
#define SYM_COLLON		":"
#define SYM_SEMI_COLLON	";"
#define SYM_DQUOTE		"\""
#define SYM_SQUOTE 		"'"

// brackets
#define LPARN 		 	"("
#define RPARN 		 	")"
#define LCRU_BRACKET 	"{"
#define RCRU_BRACKET 	"}"
#define RSQ_BRACKET 	"["
#define LSQ_BRACKET 	"]"
#define RTRI_BRACKET	"<"
#define LTRI_BRACKET	">"

// data types
#define DTYPE_BOOL 	 	"bool"
#define DTYPE_CAHR 	 	"char"
#define DTYPE_SHORT	 	"short"
#define DTYPE_INT 	 	"int"
#define DTYPE_LONG 	 	"long"
#define DTYPE_FLOAT	 	"float"
#define DTYPE_DOUBLE  	"double"

#define DTYPE_LIST 		"list"
#define DTYPE_ARRAY 	"array"
#define DTYPE_MAP 		"map"
#define DTYPE_STRING 	"string"
#define DTYPE_FUNC 		"func"

// arithmetic operators
#define OP_PLUS			"+"
#define OP_PLUSEQ 		"+="
#define OP_MINUS		"-"
#define OP_MINUSEQ 		"-="
#define OP_MUL			"*"
#define OP_MUEQ 		"*="
#define OP_DIV 			"/"
#define OP_DIVEQ 		"/="
#define OP_MOD			"%"
#define OP_DIV_FLOOR	"//"

// bool operators
#define OP_EQEQ 		"=="
#define OP_GT 			">"
#define OP_LT 			"<"
#define OP_EQ 			"="
#define OP_GTEQ 		">="
#define OP_LTEQ 		"<="


// bitwise operators
#define OP_LSHIFT		"<<"
#define OP_RSHIFT 		">>"
#define OP_OR 			"|"
#define OP_AND 			"&"
#define OP_XOR 			"^"

// keywords
#define KWORD_NULL 		"null"
#define KWORD_TRUE 		"true"
#define KWORD_FALSE		"false"
#define KWORD_IF 		"if"
#define KWORD_ELSE 		"else"
#define KWORD_WHILE 	"while"
#define KWORD_BREAK 	"break"
#define KWORD_CONTINUE 	"continue"
#define KWORD_AND 		"and"
#define KWORD_OR 		"or"
#define KWORD_NOT 		"not"
#define KWORD_RETURN 	"return"
#define KWORD_STATIC 	"static"
#define KWORD_FUNCTION 	"function"
#define KWORD_CLASS 	"class"
#define KWORD_IMPORT 	"import"

// built in func
#define BUILTIN_PRINT 	"print"
#define BUILTIN_INPUT 	"input"
#define BUILTIN_MIN   	"min"
#define BUILTIN_MAX   	"max"
#define BUILTIN_RAND  	"rand"


#endif