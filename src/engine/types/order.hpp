#pragma once

#ifndef _ENGINE_TYPES_ORDER_HPP_
#define _ENGINE_TYPES_ORDER_HPP_

struct Order {
    Order() = default;

    Order(int order, bool advance = true) :
        value(order),
        advance(advance) {}

    int value = -1;
    bool advance = true;
};

#endif