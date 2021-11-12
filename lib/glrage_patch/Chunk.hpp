#pragma once

#include <glrage_util/StringUtils.hpp>

#include <cstdint>
#include <vector>

namespace glrage {

class Chunk
{
public:
    Chunk()
    {
    }

    Chunk(const std::vector<uint8_t>& data)
    {
        m_data = data;
    }

    Chunk(const std::string& data)
    {
        m_data = StringUtils::hexToBytes(data);
    }

    Chunk(const char* data)
    {
        m_data = StringUtils::hexToBytes(data);
    }

    template <typename T>
    Chunk(const T& data)
    {
        *this << data;
    }

    void clear()
    {
        m_data.clear();
    }

    const std::vector<uint8_t>& data() const
    {
        return m_data;
    }

    template <class T>
    friend Chunk& operator<<(Chunk& chunk, T data)
    {
        auto dataRaw = reinterpret_cast<uint8_t const*>(&data);
        for (size_t i = 0; i < sizeof(T); i++) {
            chunk.m_data.push_back(dataRaw[i]);
        }
        return chunk;
    }

private:
    std::vector<uint8_t> m_data;
};

} // namespace glrage
