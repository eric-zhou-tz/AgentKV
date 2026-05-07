#include "parser/json_enforcer.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <functional>
#include <string>
#include <vector>

namespace {

using kv::parser::JsonEnforcer;
using nlohmann::json;

json ValidSetActionRequest() {
  return {
      {"op", "set_action"},
      {"session_id", "sess_001"},
      {"event_id", "event_001"},
      {"value",
       {
           {"action",
            {
                {"type", "tool_call"},
                {"name", "search_flights"},
                {"interaction", json::object()},
            }},
           {"state",
            {
                {"goal", "book flight to Iceland"},
                {"status", "checking_options"},
                {"context", json::object()},
            }},
           {"metadata",
            {
                {"provider", "openai"},
                {"model", "gpt-5.5"},
                {"timestamp", "2026-05-07T07:55:00Z"},
                {"trace_id", "trace_abc"},
            }},
       }},
  };
}

void ExpectValid(const json& request) {
  const JsonEnforcer enforcer;
  const kv::parser::ValidationResult result =
      enforcer.ValidateRequest(request);

  EXPECT_TRUE(result.ok);
  EXPECT_TRUE(result.error.empty());
}

void ExpectInvalid(const json& request, const std::string& expected_message_part) {
  const JsonEnforcer enforcer;
  const kv::parser::ValidationResult result =
      enforcer.ValidateRequest(request);

  EXPECT_FALSE(result.ok);
  EXPECT_FALSE(result.error.empty());
  EXPECT_NE(std::string::npos, result.error.find(expected_message_part))
      << result.error;
}

TEST(JsonEnforcerTest, AcceptsMinimalValidSetActionWithEmptyPayloadObjects) {
  ExpectValid(ValidSetActionRequest());
}

TEST(JsonEnforcerTest, AcceptsNestedArbitraryPayloadInsideInteraction) {
  json request = ValidSetActionRequest();
  request["value"]["action"]["interaction"] = {
      {"tool", "google_flights"},
      {"query", {
                    {"from", "ORD"},
                    {"to", "KEF"},
                }},
  };

  ExpectValid(request);
}

TEST(JsonEnforcerTest, AcceptsNestedArbitraryPayloadInsideContext) {
  json request = ValidSetActionRequest();
  request["value"]["state"]["context"] = {
      {"preferred_airlines", json::array({"Icelandair"})},
      {"max_budget", 700},
      {"constraints", {
                          {"nonstop", true},
                          {"latest_arrival_hour", 19},
                      }},
  };

  ExpectValid(request);
}

TEST(JsonEnforcerTest, RejectsInvalidSetActionContractsWithSpecificErrors) {
  struct InvalidCase {
    std::string name;
    std::function<void(json&)> mutate;
    std::string expected_message_part;
  };

  const std::vector<InvalidCase> cases = {
      {"missing op",
       [](json& request) { request.erase("op"); },
       "op"},
      {"op is not a string",
       [](json& request) { request["op"] = 123; },
       "op"},
      {"op is unsupported",
       [](json& request) { request["op"] = "get_action"; },
       "op"},
      {"missing session_id",
       [](json& request) { request.erase("session_id"); },
       "session_id"},
      {"session_id is not a string",
       [](json& request) { request["session_id"] = 123; },
       "session_id"},
      {"missing event_id",
       [](json& request) { request.erase("event_id"); },
       "event_id"},
      {"event_id is not a string",
       [](json& request) { request["event_id"] = 123; },
       "event_id"},
      {"missing value",
       [](json& request) { request.erase("value"); },
       "value"},
      {"value is not an object",
       [](json& request) { request["value"] = "not an object"; },
       "value"},
      {"missing value.action",
       [](json& request) { request["value"].erase("action"); },
       "value.action"},
      {"value.action is not an object",
       [](json& request) { request["value"]["action"] = "not an object"; },
       "value.action"},
      {"missing value.action.type",
       [](json& request) { request["value"]["action"].erase("type"); },
       "value.action.type"},
      {"value.action.type is not a string",
       [](json& request) { request["value"]["action"]["type"] = 123; },
       "value.action.type"},
      {"missing value.action.name",
       [](json& request) { request["value"]["action"].erase("name"); },
       "value.action.name"},
      {"value.action.name is not a string",
       [](json& request) { request["value"]["action"]["name"] = 123; },
       "value.action.name"},
      {"missing value.action.interaction",
       [](json& request) { request["value"]["action"].erase("interaction"); },
       "value.action.interaction"},
      {"value.action.interaction is not an object",
       [](json& request) {
         request["value"]["action"]["interaction"] = "not an object";
       },
       "value.action.interaction"},
      {"missing value.state",
       [](json& request) { request["value"].erase("state"); },
       "value.state"},
      {"value.state is not an object",
       [](json& request) { request["value"]["state"] = "not an object"; },
       "value.state"},
      {"missing value.state.goal",
       [](json& request) { request["value"]["state"].erase("goal"); },
       "value.state.goal"},
      {"value.state.goal is not a string",
       [](json& request) { request["value"]["state"]["goal"] = 123; },
       "value.state.goal"},
      {"missing value.state.status",
       [](json& request) { request["value"]["state"].erase("status"); },
       "value.state.status"},
      {"value.state.status is not a string",
       [](json& request) { request["value"]["state"]["status"] = 123; },
       "value.state.status"},
      {"missing value.state.context",
       [](json& request) { request["value"]["state"].erase("context"); },
       "value.state.context"},
      {"value.state.context is not an object",
       [](json& request) {
         request["value"]["state"]["context"] = "not an object";
       },
       "value.state.context"},
      {"missing value.metadata",
       [](json& request) { request["value"].erase("metadata"); },
       "value.metadata"},
      {"value.metadata is not an object",
       [](json& request) { request["value"]["metadata"] = "not an object"; },
       "value.metadata"},
      {"missing value.metadata.provider",
       [](json& request) { request["value"]["metadata"].erase("provider"); },
       "value.metadata.provider"},
      {"value.metadata.provider is not a string",
       [](json& request) { request["value"]["metadata"]["provider"] = 123; },
       "value.metadata.provider"},
      {"missing value.metadata.model",
       [](json& request) { request["value"]["metadata"].erase("model"); },
       "value.metadata.model"},
      {"value.metadata.model is not a string",
       [](json& request) { request["value"]["metadata"]["model"] = 123; },
       "value.metadata.model"},
      {"missing value.metadata.timestamp",
       [](json& request) { request["value"]["metadata"].erase("timestamp"); },
       "value.metadata.timestamp"},
      {"value.metadata.timestamp is not a string",
       [](json& request) { request["value"]["metadata"]["timestamp"] = 123; },
       "value.metadata.timestamp"},
      {"missing value.metadata.trace_id",
       [](json& request) { request["value"]["metadata"].erase("trace_id"); },
       "value.metadata.trace_id"},
      {"value.metadata.trace_id is not a string",
       [](json& request) { request["value"]["metadata"]["trace_id"] = 123; },
       "value.metadata.trace_id"},
      {"unknown top-level field",
       [](json& request) { request["foo"] = "bar"; },
       "foo"},
      {"unknown value field",
       [](json& request) { request["value"]["extra_field"] = true; },
       "value.extra_field"},
      {"unknown value.action field",
       [](json& request) {
         request["value"]["action"]["extra_field"] = true;
       },
       "value.action.extra_field"},
      {"unknown value.state field",
       [](json& request) {
         request["value"]["state"]["extra_field"] = true;
       },
       "value.state.extra_field"},
      {"unknown value.metadata field",
       [](json& request) {
         request["value"]["metadata"]["extra_field"] = true;
       },
       "value.metadata.extra_field"},
  };

  for (const InvalidCase& invalid_case : cases) {
    SCOPED_TRACE(invalid_case.name);
    json request = ValidSetActionRequest();
    invalid_case.mutate(request);

    ExpectInvalid(request, invalid_case.expected_message_part);
  }
}

TEST(JsonEnforcerTest, RejectsTopLevelJsonValueThatIsNotAnObject) {
  ExpectInvalid(json::array(), "request");
}

}  // namespace
