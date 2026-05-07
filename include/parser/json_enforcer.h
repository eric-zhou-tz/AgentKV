#ifndef KV_STORE_PARSER_JSON_ENFORCER_H_
#define KV_STORE_PARSER_JSON_ENFORCER_H_

#include <nlohmann/json.hpp>

#include <string>

namespace kv {
namespace parser {

/**
 * @brief Result returned by JSON contract validation.
 */
struct ValidationResult {
  /** @brief True when validation succeeds. */
  bool ok = false;
  /** @brief Specific validation error when validation fails. */
  std::string error;
};

/**
 * @brief Validates raw JSON input before it is routed into AgentKV.
 *
 * JsonEnforcer is responsible for enforcing the outer JSON contract for
 * AgentKV JSON mode. It can verify raw syntax and validate the parsed
 * canonical request shape before any command is routed into storage.
 */
class JsonEnforcer {
 public:
  /**
   * @brief Checks whether raw input is syntactically valid JSON.
   *
   * @param raw_input Raw stdin payload received by main in --json mode.
   * @return true if raw_input parses as valid JSON, false otherwise.
   */
  bool IsValidJson(const std::string& raw_input) const;

  /**
   * @brief Validates a parsed AgentKV JSON request.
   *
   * @param request Parsed JSON request object.
   * @return Validation success or a specific contract error.
   */
  ValidationResult ValidateRequest(const nlohmann::json& request) const;

  /**
   * @brief Validates a parsed canonical set_action request.
   *
   * @param request Parsed JSON request object.
   * @return Validation success or a specific contract error.
   */
  ValidationResult ValidateSetActionRequest(
      const nlohmann::json& request) const;
};

}  // namespace parser
}  // namespace kv

#endif  // KV_STORE_PARSER_JSON_ENFORCER_H_
