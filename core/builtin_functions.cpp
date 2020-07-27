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

#include "builtin_functions.h"

namespace carbon {

const char* BuiltinFunctions::get_func_name(BuiltinFunctions::Type p_func) {
	static const char* func_names[] = {
		"print",
		"input",
		"min",
		"max",
		"pow",
		nullptr,
	};
	return func_names[(int)p_func];
}
MISSED_ENUM_CHECK(BuiltinFunctions::Type::_FUNC_MAX_, 6);

void BuiltinFunctions::call(Type p_func, const stdvec<var>& p_args, var& r_ret) {
	switch (p_func) {
		case Type::PRINT: {
			for (int i = 0; i < p_args.size(); i++) {
				// printf is faster than std::cout << ...
				printf("%s", p_args[i].operator String().c_str());
			}
			printf("\n");
		} break;
		case Type::INPUT: {
			// Not safe to use scanf() possibly lead to buffer overflow.
			String input;
			std::cin >> input;
			r_ret = input;
		} break;
		case Type::MATH_MAX: {
			if (p_args.size() <= 1) throw Error(Error::INVALID_ARG_COUNT, "Expected at least 2 argument.");
			var min = p_args[0];
			for (int i = 1; i < p_args.size(); i++) {
				if (p_args[i] < min) {
					min = p_args[i];
				}
			}
			r_ret = min;
		} break;
		case Type::MATH_MIN: {
			if (p_args.size() <= 1) throw Error(Error::INVALID_ARG_COUNT, "Expected at least 2 argument.");
			var max = p_args[0];
			for (int i = 1; i < p_args.size(); i++) {
				if (p_args[i] > max) {
					max = p_args[i];
				}
			}
			r_ret = max;
		} break;
		case Type::MATH_POW: {
			if (p_args.size() != 2) throw Error(Error::INVALID_ARG_COUNT, "Expected exactly 2 arguments.");
			if (p_args[0].get_type() != var::INT && p_args[1].get_type() != var::FLOAT)
				throw Error(Error::INVALID_ARGUMENT, "Expected a numeric value at argument 0.");
			if (p_args[1].get_type() != var::INT && p_args[1].get_type() != var::FLOAT)
				throw Error(Error::INVALID_ARGUMENT, "Expected a numeric value at argument 1.");
			r_ret = pow(p_args[0].operator double(), p_args[1].operator double());
		} break;

	}
	MISSED_ENUM_CHECK(BuiltinFunctions::Type::_FUNC_MAX_, 6);
}

}