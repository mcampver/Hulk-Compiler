#include "hulk_object.hpp"
#include "../Value/value.hpp"
#include "../AST/ast.hpp"

Value HulkObject::getAttribute(const std::string& name)
{
    auto it = attributes.find(name);
    if (it != attributes.end()) {
        return it->second;
    }
    // Si no se encuentra, devolver valor por defecto
    return Value(0.0);
}

void HulkObject::setAttribute(const std::string& name, const Value& value)
{
    attributes[name] = value;
}

bool HulkObject::hasAttribute(const std::string& name) const
{
    return attributes.find(name) != attributes.end();
}

FunctionDecl* HulkObject::getMethod(const std::string& name)
{
    // Por ahora retornamos nullptr, lo implementaremos cuando tengamos 
    // el sistema de tipos más completo
    return nullptr;
}
