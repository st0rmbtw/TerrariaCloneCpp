#ifndef TYPES_SAMPLER_HPP
#define TYPES_SAMPLER_HPP

#include <LLGL/Sampler.h>
#include <LLGL/SamplerFlags.h>

class Sampler {
public:
    Sampler() = default;

    explicit Sampler(LLGL::Sampler* internal, LLGL::SamplerDescriptor descriptor) :
        m_internal(internal),
        m_descriptor(std::move(descriptor)) {}

    const LLGL::SamplerDescriptor& descriptor() { return m_descriptor; }
    LLGL::Sampler* internal() { return m_internal; }

    operator LLGL::Sampler&() { return *m_internal; }

private:
    LLGL::Sampler* m_internal = nullptr;
    LLGL::SamplerDescriptor m_descriptor;
};

#endif