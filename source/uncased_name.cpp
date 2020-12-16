#include "uncased_name.hpp"
#include <sstream>

namespace {

bool last_was_upper(std::string const& str, std::size_t pos) {
  return (pos >= 1) && std::isupper(str[pos - 1]);
}

bool last_was_underscore(std::string const& str, std::size_t pos) {
  return (pos >= 1) && str[pos - 1] == '_';
}

bool next_is_lower(std::string const& str, std::size_t pos) {
  return (pos + 1 < str.size()) && std::islower(str[pos + 1]);
}

bool part_starts_at(std::string const& str, std::size_t pos)
{
  return (str[pos] != '_')
         && (std::isupper(str[pos])
             && (!last_was_upper(str,pos) || next_is_lower(str,pos))
             || pos == 0
             || last_was_underscore(str,pos));
}

bool part_continues_at(std::string const& str, std::size_t pos)
{
  return std::isupper(str[pos]) && last_was_upper(str,pos) && !next_is_lower(str,pos)
         || std::islower(str[pos]);
}

} // namespace

uncased_name::uncased_name(std::string const& str)
{
  for (std::size_t i = 0; i < str.size(); ++i)
  {
    if (part_starts_at(str, i))
    {
      parts_.emplace_back(1, static_cast<char>(std::tolower(str[i])));
      continue;
    }
    if (part_continues_at(str, i))
    {
      parts_.back().push_back(static_cast<char>(std::tolower(str[i])));
      continue;
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
