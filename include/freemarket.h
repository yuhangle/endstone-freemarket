//
// Created by yuhang on 2025/4/13.
//
#ifndef FREEMARKET_H
#define FREEMARKET_H

#include "menu.h"
#include "market_core.h"

class FreeMarket : public endstone::Plugin {
public:
    //数据目录和配置文件检查
    void datafile_check() const;

    // 读取配置文件
    [[nodiscard]] json read_config() const;

    void onLoad() override;

    void onEnable() override;

    void onDisable() override;

    bool onCommand(endstone::CommandSender &sender, const endstone::Command &command, const std::vector<std::string> &args) override;

private:
    std::unique_ptr<Menu> menu_;
    std::unique_ptr<MarketCore> market_core_;
};
#endif