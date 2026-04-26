#include "command/command.h"

#include <optional>
#include <stdexcept>
#include <string>

namespace kv {
namespace command {

namespace {

/**
 * @brief Builds a standard success response for commands that target one key.
 *
 * @param action Executed action name.
 * @param key Resolved KV key targeted by the action.
 * @return JSON response containing the success contract fields.
 */
Json MakeOkResponse(const std::string& action, const std::string& key) {
  return Json{
      {"ok", true},
      {"action", action},
      {"key", key},
  };
}

/**
 * @brief Builds a standard not-found response for read-style commands.
 *
 * @param action Executed action name.
 * @param key Resolved KV key targeted by the action.
 * @return JSON response describing a missing key lookup.
 */
Json MakeNotFoundResponse(const std::string& action, const std::string& key) {
  return Json{
      {"ok", false},
      {"action", action},
      {"key", key},
      {"error", "not found"},
  };
}

/**
 * @brief Returns a required parameter from the request.
 *
 * @param params Action parameters object.
 * @param field Required field name.
 * @return Referenced JSON field.
 * @throws std::invalid_argument When the field is missing.
 */
const Json& RequireParam(const Json& params, const std::string& field) {
  if (!params.contains(field)) {
    throw std::invalid_argument("request.params." + field + " is required");
  }
  return params.at(field);
}

/**
 * @brief Returns a required string parameter from the request.
 *
 * @param params Action parameters object.
 * @param field Required string field name.
 * @return Extracted string value.
 * @throws std::invalid_argument When the field is missing or not a string.
 */
std::string RequireString(const Json& params, const std::string& field) {
  const Json& value = RequireParam(params, field);
  if (!value.is_string()) {
    throw std::invalid_argument("request.params." + field +
                                " must be a string");
  }
  return value.get<std::string>();
}

/**
 * @brief Returns a required step identifier suitable for a hierarchical key.
 *
 * @param params Action parameters object.
 * @return Step identifier rendered as a string path segment.
 * @throws std::invalid_argument When the identifier is missing or unsupported.
 */
std::string RequireStepId(const Json& params) {
  const Json& step_id = RequireParam(params, "step_id");
  if (step_id.is_number_integer()) {
    return std::to_string(step_id.get<long long>());
  }
  if (step_id.is_number_unsigned()) {
    return std::to_string(step_id.get<unsigned long long>());
  }
  if (step_id.is_string()) {
    return step_id.get<std::string>();
  }
  throw std::invalid_argument(
      "request.params.step_id must be a string or integer");
}

/**
 * @brief Serializes a direct KV value for the generic put action.
 *
 * Strings are stored verbatim, while all other JSON values are stored as their
 * serialized JSON representation.
 *
 * @param value Request value to store.
 * @return Value string to hand to the generic KV layer.
 */
std::string SerializePutValue(const Json& value) {
  return value.is_string() ? value.get<std::string>() : value.dump();
}

/**
 * @brief Attempts to decode a stored JSON blob for agent-facing responses.
 *
 * Stored memory and run-state values are JSON objects serialized into the KV
 * store. If a value is not valid JSON, the raw string is returned instead.
 *
 * @param stored Persisted value string.
 * @return Parsed JSON value or the original string when parsing fails.
 */
Json DecodeStoredJson(const std::string& stored) {
  try {
    return Json::parse(stored);
  } catch (const Json::parse_error&) {
    return stored;
  }
}

/**
 * @brief Builds the key used to persist a workflow step log entry.
 *
 * @param params Action parameters object.
 * @return Hierarchical step key for the run and step identifier.
 */
std::string BuildLogStepKey(const Json& params) {
  return "runs/" + RequireString(params, "run_id") + "/steps/" +
         RequireStepId(params);
}

/**
 * @brief Builds the key used to persist agent memory.
 *
 * @param params Action parameters object.
 * @return Hierarchical memory key for the requested memory identifier.
 */
std::string BuildMemoryKey(const Json& params) {
  return "memory/" + RequireString(params, "memory_id");
}

/**
 * @brief Builds the key used to persist workflow run state.
 *
 * @param params Action parameters object.
 * @return Hierarchical state key for the requested run identifier.
 */
std::string BuildRunStateKey(const Json& params) {
  return "runs/" + RequireString(params, "run_id") + "/state";
}

}  // namespace

Json execute_command(const Json& request, store::KVStore& kv) {
  const std::string action = request.at("action").get<std::string>();
  const Json& params = request.at("params");

  if (action == "put") {
    const std::string key = RequireString(params, "key");
    const Json& value = RequireParam(params, "value");
    kv.Set(key, SerializePutValue(value));
    return MakeOkResponse(action, key);
  }

  if (action == "get") {
    const std::string key = RequireString(params, "key");
    const std::optional<std::string> value = kv.Get(key);
    if (!value.has_value()) {
      return MakeNotFoundResponse(action, key);
    }

    Json response = MakeOkResponse(action, key);
    response["value"] = *value;
    return response;
  }

  if (action == "delete") {
    const std::string key = RequireString(params, "key");
    kv.Delete(key);
    return MakeOkResponse(action, key);
  }

  if (action == "log_step") {
    const std::string key = BuildLogStepKey(params);
    kv.Set(key, params.dump());
    return MakeOkResponse(action, key);
  }

  if (action == "save_memory") {
    const std::string key = BuildMemoryKey(params);
    kv.Set(key, params.dump());
    return MakeOkResponse(action, key);
  }

  if (action == "get_memory") {
    const std::string key = BuildMemoryKey(params);
    const std::optional<std::string> value = kv.Get(key);
    if (!value.has_value()) {
      return MakeNotFoundResponse(action, key);
    }

    Json response = MakeOkResponse(action, key);
    response["value"] = DecodeStoredJson(*value);
    return response;
  }

  if (action == "save_run_state") {
    const std::string key = BuildRunStateKey(params);
    kv.Set(key, params.dump());
    return MakeOkResponse(action, key);
  }

  if (action == "get_run_state") {
    const std::string key = BuildRunStateKey(params);
    const std::optional<std::string> value = kv.Get(key);
    if (!value.has_value()) {
      return MakeNotFoundResponse(action, key);
    }

    Json response = MakeOkResponse(action, key);
    response["value"] = DecodeStoredJson(*value);
    return response;
  }

  throw std::invalid_argument("unsupported action: " + action);
}

}  // namespace command
}  // namespace kv
