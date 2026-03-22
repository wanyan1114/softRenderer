#pragma once

#include "app/LayerContext.h"

#include <string>

namespace sr {

class Layer {
public:
    explicit Layer(std::string name)
        : m_Name(std::move(name))
    {
    }

    virtual ~Layer() = default;

    virtual void OnAttach(LayerContext&) { }
    virtual void OnDetach(LayerContext&) { }
    virtual void OnUpdate(float, LayerContext&) { }
    virtual void OnRender(LayerContext&) { }

    const std::string& Name() const { return m_Name; }

private:
    std::string m_Name;
};

} // namespace sr
