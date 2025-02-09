#pragma once

#ifndef _ENGINE_RENDERER_MACROS_HPP_
#define _ENGINE_RENDERER_MACROS_HPP_

#define RESOURCE_RELEASE(_PTR) if ((_PTR) != nullptr) { context->Release(*(_PTR)); (_PTR) = nullptr; }

#endif