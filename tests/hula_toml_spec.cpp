#include <catch2/catch.hpp>
#include "device_server_spec.hpp"

using namespace toml::literals::toml_literals;

TEST_CASE("device specification without properties, attributes or commands can be parsed") {
  const toml::value device = u8R"(
    name = "cool_device"
)"_toml;
  raw_device_server_spec device_spec(device);
  REQUIRE(device_spec.name.snake_cased() == "cool_device");
  REQUIRE(device_spec.device_properties.empty());
  REQUIRE(device_spec.attributes.empty());
  REQUIRE(device_spec.commands.empty());
}

TEST_CASE("device specification contains defined properties, attributes and commands") {
  const toml::value device = u8R"(
    name = "cool_device"
    [[device_properties]]
    name = "address"
    type = "string"

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
  REQUIRE(device_spec.name.snake_cased() == "cool_device");
  REQUIRE(device_spec.device_properties.size() == 1);
  REQUIRE(device_spec.attributes.size() == 3);
  REQUIRE(device_spec.commands.size() == 2);
}
