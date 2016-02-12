#include "util/csvparser.h"
#include "util/exceptions.h"

#include <gtest/gtest.h>
#include <string>
#include <sstream>

static void toCSV(std::stringstream& ss, const std::vector<std::vector<std::string>>& result, const std::string& delim, const std::string& endl) {
	for(auto fields : result ){
		ss << fields[0];
		for(size_t i = 1; i < fields.size(); ++i){
			ss << delim << fields[i];
		}
		ss << endl;
	}
}

static const std::vector<std::vector<std::string>> simple({{"a", "b", "c"}, {"testa1", "testb1", "testc1"}, {"d!\u00A7\00FC %&/()", "", "f"}});

static const std::vector<std::vector<std::string>> quotes({{"a", "b", "c"}, {"\"testa1\"", "\"testb \"\"1\"\"\"", "testc1"}, {"d!\u00A7\00FC %&/()", "", "f"}});


static std::vector<std::vector<std::string>> lineBreaksInQuotes(const std::string& endl){
	std::vector<std::vector<std::string>> result = {{"a", "b", "c"}, {"\"test" + endl + "a1\"", "\"testb" + endl + endl + "\"\"1\"\"" + endl + "\"", "testc1"}, {"d!\u00A7\00FC %&/()", "", "f"}};
	return result;
}

static std::vector<std::vector<std::string>> delimInQuotes(const std::string& delim){
	std::vector<std::vector<std::string>> result = {{"a", "b", "c"}, {"\"test" + delim + "a1\"", "\"testb" + delim + delim + "\"\"1\"\"" + delim + "\"", "testc1"}, {"d", "e", "f"}};
	return result;
}

static const std::vector<std::vector<std::string>> missingFields({{"a", "b", "c"}, {"d"}, {"e", "f", "g"}});

static const std::vector<std::vector<std::string>> tooManyFields({{"a", "b", "c"}, {"d", "e", "f", "g"}, {"h", "i", "j"}});

static void checkParseResult(CSVParser& parser, const std::vector<std::vector<std::string>> expected) {
	for(std::vector<std::string> fields : expected) {
		auto tuple = parser.readTuple();
		ASSERT_EQ(fields.size(), tuple.size());

		for(size_t i = 0; i < fields.size(); ++i) {
			ASSERT_EQ(fields[i], tuple[i]);
		}
	}
	// test if the file actually ended here
	auto tuple = parser.readTuple();
	ASSERT_EQ(0, tuple.size());
}


TEST(CSVParser, simpleComma) {
	std::string delim = ",";
	std::string endl = "\n";
	auto& input = simple;
	std::stringstream ss;
	toCSV(ss, input, delim, endl);
	CSVParser parser(ss, delim.at(0));
	checkParseResult(parser, input);
}

TEST(CSVParser, simpleSemicolon) {
	std::string delim = ";";
	std::string endl = "\n";
	auto& input = simple;
	std::stringstream ss;
	toCSV(ss, input, delim, endl);
	CSVParser parser(ss, delim.at(0));
	checkParseResult(parser, input);
}

TEST(CSVParser, simpleCommaCRLF) {
	std::string delim = ",";
	std::string endl = "\r\n";
	auto& input = simple;
	std::stringstream ss;
	toCSV(ss, input, delim, endl);
	CSVParser parser(ss, delim.at(0));
	checkParseResult(parser, input);
}

TEST(CSVParser, simpleSemicolonCRLF) {
	std::string delim = ";";
	std::string endl = "\r\n";
	auto& input = simple;
	std::stringstream ss;
	toCSV(ss, input, delim, endl);
	CSVParser parser(ss, delim.at(0));
	checkParseResult(parser, input);
}

TEST(CSVParser, DISABLED_simpleWrongDelim) {
	std::string delim = ";";
	std::string endl = "\n";
	auto& input = simple;
	std::stringstream ss;
	toCSV(ss, input, delim, endl);
	CSVParser parser(ss, ',');
	checkParseResult(parser, input);
}

TEST(CSVParser, quotes) {
	std::string delim = ",";
	std::string endl = "\n";
	auto& input = quotes;
	std::stringstream ss;
	toCSV(ss, input, delim, endl);
	CSVParser parser(ss, delim.at(0));
	checkParseResult(parser, input);
}

TEST(CSVParser, lineBreaksLF) {
	std::string delim = ",";
	std::string endl = "\n";
	auto input = lineBreaksInQuotes(endl);
	std::stringstream ss;
	toCSV(ss, input, delim, endl);
	CSVParser parser(ss, delim.at(0));
	checkParseResult(parser, input);

}

TEST(CSVParser, lineBreaksCRLF) {
	std::string delim = ",";
	std::string endl = "\r\n";
	auto input = lineBreaksInQuotes(endl);
	std::stringstream ss;
	toCSV(ss, input, delim, endl);
	CSVParser parser(ss, delim.at(0));
	checkParseResult(parser, input);
}

TEST(CSVParser, delimInQuotesComma) {
	std::string delim = ",";
	std::string endl = "\n";
	auto input = delimInQuotes(delim);
	std::stringstream ss;
	toCSV(ss, input, delim, endl);
	CSVParser parser(ss, delim.at(0));
	checkParseResult(parser, input);
}

TEST(CSVParser, delimInQuotesSemicolon) {
	std::string delim = ";";
	std::string endl = "\n";
	auto input = delimInQuotes(delim);
	std::stringstream ss;
	toCSV(ss, input, delim, endl);
	CSVParser parser(ss, delim.at(0));
	checkParseResult(parser, input);
}

TEST(CSVParser, missingFields) {
	std::string delim = ",";
	std::string endl = "\n";
	auto& input = missingFields;
	std::stringstream ss;
	toCSV(ss, input, delim, endl);
	CSVParser parser(ss, delim.at(0));
	EXPECT_THROW(checkParseResult(parser, input), ArgumentException);
}

TEST(CSVParser, tooManyFieldsFields) {
	std::string delim = ",";
	std::string endl = "\n";
	auto& input = tooManyFields;
	std::stringstream ss;
	toCSV(ss, input, delim, endl);
	CSVParser parser(ss, delim.at(0));
	EXPECT_THROW(checkParseResult(parser, input), ArgumentException);
}

