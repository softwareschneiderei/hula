#include <catch2/catch.hpp>
#include "device_server_spec.hpp"

using namespace toml::literals::toml_literals;

TEST_CASE("can_parse_scalar_attribute_type", "[attribute_type_t]")
{
  auto const input = u8R"("int32")"_toml;
  attribute_type_t parsed(input);
  REQUIRE(parsed.type == value_type::int32_t);
  REQUIRE(parsed.rank == rank_t::scalar);
  REQUIRE(parsed.max_size == std::array<std::uint32_t, 2>{});
}

TEST_CASE("can_parse_spectrum_attribute_type", "[attribute_type_t]")
{
  auto const input = u8R"("float[512]")"_toml;
  attribute_type_t parsed(input);
  REQUIRE(parsed.type == value_type::float_t);
  REQUIRE(parsed.rank == rank_t::vector);
  REQUIRE(parsed.max_size == std::array<std::uint32_t, 2>{512, 0});
}

TEST_CASE("can_parse_matrix_attribute_type", "[attribute_type_t]")
{
  auto const input = u8R"("double[800, 600]")"_toml;
  attribute_type_t parsed(input);
  REQUIRE(parsed.type == value_type::double_t);
  REQUIRE(parsed.rank == rank_t::matrix);
  REQUIRE(parsed.max_size == std::array<std::uint32_t, 2>{800, 600});
}

TEST_CASE("throws_with_empty_type_tag", "[attribute_type_t]")
{
  auto const input = u8R"("[800,600]")"_toml;
  REQUIRE_THROWS_AS(attribute_type_t{input}, std::invalid_argument);
}

TEST_CASE("throws_with_unparsable_vector", "[attribute_type_t]")
{
  REQUIRE_THROWS_AS(attribute_type_t{u8R"("int32[OO]")"_toml}, std::invalid_argument);
  REQUIRE_THROWS_AS(attribute_type_t{u8R"("float[2good]")"_toml}, std::invalid_argument);
}

TEST_CASE("throws_with_zero_vector", "[attribute_type_t]")
{
  REQUIRE_THROWS_AS(attribute_type_t{u8R"("string[0]")"_toml}, std::invalid_argument);
}

TEST_CASE("throws_with_unparsable_matrix", "[attribute_type_t]")
{
  REQUIRE_THROWS_AS(attribute_type_t{u8R"("bool[1,a]")"_toml}, std::invalid_argument);
  REQUIRE_THROWS_AS(attribute_type_t{u8R"("bool[1,2a]")"_toml}, std::invalid_argument);
  REQUIRE_THROWS_AS(attribute_type_t{u8R"("bool[2z,2]")"_toml}, std::invalid_argument);
  REQUIRE_THROWS_AS(attribute_type_t{u8R"("bool[z,2]")"_toml}, std::invalid_argument);
}

TEST_CASE("throws_with_zero_matrix", "[attribute_type_t]")
{
  REQUIRE_THROWS_AS(attribute_type_t{u8R"("int32[0,1]")"_toml}, std::invalid_argument);
  REQUIRE_THROWS_AS(attribute_type_t{u8R"("int32[1,0]")"_toml}, std::invalid_argument);
  REQUIRE_THROWS_AS(attribute_type_t{u8R"("int32[0,0]")"_toml}, std::invalid_argument);
}
