//
// Created by yuhang on 2025/11/17.
//

#ifndef FREEMARKET_MARKET_CORE_H
#define FREEMARKET_MARKET_CORE_H
#include "translate.h"
#include "item_serializer.hpp"
#include "market_action.h"
#include <string>
#include <endstone/endstone.hpp>

class FreeMarket; // 前向声明

class MarketCore {
public:
    explicit MarketCore(endstone::Plugin &plugin) :plugin_(plugin) {};
    //接入 umoney

    //检查插件存在
    [[nodiscard]] bool umoney_check_exists() const;
    //获取玩家资金
    [[nodiscard]] int umoney_get_player_money(const std::string& player_name) const;
    //更改玩家资金
    [[nodiscard]] bool umoney_change_player_money(const std::string& player_name, int money) const;

    //读取玩家可提现物品
    [[nodiscard]] vector<ItemStackData> UserItemRead(const string& uuid) const;

    static pair<bool,string> GoodsDataToString(const Market_Action::Goods_data& goods_data);

    static pair<bool,Market_Action::Goods_data> StringToGoodsData(const string& string_goods_data);

    //获取玩家资产
    [[nodiscard]] int get_player_money(const endstone::Player& player) const;

    //通用转账系统
    [[nodiscard]] bool general_change_money(const string& uuid,const string& playername,int money) const;

    // 检查玩家快捷物品栏中资产并清除
    static bool check_player_inventory_and_clear(endstone::Player& player, const string& item_id, int item_num, vector<ItemStackData>& cleared_itemData);

    // 检查是否允许触发事件
    [[nodiscard]] bool canTriggerEvent(const string& playername) const;

    //检查玩家的背包是否已满
    static bool check_player_inventory_full(const endstone::Player&player);

private:
    endstone::Plugin &plugin_;
};


#endif //FREEMARKET_MARKET_CORE_H
