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

#ifndef FILE_H
#define FILE_H

#include "native_classes.h"
#include "buffer.h"

namespace carbon {

class File : public Object {
	INHERITS_OBJECT(File, Object);
public:

	enum {
		READ   = 1 << 0, // "r"
		WRITE  = 1 << 1, // "w"
		APPEND = 1 << 2, // "a"
		BINARY = 1 << 3, // "b"
		EXTRA  = 1 << 4, // "+"
	};

	File();
	~File();


	// Methods.
	static void _bind_data();

	inline bool is_open() const { return _file != NULL; }
	void open(const String& p_path, int p_mode = APPEND | EXTRA);
	void close();
	long size();
	String get_path() const { return path; }
	int get_mode() const { return mode; }


	String read_text();
	void write_text(const String& p_text);

	ptr<Buffer> read_bytes();
	void write_bytes(const ptr<Buffer>& p_bytes);

	var read();
	void write(const var& p_what);


protected:

private:
	//std::fstream file;
	FILE* _file = NULL;
	String path;
	int mode = APPEND | EXTRA;

};
}

#endif // FILE_H

