#pragma once

#ifndef TERRARIA_LOG_HPP
#define TERRARIA_LOG_HPP

#if DEBUG
#define __LOG(level, text, ...) printf("[" level "]" " " text "\n", __VA_ARGS__)
#else
#define __LOG(level, text, ...) ((void)0)
#endif

#define LOG_ERROR(text, ...) __LOG("ERROR", text, __VA_ARGS__)
#define LOG_INFO(text, ...) __LOG("INFO", text, __VA_ARGS__)
#define LOG_DEBUG(text, ...) __LOG("DEBUG", text, __VA_ARGS__)

#endif