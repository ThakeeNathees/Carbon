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

#ifndef OS_H
#define OS_H

#include "core.h"

namespace carbon {

class OS : public Object {
	REGISTER_CLASS(OS, Object) {
		BIND_STATIC_FUNC("abs_path", &OS::abs_path, PARAMS("path"));
		BIND_STATIC_FUNC("rel_path", &OS::rel_path, PARAMS("path", "to"), DEFVALUES(""));
	}

public:
	static String abs_path(const String& p_path) {
		return std::filesystem::absolute(p_path.operator std::string()).string();
	}
	static String rel_path(const String& p_path, const String& p_to = "") {
		std::filesystem::path to;
		if (p_to == "") to = std::filesystem::current_path();
		else to = p_to.operator std::string();
		return std::filesystem::relative(p_path.operator std::string(), to).string();
	}

};

}
#endif // OS_H