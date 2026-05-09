#include "parser/validation_helpers.h"

namespace kv {
namespace parser {

namespace {

/**
 * @brief Appends a child key to a dotted JSON field path.
 *
 * @param path Current field path, or empty for the top level.
 * @param key Child field name to append.
 * @return Dotted field path such as value.action.name.
 */
std::string JoinPath(const std::string& path, const std::string& key) {
  if (path.empty()) {
    return key;
  }
  return path + "." + key;
}

}  // namespace

ValidationResult Ok() {
  return {true, ""};
}

ValidationResult Error(const std::string& message) {
  return {false, message};
}

ValidationResult RejectUnknownChildren(
    const nlohmann::json& object,
    const std::unordered_set<std::string>& allowed_keys,
    const std::string& path) {
  for (const auto& item : object.items()) {
    if (allowed_keys.find(item.key()) == allowed_keys.end()) {
      return Error("Unknown field: " + JoinPath(path, item.key()));
    }
  }

  return Ok();
}

ValidationResult RequireStringChild(const nlohmann::json& object,
                                    const std::string& field,
                                    const std::string& path) {
  const std::string field_path = JoinPath(path, field);
  if (!object.contains(field)) {
    return Error("Missing required field: " + field_path);
  }

  if (!object.at(field).is_string()) {
    return Error("Invalid field type: " + field_path + " must be string");
  }

  return Ok();
}

ValidationResult RequireObjectChild(const nlohmann::json& object,
                                    const std::string& field,
                                    const std::string& path) {
  const std::string field_path = JoinPath(path, field);
  if (!object.contains(field)) {
    return Error("Missing required field: " + field_path);
  }

  if (!object.at(field).is_object()) {
    return Error("Invalid field type: " + field_path + " must be object");
  }

  return Ok();
}

}  // namespace parser
}  // namespace kv
