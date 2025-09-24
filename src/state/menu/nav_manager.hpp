#pragma once

#ifndef STATE_MENU_NAV_MANAGER_HPP_
#define STATE_MENU_NAV_MANAGER_HPP_

#include <list>
#include <string>
#include <variant>

struct MainMenu {};
struct Settings {};
struct SelectWorld {};
struct CreateWorld {};

struct WorldSelected {
    std::string seed;
    std::string name;
};

using NavItem = std::variant<MainMenu, Settings, SelectWorld, CreateWorld, WorldSelected>;

class NavManager {
public:
    template <typename T, typename... Args>
    constexpr void push(Args&&... args) {
        m_nav_stack.push_back(T(std::forward<Args>(args)...));
    }

    void push(NavItem item) {
        m_nav_stack.push_back(std::move(item));
    }

    void pop() {
        if (m_nav_stack.size() > 1) {
            m_nav_stack.pop_back();
        }
    }

    [[nodiscard]]
    const NavItem& top() const noexcept {
        return m_nav_stack.back();
    }

    template <typename T>
    [[nodiscard]]
    constexpr bool is() const noexcept {
        return std::holds_alternative<T>(top());
    }

private:
    std::list<NavItem> m_nav_stack;
};

#endif