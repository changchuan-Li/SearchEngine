#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include <string>
#include <map>
#include <set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>

using std::cout;
using std::endl;
using std::string;
using std::map; 
using std::set;
using std::ifstream;
using std::istringstream;
using std::pair;
using std::cerr;

class Configuration{
public:
    Configuration(const Configuration&) = delete;
    Configuration& operator=(const Configuration&) = delete;
    static Configuration* getInstance(const char* filePath);
    map<string, string>& getConfigs(){
        return _configs;
    }
private:
    explicit Configuration(const string& filePath);
    ~Configuration();

private:
    static Configuration* _pInstance;
    string _configFilePath;
    map<string, string> _configs;
};

Configuration* Configuration::_pInstance = nullptr;
Configuration* Configuration::getInstance(const char* filePath){
    if(_pInstance == nullptr){
        _pInstance = new Configuration(string(filePath));
    }
    return _pInstance;
}

Configuration::Configuration(const string& filePath):_configFilePath(filePath){
    ifstream ifs;
    ifs.open(_configFilePath);
    if(!ifs.good()){
        cerr << "open en_file_dir faile" << endl;
        return;
    }

    string line;
    while(getline(ifs, line)){  
        // 跳过空行
        if(line.empty()) {
            continue;
        }
        
        istringstream iss(line);
        string key, value;
        // 确保正确读取到 key 和 value 后再插入
        if (iss >> key >> value) {
            _configs[key] = value; 
        }
    }
    ifs.close();
}

#endif // __CONFIGURATION_H__