#ifndef KV_STORE_PARSER_ACTION_VALIDATION_H_
#define KV_STORE_PARSER_ACTION_VALIDATION_H_

#include <nlohmann/json.hpp>

#include <string>

#include "parser/json_enforcer.h"

namespace kv {
namespace parser {

/**
 * @brief Canonical AgentKV action types accepted by value.action.type.
 */
enum class ActionType {
  UserMessage,
  AssistantMessage,
  Invalid,
};

/**
 * @brief Parses a canonical action type string.
 *
 * @param type Raw value.action.type string from the request.
 * @return Parsed ActionType, or ActionType::Invalid for unsupported values.
 */
ActionType ParseActionType(const std::string& type);

/**
 * @brief Validates an interaction payload for a parsed action type.
 *
 * @param action_type Parsed action type controlling the interaction schema.
 * @param interaction Parsed value.action.interaction object.
 * @return Validation success or a field-specific error.
 */
ValidationResult ValidateInteraction(ActionType action_type,
                                     const nlohmann::json& interaction);

/**
 * @brief Validates the value.action object in a set_action request.
 *
 * @param action Parsed value.action object.
 * @return Validation success or a field-specific error.
 */
ValidationResult ValidateAction(const nlohmann::json& action);

}  // namespace parser
}  // namespace kv

#endif  // KV_STORE_PARSER_ACTION_VALIDATION_H_
