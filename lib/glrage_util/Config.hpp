#pragma once

#include <cstdint>
#include <string>
#include <map>

namespace glrage {

class Config
{
public:
    static Config& instance();

    void load(const std::string& path);
    std::string getString(
        const std::string& name, const std::string& defaultValue);
    int32_t getInt(const std::string& name, const int32_t defaultValue);
    float getFloat(const std::string& name, const float defaultValue);
    bool getBool(const std::string& name, const bool defaultValue);

private:
    Config(){};
    Config(Config const&) = delete;
    void operator=(Config const&) = delete;

    static int valueHandler(
        void* user, const char* section, const char* name, const char* value);

    std::map<std::string, std::string> m_values;
};

} // namespace glrage
