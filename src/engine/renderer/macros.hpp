#pragma once

#ifndef RENDERER_MACROS_HPP
#define RENDERER_MACROS_HPP

#define RESOURCE_RELEASE(_PTR) if ((_PTR) != nullptr) { context->Release(*(_PTR)); (_PTR) = nullptr; }

#endif