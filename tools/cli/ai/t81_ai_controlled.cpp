#include "t81/ai_backend/controlled_ai_backend.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/axion/policy_serialization.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <getopt.h>

namespace t81::cli::ai {

struct ControlledAICLIConfig {
    std::string model_hash;
    std::string input_text;
    float temperature = 0.0;
    std::string determinism_level = "strict";
    std::optional<std::string> external_ai_endpoint;
    std::optional<std::string> external_ai_auth_token;
    std::optional<std::string> policy_file;
    std::optional<std::string> evidence_output_file;
    bool user_consent = false;
    bool show_evidence = false;
};

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]\n\n";
    std::cout << "T81 Controlled AI Inference Tool\n\n";
    std::cout << "Options:\n";
    std::cout << "  -m, --model <hash>          Model hash to use for inference\n";
    std::cout << "  -i, --input <text>          Input text for inference\n";
    std::cout << "  -t, --temperature <float>     Temperature for non-deterministic sampling (0.0-2.0)\n";
    std::cout << "  -d, --determinism <level>    Determinism level: strict|controlled|permissive\n";
    std::cout << "  -e, --external-endpoint <url> External AI service endpoint\n";
    std::cout << "  -a, --auth-token <token>      Authentication token for external AI\n";
    std::cout << "  -p, --policy <file>          Policy file for AI operations\n";
    std::cout << "  -o, --evidence-output <file> Evidence output file (JSON)\n";
    std::cout << "  -c, --user-consent            User consent for non-deterministic operations\n";
    std::cout << "  -s, --show-evidence           Show evidence log\n";
    std::cout << "  -h, --help                   Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " -m abc123 -i \"Hello world\" -d controlled -c\n";
    std::cout << "  " << program_name << " -m abc123 -i \"Hello world\" -e https://api.openai.com -a token123\n";
    std::cout << "\nDeterminism Levels:\n";
    std::cout << "  strict      - Fully deterministic (default)\n";
    std::cout << "  controlled   - Controlled non-determinism with policy approval\n";
    std::cout << "  permissive  - Allow non-deterministic operations\n";
}

ControlledAICLIConfig parse_arguments(int argc, char* argv[]) {
    ControlledAICLIConfig config;
    
    static option long_options[] = {
        {"model", required_argument, nullptr, 'm', "Model hash"},
        {"input", required_argument, nullptr, 'i', "Input text"},
        {"temperature", required_argument, nullptr, 't', "Temperature"},
        {"determinism", required_argument, nullptr, 'd', "Determinism level"},
        {"external-endpoint", required_argument, nullptr, 'e', "External AI endpoint"},
        {"auth-token", required_argument, nullptr, 'a', "Auth token"},
        {"policy", required_argument, nullptr, 'p', "Policy file"},
        {"evidence-output", required_argument, nullptr, 'o', "Evidence output file"},
        {"user-consent", no_argument, nullptr, 'c', "User consent"},
        {"show-evidence", no_argument, nullptr, 's', "Show evidence"},
        {"help", no_argument, nullptr, 'h', "Help"},
        {nullptr, 0, nullptr, 0, nullptr}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "m:i:t:d:e:a:p:o:csh", long_options, &option_index)) != -1) {
        switch (c) {
            case 'm':
                config.model_hash = optarg;
                break;
            case 'i':
                config.input_text = optarg;
                break;
            case 't':
                config.temperature = std::stof(optarg);
                break;
            case 'd':
                config.determinism_level = optarg;
                break;
            case 'e':
                config.external_ai_endpoint = optarg;
                break;
            case 'a':
                config.external_ai_auth_token = optarg;
                break;
            case 'p':
                config.policy_file = optarg;
                break;
            case 'o':
                config.evidence_output_file = optarg;
                break;
            case 'c':
                config.user_consent = true;
                break;
            case 's':
                config.show_evidence = true;
                break;
            case 'h':
                print_usage(argv[0]);
                exit(0);
            default:
                print_usage(argv[0]);
                exit(1);
        }
    }
    
    return config;
}

std::unique_ptr<t81::axion::PolicyEngine> load_policy(const std::optional<std::string>& policy_file) {
    if (!policy_file) {
        return nullptr; // Use default policy
    }
    
    std::ifstream file(*policy_file);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open policy file: " << *policy_file << std::endl;
        return nullptr;
    }
    
    std::string policy_text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto policy_result = t81::axion::PolicyEngine::validate_policy(policy_text);
    
    if (policy_result) {
        std::cerr << "Error: Invalid policy file: " << policy_result->message << std::endl;
        return nullptr;
    }
    
    auto policy = t81::axion::PolicyEngine::parse_policy(policy_text);
    return std::make_unique<t81::axion::PolicyEngine>(policy);
}

int run_controlled_ai_inference(const ControlledAICLIConfig& config) {
    // Validate required arguments
    if (config.model_hash.empty()) {
        std::cerr << "Error: Model hash is required" << std::endl;
        return 1;
    }
    
    if (config.input_text.empty()) {
        std::cerr << "Error: Input text is required" << std::endl;
        return 1;
    }
    
    // Load policy
    auto policy_engine = load_policy(config.policy_file);
    if (!policy_engine) {
        std::cerr << "Error: Failed to load policy" << std::endl;
        return 1;
    }
    
    // Create AI backend
    t81::ai_backend::AIBackendConfig ai_config;
    ai_config.determinism_level = config.determinism_level;
    ai_config.external_ai_endpoint = config.external_ai_endpoint;
    ai_config.external_ai_auth_token = config.external_ai_auth_token;
    ai_config.require_user_consent = !config.user_consent; // Consent flag means user has consented
    
    auto ai_backend = t81::ai_backend::create_controlled_ai_backend(
        std::move(policy_engine), ai_config);
    
    // Prepare inference request
    size_t input_tokens = config.input_text.length(); // Simplified tokenization
    t81::ai_backend::InferenceRequest request(
        config.model_hash, input_tokens, config.temperature, config.input_text);
    
    // Run inference
    std::cout << "Running AI inference..." << std::endl;
    std::cout << "Model: " << config.model_hash << std::endl;
    std::cout << "Input: \"" << config.input_text << "\"" << std::endl;
    std::cout << "Determinism: " << config.determinism_level << std::endl;
    if (config.external_ai_endpoint) {
        std::cout << "External AI: " << *config.external_ai_endpoint << std::endl;
    }
    std::cout << std::endl;
    
    auto result = ai_backend->inference(request);
    
    if (result.success) {
        std::cout << "✅ Inference successful" << std::endl;
        std::cout << "Output tokens: " << result.output_tokens << std::endl;
        std::cout << "Output: \"" << result.output_text << "\"" << std::endl;
    } else {
        std::cout << "❌ Inference failed" << std::endl;
        std::cout << "Error: " << result.error_message << std::endl;
        return 1;
    }
    
    // Handle evidence output
    if (config.evidence_output_file) {
        std::ofstream evidence_file(*config.evidence_output_file);
        if (evidence_file.is_open()) {
            evidence_file << ai_backend->get_evidence_json();
            std::cout << "Evidence saved to: " << *config.evidence_output_file << std::endl;
        } else {
            std::cerr << "Warning: Could not write evidence file" << std::endl;
        }
    }
    
    // Show evidence if requested
    if (config.show_evidence) {
        std::cout << "\n--- Evidence Log ---" << std::endl;
        std::cout << ai_backend->get_evidence_json() << std::endl;
    }
    
    return 0;
}

} // namespace t81::cli::ai

int main(int argc, char* argv[]) {
    auto config = t81::cli::ai::parse_arguments(argc, argv);
    return t81::cli::ai::run_controlled_ai_inference(config);
}
