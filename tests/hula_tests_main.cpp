#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "uncased_name.hpp"

TEST_CASE("snake_case names can be parsed")
{
  uncased_name x("this_was_snake_case");

  SECTION("and output as snake_case")
  {
    REQUIRE(x.snake_cased() == "this_was_snake_case");
  }

  SECTION("and output as CamelCase")
  {
    REQUIRE(x.camel_cased() == "ThisWasSnakeCase");
  }
}

TEST_CASE("All_caps_abbreviation_is_parsed_as_a_single_word")
{
    REQUIRE(uncased_name("ABBR").snake_cased() == "abbr");
}

TEST_CASE("All_caps_abbreviation_ends_at_CamelCase_word_boundary")
{
    REQUIRE(uncased_name("ABBRe").snake_cased() == "abb_re");
}

TEST_CASE("Numbers_are_allowed_in_names")
{
  REQUIRE(uncased_name("Test4Numbers").snake_cased() == "test_4_numbers");
}

TEST_CASE("Double_underscore_yields_the_same_as_single_underscore")
{
    REQUIRE(uncased_name("a__b").snake_cased() == "a_b");
}

TEST_CASE("CamelCase names can be parsed")
{
  uncased_name x("UsedToBeCamelCase");

  SECTION("and output as snake_case")
  {
    REQUIRE(x.snake_cased() == "used_to_be_camel_case");
  }
  SECTION("and output as CamelCase")
  {
    REQUIRE(x.camel_cased() == "UsedToBeCamelCase");
  }
}
