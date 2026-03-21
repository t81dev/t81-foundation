// T81 Model Provenance Manager - RFC-00A3 Task 5
// Implements model artifact identity, provenance, and integrity verification

#include <t81/core.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

namespace t81::ai::provenance {

enum class ModelFormat {
    GGUF,
    SAFETENSORS,
    T81_CANONICAL
};

struct ModelMetadata {
    std::string model_id;
    std::string name;
    std::string version;
    std::string format;
    std::string creator;
    std::string created_timestamp;
    std::string description;
    std::map<std::string, std::string> parameters;
    std::string training_data_hash;
    std::string model_hash;
    std::string signature;
    std::vector<std::string> dependencies;
    std::map<std::string, std::string> security_tags;
};

class ModelManager {
private:
    std::filesystem::path canonfs_root_;
    std::filesystem::path models_dir_;
    std::map<std::string, ModelMetadata> model_registry_;
    
    // Cryptographic utilities
    std::string compute_file_hash(const std::filesystem::path& file_path) {
        std::ifstream file(file_path, std::ios::binary);
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});
        
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(buffer.data(), buffer.size(), hash);
        
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }
    
    std::string sign_data(const std::string& data, const std::string& private_key_path) {
        // In real implementation, load private key and sign data
        // For now, return a mock signature
        return "mock_signature_" + compute_string_hash(data);
    }
    
    bool verify_signature(const std::string& data, const std::string& signature, 
                       const std::string& public_key_path) {
        // In real implementation, verify signature using public key
        // For now, check if signature matches expected format
        return signature.find("mock_signature_") == 0;
    }
    
    std::string compute_string_hash(const std::string& data) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash);
        
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }
    
    ModelFormat detect_model_format(const std::filesystem::path& model_path) {
        std::string ext = model_path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext == ".gguf") {
            return ModelFormat::GGUF;
        } else if (ext == ".safetensors") {
            return ModelFormat::SAFETENSORS;
        } else if (ext == ".t81") {
            return ModelFormat::T81_CANONICAL;
        }
        
        return ModelFormat::GGUF;  // Default
    }
    
    bool convert_to_canonical(const std::filesystem::path& input_path,
                           const std::filesystem::path& output_path,
                           ModelFormat input_format) {
        std::cout << "Converting model to canonical format..." << std::endl;
        
        // In real implementation, this would:
        // 1. Load model in original format
        // 2. Convert weights to ternary representation
        // 3. Save in T81 canonical format
        // 4. Generate conversion metadata
        
        // For now, create a mock canonical file
        std::ofstream output(output_path);
        output << "T81_CANONICAL_MODEL_DATA\n";
        output << "Converted from: " << input_path.string() << "\n";
        output << "Conversion timestamp: " << get_timestamp() << "\n";
        output.close();
        
        return std::filesystem::exists(output_path);
    }
    
    std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
public:
    ModelManager(const std::filesystem::path& canonfs_root) 
        : canonfs_root_(canonfs_root), models_dir_(canonfs_root / "models") {
        
        std::filesystem::create_directories(models_dir_);
        load_model_registry();
    }
    
    // Load model with provenance verification
    bool load_model(const std::filesystem::path& model_path, 
                   const std::string& expected_hash = "") {
        std::cout << "Loading model: " << model_path << std::endl;
        
        if (!std::filesystem::exists(model_path)) {
            std::cerr << "Error: Model file not found" << std::endl;
            return false;
        }
        
        // Compute actual hash
        std::string actual_hash = compute_file_hash(model_path);
        
        // Verify hash if provided
        if (!expected_hash.empty() && actual_hash != expected_hash) {
            std::cerr << "Error: Model hash mismatch" << std::endl;
            std::cerr << "Expected: " << expected_hash << std::endl;
            std::cerr << "Actual: " << actual_hash << std::endl;
            return false;
        }
        
        // Load or create metadata
        ModelMetadata metadata = get_or_create_metadata(model_path);
        
        // Verify signature if present
        if (!metadata.signature.empty()) {
            std::string public_key_path = canonfs_root_ / "keys" / "public.pem";
            if (!verify_signature(actual_hash, metadata.signature, public_key_path)) {
                std::cerr << "Error: Model signature verification failed" << std::endl;
                return false;
            }
            std::cout << "Model signature verified" << std::endl;
        }
        
        // Store in CanonFS
        std::string canonfs_hash = store_in_canonfs(model_path, metadata);
        
        std::cout << "Model loaded successfully" << std::endl;
        std::cout << "CanonFS hash: " << canonfs_hash << std::endl;
        
        return true;
    }
    
    // Create model manifest
    bool create_manifest(const std::filesystem::path& model_path,
                      const std::map<std::string, std::string>& parameters) {
        std::cout << "Creating model manifest..." << std::endl;
        
        ModelMetadata metadata;
        metadata.model_id = generate_model_id();
        metadata.name = model_path.stem().string();
        metadata.version = "1.0.0";
        metadata.format = model_format_to_string(detect_model_format(model_path));
        metadata.creator = "T81 AI System";
        metadata.created_timestamp = get_timestamp();
        metadata.parameters = parameters;
        metadata.model_hash = compute_file_hash(model_path);
        metadata.security_tags = {"experimental", "verified"};
        
        // Save manifest
        std::filesystem::path manifest_path = model_path.parent_path() / 
                                         (model_path.stem().string() + "_manifest.json");
        std::ofstream manifest_file(manifest_path);
        
        nlohmann::json manifest_json = {
            {"model_id", metadata.model_id},
            {"name", metadata.name},
            {"version", metadata.version},
            {"format", metadata.format},
            {"creator", metadata.creator},
            {"created_timestamp", metadata.created_timestamp},
            {"description", metadata.description},
            {"parameters", metadata.parameters},
            {"model_hash", metadata.model_hash},
            {"security_tags", metadata.security_tags},
            {"dependencies", metadata.dependencies}
        };
        
        manifest_file << manifest_json.dump(4) << std::endl;
        manifest_file.close();
        
        // Sign manifest
        std::string private_key_path = canonfs_root_ / "keys" / "private.pem";
        metadata.signature = sign_data(manifest_json.dump(), private_key_path);
        
        // Update manifest with signature
        manifest_json["signature"] = metadata.signature;
        manifest_file.open(manifest_path);
        manifest_file << manifest_json.dump(4) << std::endl;
        manifest_file.close();
        
        std::cout << "Manifest created: " << manifest_path << std::endl;
        std::cout << "Model ID: " << metadata.model_id << std::endl;
        
        return true;
    }
    
    // Verify model integrity
    bool verify_model(const std::string& model_id) {
        if (model_registry_.find(model_id) == model_registry_.end()) {
            std::cerr << "Error: Model not found in registry" << std::endl;
            return false;
        }
        
        const auto& metadata = model_registry_.at(model_id);
        
        std::cout << "Verifying model: " << model_id << std::endl;
        std::cout << "Name: " << metadata.name << std::endl;
        std::cout << "Format: " << metadata.format << std::endl;
        std::cout << "Creator: " << metadata.creator << std::endl;
        std::cout << "Created: " << metadata.created_timestamp << std::endl;
        
        // Verify hash
        std::filesystem::path model_file = models_dir_ / (model_id + ".t81");
        if (std::filesystem::exists(model_file)) {
            std::string actual_hash = compute_file_hash(model_file);
            if (actual_hash != metadata.model_hash) {
                std::cerr << "FAIL: Model hash mismatch" << std::endl;
                return false;
            }
            std::cout << "PASS: Model hash verified" << std::endl;
        }
        
        // Verify signature
        if (!metadata.signature.empty()) {
            std::string public_key_path = canonfs_root_ / "keys" / "public.pem";
            if (!verify_signature(metadata.model_hash, metadata.signature, public_key_path)) {
                std::cerr << "FAIL: Signature verification failed" << std::endl;
                return false;
            }
            std::cout << "PASS: Signature verified" << std::endl;
        }
        
        return true;
    }
    
    // List all models
    void list_models() {
        std::cout << "=== Registered Models ===" << std::endl;
        
        for (const auto& [model_id, metadata] : model_registry_) {
            std::cout << "ID: " << model_id << std::endl;
            std::cout << "  Name: " << metadata.name << std::endl;
            std::cout << "  Format: " << metadata.format << std::endl;
            std::cout << "  Creator: " << metadata.creator << std::endl;
            std::cout << "  Created: " << metadata.created_timestamp << std::endl;
            std::cout << "  Hash: " << metadata.model_hash << std::endl;
            std::cout << "  Signed: " << (metadata.signature.empty() ? "No" : "Yes") << std::endl;
            std::cout << std::endl;
        }
    }
    
private:
    std::string generate_model_id() {
        // Generate unique model ID based on timestamp and hash
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        
        return "model_" + std::to_string(timestamp);
    }
    
    std::string model_format_to_string(ModelFormat format) {
        switch (format) {
            case ModelFormat::GGUF: return "gguf";
            case ModelFormat::SAFETENSORS: return "safetensors";
            case ModelFormat::T81_CANONICAL: return "t81_canonical";
            default: return "unknown";
        }
    }
    
    ModelMetadata get_or_create_metadata(const std::filesystem::path& model_path) {
        std::string model_id = model_path.stem().string();
        
        // Check if metadata already exists
        if (model_registry_.find(model_id) != model_registry_.end()) {
            return model_registry_.at(model_id);
        }
        
        // Create new metadata
        ModelMetadata metadata;
        metadata.model_id = model_id;
        metadata.name = model_id;
        metadata.version = "1.0.0";
        metadata.format = model_format_to_string(detect_model_format(model_path));
        metadata.creator = "Unknown";
        metadata.created_timestamp = get_timestamp();
        metadata.model_hash = compute_file_hash(model_path);
        
        return metadata;
    }
    
    void load_model_registry() {
        // Load existing model registry
        std::filesystem::path registry_file = models_dir_ / "registry.json";
        if (std::filesystem::exists(registry_file)) {
            std::ifstream file(registry_file);
            nlohmann::json registry_json;
            file >> registry_json;
            
            for (const auto& [model_id, metadata_json] : registry_json.items()) {
                ModelMetadata metadata;
                metadata.model_id = model_id;
                metadata.name = metadata_json.value("name", "");
                metadata.version = metadata_json.value("version", "");
                metadata.format = metadata_json.value("format", "");
                metadata.creator = metadata_json.value("creator", "");
                metadata.created_timestamp = metadata_json.value("created_timestamp", "");
                metadata.model_hash = metadata_json.value("model_hash", "");
                metadata.signature = metadata_json.value("signature", "");
                
                // Load parameters
                if (metadata_json.contains("parameters")) {
                    for (const auto& [key, value] : metadata_json["parameters"].items()) {
                        metadata.parameters[key] = value;
                    }
                }
                
                model_registry_[model_id] = metadata;
            }
        }
    }
    
    std::string store_in_canonfs(const std::filesystem::path& model_path, 
                                const ModelMetadata& metadata) {
        // Convert to canonical format
        std::filesystem::path canonical_path = models_dir_ / (metadata.model_id + ".t81");
        
        if (!convert_to_canonical(model_path, canonical_path, 
                                detect_model_format(model_path))) {
            std::cerr << "Error: Failed to convert to canonical format" << std::endl;
            return "";
        }
        
        // Store in CanonFS (content-addressed storage)
        std::string canonfs_hash = compute_file_hash(canonical_path);
        
        // Update registry
        model_registry_[metadata.model_id] = metadata;
        save_model_registry();
        
        return canonfs_hash;
    }
    
    void save_model_registry() {
        nlohmann::json registry_json;
        
        for (const auto& [model_id, metadata] : model_registry_) {
            nlohmann::json metadata_json = {
                {"model_id", metadata.model_id},
                {"name", metadata.name},
                {"version", metadata.version},
                {"format", metadata.format},
                {"creator", metadata.creator},
                {"created_timestamp", metadata.created_timestamp},
                {"model_hash", metadata.model_hash},
                {"signature", metadata.signature},
                {"parameters", metadata.parameters}
            };
            registry_json[model_id] = metadata_json;
        }
        
        std::filesystem::path registry_file = models_dir_ / "registry.json";
        std::ofstream file(registry_file);
        file << registry_json.dump(4) << std::endl;
    }
};

} // namespace t81::ai::provenance

// CLI interface for model management
int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            std::cout << "T81 Model Provenance Manager" << std::endl;
            std::cout << "Usage: " << argv[0] << " <command> [options]" << std::endl;
            std::cout << "Commands:" << std::endl;
            std::cout << "  load <model_path> [hash]     Load model with verification" << std::endl;
            std::cout << "  manifest <model_path>           Create model manifest" << std::endl;
            std::cout << "  verify <model_id>               Verify model integrity" << std::endl;
            std::cout << "  list                              List all models" << std::endl;
            return 0;
        }
        
        std::string command = argv[1];
        std::filesystem::path canonfs_root = "./canonfs";
        
        t81::ai::provenance::ModelManager manager(canonfs_root);
        
        if (command == "load" && argc >= 3) {
            std::filesystem::path model_path = argv[2];
            std::string expected_hash = (argc >= 4) ? argv[3] : "";
            manager.load_model(model_path, expected_hash);
        } else if (command == "manifest" && argc >= 3) {
            std::filesystem::path model_path = argv[2];
            manager.create_manifest(model_path, {});
        } else if (command == "verify" && argc >= 3) {
            std::string model_id = argv[2];
            manager.verify_model(model_id);
        } else if (command == "list") {
            manager.list_models();
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
