#ifndef WORLD_IO_LOAD_HPP_
#define WORLD_IO_LOAD_HPP_

#include <filesystem>
#include <fstream>

#include "../world_data.hpp"
#include "types.hpp"

class BufferedReader {
public:
    BufferedReader(std::ifstream&& stream, size_t buffer_size = 1024) :
        m_stream { std::move(stream) },
        m_buffer{ new char[buffer_size] },
        m_desired_buffer_size{ buffer_size } {}

    template <typename T>
    inline T read() {
        T data;
        read(reinterpret_cast<char*>(&data), sizeof(T));
        return data;
    }

    template <typename T, size_t N>
    inline void read(T (&buf)[N]) {
        read(reinterpret_cast<char*>(buf), N * sizeof(T));
    }

    inline void read(char* dest, size_t n) {

        size_t offset = 0;
        if (m_buffer_position + n >= m_buffer_size) {
            offset = m_buffer_size - m_buffer_position;
            if (offset > 0) {
                memcpy(dest, m_buffer + m_buffer_position, offset);
            }

            m_buffer_size = m_stream.readsome(m_buffer, m_desired_buffer_size);
            m_buffer_position = 0;
        }

        n -= offset;

        if (n > 0) {
            SGE_ASSERT(m_buffer_size > 0);

            memcpy(dest + offset, m_buffer + m_buffer_position, n);
            m_buffer_position += n;
        }
    }

    inline void skip(size_t n) noexcept {
        m_buffer_position += n;
    }
    
    ~BufferedReader() {
        m_stream.close();

        if (m_buffer != nullptr) {
            delete[] m_buffer;
        }
        m_buffer = nullptr;
    }

private:
    std::ifstream m_stream;
    char* m_buffer = nullptr;
    size_t m_buffer_position = 0;
    size_t m_buffer_size = 0;
    size_t m_desired_buffer_size = 0;
};

void load_world(WorldData& world, const std::filesystem::path& path);
void read_world_header(WorldHeader& header, BufferedReader& stream);

#endif