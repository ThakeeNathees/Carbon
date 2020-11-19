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

#ifndef VM_H
#define VM_H

#include "core.h"
#include "runtime_instance.h"
#include "binary/carbon_function.h"
#include "binary/bytecode.h"
#include "binary/carbon_ref.h"

namespace carbon {

class Stack {
private:
	stdvec<var> _stack;
	uint32_t _sp = 0; // stack pointer

public:
	Stack(uint32_t p_max_size = 0) { _stack.resize(p_max_size); }
	var* get_at(uint32_t p_pos) {
		THROW_INVALID_INDEX(_stack.size(), p_pos);
		return &_stack[p_pos];
	}
};

struct RuntimeContext {
	VM* vm = nullptr;
	Stack* stack = nullptr;
	var self;
	var bytecode;

	Bytecode* _file = nullptr;

	void init();
	var* get_var_at(const Address& p_addr);
};

class VM {

public:
	int run(ptr<Bytecode> bytecode, stdvec<String> args);

private:
	friend struct RuntimeContext;
	var call_carbon_function(const CarbonFunction* p_func, ptr<Bytecode> p_bytecode, ptr<RuntimeInstance> p_self, stdvec<var*> p_args);

	var* _get_native_ref(const String& p_name);
	var* _get_builtin_func_ref(uint32_t p_type);
	var* _get_builtin_type_ref(uint32_t p_type);

	stdmap<String, var> _native_ref;
	stdmap<uint32_t, var> _builtin_func_ref;
	stdmap<uint32_t, var> _builtin_type_ref;
};

}

#endif // VM_H