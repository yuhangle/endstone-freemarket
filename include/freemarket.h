//
// Created by yuhang on 2025/4/13.
//
#ifndef FREEMARKET_H
#define FREEMARKET_H

#include "menu.h"
#include "market_core.h"
#include "database.hpp"
#include <memory>
#include <unordered_map>
#include <chrono>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

class FreeMarket : public endstone::Plugin {
public:
    // Accessors for dependency injection (replace inline globals)
    [[nodiscard]] Market_Action& getMarket() const { return *market_; }
    translate& getTranslator() { return translator_; }
    [[nodiscard]] DataBase& getDatabase() const { return *database_; }
    [[nodiscard]] const std::string& getMoneyConfig() const { return money_config_; }
    [[nodiscard]] int getPlayerMaxGoods() const { return player_max_goods_; }
    auto& getLastTriggerTime() { return last_trigger_time_; }
    [[nodiscard]] const std::string& getDataPath() const { return data_path_; }
    [[nodiscard]] const std::string& getDbFile() const { return db_file_; }
    [[nodiscard]] const std::string& getConfigPath() const { return config_path_; }
    [[nodiscard]] const std::string& getUmoneyFile() const { return umoney_file_; }

    //数据目录和配置文件检查
    void datafile_check() const;

    // 读取配置文件
    [[nodiscard]] json read_config() const;

    void onLoad() override;

    void onEnable() override;

    void onDisable() override;

    bool onCommand(endstone::CommandSender &sender, const endstone::Command &command, const std::vector<std::string> &args) override;

private:
    // Owned dependencies (replacing inline globals)
    std::unique_ptr<DataBase> database_;
    std::unique_ptr<Market_Action> market_;
    translate translator_{"plugins/freemarket/language/lang.json"};

    // Configuration
    std::string money_config_;
    int player_max_goods_ = 0;
    std::string data_path_ = "plugins/freemarket";
    std::string db_file_ = "plugins/freemarket/data.db";
    std::string config_path_ = "plugins/freemarket/config.json";
    std::string umoney_file_ = "plugins/umoney/money.json";

    // State
    std::string language_file_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_trigger_time_;

    // Owned components
    std::unique_ptr<Menu> menu_;
    std::unique_ptr<MarketCore> market_core_;
};
#endif //FREEMARKET_H
