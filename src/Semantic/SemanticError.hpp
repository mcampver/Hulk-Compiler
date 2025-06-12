#pragma once
#include <string>

/**
 * @brief Represents a semantic error with location information
 */
struct SemanticError {
    std::string message;
    int line;
    int column;
    std::string context;
    
    SemanticError(const std::string& msg, int ln, int col = 0, const std::string& ctx = "")
        : message(msg), line(ln), column(col), context(ctx) {}
    
    std::string format() const {
        std::string result = "Semantic Error at line " + std::to_string(line);
        if (column > 0) {
            result += ", column " + std::to_string(column);
        }
        result += ": " + message;
        if (!context.empty()) {
            result += " (in " + context + ")";
        }
        return result;
    }
};
