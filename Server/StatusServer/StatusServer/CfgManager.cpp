#include "CfgManager.h"

CfgManager::CfgManager()
{
	boost::filesystem::path cur_path = boost::filesystem::current_path();
	std::string fatherNode = "..";
	boost::filesystem::path config_path = cur_path / fatherNode / "config.ini";
	std::cout << "config path: " << config_path << std::endl;

	boost::property_tree::ptree pt;
	boost::property_tree::read_ini(config_path.string(), pt);

	for (const auto& section_pair : pt)
	{
		const std::string& section_name = section_pair.first;
		const boost::property_tree::ptree& section_tree = section_pair.second;

		Map_config section_config;
		for (const auto& key_value_pair : section_tree)
		{
			const std::string& key = key_value_pair.first;
			const std::string& value = key_value_pair.second.get_value <std::string>();
			section_config[key] = value;
		}

		SectionInfo sectionInfo(section_config);
		_config_map[section_name] = sectionInfo;
	}
}