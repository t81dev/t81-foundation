#pragma once

#include "t81/canonfs/interchange_ops.hpp"
#include <string>

namespace t81::canonfs {

// JSON renderer for CanonFS interchange operations
class InterchangeJSONRenderer {
public:
    virtual ~InterchangeJSONRenderer() = default;
    
    // Render operations
    virtual std::string render_import_result(const ImportOutcome& outcome) = 0;
    virtual std::string render_export_result(const ExportOutcome& outcome) = 0;
    
    // Validation operations
    virtual bool validate_import_schema(const std::string& json) const = 0;
    virtual bool validate_export_schema(const std::string& json) const = 0;
    
    // Schema information
    virtual std::string get_import_schema_version() const = 0;
    virtual std::string get_export_schema_version() const = 0;
};

// Default implementation using current interchange module
class DefaultInterchangeJSONRenderer : public InterchangeJSONRenderer {
public:
    std::string render_import_result(const ImportOutcome& outcome) override;
    std::string render_export_result(const ExportOutcome& outcome) override;
    bool validate_import_schema(const std::string& json) const override;
    bool validate_export_schema(const std::string& json) const override;
    std::string get_import_schema_version() const override;
    std::string get_export_schema_version() const override;
};

// Factory function
std::unique_ptr<InterchangeJSONRenderer> create_default_json_renderer();

} // namespace t81::canonfs
