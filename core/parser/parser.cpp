//------------------------------------------------------------------------------
// MIT License
//------------------------------------------------------------------------------
// 
// Copyright (c) 2020 Thakee Nathees
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//------------------------------------------------------------------------------

#include "parser.h"

namespace carbon {

#define IF_IDF_ALREADY_FOUND_RET_ERR(m_identifier, m_node)                                                                                \
	do {                                                                                                                                  \
		IdentifierLocation loc = _find_identifier_location(m_identifier, m_node);                                                         \
		if (loc.found) {                                                                                                                  \
			_throw(Error::ALREADY_DEFINED, String::format("Identifier %s already defined at %s:%i", loc.file_path, loc.line));            \
		}                                                                                                                                 \
	} while (false)

void Parser::_throw(Error::Type p_type, const String& p_msg, int p_line) {
    /*TODO: msg += __LINE__, __FUNCTION__*/
	if (p_line > 0) {
		throw Error(p_type, p_msg, Vect2i(p_line, 0));
	} else {
		throw Error(p_type, p_msg, Vect2i(tokenizer->get_line(), 0));
	}
}

void Parser::_throw_unexp_token(const String& p_exp) {
	if (p_exp != String()) {
		_throw(Error::SYNTAX_ERROR, String::format("Unexpected token(\"%s\"). expected \"%s\"", "<tk_name>", p_exp));
	} else {
		_throw(Error::SYNTAX_ERROR, String::format("Unexpected token(\"%s\").", "<tk_name>"));
	}
}

// TODO: refector after class
Parser::IdentifierLocation Parser::_find_identifier_location(const String& p_name, const ptr<Node> p_node) const {
	ASSERT(p_node == nullptr || p_node->type == Node::Type::BLOCK || p_node->type == Node::Type::CLASS);

	// if class scope no need to check outer scope
	if (p_node && p_node->type == Node::Type::CLASS) {
		for (const ptr<VarNode>& lv : ptrcast<ClassNode>(p_node)->members) {
			if (lv->name == p_name) {
				return IdentifierLocation(p_node, file_path);
			}
		}
		return IdentifierLocation();
	}

	ptr<Node> outer_node = p_node;
	while (outer_node) {
		switch (outer_node->type) {

			case Node::Type::BLOCK: {
				for (const ptr<VarNode>& local_var : ptrcast<BlockNode>(outer_node)->local_vars) {
					if (local_var->name == p_name) {
						return IdentifierLocation(outer_node, file_path);
					}
				}
			}
			case Node::Type::FUNCTION: {
				for (const String& arg : ptrcast<FunctionNode>(outer_node)->args) {
					if (arg == p_name) {
						return IdentifierLocation(outer_node, file_path);
					}
				}
			}
		}
		outer_node = outer_node->parernt_node;
	}

	for (const ptr<ClassNode>& struct_node : file_node->classes) {
		if (struct_node->name == p_name) {
			return IdentifierLocation(struct_node, file_path);
		}
	}
	for (const ptr<EnumNode>& enum_node : file_node->enums) {
		if (enum_node->name == p_name) {
			return IdentifierLocation(enum_node, file_path);
		}
	}
	for (const ptr<FunctionNode>& func_node : file_node->functions) {
		if (func_node->name == p_name) {
			return IdentifierLocation(func_node, file_path);
		}
	}

	// TODO: find from import lib, binary

	return IdentifierLocation();
}

void Parser::parse(String p_source, String p_file_path) {

	source = p_source;
	file_path = p_file_path;
	file_node = newptr<FileNode>();
	tokenizer->tokenize(source); // this will throw

	while (true) {
	
		const TokenData& token = tokenizer->next();
		switch (token.type) {
			case  Token::_EOF:
				return;
			case Token::KWORD_IMPORT: {
				// TODO:
				break;
			}
			case Token::KWORD_CLASS: {
				_parse_class();
				break;
			}
			case Token::KWORD_ENUM: {
				_parse_enum();
				break;
			}
			case Token::KWORD_FUNC: {
				ptr<FunctionNode> func = _parse_func(file_node, true);
				file_node->functions.push_back(func);
				break;
			}
			case Token::KWORD_VAR: {
				stdvec<ptr<VarNode>> vars = _parse_var(file_node, true);
				for (ptr<VarNode>& _var : vars) {
					file_node->file_vars.push_back(_var);
				}
				break;
			}
			// Ignore.
			case Token::SYM_SEMI_COLLON: 
			case Token::VALUE_STRING:
				break;
			default:
				_throw_unexp_token();
		}

	} // while true

}


ptr<Parser::ClassNode> Parser::_parse_class() {
	ASSERT(tokenizer->peek(-1).type == Token::KWORD_CLASS);
	ptr<ClassNode> class_node = newptr<ClassNode>();

	const TokenData* tk = &tokenizer->next();

	if (tk->type != Token::IDENTIFIER) {
		_throw_unexp_token("<identifier>");
	}
	// TODO: check identifier predefined.
	class_node->name = tk->identifier;

	tk = &tokenizer->next();

	if (tk->type == Token::SYM_COLLON) {
		const TokenData& base = tokenizer->next();
		if (base.type != Token::IDENTIFIER) {
			_throw_unexp_token("<identifier>");
		}
		// TODO: check identifier predefined.
		class_node->base = base.identifier;

		tk = &tokenizer->next();
	}

	if (tk->type != Token::BRACKET_LCUR) {
		_throw_unexp_token("{");
	}
	
	while (true) {
		const TokenData& token = tokenizer->next();
		switch (token.type) {
			case Token::_EOF:
				_throw(Error::UNEXPECTED_EOF, "Unexpected end of file.");

			case Token::BRACKET_RCUR:
				return class_node;

			case Token::SYM_SEMI_COLLON: // ignore
				break;

			case Token::KWORD_ENUM:
				_parse_enum(class_node);
				break;

			case Token::KWORD_STATIC:
				if (tokenizer->peek().type != Token::KWORD_FUNC && tokenizer->peek().type != Token::KWORD_VAR) {
					_throw_unexp_token("func or var");
				}
				break;

			case Token::KWORD_FUNC: {
				bool _static = tokenizer->peek(-2).type == Token::KWORD_STATIC;
				ptr<FunctionNode> func = _parse_func(class_node, _static);
				if (_static) {
					class_node->static_functions.push_back(func);
				} else {
					class_node->functions.push_back(func);
				}
				break;
			}

			case Token::KWORD_VAR: {
				bool _static = tokenizer->peek(-2).type == Token::KWORD_STATIC;
				stdvec<ptr<VarNode>> vars = _parse_var(class_node, _static);
				for (ptr<VarNode>& _var : vars) {
					if (_static) {
						class_node->static_members.push_back(_var);
					} else {
						class_node->members.push_back(_var);
					}
				}
				break;
			}

			default:
				_throw_unexp_token();
		}
	}

}

ptr<Parser::EnumNode> Parser::_parse_enum(ptr<Node> p_parent) {
	ASSERT(tokenizer->peek(-1).type == Token::KWORD_ENUM);

	ptr<EnumNode> enum_node = newptr<EnumNode>();
	int cur_value = -1;

	const TokenData& enum_name = tokenizer->next();
	if (enum_name.type != Token::IDENTIFIER) {
		_throw_unexp_token("<identifier>");
	}
	enum_node->name = enum_name.identifier;

	if (tokenizer->next().type != Token::BRACKET_LCUR) {
		_throw_unexp_token("{");
	}

	while (true) {
		const TokenData& token = tokenizer->next();
		switch (token.type) {
			case Token::_EOF:
				_throw(Error::UNEXPECTED_EOF, "Unexpected end of file.");

			case Token::BRACKET_RCUR:
				return enum_node;

			case Token::SYM_SEMI_COLLON: // ignore
				break;

			case Token::IDENTIFIER: {

				// TODO: check identifier with struct name, enum name, static var name, and from import, import-> import , ...
				enum_node->values[token.identifier] = ++cur_value;
				
				const TokenData* tk = &tokenizer->next();
				if (tk->type == Token::OP_EQ) {
					tk = &tokenizer->next();
					if (tk->type != Token::VALUE_INT) {
						_throw_unexp_token("<integer constant>");
					}
					ASSERT(tk->constant.get_type() == var::INT);
					cur_value = tk->constant;
					enum_node->values[token.identifier] = cur_value;
				}

				tk = &tokenizer->next();
				if (tk->type != Token::SYM_COMMA) {
					_throw_unexp_token(",");
				}
				break;
			}

			default:
				_throw_unexp_token("<identifier>");
		}
	}
}

stdvec<ptr<Parser::VarNode>> Parser::_parse_var(ptr<Node> p_node, bool p_static) {
	ASSERT(tokenizer->peek(-1).type == Token::KWORD_VAR);
	ASSERT(p_node != nullptr);
	ASSERT(p_node->type == Node::Type::FILE || p_node->type == Node::Type::BLOCK || p_node->type == Node::Type::CLASS);

	// check identifier when reducing
	// IF_IDF_ALREADY_FOUND_RET_ERR(tk->identifier, p_node);

#define PARSE_EXPR_VAR()                                              \
	while (true) {                                                    \
	                                                                  \
		tk = &tokenizer->next();                                      \
		if (tk->type != Token::IDENTIFIER) {                          \
			_throw_unexp_token("<identifier>");                       \
		}                                                             \
		ptr<VarNode> var_node = newptr<VarNode>();                    \
		var_node->name = tk->identifier;                              \
	                                                                  \
		tk = &tokenizer->next();                                      \
		if (tk->type == Token::OP_EQ) {                               \
			ptr<Node> expr = _parse_expression(p_node, p_static);     \
			var_node->assignment = expr;                              \
	                                                                  \
			tk = &tokenizer->next();                                  \
			if (tk->type == Token::SYM_COMMA) {                       \
			} else if (tk->type == Token::SYM_SEMI_COLLON) {          \
				break;                                                \
			} else {                                                  \
				_throw_unexp_token();                                 \
			}                                                         \
		} else if (tk->type == Token::SYM_COMMA) {                    \
		} else if (tk->type == Token::SYM_SEMI_COLLON) {              \
			break;                                                    \
		} else {                                                      \
			_throw_unexp_token();                                     \
		}                                                             \
		vars.push_back(var_node);                                     \
	}

	const TokenData* tk;
	stdvec<ptr<VarNode>> vars;

	switch (p_node->type) {
		case Node::Type::CLASS: {
			PARSE_EXPR_VAR();
			break;
		}
		case Node::Type::BLOCK: {
			ASSERT(!p_static);
			PARSE_EXPR_VAR();
			break;
		}
		case Node::Type::FILE: {
			ASSERT(!p_static);
			PARSE_EXPR_VAR();
		}
	}

	return vars;
}

// TODO: newptr to new_node<T> which sets the node's line, col

ptr<Parser::FunctionNode> Parser::_parse_func(ptr<Node> p_parent, bool p_static) {
	ASSERT(tokenizer->peek(-1).type == Token::KWORD_FUNC);
	ptr<FunctionNode> func_node = newptr<FunctionNode>();

	const TokenData* tk = &tokenizer->next();
	if (tk->type != Token::IDENTIFIER) {
		_throw_unexp_token("<identifier>");
	}

	func_node->name = tk->identifier;
	func_node->is_static = p_static;

	// TODO: arguments

	if (tokenizer->next().type != Token::BRACKET_LCUR) {
		_throw_unexp_token("{");
	}

	ptr<BlockNode> body = _parse_block(func_node);
	if (tokenizer->peek(-1).type != Token::BRACKET_RCUR) {
		_throw_unexp_token("}");
	}
	func_node->body = body;
	return func_node;
}

}