#include "parser/action_validation.h"

#include "parser/validation_helpers.h"

namespace kv {
namespace parser {

namespace {

/**
 * @brief Validates the common message interaction payload shape.
 *
 * @param interaction Parsed value.action.interaction object.
 * @return Validation success or a field-specific error.
 */
ValidationResult ValidateMessageContentInteraction(
    const nlohmann::json& interaction) {
  const std::string path = "value.action.interaction";
  if (!interaction.is_object()) {
    return Error("Invalid field type: " + path + " must be object");
  }

  ValidationResult result = RejectUnknownChildren(interaction, {"content"}, path);
  if (!result.ok) {
    return result;
  }

  return RequireStringChild(interaction, "content", path);
}

}  // namespace

ActionType ParseActionType(const std::string& type) {
  if (type == "user_message") {
    return ActionType::UserMessage;
  }

  if (type == "assistant_message") {
    return ActionType::AssistantMessage;
  }

  return ActionType::Invalid;
}

ValidationResult ValidateInteraction(ActionType action_type,
                                     const nlohmann::json& interaction) {
  switch (action_type) {
    case ActionType::UserMessage:
    case ActionType::AssistantMessage:
      return ValidateMessageContentInteraction(interaction);
    case ActionType::Invalid:
      return Error(
          "value.action.type must be one of: user_message, assistant_message");
  }

  return Error(
      "value.action.type must be one of: user_message, assistant_message");
}

ValidationResult ValidateAction(const nlohmann::json& action) {
  // The action wrapper is strict. The interaction payload is validated below
  // according to the parsed action type.
  ValidationResult result = RejectUnknownChildren(
      action, {"type", "name", "interaction"}, "value.action");
  if (!result.ok) {
    return result;
  }

  // Obtain type of action
  result = RequireStringChild(action, "type", "value.action");
  if (!result.ok) {
    return result;
  }

  const std::string type = action.at("type").get<std::string>();
  const ActionType action_type = ParseActionType(type);
  if (action_type == ActionType::Invalid) {
    return Error(
        "value.action.type must be one of: user_message, assistant_message");
  }

  // Obtain name (user defined)
  result = RequireStringChild(action, "name", "value.action");
  if (!result.ok) {
    return result;
  }

  // Parse type of interaction
  if (!action.contains("interaction")) {
    return Error("Missing required field: value.action.interaction");
  }
  const nlohmann::json& interaction = action.at("interaction");
  return ValidateInteraction(action_type, interaction);
}

}  // namespace parser
}  // namespace kv
