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

    [[nodiscard]] inline const LLGL::SamplerDescriptor& descriptor() const { return m_descriptor; }
    [[nodiscard]] inline LLGL::Sampler* internal() const { return m_internal; }

    inline operator LLGL::Sampler&() const { return *m_internal; }

private:
    LLGL::Sampler* m_internal = nullptr;
    LLGL::SamplerDescriptor m_descriptor;
};

#endif