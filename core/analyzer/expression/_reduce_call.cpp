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

#include "../analyzer.h"

namespace carbon {


#define GET_ARGS(m_nodes)                                                             \
	stdvec<var*> args;                                                                \
	for (int i = 0; i < (int)m_nodes.size(); i++) {                                   \
	    args.push_back(&ptrcast<Parser::ConstValueNode>(m_nodes[i])->value);          \
	}

#define SET_EXPR_CONST_NODE(m_var, m_pos)                                             \
do {                                                                                  \
	ptr<Parser::ConstValueNode> cv = new_node<Parser::ConstValueNode>(m_var);         \
	cv->pos = m_pos, p_expr = cv;                                                     \
} while (false)


void Analyzer::_reduce_call(ptr<Parser::Node>& p_expr) {
	ASSERT(p_expr->type == Parser::Node::Type::CALL);

	ptr<Parser::CallNode> call = ptrcast<Parser::CallNode>(p_expr);

	// reduce arguments.
	bool all_const = true;
	for (int i = 0; i < (int)call->r_args.size(); i++) {
		_reduce_expression(call->r_args[i]);
		if (call->r_args[i]->type != Parser::Node::Type::CONST_VALUE) {
			all_const = false;
		}
	}

	// reduce base.
	if (call->base->type == Parser::Node::Type::BUILTIN_FUNCTION || call->base->type == Parser::Node::Type::BUILTIN_TYPE) {
		// don't_reduce_anything();
	} else {
		if (call->base->type == Parser::Node::Type::UNKNOWN) {
			_reduce_expression(call->method);
			if (call->method->type == Parser::Node::Type::CONST_VALUE)
				THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("constant value is not callable."), call->pos);
			ASSERT(call->method->type == Parser::Node::Type::IDENTIFIER);
		} else {
			_reduce_expression(call->base);
		}
	}

	switch (call->base->type) {

		// print.a_method(); print(); call base is builtin function.
		case Parser::Node::Type::BUILTIN_FUNCTION: {

			if (call->method == nullptr) { // print();
				if (all_const) {
					ptr<Parser::BuiltinFunctionNode> bf = ptrcast<Parser::BuiltinFunctionNode>(call->base);
					if (BuiltinFunctions::can_const_fold(bf->func)) {
						GET_ARGS(call->r_args);
						if (BuiltinFunctions::is_compiletime(bf->func)) {
							var ret = _call_compiletime_func(bf.get(), args);
							SET_EXPR_CONST_NODE(ret, call->pos);
						} else {
							try {
								var ret;
								BuiltinFunctions::call(bf->func, args, ret);
								SET_EXPR_CONST_NODE(ret, call->pos);
							} catch (Error& err) {
								THROW_ANALYZER_ERROR(err.get_type(), err.get_msg(), call->pos);
							}
						}
					}
				}
			} else { // print.a_method();
				// TODO: check method exists, if (all_const) reduce();
			}
			
		} break;

		// String(); String.format(...); method call on base built in type
		case Parser::Node::Type::BUILTIN_TYPE: {
			if (call->method == nullptr) { // String(...); constructor.
				if (all_const) {
					ptr<Parser::BuiltinTypeNode> bt = ptrcast<Parser::BuiltinTypeNode>(call->base);
					try {
						GET_ARGS(call->r_args);
						var ret = BuiltinTypes::construct(bt->builtin_type, args);
						SET_EXPR_CONST_NODE(ret, call->pos);
					} catch (Error& err) {
						THROW_ANALYZER_ERROR(err.get_type(), err.get_msg(), call->base->pos);
					}
				}
			} else { // String.format(); static func call.
				// TODO: check if exists, reduce if compile time callable.
			}

		} break;

		// method call on base const value.
		case Parser::Node::Type::CONST_VALUE: {
			if (all_const) {
				try {
					ASSERT(call->method->type == Parser::Node::Type::IDENTIFIER);
					GET_ARGS(call->r_args); // 0 : const value, 1: name, ... args.
					var ret = ptrcast<Parser::ConstValueNode>(call->base)->value.call_method(ptrcast<Parser::IdentifierNode>(call->method)->name, args);
					SET_EXPR_CONST_NODE(ret, call->pos);
				} catch (const VarError& verr) {
					THROW_ANALYZER_ERROR(Error(verr).get_type(), verr.what(), call->method->pos);
				}
			}
		} break;

		// call base is unknown. search method from this to super, or static function.
		case Parser::Node::Type::UNKNOWN: {

			Parser::IdentifierNode* id = ptrcast<Parser::IdentifierNode>(call->method).get();
			switch (id->ref) {

				// a_var(); call `__call` method on the variable.
				case Parser::IdentifierNode::REF_PARAMETER:
				case Parser::IdentifierNode::REF_LOCAL_VAR:
				case Parser::IdentifierNode::REF_MEMBER_VAR: {
					call->base = call->method; // param(args...); -> will call param.__call(args...);
					call->method = nullptr;
				} break;

				// f(); calling a local carbon function.
				case Parser::IdentifierNode::REF_FUNCTION: {

					bool is_illegal_call = false;
					switch (id->ref_base) {
						case Parser::IdentifierNode::BASE_UNKNOWN:
						case Parser::IdentifierNode::BASE_EXTERN:
						case Parser::IdentifierNode::BASE_NATIVE:
							THROW_BUG("can't be"); // call base is empty.
						case Parser::IdentifierNode::BASE_LOCAL: {
							is_illegal_call = parser->parser_context.current_class && !id->_func->is_static;
						} break;
					}

					if (is_illegal_call) { // can't call a non-static function.
						if ((parser->parser_context.current_func && parser->parser_context.current_func->is_static) ||
							(parser->parser_context.current_var && parser->parser_context.current_var->is_static)) {
							THROW_ANALYZER_ERROR(Error::ATTRIBUTE_ERROR, String::format("can't access non-static attribute \"%s\" statically", id->name.c_str()), id->pos);
						}
					}

					int argc = (int)id->_func->args.size();
					int argc_default = (int)id->_func->default_args.size();
					int argc_given = (int)call->r_args.size();
					if (argc_given + argc_default < argc) {
						if (argc_default == 0) THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected exactly %i argument(s).", argc), id->pos);
						else THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected at least %i argument(s).", argc - argc_default), id->pos);
					} else if (argc_given > argc) {
						if (argc_default == 0) THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected exactly %i argument(s).", argc), id->pos);
						else THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected minimum of %i argument(s) and maximum of %i argument(s).", argc - argc_default, argc), id->pos);
					}
				} break;

				// Aclass(...); calling carbon class constructor.
				case Parser::IdentifierNode::REF_CARBON_CLASS: {
					if (id->_class->constructor) {
						int argc = (int)id->_class->constructor->args.size();
						int argc_default = (int)id->_class->constructor->default_args.size();
						int argc_given = (int)call->r_args.size();
						if (argc_given + argc_default < argc) {
							if (argc_default == 0) THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected exactly %i argument(s).", argc), id->pos);
							else THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected at least %i argument(s).", argc - argc_default), id->pos);
						} else if (argc_given > argc) {
							if (argc_default == 0) THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected exactly %i argument(s).", argc), id->pos);
							else THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected minimum of %i argument(s) and maximum of %i argument(s).", argc - argc_default, argc), id->pos);
						}
					} else {
						if (call->r_args.size() != 0)
							THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, "default constructor takes exactly 0 argument.", id->pos);
					}
				} break;

				// File(...); calling a native class constructor.
				case Parser::IdentifierNode::REF_NATIVE_CLASS: {
					ASSERT(NativeClasses::singleton()->is_class_registered(id->name));
					const StaticFuncBind* initializer = NativeClasses::singleton()->get_initializer(id->name);
					if (initializer) {
						// check arg counts.
						int argc = initializer->get_method_info()->get_arg_count() - 1; // -1 for self argument.
						int argc_default = initializer->get_method_info()->get_default_arg_count();
						int argc_given = (int)call->r_args.size();
						if (argc_given + argc_default < argc) {
							if (argc_default == 0) THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected exactly %i argument(s).", argc), id->pos);
							else THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected at least %i argument(s).", argc - argc_default), id->pos);
						} else if (argc_given > argc) {
							if (argc_default == 0) THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected exactly %i argument(s).", argc), id->pos);
							else THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected minimum of %i argument(s) and maximum of %i argument(s).", argc - argc_default, argc), id->pos);
						}
						// check arg types.
						const stdvec<VarTypeInfo>& arg_types = initializer->get_method_info()->get_arg_types();
						for (int i = 0; i < (int)call->r_args.size(); i++) {
							if (call->r_args[i]->type == Parser::Node::Type::CONST_VALUE) {
								var value = ptrcast<Parser::ConstValueNode>(call->r_args[i])->value;
								if (value.get_type() != arg_types[i + 1].type) // +1 for skip self argument.
									THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("expected type \"%s\" at argument %i.", var::get_type_name_s(arg_types[i].type), i), call->r_args[i]->pos);
							}
						}
					} else {
						if (call->r_args.size() != 0)
							THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, "default constructor takes exactly 0 argument.", id->pos);
					}
				} break;

				// invalid callables.
				//case Parser::IdentifierNode::REF_ENUM_NAME:
				//case Parser::IdentifierNode::REF_ENUM_VALUE:
				//case Parser::IdentifierNode::REF_FILE:
				//case Parser::IdentifierNode::REF_LOCAL_CONST:
				//case Parser::IdentifierNode::REF_MEMBER_CONST:
				default: {
					THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("attribute \"%s\" is not callable.", id->name.c_str()), id->pos);
				}
			}

		} break;

		// this.method(); super.method();
		case Parser::Node::Type::THIS:
		case Parser::Node::Type::SUPER: {
			Parser::ClassNode* _class = nullptr;
			bool base_super = false;
			if (call->base->type == Parser::Node::Type::THIS) {
				_class = parser->parser_context.current_class;
			} else {
				base_super = true;
				switch (parser->parser_context.current_class->base_type) {
					case Parser::ClassNode::NO_BASE: ASSERT(false);
					case Parser::ClassNode::BASE_LOCAL: {
						_class = parser->parser_context.current_class->base_class;
					} break;
					case Parser::ClassNode::BASE_EXTERN: ASSERT(false); // TODO:
					case Parser::ClassNode::BASE_NATIVE: ASSERT(false); // TODO:
				}
			}

			// TODO: move this, this is only `this` and base is local.
			const String& method_name = ptrcast<Parser::IdentifierNode>(call->method)->name;
			Parser::IdentifierNode _id = _find_member(_class, method_name); _id.pos = call->method->pos;
			switch (_id.ref) {
				case Parser::IdentifierNode::REF_UNKNOWN:
					THROW_ANALYZER_ERROR(Error::ATTRIBUTE_ERROR, String::format("attribute \"%s\" isn't exists in base \"%s\".", method_name.c_str(), _class->name.c_str()), call->pos);
				case Parser::IdentifierNode::REF_PARAMETER:
				case Parser::IdentifierNode::REF_LOCAL_VAR:
				case Parser::IdentifierNode::REF_LOCAL_CONST:
					THROW_BUG("can't be.");

				// this.a_var();
				case Parser::IdentifierNode::REF_MEMBER_VAR: {
					if (base_super) // super.a_var(); is illegal
						THROW_ANALYZER_ERROR(Error::ATTRIBUTE_ERROR, String::format("attribute \"%s\" cannot be access with with \"super\" use \"this\" instead.", method_name.c_str()), call->pos);
					call->base = call->method;
					call->method = nullptr;
				} break;

				// this.CONST(); inavlid callables.
				case Parser::IdentifierNode::REF_MEMBER_CONST:
				case Parser::IdentifierNode::REF_ENUM_NAME:
				case Parser::IdentifierNode::REF_ENUM_VALUE:
					THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("constant value is not callable.", method_name.c_str()), call->pos);
					break;

				// this.f(); // function call on this, super.
				case Parser::IdentifierNode::REF_FUNCTION: {
					if (parser->parser_context.current_func->is_static && !_id._func->is_static) {
						THROW_ANALYZER_ERROR(Error::ATTRIBUTE_ERROR, String::format("can't access non-static attribute \"%s\" statically", _id.name.c_str()), _id.pos);
					}

					int argc = (int)_id._func->args.size();
					int argc_default = (int)_id._func->default_args.size();

					int argc_given = (int)call->r_args.size();
					if (argc_given + argc_default < argc) {
						if (argc_default == 0) THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected exactly %i argument(s).", argc), call->pos);
						else THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected at least %i argument(s).", argc - argc_default), call->pos);
					} else if (argc_given > argc) {
						if (argc_default == 0) THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected exactly %i argument(s).", argc), call->pos);
						else THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected minimum of %i argument(s) and maximum of %i argument(s).", argc - argc_default, argc), call->pos);
					}
				} break;

				case Parser::IdentifierNode::REF_CARBON_CLASS:
				case Parser::IdentifierNode::REF_NATIVE_CLASS:
				case Parser::IdentifierNode::REF_EXTERN:
					THROW_BUG("can't be");
			}
		} break;

		// base().method(); [o1, o2][1].method(); (x + y).method();
		case Parser::Node::Type::CALL:
		case Parser::Node::Type::INDEX:
		case Parser::Node::Type::ARRAY: // TODO: the method could be validated.
		case Parser::Node::Type::MAP:   // TODO: the method could be validated.
		case Parser::Node::Type::OPERATOR:
			ASSERT(call->method->type == Parser::Node::Type::IDENTIFIER);
			break;


		// idf.method(); method call on base with identifier id.
		case Parser::Node::Type::IDENTIFIER: {
			ASSERT(call->method->type == Parser::Node::Type::IDENTIFIER);
			Parser::IdentifierNode* base = ptrcast<Parser::IdentifierNode>(call->base).get();
			Parser::IdentifierNode* id = ptrcast<Parser::IdentifierNode>(call->method).get();

			switch (base->ref) {

				// IF IDENTIFIER IS UNKNOWN IT'S ALREADY A NAME ERROR BY NOW.
				case Parser::IdentifierNode::REF_UNKNOWN: THROW_BUG("can't be");

				// p_param.method(); a_var.method(); a_member.method();
				case Parser::IdentifierNode::REF_PARAMETER:
				case Parser::IdentifierNode::REF_LOCAL_VAR:
				case Parser::IdentifierNode::REF_MEMBER_VAR: {
				} break;

				// IF AN IDENTIFIER IS REFERENCE TO A CONSTANT IT'LL BE `ConstValueNode` BY NOW.
				case Parser::IdentifierNode::REF_LOCAL_CONST:
				case Parser::IdentifierNode::REF_MEMBER_CONST: {
					THROW_BUG("can't be.");
				} break;

				// Aclass.id();
				case Parser::IdentifierNode::REF_CARBON_CLASS: {

					Parser::IdentifierNode _id = _find_member(base->_class, id->name); _id.pos = id->pos;
					switch (_id.ref) {
						case Parser::IdentifierNode::REF_UNKNOWN:
							THROW_ANALYZER_ERROR(Error::NAME_ERROR, String::format("attribute \"%s\" doesn't exists on base %s", id->name.c_str(), base->_class->name.c_str()), id->pos);

						// Aclass.a_var();
						case Parser::IdentifierNode::REF_MEMBER_VAR: {
							bool _is_member_static = false;
							switch (id->ref_base) {
								case Parser::IdentifierNode::BASE_UNKNOWN: ASSERT(false && "I must forgotten here");
								case Parser::IdentifierNode::BASE_LOCAL:
									_is_member_static = id->_var->is_static; break;
								case Parser::IdentifierNode::BASE_NATIVE:
									_is_member_static = id->_prop_info->is_static(); break;
								case Parser::IdentifierNode::BASE_EXTERN:
									// TODO: extending an extern class.
									break;
							}

							if (_id._var->is_static) {
								break; // Class.var(args...);
							} else {
								THROW_ANALYZER_ERROR(Error::ATTRIBUTE_ERROR, String::format("can't access non-static attribute \"%s\" statically", id->name.c_str()), id->pos);
							}
						} break;

						// Aclass.CONST(args...);
						case Parser::IdentifierNode::REF_LOCAL_CONST:
						case Parser::IdentifierNode::REF_MEMBER_CONST: {
							THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("constant value (\"%s.%s()\") is not callable.", base->name.c_str(), id->name.c_str()), id->pos);
						} break;

						// Aclass.Enum();
						case Parser::IdentifierNode::REF_ENUM_NAME: THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("enums (\"%s.%s()\") are not callable.", base->name.c_str(), id->name.c_str()), id->pos);
						case Parser::IdentifierNode::REF_ENUM_VALUE: THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("enum value (\"%s.%s()\") is not callable.", base->name.c_str(), id->name.c_str()), id->pos);

						case Parser::IdentifierNode::REF_FUNCTION: {
							if (_id._func->is_static) {
								break; // Class.static_func(args...);
							} else {
								THROW_ANALYZER_ERROR(Error::ATTRIBUTE_ERROR, String::format("can't call non-static method\"%s\" statically", id->name.c_str()), id->pos);
							}
						} break;

						// Aclass.Lib();
						case Parser::IdentifierNode::REF_EXTERN: {
							THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("external libraries (\"%s.%s()\") are not callable.", base->name.c_str(), id->name.c_str()), id->pos);
						} break;
					}
				} break;

				// File.method(); base is a native class.
				case Parser::IdentifierNode::REF_NATIVE_CLASS: {

					BindData* bd = NativeClasses::singleton()->get_bind_data(base->name, id->name).get();
					if (!bd) THROW_ANALYZER_ERROR(Error::ATTRIBUTE_ERROR, String::format("attribute \"%s\" does not exists on base %s.", id->name.c_str(), base->name.c_str()), id->pos);
					switch (bd->get_type()) {
						case BindData::STATIC_FUNC: {

							const MemberInfo* memi = bd->get_member_info();
							if (memi->get_type() != MemberInfo::METHOD) THROW_BUG("native member reference mismatch.");
							const MethodInfo* mi = (const MethodInfo*)memi;
							if (!mi->is_static()) THROW_BUG("native method reference mismatch.");

							int argc_given = (int)call->r_args.size();
							int argc = mi->get_arg_count(), argc_default = mi->get_default_arg_count();
							if (argc_given + argc_default < argc) {
								if (argc_default == 0) THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected exactly %i argument(s).", argc), id->pos);
								else THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected at least %i argument(s).", argc - argc_default), id->pos);
							} else if (argc_given > argc) {
								if (argc_default == 0) THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected exactly %i argument(s).", argc), id->pos);
								else THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected minimum of %i argument(s) and maximum of %i argument(s).", argc - argc_default, argc), id->pos);
							}

							for (int i = 0; i < (int)call->r_args.size(); i++) {
								if (call->r_args[i]->type == Parser::Node::Type::CONST_VALUE) {
									if (mi->get_arg_types()[i].type != ptrcast<Parser::ConstValueNode>(call->r_args[i])->value.get_type()) {
										THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("expected type \"%s\" at argument %i.", var::get_type_name_s(mi->get_arg_types()[i].type), i), call->r_args[i]->pos);
									}
								}
							}

							// TODO: check if the static function is ok to call at compile time
							//       ex: read a file at a location, print something... are runtime.
							//if (all_const) {
							//	try {
							//		GET_ARGS(call->r_args);
							//		var ret = ptrcast<StaticFuncBind>(bd)->call(args);
							//		SET_EXPR_CONST_NODE(ret, call->pos);
							//	} catch (VarError& verr) {
							//		THROW_ANALYZER_ERROR(Error(verr).get_type(), verr.what(), call->pos);
							//	}
							//}

						} break;
						case BindData::STATIC_VAR: break; // calls the "__call" at runtime.
						case BindData::STATIC_CONST: THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("constant value \"%s.%s()\" is not callable.", base->name.c_str(), id->name.c_str()), id->pos);
						case BindData::METHOD: THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("can't call non-static method \"%s.%s()\" statically.", base->name.c_str(), id->name.c_str()), id->pos);
						case BindData::MEMBER_VAR:  THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("can't call non-static member \"%s.%s()\" statically.", base->name.c_str(), id->name.c_str()), id->pos);
						case BindData::ENUM:  THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("enums (\"%s.%s()\") are not callable.", base->name.c_str(), id->name.c_str()), id->pos);
						case BindData::ENUM_VALUE: THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("enum value (\"%s.%s()\") are not callable.", base->name.c_str(), id->name.c_str()), id->pos);
					}

				} break;

				// fn.get_default_args(), fn.get_name(), ...
				case Parser::IdentifierNode::REF_FUNCTION: {
					THROW_BUG("TODO:"); // CarbonFunction*
				} break;

				// TODO: EnumType.get_value_count();
				//case Parser::IdentifierNode::REF_ENUM_NAME:
				//case Parser::IdentifierNode::REF_ENUM_VALUE:
				default: {
					THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("attribute \"%s\" doesn't support method calls.", base->name.c_str()), base->pos);
				}
			}

		} break;

		default: {
			THROW_BUG("can't reach here.");
		}
	}
}

}
