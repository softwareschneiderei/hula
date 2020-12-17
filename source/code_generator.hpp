#pragma once
#include "device_server_spec.hpp"
#include <filesystem>
#include <vector>

void generate_code(std::vector<device_server_spec> const& spec_list,
  std::filesystem::path const& output_path);
