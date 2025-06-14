#pragma once
#include <unordered_map>
#include <string>
#include <memory>

// Forward declarations
class Value;
struct TypeDecl;
struct FunctionDecl;

// Clase para representar un objeto HULK en runtime
class HulkObject
{
public:
    std::string typeName;
    std::unordered_map<std::string, Value> attributes;
    TypeDecl* typeDeclaration; // Referencia a la declaración del tipo
    
    HulkObject(const std::string& type, TypeDecl* decl = nullptr) 
        : typeName(type), typeDeclaration(decl) {}
    
    // Obtener valor de un atributo
    Value getAttribute(const std::string& name);
    
    // Establecer valor de un atributo
    void setAttribute(const std::string& name, const Value& value);
    
    // Verificar si tiene un atributo
    bool hasAttribute(const std::string& name) const;
    
    // Obtener método por nombre
    FunctionDecl* getMethod(const std::string& name);
};

using HulkObjectPtr = std::shared_ptr<HulkObject>;
