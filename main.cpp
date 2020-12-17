#include "device_server_spec.hpp"
#include "types.hpp"
#include "code_generator.hpp"
#include <fmt/format.h>
#include <iostream>
#include <filesystem>
#include <unordered_set>


void check_names(std::vector<device_server_spec> const& spec_list)
{
  std::unordered_set<std::string> seen_names;
  for (auto const& spec : spec_list)
  {
    auto name = spec.name.snake_cased();
    auto [_, inserted] = seen_names.insert(name);
    if (!inserted)
    {
      throw std::invalid_argument(fmt::format("Duplicated name: \"{0}\"", name));
    }
  }
}


int run(int argc, char* argv[])
{
  if (argc < 3)
  {
    fmt::print("{0} <spec> (<spec> ...) <output-path>\n", argv[0]);
    return EXIT_FAILURE;
  }

  // The number of specs
  auto const N = argc - 2;
  std::filesystem::path output_path{argv[argc-1]};
  if (!exists(output_path))
  {
    fmt::print("Output path {0} does not exist\n", output_path.string());
    return EXIT_FAILURE;
  }

  std::vector<device_server_spec> spec_list;
  for (int i = 0; i < N; ++i)
  {
    auto input = toml::parse(argv[i+1]);
    auto spec = toml::get<device_server_spec>(input);
    spec_list.push_back(spec);
  }

  check_names(spec_list);
  generate_code(spec_list, output_path);

  return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
  if (argc == 0)
  {
    return EXIT_FAILURE;
  }

  try
  {
    return run(argc, argv);
  }
  catch (std::exception const& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
