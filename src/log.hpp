#ifndef LOG_HPP
#define LOG_HPP

#pragma once

#define __LOG(level, text, ...) printf("[" level "]" " " text "\n" __VA_OPT__(,) ##__VA_ARGS__)

#if DEBUG
#define LOG_ERROR(text, ...) __LOG("ERROR", text, ##__VA_ARGS__)
#define LOG_INFO(text, ...) __LOG("INFO", text, ##__VA_ARGS__)
#define LOG_DEBUG(text, ...) __LOG("DEBUG", text, ##__VA_ARGS__)
#else
#define LOG_ERROR(text, ...) __LOG("ERROR", text, ##__VA_ARGS__)
#define LOG_INFO(text, ...) ((void)0)
#define LOG_DEBUG(text, ...) ((void)0)
#endif

#endif