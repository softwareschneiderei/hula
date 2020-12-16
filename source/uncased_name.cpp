#include "uncased_name.hpp"
#include <sstream>

bool last_was_upper(std::string const& str, std::size_t pos) {
  auto last_pos = pos - 1;
  if (last_pos >= 0 && last_pos < str.size()) {
    return std::isupper(str[last_pos]);
  } else {
    return false;
  }
}

bool next_is_lower(std::string const& str, std::size_t pos) {
  auto next_pos = pos + 1;
  if (next_pos >= 0 && next_pos < str.size()) {
    return std::islower(str[next_pos]);
  } else {
    return false;
  }
}

uncased_name::uncased_name(std::string const& str)
{
  for (std::size_t i = 0; i < str.size(); ++i)
  {
    if (std::isupper(str[i]) && (!last_was_upper(str,i) || next_is_lower(str,i)))
    {
      parts_.emplace_back(1, static_cast<char>(std::tolower(str[i])));
      continue;
    }
    if (std::isupper(str[i]) && last_was_upper(str,i))
    {
      parts_.back().push_back(static_cast<char>(std::tolower(str[i])));
      continue;
    }
    if (str[i] == '_')
    {
      parts_.emplace_back();
      continue;
    }

    if (parts_.empty()) {
      parts_.emplace_back(1, str[i]);
    } else {
      parts_.back().push_back(str[i]);
    }
  }
}

std::string uncased_name::snake_cased() const
{
  if (parts_.empty())
    return "";

  std::ostringstream str;
  str << parts_.front();
  for (std::size_t i = 1; i < parts_.size(); ++i)
  {
    str << '_' << parts_[i];
  }

  return str.str();
}

std::string uncased_name::camel_cased() const
{
  if (parts_.empty())
    return "";

  std::ostringstream str;
  
  for (auto const& part : parts_)
  {
    if (part.empty())
      continue;

    str << static_cast<char>(std::toupper(part.front())) << (part.c_str() + 1);
  }

  return str.str();
}

std::string uncased_name::dromedary_cased() const
{
  if (parts_.empty())
    return "";

  std::ostringstream str;
  
  bool first = false;
  for (auto const& part : parts_)
  {
    if (part.empty())
      continue;

    if (!first)
    {
      str << part;
      first = true;
      continue;
    }

    str << static_cast<char>(std::toupper(part.front())) << (part.c_str() + 1);
  }

  return str.str();
}
