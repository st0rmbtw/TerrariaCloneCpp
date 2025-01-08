#pragma once

#ifndef RENDERER_MACROS_HPP
#define RENDERER_MACROS_HPP

#define RESOURCE_RELEASE(ptr) if ((ptr) != nullptr) { Renderer::Context()->Release(*(ptr)); (ptr) = nullptr; }

#endif