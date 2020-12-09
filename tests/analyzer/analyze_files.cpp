
#include "tests/carbon_tests.h"


TEST_CASE("[analyzer_tests]:analyze_files") {
	ptr<Parser> parser = newptr<Parser>();
	Analyzer analyzer;
	//String path, source;

	Array files = Array(make_stdvec<var>(
		"tests/test_files/z_function.cb",
		"tests/test_files/gcd_subset.cb",
		"tests/test_files/newman_conway.cb",
		"" // end with comma above
	));

	for (int i = 0; i < files.size() - 1; i++) {
		try {
			parser->parse(File(files[i], File::READ).read_text(), files[i]);
			analyzer.analyze(parser);
		} catch (Throwable& err) {
			CHECK_MESSAGE(false, err.what());
		}
	}

}