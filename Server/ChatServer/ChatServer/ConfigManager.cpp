#include "ConfigManager.h"

CfgManager::CfgManager()
{
    boost::filesystem::path cur_path = boost::filesystem::current_path();
    std::string father_path = "..";
    boost::filesystem::path config_path = cur_path / father_path / "config.ini";
    std::cout << "Config path " << config_path << std::endl;

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(config_path.string(), pt);

    for (const auto& section_pair : pt){
        const std::string& section_name = section_pair.first;
        const boost::property_tree::ptree& section_tree = section_pair.second;

        std::unordered_map<std::string, std::string> section;
        for (const auto& key_value_pair : section_tree){
            const std::string& key = key_value_pair.first;
            const std::string& value = key_value_pair.second.get_value<std::string>();
            section.insert(std::make_pair(key, value));
        }

        SectionInfo sectionInfo;
        sectionInfo._section = section;
        _config_map.insert(std::make_pair(section_name, sectionInfo));
    }
}