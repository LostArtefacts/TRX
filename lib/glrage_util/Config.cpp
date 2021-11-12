#include "Config.hpp"
#include "StringUtils.hpp"
#include "Logger.hpp"
#include "ini.h"

#include <algorithm>

namespace glrage {

Config& Config::instance()
{
    static Config instance;
    return instance;
}

void Config::load(const std::wstring& path)
{
    std::string pathUtf = StringUtils::wideToUtf8(path);
    if (ini_parse(pathUtf.c_str(), valueHandler, this)) {
        LOG_INFO("Can't open ini file %s", pathUtf.c_str());
    }
}

std::string Config::getString(
    const std::string& name, const std::string& defaultValue)
{
    auto result = m_values.find(name);
    if (result == m_values.end()) {
        return defaultValue;
    } else {
        return result->second;
    }
}

int32_t Config::getInt(const std::string& name, const int32_t defaultValue)
{
    return std::stoi(getString(name, std::to_string(defaultValue)));
}

float Config::getFloat(const std::string& name, const float defaultValue)
{
    return std::stof(getString(name, std::to_string(defaultValue)));
}

bool Config::getBool(const std::string& name, const bool defaultValue)
{
    return getString(name, defaultValue ? "true" : "false") == "true";
}

int Config::valueHandler(void* user, const char* section, const char* name,
                            const char* value)
{
    std::string key = std::string() + section + "." + name;

    // convert to lower case for easier indexing
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);

    // inih doesn't filter out quotes like GetPrivateProfileString, so do it
    // here instead
    std::string v = value;
    v.erase(std::remove(v.begin(), v.end(), '"'), v.end());
    v.erase(std::remove(v.begin(), v.end(), '\''), v.end());

    instance().m_values[key] = v;
    return 1;
}

} // namespace glrage