#include "test_runtime_check.hpp"

#include "t81/cli/artifact_family.hpp"
#include "t81/tracing/canonhash.hpp"

#include <map>
#include <string>
#include <string_view>

namespace {

std::string make_ref(std::string_view label) {
  return "sha3-256:" + t81::hash::hash_string(label).to_string();
}

bool contains(std::string_view text, std::string_view pattern) {
  return text.find(pattern) != std::string_view::npos;
}

}  // namespace

int main() {
  using namespace t81::cli::artifact_family;

  T81_TEST_CHECK(is_supported_schema(assess_record_schema()));
  T81_TEST_CHECK(is_supported_schema(route_bundle_schema()));
  T81_TEST_CHECK(is_supported_schema(classify_record_schema()));
  T81_TEST_CHECK(!is_supported_schema("t81.ai.task.answer-fixed.v1"));

  const auto assess_required = required_fields_for_schema(assess_record_schema());
  T81_TEST_CHECK(assess_required.size() == 8);
  T81_TEST_CHECK(assess_required.front() == "source_result_ref");
  T81_TEST_CHECK(assess_required.back() == "action_ref");

  const auto classify_required = required_fields_for_schema(classify_record_schema());
  T81_TEST_CHECK(classify_required.size() == 6);
  T81_TEST_CHECK(classify_required[4] == "selected_rule_set");
  T81_TEST_CHECK(classify_required[5] == "rule_set_ref");

  T81_TEST_CHECK(is_supported_field(classify_record_schema(), "selected_rule_set"));
  T81_TEST_CHECK(!is_supported_field(classify_record_schema(), "selected_path"));
  T81_TEST_CHECK(is_supported_field(classify_bundle_schema(), "record_ref"));
  T81_TEST_CHECK(!is_supported_field(classify_bundle_schema(), "rule_set_ref"));

  const std::string result_ref = make_ref("result");
  const std::string provenance_ref = make_ref("provenance");
  const std::string action_ref = make_ref("action");
  const std::string record_ref = make_ref("record");

  const std::map<std::string, std::string> classify_fields{
      {"source_result_ref", result_ref},
      {"source_provenance_ref", provenance_ref},
      {"label", "POSITIVE"},
      {"termination_reason", "single_step_max_score"},
      {"selected_rule_set", "positive-default"},
      {"rule_set_ref", action_ref},
  };
  const std::string classify_record = render_artifact(classify_record_schema(), classify_fields);
  T81_TEST_CHECK(contains(classify_record,
                          "\"schema\": \"t81.ai.task.classify-fixed.rule-selection-record.v1\""));
  T81_TEST_CHECK(contains(classify_record, "\"selected_rule_set\": \"positive-default\""));

  std::string validation_error;
  T81_TEST_CHECK(validate_artifact(classify_record_schema(), classify_record, validation_error));
  T81_TEST_CHECK(validation_error.empty());

  const std::string malformed_record =
      "{\n"
      "  \"schema\": \"t81.ai.task.classify-fixed.rule-selection-record.v1\",\n"
      "  \"source_result_ref\": \"" +
      result_ref +
      "\",\n"
      "  \"source_provenance_ref\": \"" + provenance_ref +
      "\",\n"
      "  \"label\": \"POSITIVE\",\n"
      "  \"termination_reason\": \"single_step_max_score\",\n"
      "  \"selected_rule_set\": \"positive-default\",\n"
      "  \"rule_set_ref\": \"not-a-canonfs-ref\"\n"
      "}\n";
  validation_error.clear();
  T81_TEST_CHECK(!validate_artifact(classify_record_schema(), malformed_record, validation_error));
  T81_TEST_CHECK(contains(validation_error, "invalid CanonFS ref in field \"rule_set_ref\""));

  const std::string missing_field_record =
      "{\n"
      "  \"schema\": \"t81.ai.task.classify-fixed.rule-selection-record.v1\",\n"
      "  \"source_result_ref\": \"" +
      result_ref +
      "\",\n"
      "  \"source_provenance_ref\": \"" + provenance_ref +
      "\",\n"
      "  \"label\": \"POSITIVE\",\n"
      "  \"termination_reason\": \"single_step_max_score\",\n"
      "  \"rule_set_ref\": \"" +
      action_ref + "\"\n"
      "}\n";
  validation_error.clear();
  T81_TEST_CHECK(
      !validate_artifact(classify_record_schema(), missing_field_record, validation_error));
  T81_TEST_CHECK(contains(validation_error, "missing required field \"selected_rule_set\""));

  const std::map<std::string, std::string> bundle_fields{
      {"source_result_ref", result_ref},
      {"source_provenance_ref", provenance_ref},
      {"action_ref", action_ref},
      {"record_ref", record_ref},
  };
  const std::string bundle = render_artifact(classify_bundle_schema(), bundle_fields);
  validation_error.clear();
  T81_TEST_CHECK(validate_artifact(classify_bundle_schema(), bundle, validation_error));
  T81_TEST_CHECK(validation_error.empty());

  // Consumer flow starts from the bundle and should fail closed before any
  // deeper dereference if the canonical refs are malformed.
  const std::string malformed_bundle_record_ref =
      "{\n"
      "  \"schema\": \"t81.ai.task.classify-fixed.bundle.v1\",\n"
      "  \"source_result_ref\": \"" +
      result_ref +
      "\",\n"
      "  \"source_provenance_ref\": \"" + provenance_ref +
      "\",\n"
      "  \"action_ref\": \"" + action_ref +
      "\",\n"
      "  \"record_ref\": \"not-a-canonfs-ref\"\n"
      "}\n";
  validation_error.clear();
  T81_TEST_CHECK(
      !validate_artifact(classify_bundle_schema(), malformed_bundle_record_ref, validation_error));
  T81_TEST_CHECK(contains(validation_error, "invalid CanonFS ref in field \"record_ref\""));

  const std::string malformed_bundle_action_ref =
      "{\n"
      "  \"schema\": \"t81.ai.task.classify-fixed.bundle.v1\",\n"
      "  \"source_result_ref\": \"" +
      result_ref +
      "\",\n"
      "  \"source_provenance_ref\": \"" + provenance_ref +
      "\",\n"
      "  \"action_ref\": \"not-a-canonfs-ref\",\n"
      "  \"record_ref\": \"" +
      record_ref + "\"\n"
      "}\n";
  validation_error.clear();
  T81_TEST_CHECK(
      !validate_artifact(classify_bundle_schema(), malformed_bundle_action_ref, validation_error));
  T81_TEST_CHECK(contains(validation_error, "invalid CanonFS ref in field \"action_ref\""));

  T81_TEST_CHECK(!validate_artifact(assess_record_schema(), bundle, validation_error));
  T81_TEST_CHECK(next_inspect_field_for_schema(classify_record_schema()) == "selected_rule_set");
  T81_TEST_CHECK(next_inspect_field_for_schema(classify_bundle_schema()) == "record_ref");

  const std::string supported_suffix = supported_schema_error_suffix();
  T81_TEST_CHECK(contains(supported_suffix, "t81.ai.task.assess-fixed.host-action-record.v1"));
  T81_TEST_CHECK(contains(supported_suffix, "t81.ai.task.classify-fixed.bundle.v1"));

  return 0;
}
