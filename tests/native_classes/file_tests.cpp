#include "tests/carbon_tests.h"

#define TEST_FILE_PATH "tests/native_classes/test_files/test_file"
#define TEST_BIN_FILE_PATH "tests/native_classes/test_files/test_bin_file"

// POSITIVE TESTS
TEST_CASE("[native_classes:file]") {

	// native file test
	File file1;
	file1.open(TEST_FILE_PATH, File::WRITE | File::EXTRA);
	file1.write("FILE1\nLINE2");
	String read1 = file1.read();
	file1.close();
	CHECK(read1 == "FILE1\nLINE2");

	file1.open(TEST_BIN_FILE_PATH, File::WRITE | File::BINARY | File::EXTRA);
	ptr<Buffer> buff = newptr<Buffer>();
	buff->alloc(5);
	for (int i = 0; i < 5; i++) {
		buff->front()[i] = i;
	}
	file1.write(buff);
	buff = nullptr;
	buff = file1.read().cast_to<Buffer>();
	for (int i = 0; i < 5; i++) {
		CHECK(buff->front()[i] == i);
	}
	file1.close();

	// bind method test
	var file2 = newptr<File>();
	file2.call_method("open", TEST_FILE_PATH, File::APPEND);
	String read2 = file2.call_method("write", "\nappended by FILE2.");
	file2.call_method("close");
	
	file2.call_method("open", TEST_FILE_PATH, File::READ);
	read2 = file2.call_method("read").to_string();
	file2.call_method("close");
	CHECK(read2 == "FILE1\nLINE2\nappended by FILE2.");
}

// NEGATIVE TESTS
// TODO: