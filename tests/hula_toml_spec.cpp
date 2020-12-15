#include <catch2/catch.hpp>
#include "device_server_spec.hpp"

using namespace toml::literals::toml_literals;

TEST_CASE("can_parse_just_a_name") {
  const toml::value device = u8R"(
    name = "cool_device"
)"_toml;
  raw_device_server_spec device_spec(device);
  REQUIRE(device_spec.name.snake_cased() == "cool_device");
}

TEST_CASE("can_parse_device_properties") {
  const toml::value device = u8R"(
    name = "cool_device"
    [[device_properties]]
    name = "address"
    type = "string"
)"_toml;
  raw_device_server_spec device_spec(device);
  REQUIRE(device_spec.device_properties.size() == 1);
}

TEST_CASE("can_parse_commands") {
  const toml::value device = u8R"(
    name = "cool_device"
    [[commands]]
    name = "record"
    return_type = "void"
    parameter_type = "void"

    [[commands]]
    name = "square"
    return_type = "int32"
    parameter_type = "int32"
)"_toml;
  raw_device_server_spec device_spec(device);
  REQUIRE(device_spec.commands.size() == 2);
}

TEST_CASE("can_parse_device_attributes") {
  const toml::value device = u8R"(
    name = "cool_device"

    [[attributes]]
    name = "binning"
    type = "int32"
    access = ["read", "write"]

    [[attributes]]
    name = "exposure_time"
    type = "float"

    [[attributes]]
    name = "notes"
    type = "string"
    access = ["read", "write"]
)"_toml;
  raw_device_server_spec device_spec(device);
  REQUIRE(device_spec.attributes.size() == 3);
}
