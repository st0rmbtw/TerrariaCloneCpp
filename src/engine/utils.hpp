#ifndef _ENGINE_UTILS_HPP_
#define _ENGINE_UTILS_HPP_

#include <stdlib.h>
#include "log.hpp"

template <typename T> 
T* checked_alloc(size_t count) {
    T* _ptr = (T*) malloc(count * sizeof(T));
    if (_ptr == nullptr && count > 0) {
        LOG_ERROR("Out of memory");
        abort();
    }
    return _ptr;
}

#endif