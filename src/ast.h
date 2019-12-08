// https://en.wikipedia.org/wiki/Shunting-yard_algorithm
#include "tkscanner.h"

#define EXPRESSION_LIST_SIZE 10
#define STATEMENT_LIST_SIZE  50

// statement unknowns are: 3; "some str"; func_call(); method.call();
#define FOREACH_STATEMENT_TYPE(func) \
	func(STMNT_UNKNOWN)	    \
	func(STMNT_IMPORT)	    \
	func(STMNT_VAR_INIT)    \
	func(STMNT_ASSIGN) 	    \
	func(STMNT_IF)		    \
	func(STMNT_WHILE) 	    \
	func(STMNT_FOR)		    \
	func(STMNT_FOREACH)	    \
	func(STMNT_FUNC_DEFN)   \
	func(STMNT_CLASS_DEFN)
	//func(STMNT_FUNC_CALL) \ it's unknonwn

enum StatementType
{
	FOREACH_STATEMENT_TYPE(GENERATE_ENUM)
};

/* expression always ends with a semi collon */
struct Expression
{
	struct TokenList* token_list;
	int begin_pos;
	int end_pos;
};

struct ExprDtype // to store dtype int, string, map<int, map<string, list<string>>>
{
	struct Token* dtype;

	bool is_list;
	bool is_map;

	struct Token*     key; // key cant be list or map
	struct ExprDtype* value; // value is for both list and map
};

/*********************************************/
struct _StatementImport
{
	struct Token* path; // import path; path is string
};
struct _StatementVarInit
{
	struct ExprDtype* dtype;
	struct Token* idf;
	struct Expression* expr;
	bool has_expr;
};

struct _StatementVarAssign
{
	struct Expression* idf; // idf can be : my_ins.get_position().x = 2;
	struct Token* op; 		// op can be =, +=. -=. *=, ...
	struct Expression* expr;
};
struct _StatementUnknown
{
	bool has_expr;
	struct Expression* expr; // just execute the expr
};
struct _StatementIf
{
	struct Expression* expr_bool;
	struct StatementList* stmn_list;
};
struct _StatementWhile
{
	struct Expression* expr_bool;
	struct StatementList* stmn_list;
};
struct _StatementFor
{
	struct Expression* expr_ini;   // for (expr_ini; expr_bool; expr_end){ stmn_list; } TODO: change to statement list
	struct Expression* expr_bool;
	struct Expression* expr_end;
	struct StatementList* stmn_list;
};
struct _StatementForEach
{
	struct Expression* expr_ini;   // foreach(expr_ini; expr_itter){ stmn_list; } TODO: change to statement list
	struct Expression* expr_itter;  
	struct StatementList* stmn_list;
};
struct _StatementFuncDefn
{
	struct Token* idf;
	struct ExpressionList* args;  // TODO: change to list of statement ini
	struct ExprDtype* ret_type;
	struct StatementList* stmn_list;
};
struct _StatementClassDefn
{
	struct Token* idf;
	struct Token* par;
	struct StatementList* stmn_list;
};
union _Statement
{
	struct _StatementUnknown	unknown;
	struct _StatementImport 	import;
	struct _StatementVarInit 	init;
	struct _StatementVarAssign	assign;
	struct _StatementIf			stm_if;
	struct _StatementWhile		stm_while;
	struct _StatementFor 		stm_for;
	struct _StatementForEach 	stm_foreach;
	struct _StatementFuncDefn 	func_defn;
	struct _StatementClassDefn	class_defn;
};
/*********************************************/

struct Statement
{
	enum StatementType type;
	union _Statement statement;
	int indent; // for printing

};

struct ExpressionList
{
	struct TokenList* token_list;
	struct Expression** list;
	int count;
	int size;
	int growth_size;
};
struct StatementList
{
	struct Statement** list;
	int count;
	int size;
	int growth_size;
};

/*********************************************/

struct Ast
{
	char* src;
	char* file_name;
	int pos;
	struct TokenScanner* token_scanner;
	struct TokenList* tokens;
	struct StatementList* stmn_list;
};



/****************** PUBLIC API ************************************/
const char* enumStatementType_toString(enum StatementType self);

// expression
void structExpression_init(struct Expression* self, struct TokenList* token_list);
void structExpression_print(struct Expression* self, int indent);
struct Expression* structExpression_new(struct TokenList* token_list); // static

// expression dtype
void structExprDtype_init(struct ExprDtype* self, struct Token* dtype);
void structExprDtype_print(struct ExprDtype* self, int indent, bool new_line); // call with new_line true; false internal
struct ExprDtype* structExprDtype_new(struct Token* dtype);

// expression list
void structExpressionList_init(struct ExpressionList* self, int growth_size, struct TokenList* token_list);
void structExpressionList_addExpression(struct ExpressionList* self, struct Expression* expr);
struct Expression* structExpressionList_createExpression(struct ExpressionList* self);
struct ExpressionList* structExpressionList_new(struct TokenList* token_list); // static method

// statement
void structStatement_init(struct Statement* self);
void structStatement_print(struct Statement* self);
struct Statement* structStatement_new(); // static method

// statement list
void structStatementList_init(struct StatementList* self, int growth_size);
void structStatementList_print(struct StatementList* self);
void structStatementList_addStatement(struct StatementList* self, struct Statement* statement);
struct Statement* structStatementList_createStatement(struct StatementList* self);
struct StatementList* structStatementList_new(); // static method

// ast
void structAst_init(struct Ast* self, char* src, char* fiel_name);
void structAst_scane(struct Ast* self);
void structAst_makeTree(struct Ast* self);

/****************************************************************/