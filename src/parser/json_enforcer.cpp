#include "parser/json_enforcer.h"

#include <unordered_set>

namespace kv {
namespace parser {

namespace {

ValidationResult Ok() {
  return {true, ""};
}

ValidationResult Error(const std::string& message) {
  return {false, message};
}

std::string JoinPath(const std::string& path, const std::string& key) {
  if (path.empty()) {
    return key;
  }
  return path + "." + key;
}

ValidationResult HasOnlyKeys(
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

ValidationResult RequireString(const nlohmann::json& object,
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

ValidationResult RequireObject(const nlohmann::json& object,
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

ValidationResult ValidateAction(const nlohmann::json& action) {
  ValidationResult result = HasOnlyKeys(
      action, {"type", "name", "interaction"}, "value.action");
  if (!result.ok) {
    return result;
  }

  result = RequireString(action, "type", "value.action");
  if (!result.ok) {
    return result;
  }

  result = RequireString(action, "name", "value.action");
  if (!result.ok) {
    return result;
  }

  return RequireObject(action, "interaction", "value.action");
}

ValidationResult ValidateState(const nlohmann::json& state) {
  ValidationResult result =
      HasOnlyKeys(state, {"goal", "status", "context"}, "value.state");
  if (!result.ok) {
    return result;
  }

  result = RequireString(state, "goal", "value.state");
  if (!result.ok) {
    return result;
  }

  result = RequireString(state, "status", "value.state");
  if (!result.ok) {
    return result;
  }

  return RequireObject(state, "context", "value.state");
}

ValidationResult ValidateMetadata(const nlohmann::json& metadata) {
  ValidationResult result =
      HasOnlyKeys(metadata, {"provider", "model", "timestamp", "trace_id"},
                  "value.metadata");
  if (!result.ok) {
    return result;
  }

  result = RequireString(metadata, "provider", "value.metadata");
  if (!result.ok) {
    return result;
  }

  result = RequireString(metadata, "model", "value.metadata");
  if (!result.ok) {
    return result;
  }

  result = RequireString(metadata, "timestamp", "value.metadata");
  if (!result.ok) {
    return result;
  }

  return RequireString(metadata, "trace_id", "value.metadata");
}

}  // namespace

bool JsonEnforcer::IsValidJson(const std::string& raw_input) const {
  try {
    const auto parsed = nlohmann::json::parse(raw_input);
    (void)parsed;
    return true;
  } catch (const nlohmann::json::parse_error&) {
    return false;
  }
}

ValidationResult JsonEnforcer::ValidateRequest(
    const nlohmann::json& request) const {
  if (!request.is_object()) {
    return Error("Invalid field type: request must be object");
  }

  if (!request.contains("op")) {
    return Error("Missing required field: op");
  }

  if (!request.at("op").is_string()) {
    return Error("Invalid field type: op must be string");
  }

  if (request.at("op").get<std::string>() == "set_action") {
    return ValidateSetActionRequest(request);
  }

  return Error("Invalid op: expected set_action");
}

ValidationResult JsonEnforcer::ValidateSetActionRequest(
    const nlohmann::json& request) const {
  if (!request.is_object()) {
    return Error("Invalid field type: request must be object");
  }

  ValidationResult result =
      HasOnlyKeys(request, {"op", "session_id", "event_id", "value"}, "");
  if (!result.ok) {
    return result;
  }

  result = RequireString(request, "op", "");
  if (!result.ok) {
    return result;
  }

  if (request.at("op").get<std::string>() != "set_action") {
    return Error("Invalid op: expected set_action");
  }

  result = RequireString(request, "session_id", "");
  if (!result.ok) {
    return result;
  }

  result = RequireString(request, "event_id", "");
  if (!result.ok) {
    return result;
  }

  result = RequireObject(request, "value", "");
  if (!result.ok) {
    return result;
  }

  const nlohmann::json& value = request.at("value");
  result = HasOnlyKeys(value, {"action", "state", "metadata"}, "value");
  if (!result.ok) {
    return result;
  }

  result = RequireObject(value, "action", "value");
  if (!result.ok) {
    return result;
  }

  result = ValidateAction(value.at("action"));
  if (!result.ok) {
    return result;
  }

  result = RequireObject(value, "state", "value");
  if (!result.ok) {
    return result;
  }

  result = ValidateState(value.at("state"));
  if (!result.ok) {
    return result;
  }

  result = RequireObject(value, "metadata", "value");
  if (!result.ok) {
    return result;
  }

  return ValidateMetadata(value.at("metadata"));
}

}  // namespace parser
}  // namespace kv
