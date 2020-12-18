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

#include "analyzer.h"
#include "carbon_function.h"

namespace carbon {

CompileTimeError Analyzer::_analyzer_error(Error::Type p_type, const String& p_msg, Vect2i p_pos, const DBGSourceInfo& p_dbg_info) const {
	uint32_t err_len = 1;
	String token_str = "";
	if (p_pos.x > 0 && p_pos.y > 0) token_str = parser->tokenizer->get_token_at(p_pos).to_string();
	else token_str = parser->tokenizer->peek(-1, true).to_string();
	if (token_str.size() > 1 && token_str[0] == '<' && token_str[token_str.size() - 1] == '>') err_len = 1;
	else err_len = (uint32_t)token_str.size();

	Vect2i pos = (p_pos.x > 0 && p_pos.y > 0) ? p_pos : parser->tokenizer->peek(-1, true).get_pos();
	return CompileTimeError(p_type, p_msg, DBGSourceInfo(file_node->path, file_node->source,
		std::pair<int, int>((int)pos.x, (int)pos.y), err_len), p_dbg_info);
}

void Analyzer::_analyzer_warning(Warning::Type p_type, const String& p_msg, Vect2i p_pos, const DBGSourceInfo& p_dbg_info) {
	uint32_t err_len = 1;
	String token_str = "";
	if (p_pos.x > 0 && p_pos.y > 0) token_str = parser->tokenizer->get_token_at(p_pos).to_string();
	else token_str = parser->tokenizer->peek(-1, true).to_string();
	if (token_str.size() > 1 && token_str[0] == '<' && token_str[token_str.size() - 1] == '>') err_len = 1;
	else err_len = (uint32_t)token_str.size();

	Vect2i pos = (p_pos.x > 0 && p_pos.y > 0) ? p_pos : parser->tokenizer->peek(-1, true).get_pos();
	Warning warning(p_type, p_msg,
		DBGSourceInfo(file_node->path, file_node->source,
			std::pair<int, int>((int)pos.x, (int)pos.y), err_len), p_dbg_info);
	
	warnings.push_back(warning);
}

const stdvec<Warning>& Analyzer::get_warnings() const {
	return warnings;
}

void Analyzer::analyze(ptr<Parser> p_parser) {

	parser = p_parser;
	file_node = parser->file_node;

	parser->parser_context.current_class = nullptr;
	parser->parser_context.current_func = nullptr;
	parser->parser_context.current_block = nullptr;
	parser->parser_context.current_enum = nullptr;

	for (int i = 0; i < (int)file_node->classes.size(); i++) {
		_resolve_inheritance(file_node->classes[i].get());
	}

	// File/class level constants.
	for (size_t i = 0; i < file_node->constants.size(); i++) {
		parser->parser_context.current_const = file_node->constants[i].get();
		_resolve_constant(file_node->constants[i].get());
	}
	for (size_t i = 0; i < file_node->classes.size(); i++) {
		parser->parser_context.current_class = file_node->classes[i].get();
		for (size_t j = 0; j < file_node->classes[i]->constants.size(); j++) {
				parser->parser_context.current_const = file_node->classes[i]->constants[j].get();
				_resolve_constant(file_node->classes[i]->constants[j].get());
		}
	}
	parser->parser_context.current_const = nullptr;
	parser->parser_context.current_class = nullptr;

	// File/class enums/unnamed_enums.
	for (size_t i = 0; i < file_node->enums.size(); i++) {
		parser->parser_context.current_enum = file_node->enums[i].get();
		int _possible_value = 0;
		for (std::pair<String, Parser::EnumValueNode> pair : file_node->enums[i]->values) {
			_resolve_enumvalue(file_node->enums[i]->values[pair.first], &_possible_value);
		}
	}
	if (file_node->unnamed_enum != nullptr) {
		parser->parser_context.current_enum = file_node->unnamed_enum.get();
		int _possible_value = 0;
		for (std::pair<String, Parser::EnumValueNode> pair : file_node->unnamed_enum->values) {
			_resolve_enumvalue(file_node->unnamed_enum->values[pair.first], &_possible_value);
		}
	}
	parser->parser_context.current_enum = nullptr;

	for (size_t i = 0; i < file_node->classes.size(); i++) {
		parser->parser_context.current_class = file_node->classes[i].get();
		for (size_t j = 0; j < file_node->classes[i]->enums.size(); j++) {
			parser->parser_context.current_enum = file_node->classes[i]->enums[j].get();
			int _possible_value = 0;
			for (std::pair<String, Parser::EnumValueNode> pair : file_node->classes[i]->enums[j]->values) {
				_resolve_enumvalue(file_node->classes[i]->enums[j]->values[pair.first], &_possible_value);
			}
		}
		if (file_node->classes[i]->unnamed_enum != nullptr) {
			parser->parser_context.current_enum = file_node->classes[i]->unnamed_enum.get();
			int _possible_value = 0;
			for (std::pair<String, Parser::EnumValueNode> pair : file_node->classes[i]->unnamed_enum->values) {
				_resolve_enumvalue(file_node->classes[i]->unnamed_enum->values[pair.first], &_possible_value);
			}
		}
	}
	parser->parser_context.current_class = nullptr;
	parser->parser_context.current_enum = nullptr;

	// call compile time functions.
	for (auto& func : file_node->compiletime_functions) _resolve_compiletime_funcs(func);
	for (size_t i = 0; i < file_node->classes.size(); i++) {
		parser->parser_context.current_class = file_node->classes[i].get();
		for (auto& func : file_node->classes[i]->compiletime_functions)
			_resolve_compiletime_funcs(func);
	}
	parser->parser_context.current_class = nullptr;

	// File/class level variables.
	for (size_t i = 0; i < file_node->vars.size(); i++) {
		if (file_node->vars[i]->assignment != nullptr) {
			parser->parser_context.current_var = file_node->vars[i].get();
			_reduce_expression(file_node->vars[i]->assignment);
		}
	}
	for (size_t i = 0; i < file_node->classes.size(); i++) {
		parser->parser_context.current_class = file_node->classes[i].get();
		for (size_t j = 0; j < file_node->classes[i]->vars.size(); j++) {
			if (file_node->classes[i]->vars[j]->assignment != nullptr) {
				parser->parser_context.current_var = file_node->classes[i]->vars[j].get();
				_reduce_expression(file_node->classes[i]->vars[j]->assignment);
			}
		}
	}
	parser->parser_context.current_var = nullptr;
	parser->parser_context.current_class = nullptr;

	// resolve parameters.
	for (size_t i = 0; i < file_node->functions.size(); i++) {
		Parser::FunctionNode* fn = file_node->functions[i].get();
		_resolve_parameters(fn);
	}

	for (size_t i = 0; i < file_node->classes.size(); i++) {
		parser->parser_context.current_class = file_node->classes[i].get();
		for (size_t j = 0; j < file_node->classes[i]->functions.size(); j++) {
			Parser::FunctionNode* fn = file_node->classes[i]->functions[j].get();
			_resolve_parameters(fn);
		}
		parser->parser_context.current_class = nullptr;
	}

	// file level function.
	for (size_t i = 0; i < file_node->functions.size(); i++) {
		parser->parser_context.current_func = file_node->functions[i].get();
		Parser::FunctionNode* fn = file_node->functions[i].get();

		if (fn->name == GlobalStrings::main) {
			if (fn->args.size() >= 2) throw ANALYZER_ERROR(Error::INVALID_ARG_COUNT, "main function takes at most 1 argument.", fn->pos);
		}

		_reduce_block(file_node->functions[i]->body);
	}
	parser->parser_context.current_func = nullptr;

	// class function.
	for (size_t i = 0; i < file_node->classes.size(); i++) {
		parser->parser_context.current_class = file_node->classes[i].get();
		for (size_t j = 0; j < file_node->classes[i]->functions.size(); j++) {
			parser->parser_context.current_func = file_node->classes[i]->functions[j].get();
			_reduce_block(file_node->classes[i]->functions[j]->body);
		}

		// add default constructor
		Parser::ClassNode* cls = file_node->classes[i].get();
		if (!cls->has_super_ctor_call && cls->base_type != Parser::ClassNode::NO_BASE) {

			bool can_add_default_ctor = true;
			switch (cls->base_type) {
				case Parser::ClassNode::BASE_LOCAL: {
					const Parser::FunctionNode* ctor = cls->base_class->constructor;
					if (ctor && ctor->args.size() - ctor->default_args.size() != 0) can_add_default_ctor = false;
				} break;
				case Parser::ClassNode::BASE_NATIVE: {
					const StaticFuncBind* ctor = NativeClasses::singleton()->get_constructor(cls->base_class_name);
					if (ctor && ctor->get_method_info()->get_arg_count()-1/*self*/ - ctor->get_method_info()->get_default_arg_count() != 0)
						can_add_default_ctor = false;
				} break;
				case Parser::ClassNode::BASE_EXTERN: {
					const CarbonFunction* ctor = cls->base_binary->get_constructor();
					if (ctor && ctor->get_arg_count() - ctor->get_default_args().size() != 0) can_add_default_ctor = false;
				} break;
			}
			if (!can_add_default_ctor)
				throw ANALYZER_ERROR(Error::TYPE_ERROR, "super constructor call needed since base class doesn't have a default constructor.", cls->pos);

			Parser::FunctionNode* fn = cls->constructor;
			if (fn == nullptr) {
				ptr<Parser::FunctionNode> new_fn = newptr<Parser::FunctionNode>();
				new_fn->name = cls->name;
				new_fn->is_reduced = true;
				new_fn->parent_node = cls;
				new_fn->pos = cls->pos;
				new_fn->body = newptr<Parser::BlockNode>();
				cls->functions.push_back(new_fn);
				cls->constructor = new_fn.get();
				fn = new_fn.get();
			}
			ptr<Parser::CallNode> super_call = newptr<Parser::CallNode>(); super_call->pos = cls->pos;
			super_call->base = newptr<Parser::SuperNode>(); super_call->base->pos = cls->pos;
			fn->body->statements.insert(fn->body->statements.begin(), super_call);
		}
	}
	parser->parser_context.current_class = nullptr;
	parser->parser_context.current_func = nullptr;
}

var Analyzer::_call_compiletime_func(Parser::BuiltinFunctionNode* p_func, stdvec<var*>& args) {
	switch (p_func->func) {
		case BuiltinFunctions::__ASSERT: {
			if (args.size() != 1) throw ANALYZER_ERROR(Error::INVALID_ARG_COUNT, "expected exactly 1 argument.", p_func->pos);
			if (!args[0]->operator bool()) throw ANALYZER_ERROR(Error::ASSERTION, "assertion failed.", p_func->pos);
		} break;
		case BuiltinFunctions::__FUNC: {
			if (!parser->parser_context.current_func) throw ANALYZER_ERROR(Error::SYNTAX_ERROR, "__func() must be called inside a function.", p_func->pos);
			if (parser->parser_context.current_class) return parser->parser_context.current_class->name + "." + parser->parser_context.current_func->name;
			else  return parser->parser_context.current_func->name;
		} break;
		case BuiltinFunctions::__LINE: {
			return p_func->pos.x;
		} break;
		case BuiltinFunctions::__FILE: {
			return parser->file_node->path;
		} break;
		default:
			ASSERT(false);
	}
	return var();
}

void Analyzer::_resolve_compiletime_funcs(const ptr<Parser::CallNode>& p_func) {
	Parser::CallNode* call = p_func.get();
	ASSERT(call->is_compilttime);
	ASSERT(call->base->type == Parser::Node::Type::BUILTIN_FUNCTION);
	Parser::BuiltinFunctionNode* bf = ptrcast<Parser::BuiltinFunctionNode>(call->base).get();
	stdvec<var*> args;
	for (int j = 0; j < (int)call->args.size(); j++) {
		_reduce_expression(call->args[j]);
		if (call->args[j]->type != Parser::Node::Type::CONST_VALUE) {
			throw ANALYZER_ERROR(Error::TYPE_ERROR, String::format("compiletime function arguments must be compile time known values."), p_func->args[j]->pos);
		}
		args.push_back(&ptrcast<Parser::ConstValueNode>(call->args[j])->value);
	}
	_call_compiletime_func(bf, args);
}

void Analyzer::_check_member_var_shadow(void* p_base, Parser::ClassNode::BaseType p_base_type, stdvec<ptr<Parser::VarNode>>& p_vars) {
	switch (p_base_type) {
		case Parser::ClassNode::NO_BASE: // can't be
			return;
		case Parser::ClassNode::BASE_NATIVE: {
			String* base = (String*)p_base;
			for (const ptr<Parser::VarNode>& v : p_vars) {
				ptr<MemberInfo> mi = NativeClasses::singleton()->get_member_info(*base, v->name);
				if (mi == nullptr) continue;
				if (mi->get_type() == MemberInfo::PROPERTY) {
					const PropertyInfo* pi = static_cast<const PropertyInfo*>(mi.get());
					if (!pi->is_static()) throw ANALYZER_ERROR(Error::ATTRIBUTE_ERROR,
						String::format("member named \"%s\" already exists in base \"%s\"", v->name.c_str(), base->c_str()), v->pos);
				}
			}
			String parent = NativeClasses::singleton()->get_inheritance(*base);
			if (parent != "") _check_member_var_shadow((void*)&parent, Parser::ClassNode::BASE_NATIVE, p_vars);
		} break;
		case Parser::ClassNode::BASE_EXTERN: {
			Bytecode* base = (Bytecode*)p_base;
			for (const ptr<Parser::VarNode>& v : p_vars) {
				const ptr<MemberInfo> mi = base->get_member_info(v->name);
				if (mi == nullptr) continue;
				if (mi->get_type() == MemberInfo::PROPERTY) {
					const PropertyInfo* pi = static_cast<const PropertyInfo*>(mi.get());
					if (!pi->is_static()) throw ANALYZER_ERROR(Error::ATTRIBUTE_ERROR,
						String::format("member named \"%s\" already exists in base \"%s\"", v->name.c_str(), base->get_name().c_str()), v->pos);
				}
			}
			if (base->has_base()) {
				if (base->is_base_native()) _check_member_var_shadow((void*)&base->get_base_native(), Parser::ClassNode::BASE_NATIVE, p_vars);
				else _check_member_var_shadow(base->get_base_binary().get(), Parser::ClassNode::BASE_EXTERN, p_vars);
			}
		} break;
		case Parser::ClassNode::BASE_LOCAL: {
			Parser::ClassNode* base = (Parser::ClassNode*)p_base;
			for (const ptr<Parser::VarNode>& v : p_vars) {
				if (v->is_static) continue;
				for (const ptr<Parser::VarNode>& _v : base->vars) {
					if (_v->is_static) continue;
					if (_v->name == v->name) throw ANALYZER_ERROR(Error::ATTRIBUTE_ERROR,
						String::format("member named \"%s\" already exists in base \"%s\"", v->name.c_str(), base->name.c_str()), v->pos);
				}
			}
			if (base->base_type == Parser::ClassNode::BASE_LOCAL) _check_member_var_shadow((void*)base->base_class, Parser::ClassNode::BASE_LOCAL, p_vars);
			else if (base->base_type == Parser::ClassNode::BASE_EXTERN) _check_member_var_shadow((void*)base->base_binary.get(), Parser::ClassNode::BASE_EXTERN, p_vars);
			else if (base->base_type == Parser::ClassNode::BASE_NATIVE) _check_member_var_shadow((void*)&base->base_class_name, Parser::ClassNode::BASE_NATIVE, p_vars);
		} break;

	}
}

void Analyzer::_resolve_inheritance(Parser::ClassNode* p_class) {

	if (p_class->is_reduced) return;
	if (p_class->_is_reducing) throw ANALYZER_ERROR(Error::TYPE_ERROR, "cyclic inheritance. class inherits itself isn't allowed.", p_class->pos);
	p_class->_is_reducing = true;

	// resolve inheritance.
	if (p_class->base_type == Parser::ClassNode::BASE_LOCAL) {
		bool found = false;
		for (int i = 0; i < (int)file_node->classes.size(); i++) {
			if (p_class->base_class_name == file_node->classes[i]->name) {
				found = true;
				_resolve_inheritance(file_node->classes[i].get());
				p_class->base_class = file_node->classes[i].get();
			}
		}
		if (!found) throw ANALYZER_ERROR(Error::TYPE_ERROR, String::format("base class \"%s\" not found.", p_class->base_class_name.c_str()), p_class->pos);
	}

	// check if a member is already exists in the parent class.
	if (p_class->base_type == Parser::ClassNode::BASE_LOCAL) _check_member_var_shadow((void*)p_class->base_class, Parser::ClassNode::BASE_LOCAL, p_class->vars);
	else if (p_class->base_type == Parser::ClassNode::BASE_EXTERN) _check_member_var_shadow((void*)p_class->base_binary.get(), Parser::ClassNode::BASE_EXTERN, p_class->vars);
	else if (p_class->base_type == Parser::ClassNode::BASE_NATIVE) _check_member_var_shadow((void*)&p_class->base_class_name, Parser::ClassNode::BASE_NATIVE, p_class->vars);

	p_class->_is_reducing = false;
	p_class->is_reduced = true;
}

void Analyzer::_resolve_constant(Parser::ConstNode* p_const) {
	if (p_const->is_reduced) return;
	if (p_const->_is_reducing) throw ANALYZER_ERROR(Error::TYPE_ERROR, "cyclic constant value dependancy found.", p_const->pos);
	p_const->_is_reducing = true;

	ASSERT(p_const->assignment != nullptr);
	_reduce_expression(p_const->assignment);

	if (p_const->assignment->type == Parser::Node::Type::CONST_VALUE) {
		ptr<Parser::ConstValueNode> cv = ptrcast<Parser::ConstValueNode>(p_const->assignment);
		if (cv->value.get_type() != var::INT && cv->value.get_type() != var::FLOAT &&
			cv->value.get_type() != var::BOOL && cv->value.get_type() != var::STRING &&
			cv->value.get_type() != var::_NULL) {
			throw ANALYZER_ERROR(Error::TYPE_ERROR, "expected a constant expression.", p_const->assignment->pos);
		}
		p_const->value = cv->value;
		p_const->_is_reducing = false;
		p_const->is_reduced = true;

	} else throw ANALYZER_ERROR(Error::TYPE_ERROR, "expected a contant expression.", p_const->assignment->pos);
}

void Analyzer::_resolve_parameters(Parser::FunctionNode* p_func) {
	for (int i = 0; i < p_func->args.size(); i++) {
		if (p_func->args[i].default_value != nullptr) {
			_reduce_expression(p_func->args[i].default_value);
			if (p_func->args[i].default_value->type != Parser::Node::Type::CONST_VALUE) 
				throw ANALYZER_ERROR(Error::TYPE_ERROR, "expected a contant expression.", p_func->args[i].default_value->pos);
			ptr<Parser::ConstValueNode> cv = ptrcast<Parser::ConstValueNode>(p_func->args[i].default_value);
			if (cv->value.get_type() != var::INT && cv->value.get_type() != var::FLOAT &&
				cv->value.get_type() != var::BOOL && cv->value.get_type() != var::STRING &&
				cv->value.get_type() != var::_NULL) {
				throw ANALYZER_ERROR(Error::TYPE_ERROR, "expected a constant expression.", p_func->args[i].default_value->pos);
			}
			p_func->default_args.push_back(cv->value);
		}
	}
}

void Analyzer::_resolve_enumvalue(Parser::EnumValueNode& p_enumvalue, int* p_possible) {
	if (p_enumvalue.is_reduced) return;
	if (p_enumvalue._is_reducing) throw ANALYZER_ERROR(Error::TYPE_ERROR, "cyclic enum value dependancy found.", p_enumvalue.expr->pos);
	p_enumvalue._is_reducing = true;

	if (p_enumvalue.expr != nullptr) {
		_reduce_expression(p_enumvalue.expr);
		if (p_enumvalue.expr->type != Parser::Node::Type::CONST_VALUE)
			throw ANALYZER_ERROR(Error::TYPE_ERROR, "enum value must be a constant integer.", p_enumvalue.expr->pos);
		ptr<Parser::ConstValueNode> cv = ptrcast<Parser::ConstValueNode>(p_enumvalue.expr);
		if (cv->value.get_type() != var::INT) throw ANALYZER_ERROR(Error::TYPE_ERROR, "enum value must be a constant integer.", p_enumvalue.expr->pos);
		p_enumvalue.value = cv->value;
	} else {
		p_enumvalue.value = (p_possible) ? *p_possible: -1;
	}
	if (p_possible) *p_possible = (int)p_enumvalue.value + 1;

	p_enumvalue._is_reducing = false;
	p_enumvalue.is_reduced = true;
}

void Analyzer::_check_super_constructor_call(const Parser::BlockNode* p_block) {
	int constructor_argc = 0;
	int default_argc = 0;

	Parser::ClassNode* current_class = parser->parser_context.current_class;
	switch (parser->parser_context.current_class->base_type) {
		case Parser::ClassNode::BASE_LOCAL: {
			Parser::FunctionNode* constructor = current_class->base_class->constructor;
			if (constructor == nullptr) return;
			constructor_argc = (int)constructor->args.size();
			default_argc = (int)constructor->default_args.size();
		} break;
		case Parser::ClassNode::BASE_EXTERN: {
			const CarbonFunction* constructor = current_class->base_binary->get_constructor();
			if (constructor == nullptr) return;
			constructor_argc = (int)constructor->get_arg_count();
			default_argc = (int)constructor->get_default_args().size();
		} break;
		case Parser::ClassNode::BASE_NATIVE: {
			const StaticFuncBind* constructor = NativeClasses::singleton()->get_constructor(current_class->base_class_name);
			constructor_argc = (constructor) ? constructor->get_argc() : 0;
			default_argc = (constructor) ? constructor->get_method_info()->get_default_arg_count() : 0;
		} break;
	}

	if (constructor_argc - default_argc > 0) { // super call needed.
		if ((p_block->statements.size() == 0) || (p_block->statements[0]->type != Parser::Node::Type::CALL))
			throw ANALYZER_ERROR(Error::NOT_IMPLEMENTED, "super constructor call expected since base class doesn't have a default constructor.", p_block->pos);
		const Parser::CallNode* call = static_cast<const Parser::CallNode*>(p_block->statements[0].get());
		if (call->base->type != Parser::Node::Type::SUPER || call->method != nullptr)
			throw ANALYZER_ERROR(Error::NOT_IMPLEMENTED, "super constructor call expected since base class doesn't have a default constructor.", call->pos);
		current_class->has_super_ctor_call = true;
		_check_arg_count(constructor_argc, default_argc, (int)call->args.size(), call->pos);
	}

	if ((p_block->statements.size() > 0) && (p_block->statements[0]->type == Parser::Node::Type::CALL)) {
		const Parser::CallNode* call = static_cast<const Parser::CallNode*>(p_block->statements[0].get());
		if (call->base->type == Parser::Node::Type::SUPER && call->method == nullptr) current_class->has_super_ctor_call = true;
	}
}

void Analyzer::_check_arg_count(int p_argc, int p_default_argc, int p_args_given, Vect2i p_err_pos) {
	if (p_argc == -1 /*va args*/) return;

	// TODO: error message 
	int required_min_argc = p_argc - p_default_argc;
	if (required_min_argc > 0) {
		if (p_default_argc == 0) {
			if (p_args_given != p_argc) throw ANALYZER_ERROR(Error::INVALID_ARG_COUNT,
				String::format("expected excatly %i argument(s) for super constructor call", p_argc), p_err_pos);
		} else {
			if (p_args_given < required_min_argc) throw ANALYZER_ERROR(Error::INVALID_ARG_COUNT,
				String::format("expected at least %i argument(s) for super constructor call", required_min_argc), p_err_pos);
			else if (p_args_given > p_argc) throw ANALYZER_ERROR(Error::INVALID_ARG_COUNT,
				String::format("expected at most %i argument(s) for super constructor call", p_argc), p_err_pos);
		}
		// no argument is required -> check if argc exceeding
	} else if (p_args_given > p_argc) {
		throw ANALYZER_ERROR(Error::INVALID_ARG_COUNT,
			String::format("expected at most %i argument(s) for super constructor call", p_argc), p_err_pos);
	}
}

}