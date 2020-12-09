
#include "tests/carbon_tests.h"

TEST_CASE("[codeten_tests]:codegen_tests") {
	ptr<Parser> parser = newptr<Parser>();
	ptr<Analyzer> analyzer = newptr<Analyzer>();
	CodeGen codegen;
	ptr<Bytecode> bytecode;

	CHECK_NOTHROW__CODEGEN(R"(
	var x = "some string";
	const C = "another string";
	func f(arg, arg2 = 2) { var x = arg; }
)");
	CHECK(bytecode->get_static_var("x") != nullptr); // but value of v is null till runtime.
	CHECK(bytecode->get_constant("C") == "another string");
	ptr<CarbonFunction> f = bytecode->get_function("f");
	CHECK(f != nullptr);
	CHECK(f->get_arg_count() == 2);
	CHECK(f->get_default_args().size() == 1);
	CHECK(f->get_default_args()[0] == 2);

	{
		// stack = parameter;
		auto opcodes = f->get_opcodes();
		CHECK(opcodes[0] == Opcode::ASSIGN);
		CHECK(Address(opcodes[1]).get_type() == Address::STACK);
		CHECK(Address(opcodes[2]).get_type() == Address::PARAMETER);
	}

	CHECK_NOTHROW__CODEGEN(R"(
	class B : A {}
	class A {}
)");
	CHECK(bytecode->get_class("B")->get_base_binary().get() == bytecode->get_class("A").get());

}