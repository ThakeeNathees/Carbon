
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

#ifndef BUILTIN_CLASSES_H
#define BUILTIN_CLASSES_H

#include "core.h"

namespace carbon {

class BuiltinTypes {
public:
	enum Type {
		UNKNOWN,

		_NULL,
		BOOL,
		INT,
		FLOAT,
		STRING,
		// Vectors are not exposed.
		//VECT2F,
		//VECT2I,
		//VECT3F,
		//VECT3I,
		ARRAY,
		MAP,
		//OBJECT, // Object is considered as Native.

		// Native classes.
		//BUFFER,
		//FILE,
		//DYNAMIC_LIBRARY,
		_TYPE_MAX_,
	};

	static String get_type_name(Type p_func) {
		return _type_list[p_func];
	}

	static Type get_type_type(const String& p_func) {
		for (const std::pair<Type, String>& pair : _type_list) {
			if (pair.second == p_func) {
				return pair.first;
			}
		}
		return BuiltinTypes::UNKNOWN;
	}

private:
	static stdmap<Type, String> _type_list;
};

}

#endif // BUILTIN_CLASSES_H
