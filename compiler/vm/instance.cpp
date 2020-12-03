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

#include "instance.h"
#include "vm.h"

namespace carbon {

Instance::Instance() {}
Instance::Instance(ptr<Bytecode>& p_blueprint) {
	blueprint = p_blueprint;
	members.resize(blueprint->get_member_count());
}

bool Instance::_is_registered() const { return false; }
var Instance::get_member(const String& p_name) {
	uint32_t pos = blueprint->get_member_index(p_name);
	return members[pos];
}
void Instance::set_member(const String& p_name, var& p_value) {
	uint32_t pos = blueprint->get_member_index(p_name);
	members[pos] = p_value;
}

var Instance::call_method(const String& p_method_name, stdvec<var*>& p_args) {

	CarbonFunction* fn = nullptr;
	Bytecode* _class = blueprint.get();
	while (_class) {
		auto& functions = _class->get_functions();
		// TODO: if not found in functions check in member/static members too.
		auto it = functions.find(p_method_name);
		if (it != functions.end()) {
			fn = it->second.get();
			break;
		} else {
			if (!_class->has_base()) {
				THROW_ERROR(Error::ATTRIBUTE_ERROR,
					String::format("attribute \"%s\" doesn't exists on base %s.", p_method_name.c_str(), blueprint->get_name().c_str()));
			}
			if (_class->is_base_native()) {
				BindData* bd = NativeClasses::singleton()->find_bind_data(_class->get_base_native(), p_method_name).get();
				switch (bd->get_type()) {
					case BindData::METHOD:
						return static_cast<MethodBind*>(bd)->call(native_instance, p_args);
					case BindData::STATIC_FUNC:
						return static_cast<StaticFuncBind*>(bd)->call(p_args);
					case BindData::MEMBER_VAR:
						return static_cast<PropertyBind*>(bd)->get(native_instance).__call(p_args);
					case BindData::STATIC_VAR:
						return static_cast<StaticPropertyBind*>(bd)->get().__call(p_args);
					case BindData::STATIC_CONST:
						return static_cast<ConstantBind*>(bd)->get().__call(p_args);
					case BindData::ENUM:
						return static_cast<EnumBind*>(bd)->get()->__call(p_args);
					case BindData::ENUM_VALUE:
						THROW_ERROR(Error::OPERATOR_NOT_SUPPORTED, "enums are not callable.");
				}
				THROW_BUG("can't reach here");
				return var();
			} else {
				_class = _class->get_base_binary().get();
			}
		}
	}

	ASSERT(fn != nullptr);
	if (fn->is_static()) { // calling static method using instance (acceptable)
		return VM::singleton()->call_function(fn, _class, nullptr, p_args);
	} else {
		return VM::singleton()->call_function(fn, _class, shared_from_this(), p_args);
	}
}

ptr<Object> Instance::copy(bool p_deep) {
	ptr<Instance> ins_copy = newptr<Instance>(blueprint);
	for (int i = 0; i < (int)members.size(); i++) {
		ins_copy->members[i] = members[i].copy(p_deep);
	}
	return ins_copy;
}

String Instance::to_string() {
	ptr<CarbonFunction> fn = blueprint->get_function(GlobalStrings::to_string);
	if (fn == nullptr) return Super::to_string();
	return VM::singleton()->call_function(fn.get(), blueprint.get(), shared_from_this(), stdvec<var*>());
}


var Instance::__call(stdvec<var*>& p_args) {
	ptr<CarbonFunction> fn = blueprint->get_function(GlobalStrings::__call);
	if (fn == nullptr) return Super::__call(p_args);
	return VM::singleton()->call_function(fn.get(), blueprint.get(), shared_from_this(), p_args);
}

var Instance::__iter_begin() {
	ptr<CarbonFunction> fn = blueprint->get_function(GlobalStrings::__iter_begin);
	if (fn == nullptr) return Super::__iter_begin();
	return VM::singleton()->call_function(fn.get(), blueprint.get(), shared_from_this(), stdvec<var*>());
}
bool Instance::__iter_has_next() {
	ptr<CarbonFunction> fn = blueprint->get_function(GlobalStrings::__iter_has_next);
	if (fn == nullptr) return Super::__iter_has_next();
	return VM::singleton()->call_function(fn.get(), blueprint.get(), shared_from_this(), stdvec<var*>());
}
var Instance::__iter_next() {
	ptr<CarbonFunction> fn = blueprint->get_function(GlobalStrings::__iter_next);
	if (fn == nullptr) return Super::__iter_next();
	return VM::singleton()->call_function(fn.get(), blueprint.get(), shared_from_this(), stdvec<var*>());
}

var Instance::__get_mapped(const var& p_key){
	ptr<CarbonFunction> fn = blueprint->get_function(GlobalStrings::__get_mapped);
	if (fn == nullptr) return Super::__get_mapped(p_key);
	stdvec<var*> args; args.push_back(const_cast<var*>(&p_key));
	return VM::singleton()->call_function(fn.get(), blueprint.get(), shared_from_this(), args);
}
void Instance::__set_mapped(const var& p_key, const var& p_val) {
	ptr<CarbonFunction> fn = blueprint->get_function(GlobalStrings::__set_mapped);
	if (fn == nullptr) Super::__set_mapped(p_key, p_val);
	stdvec<var*> args = { const_cast<var*>(&p_key), const_cast<var*>(&p_val) };
	VM::singleton()->call_function(fn.get(), blueprint.get(), shared_from_this(), args);
}
int64_t Instance::__hash() {
	ptr<CarbonFunction> fn = blueprint->get_function(GlobalStrings::__hash);
	if (fn == nullptr) return Super::__hash();
	return VM::singleton()->call_function(fn.get(), blueprint.get(), shared_from_this(), stdvec<var*>());
}

#define CALL_OPERATOR(m_ret, m_operator)                                                            \
	m_ret Instance::m_operator(const var& p_other) {												\
		ptr<CarbonFunction> fn = blueprint->get_function(GlobalStrings::m_operator);				\
		if (fn == nullptr) return Super::m_operator(p_other);										\
		stdvec<var*> args = { const_cast<var*>(&p_other) };										    \
		return VM::singleton()->call_function(fn.get(), blueprint.get(), shared_from_this(), args);	\
	}
CALL_OPERATOR(bool, __gt);
CALL_OPERATOR(bool, __lt);
CALL_OPERATOR(bool, __eq);

CALL_OPERATOR(var, __add);
CALL_OPERATOR(var, __sub);
CALL_OPERATOR(var, __mul);
CALL_OPERATOR(var, __div);


}