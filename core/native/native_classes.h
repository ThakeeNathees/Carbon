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

#ifndef NATIVE_CLASSES_H
#define NATIVE_CLASSES_H

#include "core.h"

#define DEFVAL(m_val) m_val
#define DEFVALUES(...) var::vector(__VA_ARGS__)
#define PARAMS(...) __VA_ARGS__

#define BIND_METHOD(m_name, m_method, ...) NativeClasses::bind_data(_bind_method(m_name, get_class_name_s(), m_method, __VA_ARGS__))
#define BIND_METHOD_VA(m_name, m_method) NativeClasses::bind_data(_bind_va_method(m_name, get_class_name_s(), m_method))
#define BIND_STATIC_FUNC(m_name, m_func, ...) NativeClasses::bind_data(_bind_static_func(m_name, get_class_name_s(), m_func, __VA_ARGS__))
#define BIND_STATIC_FUNC_VA(m_name, m_func) NativeClasses::bind_data(_bind_va_static_func(m_name, get_class_name_s(), m_func))
#define BIND_MEMBER(m_name, m_member, ...) NativeClasses::bind_data(_bind_member(m_name, get_class_name_s(), m_member, __VA_ARGS__))
#define BIND_STATIC_MEMBER(m_name, m_member) NativeClasses::bind_data(_bind_static_member(m_name, get_class_name_s(), m_member))
#define BIND_CONST(m_name, m_const) NativeClasses::bind_data(_bind_static_const(m_name, get_class_name_s(), m_const))
#define BIND_ENUM(m_name, ...) NativeClasses::bind_data(_bind_enum(m_name, get_class_name_s(), __VA_ARGS__));
#define BIND_ENUM_VALUE(m_name, m_value) NativeClasses::bind_data(newptr<EnumValueBind>(m_name, get_class_name_s(), m_value, newptr<EnumValueInfo>(m_name, m_value)));

//#define BIND_METHOD(m_name, m_method)
//#define BIND_METHOD_VA(m_name, m_method)
//#define BIND_STATIC_FUNC(m_name, m_func)
//#define BIND_STATIC_FUNC_VA(m_name, m_func)
//#define BIND_MEMBER(m_name, m_member)
//#define BIND_STATIC_MEMBER(m_name, m_member)
//#define BIND_CONST(m_name, m_const)
//#define BIND_ENUM(m_name, ...);
//#define BIND_ENUM_VALUE(m_name, m_value);

/* BIND_ENUM(m_name, ...) usage:
	BIND_ENUM("MyEnum", {
		{ "V1", MyEnum::V1 },
		{ "V2", MyEnum::V2 },
		{ "V3", MyEnum::V3 },
	});
*/

namespace carbon {

class NativeClasses {
	struct ClassEntries {
		String class_name;
		String parent_class_name;
		stdhashtable<size_t, ptr<BindData>> bind_data;
	};

private:
	static stdhashtable<size_t, ClassEntries> classes;

public:
	static void bind_data(ptr<BindData> p_bind_data);
	static ptr<BindData> get_bind_data(const String& cls, const String& attrib);
	static ptr<BindData> find_bind_data(const String& cls, const String& attrib);
	static void set_inheritance(const String& p_class_name, const String& p_parent_class_name);
	static String get_inheritance(const String& p_class_name);
	static bool is_class_registered(const String& p_class_name);
};

}

#endif // NATIVE_CLASSES_H