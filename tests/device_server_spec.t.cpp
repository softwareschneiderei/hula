#include <catch2/catch.hpp>
#include "device_server_spec.hpp"

using namespace toml::literals::toml_literals;

TEST_CASE("can_parse_scalar_attribute_type", "[device_server_spec]")
{
  const toml::value input = u8R"(
    type = "int32"
)"_toml;
  attribute_type_t parsed(input);
  REQUIRE(parsed.type == value_type::int32_t);
  REQUIRE(parsed.rank == rank_t::scalar);
  REQUIRE(parsed.max_size == std::array<std::uint32_t, 2>{});
}

TEST_CASE("can_parse_spectrum_attribute_type", "[device_server_spec]")
{
  const toml::value input = u8R"(
    type = "float[512]"
)"_toml;
  attribute_type_t parsed(input);
  REQUIRE(parsed.type == value_type::float_t);
  REQUIRE(parsed.rank == rank_t::vector);
  REQUIRE(parsed.max_size == std::array<std::uint32_t, 2>{512, 0});
}

TEST_CASE("can_parse_matrix_attribute_type", "[device_server_spec]")
{
  const toml::value input = u8R"(
    type = "double[800, 600]"
)"_toml;
  attribute_type_t parsed(input);
  REQUIRE(parsed.type == value_type::double_t);
  REQUIRE(parsed.rank == rank_t::matrix);
  REQUIRE(parsed.max_size == std::array<std::uint32_t, 2>{800, 600});
}
