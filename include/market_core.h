//
// Created by yuhang on 2025/11/17.
//

#ifndef FREEMARKET_MARKET_CORE_H
#define FREEMARKET_MARKET_CORE_H
#include "translate.h"
#include <nlohmann/json.hpp>
#include <database.hpp>
#include "market_action.h"
#include <string>
#include <endstone/endstone.hpp>
inline string data_path = "plugins/freemarket";
inline string db_file = "plugins/freemarket/data.db";
inline std::string config_path = "plugins/freemarket/config.json";
inline std::string umoney_file = "plugins/umoney/money.json";
using json = nlohmann::json;

inline std::string money_config;
inline int player_max_goods;

// 存储每个玩家的上次触发时间
inline std::unordered_map<string, std::chrono::steady_clock::time_point> lastTriggerTime;
//初始化语言
inline translate Tran;
//初始化其它实例
inline DataBase Database(db_file);
inline Market_Action market(Database);
//meta 数据
struct Meta_data{
    std::vector<std::string> lore;//lore 数据
    int damage{};//耐久损耗
    std::string display_name;//物品命名
    std::unordered_map< std::string, int > enchants;//物品附魔
};

//物品数据
struct ItemData{
    string item_id;
    int item_num{};
    std::optional<Meta_data> item_meta;
};
class MarketCore {
public:
    explicit MarketCore(endstone::Plugin &plugin) :plugin_(plugin) {};
    //接入 umoney

    //检查插件存在
    [[nodiscard]] bool umoney_check_exists() const;
    //获取玩家资金
    static int umoney_get_player_money(const std::string& player_name);
    //更改玩家资金
    [[nodiscard]] bool umoney_change_player_money(const std::string& player_name, const int money) const;

    //物品数据转换
    static string ItemMetaToString(const Meta_data& meta_data);

    //反过来
    static Meta_data StringToItemMeta(const string& data);

    //还原回 ItemData
    static ItemData BackItemData(const string& item,const string& item_data);

    //将 ItemData 转换存入玩家账户的数据
    static string ItemDataToString(const ItemData& itemData);

    //反过来将字符串转换为 ItemData
    static ItemData StringToItemData(const string& string_data);

    static vector<ItemData> UserItemRead(const string& uuid);

    static pair<bool,string> GoodsDataToString(const Market_Action::Goods_data& goods_data);

    static pair<bool,Market_Action::Goods_data> StringToGoodsData(const string& string_goods_data);

    static std::unordered_map<std::string, int>
    EnchantToSimMap(const std::unordered_map<const endstone::Enchantment*, int>& enchants);

    //获取玩家资产
    static int get_player_money(const endstone::Player& player);

    //通用转账系统
    [[nodiscard]] bool general_change_money(const string& uuid,const string& playername,int money) const;

    // 检查玩家快捷物品栏中资产并清除
    static bool check_player_inventory_and_clear(endstone::Player& player, const string& item_id, int item_num, vector<ItemData>& cleared_itemData);

    // 检查是否允许触发事件
    static bool canTriggerEvent(const string& playername);

    //检查玩家的背包是否已满
    static bool check_player_inventory_full(const endstone::Player&player);

private:
    endstone::Plugin &plugin_;
};


#endif //FREEMARKET_MARKET_CORE_H
