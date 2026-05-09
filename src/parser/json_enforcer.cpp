#include "parser/json_enforcer.h"

#include "parser/action_validation.h"
#include "parser/validation_helpers.h"

namespace kv {
namespace parser {

namespace {

/**
 * @brief Checks whether an op is an allowed semantic write operation.
 *
 * @param op Operation name from the parsed request.
 * @return true when op is a currently supported semantic write operation.
 */
bool IsValidSetOperation(const std::string& op) {
  return op == "log_step" || op == "set_state";
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
      RejectUnknownChildren(state, {"goal", "status", "context"}, "value.state");
  if (!result.ok) {
    return result;
  }

  result = RequireStringChild(state, "goal", "value.state");
  if (!result.ok) {
    return result;
  }

  result = RequireStringChild(state, "status", "value.state");
  if (!result.ok) {
    return result;
  }

  return RequireObjectChild(state, "context", "value.state");
}

/**
 * @brief Validates the value.metadata object in a set_action request.
 *
 * @param metadata Parsed value.metadata object.
 * @return Validation success or a field-specific error.
 */
ValidationResult ValidateMetadata(const nlohmann::json& metadata) {
  ValidationResult result = RejectUnknownChildren(
      metadata, {"provider", "model", "timestamp", "trace_id"},
      "value.metadata");
  if (!result.ok) {
    return result;
  }

  result = RequireStringChild(metadata, "provider", "value.metadata");
  if (!result.ok) {
    return result;
  }

  result = RequireStringChild(metadata, "model", "value.metadata");
  if (!result.ok) {
    return result;
  }

  result = RequireStringChild(metadata, "timestamp", "value.metadata");
  if (!result.ok) {
    return result;
  }

  return RequireStringChild(metadata, "trace_id", "value.metadata");
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

  // Keep this as the operation router so future commands can add their own
  // validators without weakening the semantic write contract.
  if (IsValidSetOperation(request.at("op").get<std::string>())) {
    return ValidateSetActionRequest(request);
  }

  return Error("Invalid op: expected semantic write operation");
}

ValidationResult JsonEnforcer::ValidateSetActionRequest(
    const nlohmann::json& request) const {
  if (!request.is_object()) {
    return Error("Invalid field type: request must be object");
  }

  // First reject extra children, then require each canonical child so missing
  // fields and unknown fields produce distinct errors.
  ValidationResult result =
      RejectUnknownChildren(request, {"op", "session_id", "event_id", "value"}, "");
  if (!result.ok) {
    return result;
  }

  result = RequireStringChild(request, "op", "");
  if (!result.ok) {
    return result;
  }

  if (!IsValidSetOperation(request.at("op").get<std::string>())) {
    return Error("Invalid op: expected semantic write operation");
  }

  result = RequireStringChild(request, "session_id", "");
  if (!result.ok) {
    return result;
  }
  result = RequireStringChild(request, "event_id", "");
  if (!result.ok) {
    return result;
  }
  result = RequireObjectChild(request, "value", "");
  if (!result.ok) {
    return result;
  }

  const nlohmann::json& value = request.at("value");
  // value is a strict envelope. Nested payload flexibility is handled by the
  // child validators rather than by accepting unknown fields here.
  result = RejectUnknownChildren(value, {"action", "state", "metadata"}, "value");
  if (!result.ok) {
    return result;
  }

  result = RequireObjectChild(value, "action", "value");
  if (!result.ok) {
    return result;
  }
  result = ValidateAction(value.at("action"));
  if (!result.ok) {
    return result;
  }

  result = RequireObjectChild(value, "state", "value");
  if (!result.ok) {
    return result;
  }

  result = ValidateState(value.at("state"));
  if (!result.ok) {
    return result;
  }

  result = RequireObjectChild(value, "metadata", "value");
  if (!result.ok) {
    return result;
  }

  return ValidateMetadata(value.at("metadata"));
}

}  // namespace parser
}  // namespace kv
