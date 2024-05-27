#include <catch2/catch_test_macros.hpp>
#include "device_server_spec.hpp"

using namespace toml::literals::toml_literals;
using namespace std::string_literals;

TEST_CASE("can_parse_scalar_attribute_type", "[attribute_type_t]")
{
  attribute_type_t parsed("int32"s);
  REQUIRE(parsed.type == value_type::int32_t);
  REQUIRE(parsed.rank == attribute_rank_t::scalar);
  REQUIRE(parsed.max_size == std::array<std::uint32_t, 2>{});
}

TEST_CASE("can_parse_spectrum_attribute_type", "[attribute_type_t]")
{
  attribute_type_t parsed("float[512]"s);
  REQUIRE(parsed.type == value_type::float_t);
  REQUIRE(parsed.rank == attribute_rank_t::spectrum);
  REQUIRE(parsed.max_size == std::array<std::uint32_t, 2>{512, 0});
}

TEST_CASE("can_parse_matrix_attribute_type", "[attribute_type_t]")
{
  attribute_type_t parsed("double[800, 600]"s);
  REQUIRE(parsed.type == value_type::double_t);
  REQUIRE(parsed.rank == attribute_rank_t::image);
  REQUIRE(parsed.max_size == std::array<std::uint32_t, 2>{800, 600});
}

TEST_CASE("throws_with_empty_type_tag", "[attribute_type_t]")
{
  REQUIRE_THROWS_AS(attribute_type_t{"[800,600]"s}, std::invalid_argument);
}

TEST_CASE("throws_with_unparsable_vector", "[attribute_type_t]")
{
  REQUIRE_THROWS_AS(attribute_type_t{"int32[OO]"s}, std::invalid_argument);
  REQUIRE_THROWS_AS(attribute_type_t{"float[2good]"s}, std::invalid_argument);
}

TEST_CASE("throws_with_zero_vector", "[attribute_type_t]")
{
  REQUIRE_THROWS_AS(attribute_type_t{"string[0]"s}, std::invalid_argument);
}

TEST_CASE("throws_with_unparsable_matrix", "[attribute_type_t]")
{
  REQUIRE_THROWS_AS(attribute_type_t{"bool[1,a]"s}, std::invalid_argument);
  REQUIRE_THROWS_AS(attribute_type_t{"bool[1,2a]"s}, std::invalid_argument);
  REQUIRE_THROWS_AS(attribute_type_t{"bool[2z,2]"s}, std::invalid_argument);
  REQUIRE_THROWS_AS(attribute_type_t{"bool[z,2]"s}, std::invalid_argument);
}

TEST_CASE("throws_with_zero_matrix", "[attribute_type_t]")
{
  REQUIRE_THROWS_AS(attribute_type_t{"int32[0,1]"s}, std::invalid_argument);
  REQUIRE_THROWS_AS(attribute_type_t{"int32[1,0]"s}, std::invalid_argument);
  REQUIRE_THROWS_AS(attribute_type_t{"int32[0,0]"s}, std::invalid_argument);
}

TEST_CASE("can_parse_non_array_command_type", "[command_type_t]")
{
  command_type_t parsed{"string"s};
  REQUIRE(parsed.type == value_type::string_t);
  REQUIRE(parsed.is_array == false);
}

TEST_CASE("can_parse_array_command_type", "[command_type_t]")
{
  command_type_t parsed{"string[]"s};
  REQUIRE(parsed.type == value_type::string_t);
  REQUIRE(parsed.is_array == true);
}

TEST_CASE("command_type_throws_on_void_arrays", "[command_type_t]")
{
  REQUIRE_THROWS_AS(command_type_t{"void[]"s}, std::invalid_argument);
}

TEST_CASE("attribute_type_throws_on_void_arrays", "[attribute_type_t]")
{
  REQUIRE_THROWS_AS(attribute_type_t{"void[2]"s}, std::invalid_argument);
  REQUIRE_THROWS_AS(attribute_type_t{"void[3,5]"s}, std::invalid_argument);
}
