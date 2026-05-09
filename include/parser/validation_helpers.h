#ifndef KV_STORE_PARSER_VALIDATION_HELPERS_H_
#define KV_STORE_PARSER_VALIDATION_HELPERS_H_

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_set>

#include "parser/json_enforcer.h"

namespace kv {
namespace parser {

/**
 * @brief Builds a successful validation result.
 *
 * @return ValidationResult with ok set to true and no error message.
 */
ValidationResult Ok();

/**
 * @brief Builds a failed validation result.
 *
 * @param message Specific validation failure message.
 * @return ValidationResult with ok set to false and the supplied message.
 */
ValidationResult Error(const std::string& message);

/**
 * @brief Rejects object keys outside a strict allowed set.
 *
 * @param object JSON object to inspect.
 * @param allowed_keys Keys allowed at this object level.
 * @param path Dotted field path for error messages.
 * @return Success when all keys are allowed, otherwise an unknown-field error.
 */
ValidationResult RejectUnknownChildren(
    const nlohmann::json& object,
    const std::unordered_set<std::string>& allowed_keys,
    const std::string& path);

/**
 * @brief Requires a child field to exist and be a JSON string.
 *
 * @param object Parent JSON object.
 * @param field Required child field name.
 * @param path Dotted parent path for error messages.
 * @return Success when the field exists and is a string.
 */
ValidationResult RequireStringChild(const nlohmann::json& object,
                                    const std::string& field,
                                    const std::string& path);

/**
 * @brief Requires a child field to exist and be a JSON object.
 *
 * @param object Parent JSON object.
 * @param field Required child field name.
 * @param path Dotted parent path for error messages.
 * @return Success when the field exists and is an object.
 */
ValidationResult RequireObjectChild(const nlohmann::json& object,
                                    const std::string& field,
                                    const std::string& path);

}  // namespace parser
}  // namespace kv

#endif  // KV_STORE_PARSER_VALIDATION_HELPERS_H_
