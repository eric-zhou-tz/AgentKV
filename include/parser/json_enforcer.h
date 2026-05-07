#ifndef KV_STORE_PARSER_JSON_ENFORCER_H_
#define KV_STORE_PARSER_JSON_ENFORCER_H_

#include <string>

namespace kv {
namespace parser {

/**
 * @brief Validates raw JSON input before it is routed into AgentKV.
 *
 * JsonEnforcer is responsible for enforcing the outer JSON contract for
 * AgentKV JSON mode. In the current version, it only verifies that the raw
 * input is syntactically valid JSON. Full AgentKV canonical schema validation
 * will be added later.
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
};

}  // namespace parser
}  // namespace kv

#endif  // KV_STORE_PARSER_JSON_ENFORCER_H_
