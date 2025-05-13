//
// Created by yuhang on 2025/3/14.
//

#ifndef FREEMARKET_TRANSLATE_H
#define FREEMARKET_TRANSLATE_H

#include <fstream>
#include <nlohmann/json.hpp>
using namespace std;

class translate {
public:
    const string langFile = "plugins/freemarket/lang.json";
    using json = nlohmann::json;
    json languageResource; // 存储从 lang.json 加载的语言资源

    // 构造函数中加载语言资源文件
    translate();

    // 加载语言资源文件
    pair<bool,string> loadLanguage() {
        if (std::ifstream f(langFile); f.is_open()) {
            languageResource = json::parse(f);
            f.close();
            return {true,"lang.json is normal"};
        } else {
            return {false,"you can download lang.json from github to change freemarket plugin language"};
        }
    }

    // 获取本地化字符串
    std::string getLocal(const std::string &key) {
        if (languageResource.find(key) != languageResource.end()) {
            return languageResource[key].get<std::string>();
        }
        return key; // 如果找不到，返回原始 key
    }
};

inline translate::translate() {
        loadLanguage();
    }

#endif //FREEMARKET_TRANSLATE_H