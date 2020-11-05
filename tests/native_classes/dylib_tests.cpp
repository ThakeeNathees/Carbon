#include "tests/carbon_tests.h"

TEST_CASE("[native_classes:dylib]") {

#if defined(PLATFORM_WINDOWS)
	DynamicLibrary dlib("bin/dynamic.dll");
#elif defined(PLATFORM_X11)
	DynamicLibrary lib("bin/dynamic.so");
#else
#error "dynamic library is not implemented in this platform yet"
#endif

	var i = 42, f = 3.14, s = "hello";
	int ret = 0;
	ret = dlib.call("r0_func_a0");
	CHECK(ret == 0);

	ret = dlib.call("ra1_func_a1", i);
	CHECK(ret == (int)i);

	dlib.close();

	var lib_path = "bin/dynamic.dll";
	var lib2 = NativeClasses::singleton()->construct(DynamicLibrary::get_class_name_s(), make_stdvec<var*>(&lib_path));
	ret = lib2.call_method("r0_func_a3", i, f, s); // lib2.call_method("call", "r0_func_a3", i, f, s);
	CHECK(ret == 0);
	lib2.call_method("close");

	NativeLib nlib("bin/native.dll");
	var a_instance = NativeClasses::singleton()->construct("Aclass");
	a_instance.call_method("a_method");
	
}

