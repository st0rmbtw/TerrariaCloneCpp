#pragma once

#ifndef _ENGINE_TYPES_ORDER_HPP_
#define _ENGINE_TYPES_ORDER_HPP_

struct Order {

    Order() = default;

    Order(int order, bool advance = true) :
        value(order),
        advance(advance) {}

    Order(int order, bool advance, bool depth_enabled) :
        value(order),
        advance(advance),
        depth_enabled(depth_enabled) {}

    int value = -1;
    bool advance = true;
    bool depth_enabled = false;
};

#endif