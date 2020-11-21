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

#include "error.h"

namespace carbon {

#if DEBUG_BUILD
const char* Error::what() const noexcept {
	if (line.size() != 0 || pos != Vect2i(-1, -1)) {
		_what = String::format("\nERROR: %s: %s at: (%s:%i)\n   at: %s (%s:%i)\n%s\n%s",
			get_err_name(type).c_str(), msg.c_str(), file.c_str(), pos.x, __dbg_func__.c_str(), __dbg_file__.c_str(), __dbg_line__,
			line.c_str(), get_line_pos().c_str());
	} else {
		_what = String::format("\nERROR: %s: %s\n   at: %s (%s:%i)",
			get_err_name(type).c_str(), msg.c_str(), __dbg_func__.c_str(), __dbg_file__.c_str(), __dbg_line__);
	}
	return _what.c_str();
}
#else // TODO:
const char* Error::what() const noexcept {
	_what = String::format("\nERROR: %s: %s", get_err_name(type).c_str(), msg.c_str());
	return _what.c_str();
}
#endif // DEBUG_BUILD

String Error::get_line() const { 
	return line;
}
String Error::get_line_pos() const {
	std::stringstream ss_pos;
	size_t cur_col = 0;
	bool done = false;
	for (size_t i = 0; i < line.size(); i++) {
		cur_col++;
		if (cur_col == pos.y) {
			for (uint32_t i = 0; i < err_len; i++) {
				ss_pos << '^';
			}
			done = true;
			break;
		} else if (line[i] != '\t') {
			ss_pos << ' ';
		} else {
			ss_pos << '\t';
		}
	}
	if (!done) ss_pos << '^';
	return ss_pos.str();
}

static const char* _error_names[Error::_ERROR_MAX_] = {
	"OK",
	"BUG",
	"NULL_POINTER",
	"OPERATOR_NOT_SUPPORTED",
	"NOT_IMPLEMENTED",
	"ZERO_DIVISION",
	"TYPE_ERROR",
	"ATTRIBUTE_ERROR",
	"INVALID_ARGUMENT_COUNT",
	"INVALID_INDEX",
	"SYNTAX_ERROR",
	"ASSERTION",
	"UNEXPECTED_EOF",
	"NAME_ERROR",
	"IO_ERROR",
	//_ERROR_MAX_
};
MISSED_ENUM_CHECK(Error::Type::_ERROR_MAX_, 15);

String Error::get_err_name(Error::Type p_type) {
	return _error_names[p_type];
}

static const char* _warning_names[Warning::_WARNING_MAX_] = {
	"VARIABLE_SHADOWING",
	"MISSED_ENUM_IN_SWITCH",
	"NON_TERMINATING_LOOP",
	"UNREACHABLE_CODE",
	"STAND_ALONE_EXPRESSION",
	//_WARNING_MAX_
};
MISSED_ENUM_CHECK(Warning::Type::_WARNING_MAX_, 5);

String Warning::get_warning_name(Warning::Type p_type) {
	return _warning_names[p_type];
}

}