#include "parser/json_enforcer.h"

#include <unordered_set>

namespace kv {
namespace parser {

namespace {

/**
 * @brief Builds a successful validation result.
 *
 * @return ValidationResult with ok set to true and no error message.
 */
ValidationResult Ok() {
  return {true, ""};
}

/**
 * @brief Builds a failed validation result.
 *
 * @param message Specific validation failure message.
 * @return ValidationResult with ok set to false and the supplied message.
 */
ValidationResult Error(const std::string& message) {
  return {false, message};
}

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

/**
 * @brief Rejects object keys outside a strict allowed set.
 *
 * @param object JSON object to inspect.
 * @param allowed_keys Keys allowed at this object level.
 * @param path Dotted field path for error messages.
 * @return Success when all keys are allowed, otherwise an unknown-field error.
 */
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

/**
 * @brief Requires a child field to exist and be a JSON string.
 *
 * @param object Parent JSON object.
 * @param field Required child field name.
 * @param path Dotted parent path for error messages.
 * @return Success when the field exists and is a string.
 */
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

/**
 * @brief Requires a child field to exist and be a JSON object.
 *
 * @param object Parent JSON object.
 * @param field Required child field name.
 * @param path Dotted parent path for error messages.
 * @return Success when the field exists and is an object.
 */
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

/**
 * @brief Validates the value.action object in a set_action request.
 *
 * @param action Parsed value.action object.
 * @return Validation success or a field-specific error.
 */
ValidationResult ValidateAction(const nlohmann::json& action) {
  // The action wrapper is strict, but interaction remains the open-ended
  // payload object where tool-specific details can live.
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

/**
 * @brief Validates the value.state object in a set_action request.
 *
 * @param state Parsed value.state object.
 * @return Validation success or a field-specific error.
 */
ValidationResult ValidateState(const nlohmann::json& state) {
  // The state wrapper is strict, while context remains the open-ended durable
  // state payload that later replay/debugging features can inspect.
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

/**
 * @brief Validates the value.metadata object in a set_action request.
 *
 * @param metadata Parsed value.metadata object.
 * @return Validation success or a field-specific error.
 */
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

/**
 * @brief Checks whether raw input parses as syntactically valid JSON.
 *
 * @param raw_input Raw stdin payload received by main in JSON mode.
 * @return true when parsing succeeds, false when nlohmann reports parse_error.
 */
bool JsonEnforcer::IsValidJson(const std::string& raw_input) const {
  try {
    const auto parsed = nlohmann::json::parse(raw_input);
    (void)parsed;
    return true;
  } catch (const nlohmann::json::parse_error&) {
    return false;
  }
}

/**
 * @brief Dispatches a parsed request to the validator for its operation.
 *
 * @param request Parsed JSON request object.
 * @return Validation success or a specific operation/contract error.
 */
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

  // Keep this as the operation router so future commands can add their own
  // validators without weakening the set_action contract.
  if (request.at("op").get<std::string>() == "set_action") {
    return ValidateSetActionRequest(request);
  }

  return Error("Invalid op: expected set_action");
}

/**
 * @brief Validates the canonical AgentKV set_action request shape.
 *
 * @param request Parsed JSON request object.
 * @return Validation success or a field-specific contract error.
 */
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
  // value is a strict envelope. Only interaction and context below are allowed
  // to carry arbitrary nested payloads.
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
