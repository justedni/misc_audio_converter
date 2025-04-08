#pragma once

#include <fstream>
#include <sstream>

namespace Utils
{
    uint16_t swap16(uint16_t x);
    int16_t swap16(int16_t x);

    template<typename T>
    T clamp(T val, T lower, T upper)
    {
        return std::max(lower, std::min(val, upper));
    }

    void peekChar(std::ifstream& stream, char* data, size_t size);

    std::string str_to_lower(const std::string& inputStr);

    template<typename T>
    T readBytes(std::ifstream& stream)
    {
        T a;
        stream.read((char*)&a, sizeof(T));
        return a;
    }

    template<typename T>
    auto writeInt(std::ostream& os, T val)
    {
        os.write(reinterpret_cast<const char*>(&val), sizeof(val));
    };

    template<typename ... Args>
    std::string str_format(const std::string& format, Args ... args)
    {
        int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
        if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
        auto size = static_cast<size_t>(size_s);
        std::unique_ptr<char[]> buf(new char[size]);
        std::snprintf(buf.get(), size, format.c_str(), args ...);
        return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    }

} // namespace Utils
