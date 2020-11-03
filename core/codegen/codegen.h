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

#ifndef CODEGEN_H
#define CODEGEN_H

#include "analyzer/analyzer.h"

namespace carbon {

#define ADDR_BITS       32
#define ADDR_TYPE_BITS  8
#define ADDR_VALUE_BITS (ADDR_BITS - ADDR_TYPE_BITS)
#define ADDR_TYPE_MASK  (((1 << ADDR_TYPE_BITS) - 1) << ADDR_VALUE_BITS)
#define ADDR_VALUE_MASK ((1 << ADDR_VALUE_BITS) - 1)

#define ADDR_GET_TYPE (m_addr) ((m_addr & ADDR_TYPE_MASK) >> ADDR_VALUE_BITS)
#define ADDR_GET_VALUE(m_addr) (m_addr & ADDR_VALUE_MASK)

class CodeGen {
public:
	
};

}

#endif // CODEGEN_H
