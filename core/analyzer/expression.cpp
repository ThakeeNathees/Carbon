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

namespace carbon {

void Analyzer::_reduce_expression(ptr<Parser::Node>& p_expr) {

	if (p_expr == nullptr) return;

	// Prevent stack overflow.
	if (p_expr->is_reduced) return;
	p_expr->is_reduced = true;

	switch (p_expr->type) {

		case Parser::Node::Type::THIS:
		case Parser::Node::Type::SUPER:
		case Parser::Node::Type::BUILTIN_TYPE:
		case Parser::Node::Type::BUILTIN_FUNCTION:
			break;

		case Parser::Node::Type::IDENTIFIER: {
			ptr<Parser::IdentifierNode> id = ptrcast<Parser::IdentifierNode>(p_expr);
			do { // do ... while() loop is for jump out from the middle.
				if (parser->parser_context.current_func) {
					for (int i = 0; i < (int)parser->parser_context.current_func->args.size(); i++) {
						if (parser->parser_context.current_func->args[i].name == id->name) {
							id->ref = Parser::IdentifierNode::REF_PARAMETER;
							id->param_index = i;
							break;
						}
					}
				}
				if (id->ref != Parser::IdentifierNode::REF_UNKNOWN) break;

				Parser::BlockNode* outer_block = parser->parser_context.current_block;
				while (outer_block && id->ref == Parser::IdentifierNode::REF_UNKNOWN) {
					for (int i = 0; i < (int)outer_block->local_vars.size(); i++) {
						if (outer_block->local_vars[i]->name == id->name) {
							id->ref = Parser::IdentifierNode::REF_LOCAL_VAR;
							id->_var = outer_block->local_vars[i].get();
							break;
						}
					}
					if (id->ref != Parser::IdentifierNode::REF_UNKNOWN) break;

					for (int i = 0; i < (int)outer_block->local_const.size(); i++) {
						if (outer_block->local_const[i]->name == id->name) {
							id->ref = Parser::IdentifierNode::REF_LOCAL_CONST;
							_resolve_constant(outer_block->local_const[i].get());
							id->_const = outer_block->local_const[i].get();
							break;
						}
					}
					if (id->ref != Parser::IdentifierNode::REF_UNKNOWN) break;

					if (outer_block->parernt_node->type == Parser::Node::Type::BLOCK) {
						outer_block = ptrcast<Parser::BlockNode>(outer_block->parernt_node).get();
					} else {
						outer_block = nullptr;
					}
				}
				if (id->ref != Parser::IdentifierNode::REF_UNKNOWN) break;

				Parser::IdentifierNode _id = _get_member(parser->parser_context.current_class, id->name);
				if (_id.ref != Parser::IdentifierNode::REF_UNKNOWN) {
					id = newptr<Parser::IdentifierNode>(_id); break;
				}

				for (int i = 0; i < (int)file_node->vars.size(); i++) {
					if (file_node->vars[i]->name == id->name) {
						id->ref = Parser::IdentifierNode::REF_MEMBER_VAR;
						id->_var = file_node->vars[i].get();
						break;
					}
				}
				if (id->ref != Parser::IdentifierNode::REF_UNKNOWN) break;

				for (int i = 0; i < (int)file_node->constants.size(); i++) {
					if (file_node->constants[i]->name == id->name) {
						id->ref = Parser::IdentifierNode::REF_MEMBER_CONST;
						_resolve_constant(file_node->constants[i].get());
						id->_const = file_node->constants[i].get();
						break;
					}
				}
				if (id->ref != Parser::IdentifierNode::REF_UNKNOWN) break;

				if (parser->parser_context.current_enum != nullptr) {
					for (std::pair<String, Parser::EnumValueNode> pair : parser->parser_context.current_enum->values) {
						if (pair.first == id->name) {
							id->ref = Parser::IdentifierNode::REF_ENUM_VALUE;
							_resolve_enumvalue(parser->parser_context.current_enum->values[pair.first]);
							id->enum_value = &pair.second;
							break;
						}
					}
				}
				if (id->ref != Parser::IdentifierNode::REF_UNKNOWN) break;

				if (file_node->unnamed_enum != nullptr) {
					for (std::pair<String, Parser::EnumValueNode> pair : file_node->unnamed_enum->values) {
						if (pair.first == id->name) {
							id->ref = Parser::IdentifierNode::REF_ENUM_VALUE;
							_resolve_enumvalue(file_node->unnamed_enum->values[pair.first]);
							id->enum_value = &pair.second;
							break;
						}
					}
				}
				if (id->ref != Parser::IdentifierNode::REF_UNKNOWN) break;

				for (int i = 0; i < (int)file_node->enums.size(); i++) {
					if (file_node->enums[i]->name == id->name) {
						id->ref = Parser::IdentifierNode::REF_ENUM_NAME;
						id->enum_node = file_node->enums[i].get();
						break;
					}
				}
				if (id->ref != Parser::IdentifierNode::REF_UNKNOWN) break;

				for (int i = 0; i < (int)file_node->classes.size(); i++) {
					if (file_node->classes[i]->name == id->name) {
						id->ref = Parser::IdentifierNode::REF_CARBON_CLASS;
						id->_class = file_node->classes[i].get();
						break;
					}
				}
				if (id->ref != Parser::IdentifierNode::REF_UNKNOWN) break;

				for (int i = 0; i < (int)file_node->functions.size(); i++) {
					if (file_node->functions[i]->name == id->name) {
						id->ref = Parser::IdentifierNode::REF_CARBON_FUNCTION;
						id->_func = file_node->functions[i].get();
						break;
					}
				}
				if (id->ref != Parser::IdentifierNode::REF_UNKNOWN) break;

				if (NativeClasses::is_class_registered(id->name)) {
					id->ref = Parser::IdentifierNode::REF_NATIVE_CLASS;
					break;
				}

				// TODO: REF_FILE for import.
			} while (false);

			switch (id->ref) {
				case Parser::IdentifierNode::REF_UNKNOWN:
					THROW_ANALYZER_ERROR(Error::NAME_ERROR, String::format("identifier \"%s\" isn't defined.", id->name.c_str()), id->pos);
				case Parser::IdentifierNode::REF_LOCAL_CONST:
				case Parser::IdentifierNode::REF_MEMBER_CONST: {
					ptr<Parser::ConstValueNode> cv = new_node<Parser::ConstValueNode>(id->_const->value);
					cv->pos = id->pos; p_expr = cv;
				} break;
				case Parser::IdentifierNode::REF_ENUM_VALUE: {
					ptr<Parser::ConstValueNode> cv = new_node<Parser::ConstValueNode>(id->enum_value->value);
					cv->pos = id->pos; p_expr = cv;
				} break;
				default: { // variable, parameter, function name, ...
					p_expr = id;
					break;
				}
			}
		} break; /// reduce IdentifierNode ///////////////////////////////////

		case Parser::Node::Type::ARRAY: {
			ptr<Parser::ArrayNode> arr = ptrcast<Parser::ArrayNode>(p_expr);
			bool all_const = true;
			for (int i = 0; i < (int)arr->elements.size(); i++) {
				_reduce_expression(arr->elements[i]);
				if (arr->elements[i]->type != Parser::Node::Type::CONST_VALUE) {
					all_const = false;
				}
			}
			if (all_const) {
				Array arr_value;
				for (int i = 0; i < (int)arr->elements.size(); i++) {
					arr_value.push_back(ptrcast<Parser::ConstValueNode>(arr->elements[i])->value);
				}
				ptr<Parser::ConstValueNode> cv = new_node<Parser::ConstValueNode>(arr_value);
				cv->pos = p_expr->pos; p_expr = cv;
			}
			// TODO: change array node to CallNode i.e. [a, b, c] -> Array(a, b, c);
		} break; /// reduce ArrayNode ///////////////////////////////////

		case Parser::Node::Type::MAP: {
			ptr<Parser::MapNode> map = ptrcast<Parser::MapNode>(p_expr);
			bool all_const = true;
			for (int i = 0; i < (int)map->elements.size(); i++) {
				_reduce_expression(map->elements[i].key);
				// TODO: if key is const value and two keys are the same throw error.
				if (map->elements[i].key->type == Parser::Node::Type::CONST_VALUE) {
					var& key_v = ptrcast<Parser::ConstValueNode>(map->elements[i].key)->value;
					if (!var::is_hashable(key_v.get_type())) THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("unhasnable type %s used as map key.", key_v.get_type_name().c_str()), map->pos);
				}
				_reduce_expression(map->elements[i].value);

				if (map->elements[i].key->type != Parser::Node::Type::CONST_VALUE || map->elements[i].value->type != Parser::Node::Type::CONST_VALUE) {
					all_const = false;
				}
			}
			if (all_const) {
				Map map_value;
				for (int i = 0; i < (int)map->elements.size(); i++) {
					var& _key = ptrcast<Parser::ConstValueNode>(map->elements[i].key)->value;
					var& _val = ptrcast<Parser::ConstValueNode>(map->elements[i].value)->value;
					map_value[_key] = _val;
				}
				ptr<Parser::ConstValueNode> cv = new_node<Parser::ConstValueNode>(map_value);
				cv->pos = p_expr->pos; p_expr = cv;
			}
			// TODO: change array node to CallNode i.e. {"key":"valeu"} -> Map( Pair("key", "value") ); or similler.
		} break; /// reduce MapNode ///////////////////////////////////

#define GET_ARGS(m_nodes)                                                             \
	stdvec<var> args;                                                                 \
	for (int i = 0; i < (int)m_nodes.size(); i++) {                                   \
	    args.push_back(ptrcast<Parser::ConstValueNode>(m_nodes[i])->value);           \
	}

#define SET_EXPR_CONST_NODE(m_var, m_pos)                                             \
do {                                                                                  \
	ptr<Parser::ConstValueNode> cv = new_node<Parser::ConstValueNode>(m_var);         \
	cv->pos = m_pos;                                                                  \
	p_expr = cv;                                                                      \
} while (false)

		case Parser::Node::Type::CALL: {
			ptr<Parser::CallNode> call = ptrcast<Parser::CallNode>(p_expr);
			bool all_const = true;
			for (int i = 0; i < (int)call->r_args.size(); i++) {
				_reduce_expression(call->r_args[i]);
				if (call->r_args[i]->type != Parser::Node::Type::CONST_VALUE) {
					all_const = false;
				}
			}
			if (call->base->type == Parser::Node::Type::BUILTIN_FUNCTION || call->base->type == Parser::Node::Type::BUILTIN_TYPE) {
				// don't_reduce_anything();
			} else {
				if (call->base->type == Parser::Node::Type::UNKNOWN) {
					_reduce_expression(call->method);
					ASSERT(call->method->type == Parser::Node::Type::IDENTIFIER);
				} else {
					_reduce_expression(call->base);
				}
			}

			// reduce builtin function
			if (call->base->type == Parser::Node::Type::BUILTIN_FUNCTION) {
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
								throw err
									.set_file(file_node->path)
									.set_line(file_node->source.get_line(call->pos.x))
									.set_pos(call->pos)
									.set_err_len((uint32_t)String(BuiltinFunctions::get_func_name(bf->func)).size())
									;
							}
						}
					}
				}

				// reduce builtin type construction
			} else if (call->base->type == Parser::Node::Type::BUILTIN_TYPE) {
				if (all_const) {
					ptr<Parser::BuiltinTypeNode> bt = ptrcast<Parser::BuiltinTypeNode>(call->base);
					try {
						GET_ARGS(call->r_args);
						var ret = BuiltinTypes::construct(bt->builtin_type, args);
						SET_EXPR_CONST_NODE(ret, call->pos);
					} catch (Error& err) {
						throw err
							.set_file(file_node->path)
							.set_line(file_node->source.get_line(call->base->pos.x))
							.set_pos(call->base->pos)
							.set_err_len((uint32_t)String(BuiltinTypes::get_type_name(bt->builtin_type)).size())
							;
					}
				}

				// method call on base const value.
			} else if (call->base->type == Parser::Node::Type::CONST_VALUE) {
				if (all_const) {
					try {
						ASSERT(call->method->type == Parser::Node::Type::IDENTIFIER);
						GET_ARGS(call->r_args); // 0 : const value, 1: name, ... args.
						var ret = ptrcast<Parser::ConstValueNode>(call->base)->value.call_method(ptrcast<Parser::IdentifierNode>(call->method)->name, args);
						SET_EXPR_CONST_NODE(ret, call->pos);
					} catch (.../*VarError& err*/) {
						ASSERT(false && "TODO: catch and throw var error as cb error");
					}
				}

				// method call on this, super, super.super ...
			} else if (call->base->type == Parser::Node::Type::UNKNOWN) {
				
				Parser::IdentifierNode* id = ptrcast<Parser::IdentifierNode>(call->method).get();
				switch (id->ref) {

					// call `__call` method on the variable.
					case Parser::IdentifierNode::REF_PARAMETER:
					case Parser::IdentifierNode::REF_LOCAL_VAR:
					case Parser::IdentifierNode::REF_MEMBER_VAR: {
						call->base = call->method; // param(args...); -> will call param.__call(args...);
						call->method = nullptr;
					} break;

						// check arguments.
					case Parser::IdentifierNode::REF_CARBON_FUNCTION: {
						int argc = (int)id->_func->args.size();
						int argc_default = (int)id->_func->default_parameters.size();
						int argc_given = (int)call->r_args.size();
						if (argc_given + argc_default < argc) {
							if (argc_default == 0) THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected exactly %i argument(s).", argc), id->pos);
							else THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected at least %i argument(s).", argc - argc_default), id->pos);
						} else if (argc_given > argc) {
							if (argc_default == 0) THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected exactly %i argument(s).", argc), id->pos);
							else THROW_ANALYZER_ERROR(Error::INVALID_ARG_COUNT, String::format("expected minimum of %i argument(s) and maximum of %i argument(s).", argc - argc_default, argc), id->pos);
						}
					} break;

					// call constructor.
					case Parser::IdentifierNode::REF_CARBON_CLASS: {
						if (id->_class->constructor) {
							int argc = (int)id->_class->constructor->args.size();
							int argc_default = (int)id->_class->constructor->default_parameters.size();
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

					case Parser::IdentifierNode::REF_NATIVE_CLASS: {
						ASSERT(NativeClasses::is_class_registered(id->name));
						const StaticFuncBind* initializer = NativeClasses::get_initializer(id->name);
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

						// invalid.
						//case Parser::IdentifierNode::REF_ENUM_NAME:
						//case Parser::IdentifierNode::REF_ENUM_VALUE:
						//case Parser::IdentifierNode::REF_FILE:
						//case Parser::IdentifierNode::REF_LOCAL_CONST:
						//case Parser::IdentifierNode::REF_MEMBER_CONST:
					default: {
						THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("attribute \"%s\" is not callable.", id->name.c_str()), id->pos);
					}
				}

			} else if (call->base->type == Parser::Node::Type::ARRAY) {
				ASSERT(call->method->type == Parser::Node::Type::IDENTIFIER);
				ASSERT(false);
			} else if (call->base->type == Parser::Node::Type::MAP) {
				ASSERT(call->method->type == Parser::Node::Type::IDENTIFIER);
				ASSERT(false);
			} else if (call->base->type == Parser::Node::Type::THIS) {
				ASSERT(call->method->type == Parser::Node::Type::IDENTIFIER);
				ASSERT(false);
			} else if (call->base->type == Parser::Node::Type::SUPER) {
				ASSERT(call->method->type == Parser::Node::Type::IDENTIFIER);
				ASSERT(false);
			} else if (call->base->type == Parser::Node::Type::OPERATOR) {
				ASSERT(call->method->type == Parser::Node::Type::IDENTIFIER);
				ASSERT(false);

				// method call on base with identifier id.
			} else if (call->base->type == Parser::Node::Type::IDENTIFIER) {
				ASSERT(call->method->type == Parser::Node::Type::IDENTIFIER);
				Parser::IdentifierNode* base = ptrcast<Parser::IdentifierNode>(call->base).get();
				Parser::IdentifierNode* id = ptrcast<Parser::IdentifierNode>(call->method).get();

				switch (base->ref) {
					case Parser::IdentifierNode::REF_PARAMETER:
					case Parser::IdentifierNode::REF_LOCAL_VAR:
					case Parser::IdentifierNode::REF_MEMBER_VAR: {
					} break;

					case Parser::IdentifierNode::REF_LOCAL_CONST:
					case Parser::IdentifierNode::REF_MEMBER_CONST: {
						// IF AN IDENTIFIER IS REFERENCE TO A CONSTANT IT'LL BE `ConstValueNode` BY NOW.
						ASSERT(false && "can't be.");
					} break;

					case Parser::IdentifierNode::REF_CARBON_CLASS: {

						Parser::IdentifierNode _id = _get_member(base->_class, id->name);
						switch (_id.ref) {
							case Parser::IdentifierNode::REF_UNKNOWN:
								THROW_ANALYZER_ERROR(Error::NAME_ERROR, String::format("attribute \"%s\" doesn't exists on base %s", id->name.c_str(), base->_class->name.c_str()), id->pos);
							case Parser::IdentifierNode::REF_MEMBER_VAR: {
								if (_id._var->is_static) {
									break; // Class.var(args...);
								} else {
									THROW_ANALYZER_ERROR(Error::ATTRIBUTE_ERROR, String::format("can't access non-static attribute \"%s\" statically", id->name.c_str()), id->pos);
								}
							} break;

							case Parser::IdentifierNode::REF_LOCAL_CONST:
							case Parser::IdentifierNode::REF_MEMBER_CONST: {
								// Class.CONST(args...);
							} break;

							case Parser::IdentifierNode::REF_ENUM_NAME: THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("enums (\"%s.%s()\") are not callable.", base->name.c_str(), id->name.c_str()), id->pos);
							case Parser::IdentifierNode::REF_ENUM_VALUE: THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("enum value (\"%s.%s()\") are not callable.", base->name.c_str(), id->name.c_str()), id->pos);

							case Parser::IdentifierNode::REF_CARBON_FUNCTION: {
								if (_id._func->is_static) {
									break; // Class.static_func(args...);
								} else {
									THROW_ANALYZER_ERROR(Error::ATTRIBUTE_ERROR, String::format("can't call non-static method\"%s\" statically", id->name.c_str()), id->pos);
								}
							}
							//case Parser::IdentifierNode::REF_FILE: //TODO:
						}
					} break;

					case Parser::IdentifierNode::REF_NATIVE_CLASS: {

						BindData* bd = NativeClasses::get_bind_data(base->name, id->name).get();
						if (!bd) THROW_ANALYZER_ERROR(Error::ATTRIBUTE_ERROR, String::format("attribute \"%s\" does not exists on base %s.", id->name.c_str(), base->name.c_str()), id->pos);
						switch (bd->get_type()) {
							case BindData::STATIC_FUNC: {
								// TODO: check if the native func consexpr.
							} break;
							case BindData::STATIC_VAR: break; // calls the "__call" at runtime.
							case BindData::STATIC_CONST: {
								const MemberInfo* memi = base->_const->value.get_member_info(id->name);
								if (!memi) THROW_ANALYZER_ERROR(Error::NAME_ERROR, String::format("attribute \"%s\" doesn't exists on base %s.", id->name.c_str(), base->_const->value.get_type_name().c_str()), id->pos);
								if (memi->get_type() != MemberInfo::METHOD) THROW_ANALYZER_ERROR(Error::ATTRIBUTE_ERROR, String::format("attribute \"%s\" on base %s isn't callable.", id->name.c_str(), base->_const->value.get_type_name().c_str()), id->pos);

								const MethodInfo* mi = (const MethodInfo*)memi;
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
								if (all_const) {
									try {
										GET_ARGS(call->r_args);
										var ret = base->_const->value.call_method("__call", args);
										SET_EXPR_CONST_NODE(ret, call->pos);
									} catch (...) {
										ASSERT(false && "catch var error.");
									}
								}
							} break;
							case BindData::METHOD: THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("can't call non-static method \"%s.%s()\" statically.", base->name.c_str(), id->name.c_str()), id->pos);
							case BindData::MEMBER_VAR:  THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("can't call non-static member \"%s.%s()\" statically.", base->name.c_str(), id->name.c_str()), id->pos);
							case BindData::ENUM:  THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("enums (\"%s.%s()\") are not callable.", base->name.c_str(), id->name.c_str()), id->pos);
							case BindData::ENUM_VALUE: THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("enum value (\"%s.%s()\") are not callable.", base->name.c_str(), id->name.c_str()), id->pos);
						}

					} break;

						// fn.get_default_args(), fn.get_name(), ...
					case Parser::IdentifierNode::REF_CARBON_FUNCTION: {
						ASSERT(false && "TODO:"); // FuncRef object.
					} break;

						//case Parser::IdentifierNode::REF_UNKNOWN:
						//case Parser::IdentifierNode::REF_ENUM_NAME:
						//case Parser::IdentifierNode::REF_ENUM_VALUE:
					default: {
						THROW_ANALYZER_ERROR(Error::TYPE_ERROR, String::format("attribute \"%s\" doesn't support method calls.", base->name.c_str()), base->pos);
					}
				}

			} else {
				ASSERT(false && "can't reach here.");
			}

		} break; /// reduce CallNode ///////////////////////////////////

		case Parser::Node::Type::INDEX: {
			ptr<Parser::IndexNode> index = ptrcast<Parser::IndexNode>(p_expr);
			_reduce_expression(index->base);
			ASSERT(index->member->type == Parser::Node::Type::IDENTIFIER);
			ptr<Parser::IdentifierNode> member = ptrcast<Parser::IdentifierNode>(index->member);

			switch (index->base->type) {

				// used in: Vect2.ZERO;
				case Parser::Node::Type::BUILTIN_TYPE: {
					Parser::BuiltinTypeNode* bt = ptrcast<Parser::BuiltinTypeNode>(index->base).get();
					const MemberInfo* mi = var::get_member_info_s(BuiltinTypes::get_var_type(bt->builtin_type), member->name);
					if (!mi) THROW_ANALYZER_ERROR(Error::NAME_ERROR, String::format("attribute \"%s\" doesn't exists on base %s.", member->name.c_str(), BuiltinTypes::get_type_name(bt->builtin_type).c_str()), member->pos);
					switch (mi->get_type()) {
						case MemberInfo::METHOD:
							break; // FuncRef.
						case MemberInfo::PROPERTY: {
							PropertyInfo* pi = (PropertyInfo*)mi;
							if (pi->is_const()) SET_EXPR_CONST_NODE(pi->get_value(), index->pos);
							else ASSERT(false && "can't be.");
						} break;
						case MemberInfo::ENUM:
						case MemberInfo::ENUM_VALUE:
							ASSERT(false && "can't be.");
					}
				} break;

				// [1, 2].member;
				case Parser::Node::Type::CONST_VALUE: {
					Parser::ConstValueNode* base = ptrcast<Parser::ConstValueNode>(index->base).get();
					try {
						ptr<Parser::ConstValueNode> cv = new_node<Parser::ConstValueNode>(base->value.get_member(member->name));
						cv->pos = member->pos; p_expr = cv;
					} catch (VarError& err) {
						if (err.get_type() == VarError::NULL_POINTER) {
							THROW_ANALYZER_ERROR(Error::NULL_POINTER, "", index->pos);
						} else if (err.get_type() == VarError::ATTRIBUTE_ERROR) {
							THROW_ANALYZER_ERROR(Error::ATTRIBUTE_ERROR, String::format("name \"%s\" isn't exists on base %s.", member->name.c_str(), base->value.get_type_name().c_str()), index->pos);
						} else {
							ASSERT(false && "can't be.");
						}
					}
				} break;

				// this.member; super.member; idf.member;
				case Parser::Node::Type::THIS:
				case Parser::Node::Type::SUPER:
				case Parser::Node::Type::IDENTIFIER: {

					Parser::IdentifierNode* base;
					ptr<Parser::IdentifierNode> _keep_alive;

					if (index->base->type == Parser::Node::Type::THIS) {
						_keep_alive = newptr<Parser::IdentifierNode>(parser->parser_context.current_class->name);
						_keep_alive->ref = Parser::IdentifierNode::REF_CARBON_CLASS;
						_keep_alive->_class = parser->parser_context.current_class;
						base = _keep_alive.get();
					} else if (index->base->type == Parser::Node::Type::SUPER) {
						if (parser->parser_context.current_class->base_type == Parser::ClassNode::BASE_LOCAL) {
							_keep_alive = newptr<Parser::IdentifierNode>(parser->parser_context.current_class->base_local->name);
							_keep_alive->ref = Parser::IdentifierNode::REF_CARBON_CLASS;
							_keep_alive->_class = parser->parser_context.current_class->base_local;
							base = _keep_alive.get();
						} else if (parser->parser_context.current_class->base_type == Parser::ClassNode::BASE_EXTERN) {
							ASSERT(false && "TODO:");
						}
					} else {
						base = ptrcast<Parser::IdentifierNode>(index->base).get();
					}

					switch (base->ref) {
						case Parser::IdentifierNode::REF_UNKNOWN: {
							THROW_BUG("base can't be unknown.");
						} break;

						case Parser::IdentifierNode::REF_PARAMETER:
						case Parser::IdentifierNode::REF_LOCAL_VAR:
							break; // Can't reduce anymore.
						case Parser::IdentifierNode::REF_LOCAL_CONST: {
						} break;
						case Parser::IdentifierNode::REF_MEMBER_VAR:
							break; // Can't reduce anymore.

						case Parser::IdentifierNode::REF_MEMBER_CONST: {
							try {
								base->_const->value.get_member(member->name);
							} catch (VarError& err) {
								THROW_ANALYZER_ERROR(Error::ATTRIBUTE_ERROR, err.what(), member->pos);
							}
							ASSERT(false && "there isn't any contant value currently support attribute access and most probably in the future");
						} break;

						case Parser::IdentifierNode::REF_ENUM_NAME: {
							for (std::pair<String, Parser::EnumValueNode> pair : base->enum_node->values) {
								if (pair.first == member->name) {
									member->ref = Parser::IdentifierNode::REF_ENUM_VALUE;
									member->enum_value = &pair.second;
									_resolve_enumvalue(base->enum_node->values[pair.first]);
									ptr<Parser::ConstValueNode> cv = new_node<Parser::ConstValueNode>(base->enum_node->values[pair.first].value);
									cv->pos = member->pos; p_expr = cv;
									break;
								}
							}
						} break;

						case Parser::IdentifierNode::REF_ENUM_VALUE:
							THROW_ANALYZER_ERROR(Error::OPERATOR_NOT_SUPPORTED, "enum value doesn't support attribute access.", member->pos);

						case Parser::IdentifierNode::REF_CARBON_CLASS: {
							Parser::IdentifierNode _id = _get_member(base->_class, member->name);
							switch (_id.ref) {
								case Parser::IdentifierNode::REF_UNKNOWN:
									THROW_ANALYZER_ERROR(Error::ATTRIBUTE_ERROR, String::format("attribute \"%s\" isn't exists in base \"%s\".", member->name.c_str(), base->name.c_str()), member->pos);
								case Parser::IdentifierNode::REF_PARAMETER:
								case Parser::IdentifierNode::REF_LOCAL_VAR:
								case Parser::IdentifierNode::REF_LOCAL_CONST:
									ASSERT(false && "can't be.");

								case Parser::IdentifierNode::REF_MEMBER_VAR: {
									_id.pos = member->pos; _id.name = base->name + "." + _id.name;
									p_expr = newptr<Parser::IdentifierNode>(_id);
								} break;

								case Parser::IdentifierNode::REF_MEMBER_CONST: {
									ptr<Parser::ConstValueNode> cv = new_node<Parser::ConstValueNode>(_id._const->value);
									cv->pos = member->pos; p_expr = cv;
								} break;

								case Parser::IdentifierNode::REF_ENUM_NAME: {
									_id.pos = member->pos; _id.name = base->name + "." + _id.name;
									p_expr = newptr<Parser::IdentifierNode>(_id);
								} break;

								case Parser::IdentifierNode::REF_ENUM_VALUE: {
									ptr<Parser::ConstValueNode> cv = new_node<Parser::ConstValueNode>(_id.enum_value);
									cv->pos = member->pos; p_expr = cv;
								} break;

								case Parser::IdentifierNode::REF_CARBON_FUNCTION: {
									_id.pos = member->pos; _id.name = base->name + "." + _id.name;
									p_expr = newptr<Parser::IdentifierNode>(_id);
								} break;

								case Parser::IdentifierNode::REF_CARBON_CLASS:
								case Parser::IdentifierNode::REF_NATIVE_CLASS:
								case Parser::IdentifierNode::REF_FILE:
									ASSERT(false && "can't be");
							}
						} break;

						case Parser::IdentifierNode::REF_NATIVE_CLASS: {
							ASSERT(NativeClasses::is_class_registered(base->name));
							BindData* bd = NativeClasses::get_bind_data(base->name, member->name).get();
							if (!bd) THROW_ANALYZER_ERROR(Error::NAME_ERROR, String::format("attribute \"%s\" doesn't exists on base %s.", member->name.c_str(), base->name.c_str()), member->pos);
							switch (bd->get_type()) {
								case BindData::METHOD:
								case BindData::STATIC_FUNC:
								case BindData::MEMBER_VAR:
								case BindData::STATIC_VAR:
								case BindData::ENUM:
									break;

								case BindData::STATIC_CONST: {
									ptr<Parser::ConstValueNode> cv = new_node<Parser::ConstValueNode>(((ConstantBind*)bd)->get());
									cv->pos = member->pos; p_expr = cv;
								} break;
								case BindData::ENUM_VALUE: {
									ptr<Parser::ConstValueNode> cv = new_node<Parser::ConstValueNode>(((EnumValueBind*)bd)->get());
									cv->pos = member->pos; p_expr = cv;
								} break;
							}

						} break;

							// TODO: fn.NAME, fn.ARGC, fn.DEFAULT_ARGC, ... TODO: implement FuncRef
						case Parser::IdentifierNode::REF_CARBON_FUNCTION: {
							THROW_ANALYZER_ERROR(Error::OPERATOR_NOT_SUPPORTED, "function object doesn't support attribute access.", member->pos);
						} break;

						case Parser::IdentifierNode::REF_FILE: { // TODO: change the name.
							ASSERT(false && "TODO:");
						} break;

							// TODO: REF binary version of everything above.
					}
				}

				default:
					break;
					// RUNTIME.
			}

		} break; /// reduce IndexNode ///////////////////////////////////

		case Parser::Node::Type::MAPPED_INDEX: {
			ptr<Parser::MappedIndexNode> mapped_index = ptrcast<Parser::MappedIndexNode>(p_expr);
			_reduce_expression(mapped_index->base);
			_reduce_expression(mapped_index->key);
			if (mapped_index->base->type == Parser::Node::Type::CONST_VALUE && mapped_index->key->type == Parser::Node::Type::CONST_VALUE) {
				Parser::ConstValueNode* base = ptrcast<Parser::ConstValueNode>(mapped_index->base).get();
				Parser::ConstValueNode* key = ptrcast<Parser::ConstValueNode>(mapped_index->key).get();
				try {
					ptr<Parser::ConstValueNode> cv = new_node<Parser::ConstValueNode>(base->value.__get_mapped(key->value));
					cv->pos = base->pos; p_expr = cv;
				} catch (.../*VarError& err*/) {
					ASSERT(false && "TODO: var error to carbon error");
				}
			}
		} break; /// reduce MappedIndexNode ///////////////////////////////////

		///////////////////////////////////////////////////////////////////////////////////////////////

		case Parser::Node::Type::OPERATOR: {
			ptr<Parser::OperatorNode> op = ptrcast<Parser::OperatorNode>(p_expr);

			bool all_const = true;
			for (int i = 0; i < (int)op->args.size(); i++) {
				_reduce_expression(op->args[i]);
				if (op->args[i]->type != Parser::Node::Type::CONST_VALUE) all_const = false;
			}

			switch (op->op_type) {

				case Parser::OperatorNode::OpType::OP_EQ:
				case Parser::OperatorNode::OpType::OP_PLUSEQ:
				case Parser::OperatorNode::OpType::OP_MINUSEQ:
				case Parser::OperatorNode::OpType::OP_MULEQ:
				case Parser::OperatorNode::OpType::OP_DIVEQ:
				case Parser::OperatorNode::OpType::OP_MOD_EQ:
				case Parser::OperatorNode::OpType::OP_LTEQ:
				case Parser::OperatorNode::OpType::OP_GTEQ:
				case Parser::OperatorNode::OpType::OP_BIT_LSHIFT_EQ:
				case Parser::OperatorNode::OpType::OP_BIT_RSHIFT_EQ:
				case Parser::OperatorNode::OpType::OP_BIT_OR_EQ:
				case Parser::OperatorNode::OpType::OP_BIT_AND_EQ:
				case Parser::OperatorNode::OpType::OP_BIT_XOR_EQ: {

					if (op->args[0]->type == Parser::Node::Type::IDENTIFIER) {
						switch (ptrcast<Parser::IdentifierNode>(op->args[0])->ref) {
							case Parser::IdentifierNode::REF_PARAMETER:
							case Parser::IdentifierNode::REF_LOCAL_VAR:
							case Parser::IdentifierNode::REF_MEMBER_VAR:
								break;
							case Parser::IdentifierNode::REF_LOCAL_CONST:
							case Parser::IdentifierNode::REF_MEMBER_CONST:
							case Parser::IdentifierNode::REF_ENUM_NAME:
							case Parser::IdentifierNode::REF_ENUM_VALUE:
							case Parser::IdentifierNode::REF_CARBON_CLASS:
							case Parser::IdentifierNode::REF_NATIVE_CLASS:
							case Parser::IdentifierNode::REF_CARBON_FUNCTION:
							case Parser::IdentifierNode::REF_FILE:
								THROW_ANALYZER_ERROR(Error::TYPE_ERROR, "invalid assignment (only assignment on variables/parameters are valid).", op->args[0]->pos);
						}
					} else if (op->args[0]->type == Parser::Node::Type::THIS) {
						THROW_ANALYZER_ERROR(Error::TYPE_ERROR, "can't assign anything to \"this\".", op->args[0]->pos);
					} else if (op->args[0]->type == Parser::Node::Type::SUPER) {
						THROW_ANALYZER_ERROR(Error::TYPE_ERROR, "can't assign anything to \"super\".", op->args[0]->pos);
					} else if (op->args[0]->type == Parser::Node::Type::CONST_VALUE) {
						THROW_ANALYZER_ERROR(Error::TYPE_ERROR, "can't assign anything to constant values.", op->args[0]->pos);
					} else if (op->args[0]->type == Parser::Node::Type::ARRAY) {
						THROW_ANALYZER_ERROR(Error::TYPE_ERROR, "can't assign anything to array literal.", op->args[0]->pos);
					} else if (op->args[0]->type == Parser::Node::Type::MAP) {
						THROW_ANALYZER_ERROR(Error::TYPE_ERROR, "can't assign anything to map literal.", op->args[0]->pos);
					}
				} break;
				default: { // Remaining binary/unary operators.
					if (!all_const) break;
					GET_ARGS(op->args);
					switch (op->op_type) {
						case Parser::OperatorNode::OpType::OP_EQEQ:
							SET_EXPR_CONST_NODE(args[0] == args[1], op->pos);
							break;
						case Parser::OperatorNode::OpType::OP_PLUS:
							SET_EXPR_CONST_NODE(args[0] + args[1], op->pos);
							break;
						case Parser::OperatorNode::OpType::OP_MINUS:
							SET_EXPR_CONST_NODE(args[0] - args[1], op->pos);
							break;
						case Parser::OperatorNode::OpType::OP_MUL:
							SET_EXPR_CONST_NODE(args[0] * args[1], op->pos);
							break;
						case Parser::OperatorNode::OpType::OP_DIV:
							SET_EXPR_CONST_NODE(args[0] / args[1], op->pos);
							break;
						case Parser::OperatorNode::OpType::OP_MOD:
							SET_EXPR_CONST_NODE(args[0] % args[1], op->pos);
							break;
						case Parser::OperatorNode::OpType::OP_LT:
							SET_EXPR_CONST_NODE(args[0] < args[1], op->pos);
							break;
						case Parser::OperatorNode::OpType::OP_GT:
							SET_EXPR_CONST_NODE(args[0] > args[1], op->pos);
							break;
						case Parser::OperatorNode::OpType::OP_AND:
							SET_EXPR_CONST_NODE(args[0].operator bool() && args[1].operator bool(), op->pos);
							break;
						case Parser::OperatorNode::OpType::OP_OR:
							SET_EXPR_CONST_NODE(args[0].operator bool() || args[1].operator bool(), op->pos);
							break;
						case Parser::OperatorNode::OpType::OP_NOTEQ:
							SET_EXPR_CONST_NODE(args[0] != args[1], op->pos);
							break;
						case Parser::OperatorNode::OpType::OP_BIT_LSHIFT:
							SET_EXPR_CONST_NODE(args[0].operator int64_t() << args[1].operator int64_t(), op->pos);
							break;
						case Parser::OperatorNode::OpType::OP_BIT_RSHIFT:
							SET_EXPR_CONST_NODE(args[0].operator int64_t() >> args[1].operator int64_t(), op->pos);
							break;
						case Parser::OperatorNode::OpType::OP_BIT_OR:
							SET_EXPR_CONST_NODE(args[0].operator int64_t() | args[1].operator int64_t(), op->pos);
							break;
						case Parser::OperatorNode::OpType::OP_BIT_AND:
							SET_EXPR_CONST_NODE(args[0].operator int64_t() & args[1].operator int64_t(), op->pos);
							break;
						case Parser::OperatorNode::OpType::OP_BIT_XOR:
							SET_EXPR_CONST_NODE(args[0].operator int64_t() ^ args[1].operator int64_t(), op->pos);
							break;

						case Parser::OperatorNode::OpType::OP_NOT:
							SET_EXPR_CONST_NODE(!args[0].operator bool(), op->pos);
							break;
						case Parser::OperatorNode::OpType::OP_BIT_NOT:
							SET_EXPR_CONST_NODE(~args[0].operator int64_t(), op->pos);
							break;
						case Parser::OperatorNode::OpType::OP_POSITIVE:
							switch (args[0].get_type()) {
								case var::BOOL:
								case var::INT:
								case var::FLOAT: {
									SET_EXPR_CONST_NODE(args[0], op->pos);
								} break;
								default:
									THROW_ANALYZER_ERROR(Error::OPERATOR_NOT_SUPPORTED,
										String::format("unary operator \"+\" not supported on %s.", args[0].get_type_name()), op->pos);
							}
							break;
						case Parser::OperatorNode::OpType::OP_NEGATIVE:
							switch (args[0].get_type()) {
								case var::BOOL:
								case var::INT:
									SET_EXPR_CONST_NODE(-args[0].operator int64_t(), op->pos);
									break;
								case var::FLOAT:
									SET_EXPR_CONST_NODE(-args[0].operator double(), op->pos);
									break;
								default:
									THROW_ANALYZER_ERROR(Error::OPERATOR_NOT_SUPPORTED,
										String::format("unary operator \"+\" not supported on %s.", args[0].get_type_name()), op->pos);
							}
							break;
					}
				}
				MISSED_ENUM_CHECK(Parser::OperatorNode::OpType::_OP_MAX_, 33);
			}
#undef SET_EXPR_CONST_NODE
		} break;
		case Parser::Node::Type::CONST_VALUE: {
		} break; // Can't reduce anymore.

		default: {
			ASSERT(false); // ???
		}
	}
}


}