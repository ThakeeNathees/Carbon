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

void Parser::parse(String p_source, String p_file_path) {

	file_node = new_node<FileNode>();
	file_node->source = p_source;
	file_node->path = p_file_path;
	tokenizer->tokenize(file_node->source); // this will throw

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
				ptr<FunctionNode> func = _parse_func(file_node);
				file_node->functions.push_back(func);
				break;
			}
			case Token::KWORD_VAR: {
				stdvec<ptr<VarNode>> vars = _parse_var(file_node);
				for (ptr<VarNode>& _var : vars) {
					file_node->vars.push_back(_var);
				}
				break;
			}
			// Ignore.
			case Token::SYM_SEMI_COLLON: 
			case Token::VALUE_STRING:
				break;
			default:
				THROW_UNEXP_TOKEN("");
		}

	} // while true

}

ptr<Parser::ClassNode> Parser::_parse_class() {
	ASSERT(tokenizer->peek(-1).type == Token::KWORD_CLASS);
	ptr<ClassNode> class_node = new_node<ClassNode>();
	parser_context.current_class = class_node.get();

	const TokenData* tk = &tokenizer->next();

	if (tk->type != Token::IDENTIFIER) {
		THROW_UNEXP_TOKEN("an identifier");
	}
	// TODO: check identifier predefined.
	class_node->name = tk->identifier;

	tk = &tokenizer->next();

	if (tk->type == Token::SYM_COLLON) {
		const TokenData& base = tokenizer->next();
		// TODO: base could be builtin class (Object, File, Map)
		if (base.type != Token::IDENTIFIER) {
			THROW_UNEXP_TOKEN("an identifier");
		}
		// TODO: check identifier predefined.
		class_node->base = base.identifier;

		tk = &tokenizer->next();
	}

	if (tk->type != Token::BRACKET_LCUR) {
		THROW_UNEXP_TOKEN("symbol \"{\"");
	}
	
	while (true) {
		const TokenData& token = tokenizer->next();
		switch (token.type) {

			case Token::_EOF: {
				THROW_PARSER_ERR(Error::UNEXPECTED_EOF, "Unexpected end of file.", Vect2i());
			} break;

			case Token::BRACKET_RCUR: {
				return class_node;
			}

			case Token::SYM_SEMI_COLLON: { // ignore
			} break;

			case Token::KWORD_ENUM: {
				_parse_enum(class_node);
			} break;

			case Token::KWORD_STATIC: {
				if (tokenizer->peek().type != Token::KWORD_FUNC && tokenizer->peek().type != Token::KWORD_VAR) {
					THROW_UNEXP_TOKEN("keyword \"func\" or \"var\"");
				}

			} break;

			case Token::KWORD_FUNC: {
				bool _static = tokenizer->peek(-2).type == Token::KWORD_STATIC;
				ptr<FunctionNode> func = _parse_func(class_node);
				if (_static) {
					class_node->static_functions.push_back(func);
				} else {
					class_node->functions.push_back(func);
				}
			} break;

			case Token::KWORD_VAR: {
				bool _static = tokenizer->peek(-2).type == Token::KWORD_STATIC;
				stdvec<ptr<VarNode>> vars = _parse_var(class_node);
				for (ptr<VarNode>& _var : vars) {
					if (_static) {
						class_node->static_members.push_back(_var);
					} else {
						class_node->members.push_back(_var);
					}
				}
			} break;

			default: {
				THROW_UNEXP_TOKEN("");
			}
		}
	}

	parser_context.current_class = nullptr;
}

ptr<Parser::EnumNode> Parser::_parse_enum(ptr<Node> p_parent) {
	ASSERT(tokenizer->peek(-1).type == Token::KWORD_ENUM);

	ptr<EnumNode> enum_node = new_node<EnumNode>();
	int cur_value = -1;

	const TokenData& enum_name = tokenizer->next();
	if (enum_name.type != Token::IDENTIFIER) {
		THROW_UNEXP_TOKEN("an identifier");
	}
	enum_node->name = enum_name.identifier;

	if (tokenizer->next().type != Token::BRACKET_LCUR) {
		THROW_UNEXP_TOKEN("symbol \"{\"");
	}

	while (true) {
		const TokenData& token = tokenizer->next();
		switch (token.type) {

			case Token::_EOF: {
				THROW_PARSER_ERR(Error::UNEXPECTED_EOF, "Unexpected end of file.", Vect2i());
			} break;

			case Token::BRACKET_RCUR: {
				return enum_node;
			} break;

			case Token::SYM_SEMI_COLLON: { // ignore
			} break;

			case Token::IDENTIFIER: {
				// TODO: check identifier with struct name, enum name, static var name, and from import, import-> import , ...
				enum_node->values[token.identifier] = ++cur_value;
				
				const TokenData* tk = &tokenizer->next();
				if (tk->type == Token::OP_EQ) {
					tk = &tokenizer->next();
					if (tk->type != Token::VALUE_INT) {
						THROW_UNEXP_TOKEN("an integer constant");
					}
					ASSERT(tk->constant.get_type() == var::INT);
					cur_value = tk->constant;
					enum_node->values[token.identifier] = cur_value;
				}

				tk = &tokenizer->next();
				if (tk->type != Token::SYM_COMMA) {
					THROW_UNEXP_TOKEN("symbol \",\"");
				}
			} break;

			default: {
				THROW_UNEXP_TOKEN("an identifier");
			} break;
		}
	}
}

stdvec<ptr<Parser::VarNode>> Parser::_parse_var(ptr<Node> p_node) {
	ASSERT(tokenizer->peek(-1).type == Token::KWORD_VAR);
	ASSERT(p_node != nullptr);
	ASSERT(p_node->type == Node::Type::FILE || p_node->type == Node::Type::BLOCK || p_node->type == Node::Type::CLASS);

	const TokenData* tk;
	stdvec<ptr<VarNode>> vars;

	while (true) {
		tk = &tokenizer->next();
		if (tk->type != Token::IDENTIFIER) {
			THROW_UNEXP_TOKEN("an identifier");
		}
		ptr<VarNode> var_node = new_node<VarNode>();
		var_node->name = tk->identifier;

		tk = &tokenizer->next();
		if (tk->type == Token::OP_EQ) {
			ptr<Node> expr = _parse_expression(p_node);
			//_reduce_expression(expr); TODO: reduce after all are parsed.
			var_node->assignment = expr;

			tk = &tokenizer->next();
			if (tk->type == Token::SYM_COMMA) {
			} else if (tk->type == Token::SYM_SEMI_COLLON) {
				vars.push_back(var_node);
				break;
			} else {
				THROW_UNEXP_TOKEN("");
			}
		} else if (tk->type == Token::SYM_COMMA) {
		} else if (tk->type == Token::SYM_SEMI_COLLON) {
			vars.push_back(var_node);
			break;
		} else {
			THROW_UNEXP_TOKEN("");
		}
		vars.push_back(var_node);
	}

	return vars;
}

ptr<Parser::FunctionNode> Parser::_parse_func(ptr<Node> p_parent) {
	ASSERT(tokenizer->peek(-1).type == Token::KWORD_FUNC);
	ptr<FunctionNode> func_node = new_node<FunctionNode>();
	
	if (tokenizer->peek(-2, true).type == Token::KWORD_STATIC) {
		// TODO: static keyword must only be found in class.
		func_node->is_static = true;
	}

	const TokenData* tk = &tokenizer->next();
	if (tk->type != Token::IDENTIFIER) {
		THROW_UNEXP_TOKEN("an identifier");
	}

	func_node->name = tk->identifier;

	// TODO: arguments

	if (tokenizer->next().type != Token::BRACKET_LCUR) {
		THROW_UNEXP_TOKEN("symbol \"{\"");
	}

	ptr<BlockNode> body = _parse_block(func_node);
	if (tokenizer->peek(-1).type != Token::BRACKET_RCUR) {
		THROW_UNEXP_TOKEN("symbol \"}\"");
	}
	func_node->body = body;
	return func_node;
}

}