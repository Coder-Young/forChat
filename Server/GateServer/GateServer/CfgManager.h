#pragma once
#include "const.h"

typedef std::map<std::string, std::string> Map_config;

struct SectionInfo {
	SectionInfo() {}
	explicit SectionInfo(Map_config section) : _section_datas(section) {}
	~SectionInfo() {}

	SectionInfo(const SectionInfo& src) {
		_section_datas = src._section_datas;
	}

	SectionInfo& operator=(const SectionInfo& src) {
		if (&src == this)
			return *this;

		this->_section_datas = src._section_datas;
		return *this;
	}

	std::string operator[](const std::string& key) {
		if (_section_datas.find(key) == _section_datas.end()) {
			return "";
		}

		return _section_datas[key];
	}

	std::map<std::string, std::string> _section_datas;
};
class CfgManager
{
public:
	~CfgManager() = default;

	SectionInfo operator[](const std::string& key) {
		if (_config_map.find(key) == _config_map.end())
		{
			return SectionInfo();
		}

		return _config_map[key];
	}

	CfgManager();
	CfgManager(const CfgManager& src) {
		_config_map = src._config_map;
	}


	CfgManager& operator=(const CfgManager& src)
	{
		if (&src == this)
			return *this;

		_config_map = src._config_map;
		return *this;
	}

private:
	std::map<std::string, SectionInfo> _config_map;
};

