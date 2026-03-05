// T81 Axion Policy Hooks for AI Events - RFC-00A6 Task 8
// Extends Axion policy system with AI-specific event handling and audit logging

#include <t81/core.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <chrono>
#include <nlohmann/json.hpp>

namespace t81::ai::policy_hooks {

enum class AIEventType {
    MODEL_LOAD,
    MODEL_UNLOAD,
    INFERENCE_START,
    INFERENCE_COMPLETE,
    TOOL_USE_START,
    TOOL_USE_COMPLETE,
    QUANTIZATION_START,
    QUANTIZATION_COMPLETE,
    POLICY_VIOLATION
};

enum class PolicyDecision {
    ALLOW,
    DENY,
    LOG_ONLY,
    REQUIRE_APPROVAL
};

struct AIEvent {
    std::string event_id;
    AIEventType event_type;
    std::string timestamp;
    std::string model_id;
    std::string user_id;
    std::string session_id;
    std::map<std::string, std::string> event_data;
    std::map<std::string, std::string> context;
};

struct PolicyRule {
    std::string rule_id;
    std::string name;
    std::string description;
    std::vector<AIEventType> applicable_events;
    std::map<std::string, std::string> conditions;
    PolicyDecision default_decision;
    bool enabled;
    std::string created_timestamp;
    std::string modified_timestamp;
};

struct PolicyResult {
    PolicyDecision decision;
    std::string rule_id;
    std::string reasoning;
    std::map<std::string, std::string> metadata;
    std::vector<std::string> violations;
};

class AxionPolicyHook {
private:
    std::vector<PolicyRule> policy_rules_;
    std::filesystem::path audit_log_path_;
    std::filesystem::path policy_config_path_;
    bool initialized_;
    
    // Event processing utilities
    std::string generate_event_id() {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        return "event_" + std::to_string(timestamp);
    }
    
    std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    std::string event_type_to_string(AIEventType type) {
        switch (type) {
            case AIEventType::MODEL_LOAD: return "model_load";
            case AIEventType::MODEL_UNLOAD: return "model_unload";
            case AIEventType::INFERENCE_START: return "inference_start";
            case AIEventType::INFERENCE_COMPLETE: return "inference_complete";
            case AIEventType::TOOL_USE_START: return "tool_use_start";
            case AIEventType::TOOL_USE_COMPLETE: return "tool_use_complete";
            case AIEventType::QUANTIZATION_START: return "quantization_start";
            case AIEventType::QUANTIZATION_COMPLETE: return "quantization_complete";
            case AIEventType::POLICY_VIOLATION: return "policy_violation";
            default: return "unknown";
        }
    }
    
    std::string decision_to_string(PolicyDecision decision) {
        switch (decision) {
            case PolicyDecision::ALLOW: return "allow";
            case PolicyDecision::DENY: return "deny";
            case PolicyDecision::LOG_ONLY: return "log_only";
            case PolicyDecision::REQUIRE_APPROVAL: return "require_approval";
            default: return "unknown";
        }
    }
    
    // Policy evaluation
    std::vector<PolicyRule> find_applicable_rules(AIEventType event_type) {
        std::vector<PolicyRule> applicable_rules;
        
        for (const auto& rule : policy_rules_) {
            if (!rule.enabled) continue;
            
            // Check if rule applies to this event type
            bool applies = false;
            for (AIEventType applicable_type : rule.applicable_events) {
                if (applicable_type == event_type) {
                    applies = true;
                    break;
                }
            }
            
            if (applies) {
                applicable_rules.push_back(rule);
            }
        }
        
        return applicable_rules;
    }
    
    bool evaluate_conditions(const PolicyRule& rule, const AIEvent& event) {
        // Evaluate all conditions in the rule
        for (const auto& [condition_key, condition_value] : rule.conditions) {
            if (condition_key == "model_id") {
                if (event.model_id != condition_value) {
                    return false;
                }
            } else if (condition_key == "user_id") {
                if (event.user_id != condition_value) {
                    return false;
                }
            } else if (condition_key == "time_of_day") {
                // Simple time-based condition
                auto now = std::chrono::system_clock::now();
                auto hour = std::chrono::duration_cast<std::chrono::hours>(
                    now.time_since_epoch()).count() % 24;
                
                int allowed_start = std::stoi(condition_value.substr(0, 2));
                int allowed_end = std::stoi(condition_value.substr(3, 2));
                
                if (hour < allowed_start || hour > allowed_end) {
                    return false;
                }
            } else if (condition_key == "model_size_limit") {
                // Check model size limit
                if (event.event_data.find("model_size") != event.event_data.end()) {
                    uint64_t model_size = std::stoull(event.event_data.at("model_size"));
                    uint64_t size_limit = std::stoull(condition_value);
                    
                    if (model_size > size_limit) {
                        return false;
                    }
                }
            }
        }
        
        return true;
    }
    
    void log_audit_event(const AIEvent& event, const PolicyResult& result) {
        nlohmann::json audit_entry = {
            {"event_id", event.event_id},
            {"event_type", event_type_to_string(event.event_type)},
            {"timestamp", event.timestamp},
            {"model_id", event.model_id},
            {"user_id", event.user_id},
            {"session_id", event.session_id},
            {"event_data", event.event_data},
            {"context", event.context},
            {"policy_result", {
                {"decision", decision_to_string(result.decision)},
                {"rule_id", result.rule_id},
                {"reasoning", result.reasoning},
                {"metadata", result.metadata},
                {"violations", result.violations}
            }}
        };
        
        // Write to audit log
        std::ofstream audit_file(audit_log_path_, std::ios::app);
        audit_file << audit_entry.dump() << std::endl;
        audit_file.close();
        
        std::cout << "Audit event logged: " << event.event_id << std::endl;
    }
    
public:
    AxionPolicyHook(const std::filesystem::path& config_path, 
                    const std::filesystem::path& audit_path)
        : policy_config_path_(config_path), audit_log_path_(audit_path), initialized_(false) {
        
        // Create audit directory
        std::filesystem::create_directories(audit_path_.parent_path());
    }
    
    bool initialize() {
        std::cout << "Initializing Axion policy hooks..." << std::endl;
        
        // Load policy rules
        if (!load_policy_rules()) {
            std::cerr << "Failed to load policy rules" << std::endl;
            return false;
        }
        
        // Initialize audit log
        if (!initialize_audit_log()) {
            std::cerr << "Failed to initialize audit log" << std::endl;
            return false;
        }
        
        initialized_ = true;
        std::cout << "Axion policy hooks initialized successfully" << std::endl;
        return true;
    }
    
    PolicyResult evaluate_event(const AIEvent& event) {
        if (!initialized_) {
            PolicyResult result;
            result.decision = PolicyDecision::DENY;
            result.reasoning = "Policy hooks not initialized";
            return result;
        }
        
        std::cout << "Evaluating policy for event: " << event.event_id << std::endl;
        std::cout << "Event type: " << event_type_to_string(event.event_type) << std::endl;
        
        // Find applicable rules
        auto applicable_rules = find_applicable_rules(event.event_type);
        
        if (applicable_rules.empty()) {
            PolicyResult result;
            result.decision = PolicyDecision::ALLOW;
            result.reasoning = "No applicable policy rules";
            return result;
        }
        
        // Evaluate each applicable rule
        PolicyResult final_result;
        final_result.decision = PolicyDecision::ALLOW;
        final_result.reasoning = "All applicable rules passed";
        
        for (const auto& rule : applicable_rules) {
            bool conditions_met = evaluate_conditions(rule, event);
            
            if (!conditions_met) {
                // Rule conditions not met, apply default decision
                if (rule.default_decision == PolicyDecision::DENY) {
                    final_result.decision = PolicyDecision::DENY;
                    final_result.rule_id = rule.rule_id;
                    final_result.reasoning = "Rule conditions not met: " + rule.description;
                    final_result.violations.push_back(rule.name);
                    break;
                }
            } else {
                // Conditions met, rule allows action
                final_result.rule_id = rule.rule_id;
                final_result.reasoning = "Allowed by rule: " + rule.name;
            }
        }
        
        // Log the event and result
        log_audit_event(event, final_result);
        
        std::cout << "Policy decision: " << decision_to_string(final_result.decision) << std::endl;
        std::cout << "Reasoning: " << final_result.reasoning << std::endl;
        
        return final_result;
    }
    
    // Event creation helpers
    AIEvent create_model_load_event(const std::string& model_id, 
                                  const std::string& user_id,
                                  const std::string& session_id,
                                  const std::map<std::string, std::string>& model_data) {
        AIEvent event;
        event.event_id = generate_event_id();
        event.event_type = AIEventType::MODEL_LOAD;
        event.timestamp = get_timestamp();
        event.model_id = model_id;
        event.user_id = user_id;
        event.session_id = session_id;
        event.event_data = model_data;
        return event;
    }
    
    AIEvent create_inference_start_event(const std::string& model_id,
                                      const std::string& user_id,
                                      const std::string& session_id,
                                      const std::string& prompt) {
        AIEvent event;
        event.event_id = generate_event_id();
        event.event_type = AIEventType::INFERENCE_START;
        event.timestamp = get_timestamp();
        event.model_id = model_id;
        event.user_id = user_id;
        event.session_id = session_id;
        event.event_data = {{"prompt", prompt}};
        return event;
    }
    
    AIEvent create_tool_use_event(const std::string& model_id,
                                 const std::string& user_id,
                                 const std::string& session_id,
                                 const std::string& tool_name,
                                 const std::string& tool_args) {
        AIEvent event;
        event.event_id = generate_event_id();
        event.event_type = AIEventType::TOOL_USE_START;
        event.timestamp = get_timestamp();
        event.model_id = model_id;
        event.user_id = user_id;
        event.session_id = session_id;
        event.event_data = {
            {"tool_name", tool_name},
            {"tool_args", tool_args}
        };
        return event;
    }
    
    // Policy management
    bool add_policy_rule(const PolicyRule& rule) {
        policy_rules_.push_back(rule);
        return save_policy_rules();
    }
    
    bool remove_policy_rule(const std::string& rule_id) {
        auto it = std::remove_if(policy_rules_.begin(), policy_rules_.end(),
            [&rule_id](const PolicyRule& rule) {
                return rule.rule_id == rule_id;
            });
        
        bool removed = (it != policy_rules_.end());
        if (removed) {
            policy_rules_.erase(it, policy_rules_.end());
            return save_policy_rules();
        }
        
        return false;
    }
    
    std::vector<PolicyRule> list_policy_rules() const {
        return policy_rules_;
    }
    
    // Audit and reporting
    std::vector<nlohmann::json> get_audit_log(const std::string& filter = "") {
        std::vector<nlohmann::json> audit_entries;
        
        if (!std::filesystem::exists(audit_log_path_)) {
            return audit_entries;
        }
        
        std::ifstream audit_file(audit_log_path_);
        std::string line;
        
        while (std::getline(audit_file, line)) {
            try {
                nlohmann::json entry = nlohmann::json::parse(line);
                
                // Apply filter if provided
                if (filter.empty() || 
                    entry["event_type"].get<std::string>().find(filter) != std::string::npos ||
                    entry["model_id"].get<std::string>().find(filter) != std::string::npos) {
                    audit_entries.push_back(entry);
                }
            } catch (const std::exception& e) {
                std::cerr << "Error parsing audit entry: " << e.what() << std::endl;
            }
        }
        
        return audit_entries;
    }
    
    void generate_policy_report() {
        nlohmann::json report = {
            {"generated_timestamp", get_timestamp()},
            {"total_rules", policy_rules_.size()},
            {"enabled_rules", std::count_if(policy_rules_.begin(), policy_rules_.end(),
                [](const PolicyRule& rule) { return rule.enabled; })},
            {"rules", {}}
        };
        
        for (const auto& rule : policy_rules_) {
            nlohmann::json rule_json = {
                {"rule_id", rule.rule_id},
                {"name", rule.name},
                {"description", rule.description},
                {"applicable_events", {}},
                {"conditions", rule.conditions},
                {"default_decision", decision_to_string(rule.default_decision)},
                {"enabled", rule.enabled},
                {"created_timestamp", rule.created_timestamp},
                {"modified_timestamp", rule.modified_timestamp}
            };
            
            for (AIEventType event_type : rule.applicable_events) {
                rule_json["applicable_events"].push_back(event_type_to_string(event_type));
            }
            
            report["rules"].push_back(rule_json);
        }
        
        std::filesystem::path report_file = policy_config_path_.parent_path() / "policy_report.json";
        std::ofstream file(report_file);
        file << report.dump(4) << std::endl;
        
        std::cout << "Policy report generated: " << report_file << std::endl;
    }
    
private:
    bool load_policy_rules() {
        if (!std::filesystem::exists(policy_config_path_)) {
            // Create default policy rules
            return create_default_policy_rules();
        }
        
        std::ifstream config_file(policy_config_path_);
        nlohmann::json config;
        
        try {
            config_file >> config;
        } catch (const std::exception& e) {
            std::cerr << "Error parsing policy config: " << e.what() << std::endl;
            return false;
        }
        
        policy_rules_.clear();
        
        for (const auto& rule_json : config["rules"]) {
            PolicyRule rule;
            rule.rule_id = rule_json.value("rule_id", "");
            rule.name = rule_json.value("name", "");
            rule.description = rule_json.value("description", "");
            rule.default_decision = string_to_decision(
                rule_json.value("default_decision", "allow"));
            rule.enabled = rule_json.value("enabled", true);
            rule.created_timestamp = rule_json.value("created_timestamp", "");
            rule.modified_timestamp = rule_json.value("modified_timestamp", "");
            
            // Parse applicable events
            for (const auto& event_str : rule_json["applicable_events"]) {
                rule.applicable_events.push_back(string_to_event_type(event_str));
            }
            
            // Parse conditions
            if (rule_json.contains("conditions")) {
                for (const auto& [key, value] : rule_json["conditions"].items()) {
                    rule.conditions[key] = value;
                }
            }
            
            policy_rules_.push_back(rule);
        }
        
        std::cout << "Loaded " << policy_rules_.size() << " policy rules" << std::endl;
        return true;
    }
    
    bool save_policy_rules() {
        nlohmann::json config = {
            {"rules", nlohmann::json::array()}
        };
        
        for (const auto& rule : policy_rules_) {
            nlohmann::json rule_json = {
                {"rule_id", rule.rule_id},
                {"name", rule.name},
                {"description", rule.description},
                {"applicable_events", {}},
                {"conditions", rule.conditions},
                {"default_decision", decision_to_string(rule.default_decision)},
                {"enabled", rule.enabled},
                {"created_timestamp", rule.created_timestamp},
                {"modified_timestamp", rule.modified_timestamp}
            };
            
            for (AIEventType event_type : rule.applicable_events) {
                rule_json["applicable_events"].push_back(event_type_to_string(event_type));
            }
            
            config["rules"].push_back(rule_json);
        }
        
        std::ofstream config_file(policy_config_path_);
        config_file << config.dump(4) << std::endl;
        
        return true;
    }
    
    bool initialize_audit_log() {
        // Create audit log file with header
        std::ofstream audit_file(audit_log_path_, std::ios::app);
        if (!audit_file.is_open()) {
            return false;
        }
        
        // Check if file is empty, add header
        audit_file.seekp(0, std::ios::end);
        if (audit_file.tellp() == 0) {
            audit_file << "# T81 AI Policy Audit Log" << std::endl;
            audit_file << "# Generated: " << get_timestamp() << std::endl;
            audit_file << "# Format: JSON per line" << std::endl;
            audit_file << std::endl;
        }
        
        return true;
    }
    
    bool create_default_policy_rules() {
        // Create default security policies
        std::vector<PolicyRule> default_rules;
        
        // Rule 1: Model size limit
        PolicyRule size_limit;
        size_limit.rule_id = "model_size_limit";
        size_limit.name = "Model Size Limit";
        size_limit.description = "Limit model loading to models under 1GB";
        size_limit.applicable_events = {AIEventType::MODEL_LOAD};
        size_limit.conditions = {{"model_size_limit", "1073741824"}}; // 1GB
        size_limit.default_decision = PolicyDecision::DENY;
        size_limit.enabled = true;
        size_limit.created_timestamp = get_timestamp();
        size_limit.modified_timestamp = get_timestamp();
        
        // Rule 2: Business hours restriction
        PolicyRule business_hours;
        business_hours.rule_id = "business_hours";
        business_hours.name = "Business Hours Only";
        business_hours.description = "Allow AI operations only during business hours (9 AM - 5 PM)";
        business_hours.applicable_events = {AIEventType::INFERENCE_START, AIEventType::TOOL_USE_START};
        business_hours.conditions = {{"time_of_day", "09:00-17:00"}};
        business_hours.default_decision = PolicyDecision::DENY;
        business_hours.enabled = false; // Disabled by default
        business_hours.created_timestamp = get_timestamp();
        business_hours.modified_timestamp = get_timestamp();
        
        // Rule 3: Model whitelist
        PolicyRule model_whitelist;
        model_whitelist.rule_id = "model_whitelist";
        model_whitelist.name = "Model Whitelist";
        model_whitelist.description = "Only allow approved models";
        model_whitelist.applicable_events = {AIEventType::MODEL_LOAD};
        model_whitelist.conditions = {{"model_id", "approved_model_123"}}; // Example
        model_whitelist.default_decision = PolicyDecision::DENY;
        model_whitelist.enabled = false; // Disabled by default
        model_whitelist.created_timestamp = get_timestamp();
        model_whitelist.modified_timestamp = get_timestamp();
        
        default_rules.push_back(size_limit);
        default_rules.push_back(business_hours);
        default_rules.push_back(model_whitelist);
        
        policy_rules_ = default_rules;
        return save_policy_rules();
    }
    
    PolicyDecision string_to_decision(const std::string& decision_str) {
        if (decision_str == "allow") return PolicyDecision::ALLOW;
        if (decision_str == "deny") return PolicyDecision::DENY;
        if (decision_str == "log_only") return PolicyDecision::LOG_ONLY;
        if (decision_str == "require_approval") return PolicyDecision::REQUIRE_APPROVAL;
        return PolicyDecision::ALLOW; // Default
    }
    
    AIEventType string_to_event_type(const std::string& event_str) {
        if (event_str == "model_load") return AIEventType::MODEL_LOAD;
        if (event_str == "model_unload") return AIEventType::MODEL_UNLOAD;
        if (event_str == "inference_start") return AIEventType::INFERENCE_START;
        if (event_str == "inference_complete") return AIEventType::INFERENCE_COMPLETE;
        if (event_str == "tool_use_start") return AIEventType::TOOL_USE_START;
        if (event_str == "tool_use_complete") return AIEventType::TOOL_USE_COMPLETE;
        if (event_str == "quantization_start") return AIEventType::QUANTIZATION_START;
        if (event_str == "quantization_complete") return AIEventType::QUANTIZATION_COMPLETE;
        if (event_str == "policy_violation") return AIEventType::POLICY_VIOLATION;
        return AIEventType::MODEL_LOAD; // Default
    }
};

} // namespace t81::ai::policy_hooks

// CLI interface for policy hooks
int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            std::cout << "T81 Axion Policy Hooks for AI Events" << std::endl;
            std::cout << "Usage: " << argv[0] << " <command> [options]" << std::endl;
            std::cout << "Commands:" << std::endl;
            std::cout << "  init <config> <audit>              Initialize policy hooks" << std::endl;
            std::cout << "  evaluate <event_type> <data>        Evaluate policy for event" << std::endl;
            std::cout << "  add-rule <name> <description>    Add new policy rule" << std::endl;
            std::cout << "  list-rules                          List all policy rules" << std::endl;
            std::cout << "  audit [filter]                       Show audit log" << std::endl;
            std::cout << "  report                              Generate policy report" << std::endl;
            return 0;
        }
        
        std::string command = argv[1];
        
        if (command == "init" && argc >= 4) {
            std::filesystem::path config_path = argv[2];
            std::filesystem::path audit_path = argv[3];
            
            t81::ai::policy_hooks::AxionPolicyHook hooks(config_path, audit_path);
            if (hooks.initialize()) {
                std::cout << "Policy hooks initialized successfully" << std::endl;
            } else {
                std::cerr << "Failed to initialize policy hooks" << std::endl;
                return 1;
            }
            
        } else if (command == "evaluate" && argc >= 4) {
            std::filesystem::path config_path = "./policy_config.json";
            std::filesystem::path audit_path = "./audit.log";
            
            t81::ai::policy_hooks::AxionPolicyHook hooks(config_path, audit_path);
            hooks.initialize();
            
            std::string event_type = argv[2];
            std::string data = argv[3];
            
            // Create test event
            t81::ai::policy_hooks::AIEvent event = hooks.create_model_load_event(
                "test_model", "test_user", "test_session", 
                {{"model_size", "1048576"}}
            );
            
            auto result = hooks.evaluate_event(event);
            
            std::cout << "Evaluation result: " << hooks.decision_to_string(result.decision) << std::endl;
            std::cout << "Reasoning: " << result.reasoning << std::endl;
            
        } else if (command == "list-rules") {
            std::filesystem::path config_path = "./policy_config.json";
            std::filesystem::path audit_path = "./audit.log";
            
            t81::ai::policy_hooks::AxionPolicyHook hooks(config_path, audit_path);
            hooks.initialize();
            
            auto rules = hooks.list_policy_rules();
            std::cout << "Policy Rules:" << std::endl;
            for (const auto& rule : rules) {
                std::cout << "  ID: " << rule.rule_id << std::endl;
                std::cout << "  Name: " << rule.name << std::endl;
                std::cout << "  Enabled: " << (rule.enabled ? "Yes" : "No") << std::endl;
                std::cout << "  Description: " << rule.description << std::endl;
                std::cout << std::endl;
            }
            
        } else if (command == "audit") {
            std::filesystem::path config_path = "./policy_config.json";
            std::filesystem::path audit_path = "./audit.log";
            std::string filter = (argc >= 3) ? argv[2] : "";
            
            t81::ai::policy_hooks::AxionPolicyHook hooks(config_path, audit_path);
            hooks.initialize();
            
            auto audit_entries = hooks.get_audit_log(filter);
            std::cout << "Audit Log (" << audit_entries.size() << " entries):" << std::endl;
            for (const auto& entry : audit_entries) {
                std::cout << "  Event: " << entry["event_type"] << std::endl;
                std::cout << "  Decision: " << entry["policy_result"]["decision"] << std::endl;
                std::cout << "  Timestamp: " << entry["timestamp"] << std::endl;
                std::cout << std::endl;
            }
            
        } else if (command == "report") {
            std::filesystem::path config_path = "./policy_config.json";
            std::filesystem::path audit_path = "./audit.log";
            
            t81::ai::policy_hooks::AxionPolicyHook hooks(config_path, audit_path);
            hooks.initialize();
            hooks.generate_policy_report();
            
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
