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

#ifndef GLOBALS_H
#define GLOBALS_H

namespace carbon {

class GlobalStrings {
#define _GLOBAL_STR(m_name) constexpr static const char* m_name = #m_name
public:
	// function names
	_GLOBAL_STR(main);
	_GLOBAL_STR(copy);
	_GLOBAL_STR(to_string);

	// operator function names
	_GLOBAL_STR(__call);
	_GLOBAL_STR(__iter_begin);
	_GLOBAL_STR(__iter_has_next);
	_GLOBAL_STR(__iter_next);
	_GLOBAL_STR(__get_mapped);
	_GLOBAL_STR(__set_mapped);
	_GLOBAL_STR(__hash);

	_GLOBAL_STR(__add);
	_GLOBAL_STR(__sub);
	_GLOBAL_STR(__mul);
	_GLOBAL_STR(__div);

	_GLOBAL_STR(__add_eq);
	_GLOBAL_STR(__sub_eq);
	_GLOBAL_STR(__mul_eq);
	_GLOBAL_STR(__div_eq);

	_GLOBAL_STR(__gt);
	_GLOBAL_STR(__lt);
	_GLOBAL_STR(__eq);
};
#undef _GLOBAL_STR
}

#endif // GLOBALS_H


