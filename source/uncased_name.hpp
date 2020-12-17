#pragma once
#include <vector>
#include <string>

/** A name to be used in the resulting code. Can be actualized in different naming styles, i.e. CamelCase or snake_case.
 */
class uncased_name
{
public:
  uncased_name() = default;
  explicit uncased_name(std::string const& str);

  [[nodiscard]] std::string snake_cased() const;
  [[nodiscard]] std::string camel_cased() const;
  [[nodiscard]] std::string dromedary_cased() const;

private:
  std::vector<std::string> parts_;
};