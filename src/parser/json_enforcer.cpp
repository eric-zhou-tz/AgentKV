#include "parser/json_enforcer.h"

#include <nlohmann/json.hpp>

namespace kv {
namespace parser {

bool JsonEnforcer::IsValidJson(const std::string& raw_input) const {
  try {
    const auto parsed = nlohmann::json::parse(raw_input);
    (void)parsed;
    return true;
  } catch (const nlohmann::json::parse_error&) {
    return false;
  }
}

}  // namespace parser
}  // namespace kv
