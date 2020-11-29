#include "tests/carbon_tests.h"

class _TestClass_A : public Object {
	REGISTER_CLASS(_TestClass_A, Object) {
		BIND_STATIC_FUNC("A_static_func", &_TestClass_A::A_static_func);
		BIND_METHOD("A_virtual_func", &_TestClass_A::A_virtual_func);

		BIND_MEMBER("member", &_TestClass_A::member);
		BIND_STATIC_MEMBER("static_member", &_TestClass_A::static_member);
		BIND_CONST("_const", &_TestClass_A::_const);
	}
public:
	static int A_static_func() { return 1; }
	virtual int A_virtual_func() { return 2; }

	var member = Array(make_stdvec<var>(1, "2", 3.0));
	static var static_member;
	static const int _const;
};
var _TestClass_A::static_member = "static member";
const int _TestClass_A::_const = 42;

class _TestClass_B1 : public _TestClass_A {
	REGISTER_CLASS(_TestClass_B1, _TestClass_A) {
		BIND_STATIC_FUNC("A_static_func", &_TestClass_B1::A_static_func);
		BIND_METHOD("A_virtual_func", &_TestClass_B1::A_virtual_func);
	}
public:
	static int A_static_func() { return 3; }
	virtual int A_virtual_func() override { return 4; }
};
class _TestClass_B2 : public _TestClass_A {
	REGISTER_CLASS(_TestClass_B2, _TestClass_A) {
		BIND_STATIC_FUNC("B2_static_func", &_TestClass_B2::B2_static_func);
		BIND_METHOD("B2_member_func", &_TestClass_B2::B2_member_func, PARAMS("str"));
	}
public:
	static String B2_static_func() { return "B2-static"; }
	void B2_member_func(const String& p_str) {}
};

class _TestClass_C : public _TestClass_B2 {
	REGISTER_CLASS(_TestClass_C, _TestClass_B2) {
		BIND_METHOD("C_member_func", &_TestClass_C::C_member_func, PARAMS("v"));
		BIND_METHOD("C_default_arg_func", &_TestClass_C::C_default_arg_func, PARAMS("a0", "a1", "a2"), DEFVALUES(3.14, "defval"));
	}
public:
	var C_member_func(var v) { return v; }
	void C_default_arg_func(int a0, double a1 = 3.14, String a2 = "defarg") { }
};
/////////////////////////////////////////////////////////////////

static void register_classes() {
	static bool registered = false;
	if (registered) return;
	NativeClasses::singleton()->register_class<_TestClass_A>();
	NativeClasses::singleton()->register_class<_TestClass_B1>();
	NativeClasses::singleton()->register_class<_TestClass_B2>();
	NativeClasses::singleton()->register_class<_TestClass_C>();
	registered = true;
}

// POSITIVE TESTS
TEST_CASE("[native_classes]:method_bind+") {

	var a  = newptr<_TestClass_A>();
	var b1 = newptr<_TestClass_B1>();
	var b2 = newptr<_TestClass_B2>();
	var c  = newptr<_TestClass_C>();
	register_classes();

	var r;
	r = call_method(a, "A_virtual_func");
	REQUIRE(r.get_type() == var::INT);
	CHECK(r.operator int() == 2);

	{
		CHECK(a.get_member("member").operator Array() == Array(make_stdvec<var>(1, "2", 3.0)));

		CHECK(a.get_member("static_member").operator String() == "static member");
		ptr<BindData> bd = NativeClasses::singleton()->find_bind_data(a.get_type_name(), "static_member");
		REQUIRE(bd != nullptr);
		REQUIRE(bd->get_type() == BindData::STATIC_VAR);
		CHECK(ptrcast<StaticPropertyBind>(bd)->get() == "static member");
	}
	{
		CHECK(c.get_member("_const") == 42);
		ptr<BindData> bd = NativeClasses::singleton()->find_bind_data(c.get_type_name(), "_const");
		REQUIRE(bd != nullptr);
		REQUIRE(bd->get_type() == BindData::STATIC_CONST);
		CHECK(ptrcast<ConstantBind>(bd)->get() == 42);
	}

	r = call_method(b1, "A_virtual_func");
	REQUIRE(r.get_type() == var::INT);
	CHECK(r.operator int() == 4);
	r = call_method(b2, "A_virtual_func");
	REQUIRE(r.get_type() == var::INT);
	CHECK(r.operator int() == 2);

	r = call_method(a, "A_static_func");
	REQUIRE(r.get_type() == var::INT);
	CHECK(r.operator int() == 1);

	r = call_method(b1, "A_static_func");
	REQUIRE(r.get_type() == var::INT);
	CHECK(r.operator int() == 3);

	r = call_method(b2, "B2_static_func");
	REQUIRE(r.get_type() == var::STRING);
	CHECK(r.to_string() == "B2-static");

	Array arr(make_stdvec<var>(1, 2.1, String("test")));
	r = call_method(c, "C_member_func", arr);
	REQUIRE(r.get_type() == var::ARRAY);
	CHECK(r.operator Array()[0].operator int() == 1);
	CHECK(r.operator Array()[1].operator double() == 2.1);
	CHECK(r.operator Array()[2].to_string() == "test");

	// default value tests
	call_method(c, "C_default_arg_func", 42);
	call_method(c, "C_default_arg_func", 42, 3.14);
	call_method(c, "C_default_arg_func", 42, 3.14, "str");
	CHECK_THROWS(call_method(c, "C_default_arg_func", 42, "invalid type")); // INVALID TYPE.

	// built in types ///////////////////

	var str = "hello world!";
	r = call_method(str, "substr", 0, 2);
	REQUIRE(r.get_type() == var::STRING);
	CHECK(r.operator String() == "he");
	
	r = call_method(r, "hash");
	REQUIRE(r.get_type() == var::INT);
	CHECK(628906390544363382l == r.operator int64_t());
}

TEST_CASE("[native_classes]:method_bind-") {
	var c = newptr<_TestClass_C>();
	var b2 = newptr<_TestClass_B2>();
	register_classes();

	CHECK_THROWS_ERR(Error::ATTRIBUTE_ERROR, call_method(c, "blah blah..."));
	CHECK_THROWS_ERR(Error::INVALID_ARG_COUNT, call_method(c, "C_member_func"));
}