#pragma once
#include "const.h"

struct SectionInfo
{
    SectionInfo() = default;
    ~SectionInfo() { _section.clear(); }

    std::string operator[](const std::string& key){
        if (_section.find(key) == _section.end()){
            return "";
        }

        return _section[key];
    }

    std::unordered_map<std::string, std::string> _section;
};

class CfgManager
{
public:
    CfgManager();

    SectionInfo operator[](const std::string& key)
    {
        if (_config_map.find(key) == _config_map.end()){
            return SectionInfo();
        }

        return _config_map[key];
    }
private:
    std::unordered_map<std::string, SectionInfo> _config_map;
};