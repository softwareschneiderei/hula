#include "device_server_spec.hpp"

namespace
{
  bool contains(std::vector<toml::value> const& parts, std::string const& key)
  {
    return std::find_if(parts.begin(), parts.end(), [&](toml::value const& lhs)
    {
      return lhs.as_string() == key;
    }) != parts.end();
  }
}

access_type toml::from<access_type>::from_toml(value const& v)
{
  auto const& parts = v.as_array();
  auto readable = contains(parts, "read");
  auto writable = contains(parts, "write");
  if (readable)
  {
    if (writable)
      return access_type::read_write;
    return access_type::read_only;
  }
  if (writable)
    return access_type::write_only;
  // default to read only..
  return access_type::read_only;
}

attribute_type_t::attribute_type_t(toml::value const& rhs)
{
  // Need an explicit type here to convert from toml::string to std::string
  std::string const& type_code = rhs.as_string();

  auto suffix_begin = type_code.find('[');
  if (suffix_begin == std::string::npos)
  {
    this->type = from_input_type(type_code);
    return;
  }

  auto suffix_end = type_code.find(']', suffix_begin);
  if (suffix_end == std::string::npos)
  {
    throw std::invalid_argument("Malformed attribute type suffix: no end brace.");
  }

  // TODO: Throws if starts with suffix?
  auto type_tag = type_code.substr(0, suffix_begin);
  this->type = from_input_type(type_tag);
  auto suffix = type_code.substr(suffix_begin+1, suffix_end-suffix_begin-1);
  
  auto separator = suffix.find(',');
  if (separator == std::string::npos)
  {
    // TODO: throw if content not parsable
    std::size_t count = 0;
    max_size[0] = std::stoi(suffix, &count);
    rank = rank_t::vector;
    return;
  }
  
  auto first = suffix.substr(0, separator);
  auto second = suffix.substr(separator+1);

  // TODO: throw if either content not parsable
  this->rank = rank_t::matrix;
  std::size_t count = 0;
  max_size[0] = std::stoi(first, &count);
  max_size[1] = std::stoi(second, &count);
}
