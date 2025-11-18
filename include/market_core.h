//
// Created by yuhang on 2025/11/17.
//

#ifndef FREEMARKET_MARKET_CORE_H
#define FREEMARKET_MARKET_CORE_H
#include "translate.h"
#include <nlohmann/json.hpp>
#include <ranges>
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
//meta数据
struct Meta_data{
    std::vector<std::string> lore;//lore数据
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
    //接入umoney

    //检查插件存在
    [[nodiscard]] bool umoney_check_exists() const {
        if (plugin_.getServer().getPluginManager().getPlugin("umoney")) {
            return true;
        }
        return false;
    }
    //获取玩家资金
    static int umoney_get_player_money(const std::string& player_name) {
        std::ifstream f(umoney_file);
        if (!f.is_open()) {
            std::cerr << "Error: Could not open file: " << umoney_file << std::endl;
            return -1; // 返回一个错误值表示文件无法打开
        }

        try {
            if (json data = json::parse(f); data.contains(player_name)) {
                return data[player_name].get<int>();
            }
            std::cerr << "Warning: Player '" << player_name << "' not found in the data." << std::endl;
            return 0; // 返回 0 表示玩家不存在或没有资金记录
        } catch (const json::parse_error& e) {
            std::cerr << "Error: JSON parse error in file '" << umoney_file << "': " << e.what() << std::endl;
            return -1; // 返回一个错误值表示 JSON 解析失败
        }
    }
    //更改玩家资金
    [[nodiscard]] bool umoney_change_player_money(const std::string& player_name, const int money) const {
        std::ifstream ifs(umoney_file);
        if (!ifs.is_open()) {
            std::cerr << "Error: Could not open file for reading: " << umoney_file << std::endl;
            return false;
        }
        //优先使用money_connect插件的命令操作umoney
        if (plugin_.getServer().getPluginManager().getPlugin("money_connect")) {
            string command = "myct umoney change \"" + player_name + "\" " + to_string(money);
            return plugin_.getServer().dispatchCommand(plugin_.getServer().getCommandSender(),command);
        }
        //未能使用money_connect,直接数据操作
        try {
            json data = json::parse(ifs);
            ifs.close(); // 关闭读取流

            if (data.contains(player_name)) {
                data[player_name] = data[player_name].get<int>() + money;
            } else {
                // 如果玩家不存在，则创建一个新记录
                data[player_name] = money;
            }

            std::ofstream ofs(umoney_file);
            if (!ofs.is_open()) {
                std::cerr << "Error: Could not open file for writing: " << umoney_file << std::endl;
                return false;
            }
            ofs << data.dump(4); // 将修改后的 JSON 写回文件
            ofs.close();

            return true;

        } catch (const json::parse_error& e) {
            std::cerr << "Error: JSON parse error in file '" << umoney_file << "': " << e.what() << std::endl;
            return false;
        } catch (const std::exception& e) {
            std::cerr << "Error: An unexpected error occurred: " << e.what() << std::endl;
            return false;
        }
    }

    //物品数据转换
    static string ItemMetaToString(const Meta_data& meta_data) {
        string lore;
        const string damage = to_string(meta_data.damage);
        string display_name;
        string enchants;
        //名字
        if (!meta_data.display_name.empty()) {
            display_name = meta_data.display_name;
        } else {
            display_name = "None";
        }
        //lore数据
        if (!meta_data.lore.empty()) {
            lore = "{" + DataBase::vectorToString(meta_data.lore) + "}";
        }else {
            lore = "None";
        }
        //附魔数据
        if (!meta_data.enchants.empty()) {
            enchants = "{" + DataBase::enchantsToString(meta_data.enchants) + "}";
        } else {
            enchants = "None";
        }
        return lore+","+damage+","+display_name+","+enchants;
    }

    //反过来
    static Meta_data StringToItemMeta(const string& data) {
        auto vec = DataBase::splitString(data);
        std::vector<std::string> lore;
        std::unordered_map<std::string, int> enchants;
        string display_name;

        //lore数据
        if (string lore_str = vec[0]; lore_str != "None") {
            lore_str = lore_str.substr(1, lore_str.size() - 2);
            lore = DataBase::splitString(lore_str);
        }
        //显示名字
        if (vec[2] != "None") {
            display_name = vec[2];
        }
        //耐久
        const int damage = DataBase::stringToInt(vec[1]);
        //附魔
        if (string enchants_str = vec[3]; enchants_str != "None") {
            enchants_str = enchants_str.substr(1, enchants_str.size() - 2);
            enchants = DataBase::stringToEnchants(enchants_str);
        }

        return Meta_data{lore,damage,display_name, enchants};
    }

    //还原回ItemData
    static ItemData BackItemData(const string& item,const string& item_data) {
        Meta_data the_item_data;
        if (item_data.empty()) {
            the_item_data = {};
        } else {
            the_item_data = StringToItemMeta(item_data);
        }
        auto item_type_num = DataBase::splitString(item);
        ItemData itemData = {item_type_num[0],DataBase::stringToInt(item_type_num[1]),the_item_data};
        return itemData;
    }

    //将ItemData转换存入玩家账户的数据
    static string ItemDataToString(const ItemData& itemData) {
        string string_data;
        if (itemData.item_meta.has_value()) {
            string_data = itemData.item_id+","+to_string(itemData.item_num)+","+ ItemMetaToString(itemData.item_meta.value());
        } else {
            string_data = itemData.item_id+","+to_string(itemData.item_num);
        }
        return string_data;
    }

    //反过来将字符串转换为ItemData
    static ItemData StringToItemData(const string& string_data) {
        ItemData itemData;
        auto vec = DataBase::splitString(string_data);
        //有metadata
        if (vec.size()>2) {
            itemData.item_id = vec[0];
            itemData.item_num = DataBase::stringToInt(vec[1]);
            vec.erase(vec.begin(), vec.begin() + 2);
            string string_vec = DataBase::vectorToString(vec);
            auto meta = StringToItemMeta(string_vec);
            itemData.item_meta = meta;
        } else {
            itemData.item_id = vec[0];
            itemData.item_num = DataBase::stringToInt(vec[1]);
        }
        return itemData;
    }

    static vector<ItemData> UserItemRead(const string& uuid) {
        vector<ItemData> vec_itemData;
        auto user_data = market.user_get(uuid);
        if (user_data.status) {
            auto items = DataBase::splitString(user_data.item);
            for (const auto & one_item:items) {
                auto one_str = one_item.substr(1, one_item.size() - 2);
                if (one_str.empty()) {
                    continue;
                }
                auto one_data = StringToItemData(one_str);
                vec_itemData.push_back(one_data);
            }
        }
        return vec_itemData;
    }

    static pair<bool,string> GoodsDataToString(const Market_Action::Goods_data& goods_data) {
        if (!goods_data.status) {
            return {false,"Goods data is empty"};
        }
        string string_goods_data;
        if (!goods_data.data.empty()) {
            string_goods_data = "{"+goods_data.data+"}";
        } else {
            string_goods_data = "None";
        }
        auto string_goods = to_string(goods_data.gid) + "," + goods_data.uuid + "," + goods_data.name + "," + goods_data.text + "," + goods_data.item + "," +
                            string_goods_data + "," + goods_data.image + "," + to_string(goods_data.price) + "," + goods_data.money_type + "," + goods_data.tag;
        cout << string_goods << endl;
        return {true,string_goods};
    }

    static pair<bool,Market_Action::Goods_data> StringToGoodsData(const string& string_goods_data) {
        auto vec_goods_data = DataBase::splitString(string_goods_data);
        string goods_meta_str = vec_goods_data[6];
        if (goods_meta_str.length() >= 2 && goods_meta_str.front() == '{' && goods_meta_str.back() == '}') {
            goods_meta_str = goods_meta_str.substr(1, goods_meta_str.length() - 2);
        }
        Market_Action::Goods_data goods_data = {true,DataBase::stringToInt(vec_goods_data[0]),vec_goods_data[1],vec_goods_data[2],vec_goods_data[3],vec_goods_data[4]+","+vec_goods_data[5],goods_meta_str,vec_goods_data[7],DataBase::stringToInt(vec_goods_data[8]),vec_goods_data[9],vec_goods_data[10]};
        return {true,goods_data};
    }

    static std::unordered_map<std::string, int>
    EnchantToSimMap(const std::unordered_map<const endstone::Enchantment*, int>& enchants) {
        std::unordered_map<std::string, int> enchant_sim_map;
        for (const auto& [fst, snd] : enchants) {
            std::string key{fst->getId().getKey()}; // fst is const Enchantment*
            enchant_sim_map[key] = snd;
        }
        return enchant_sim_map;
    }

    //获取玩家资产
    static int get_player_money(const endstone::Player& player) {
        int buyer_money;
        if (money_config == "freemarket") {
            if (const auto buyer_data = market.user_get(player.getUniqueId().str()); buyer_data.status) {
                buyer_money = buyer_data.money;
            } else {
                buyer_money = 0;
            }
        } else {
            buyer_money = umoney_get_player_money(player.getName());
        }
        return buyer_money;
    }

    //通用转账系统
    [[nodiscard]] bool general_change_money(const string& uuid,const string& playername,int money) const {
        if (money_config == "freemarket") {
            if (const auto buyer_data = market.user_get(uuid); buyer_data.status) {
                const auto [fst, snd] = market.user_money(buyer_data.uuid,buyer_data.money + money);
                return fst;
            }
            return false;
        }
        return umoney_change_player_money(playername,money);
    }

    //检查玩家快捷物品栏中资产并清除
    static bool check_player_inventory_and_clear(endstone::Player&player,const string& item_id,int item_num,vector<ItemData> &cleared_itemData) {
        vector<pair<pair<string,int>,int>> items;
        int player_own_num = 0;
        for (int i = 0;i <= 35;i++) {
            if (auto the_one = player.getInventory().getItem(i)) {
                if (the_one->getType().getKey() == item_id) {
                    player_own_num += the_one->getAmount();
                    items.emplace_back(
                        std::make_pair(the_one->getType().getKey(), the_one->getAmount()),
                        i
                    );
                }
            }
        }
        if (player_own_num >= item_num) {
            int total = item_num;
            for (const auto&[fst, snd] : items) {
                //剩余总数小于等于0直接返回true(理论上不会执行)
                if (total <= 0) {
                    return true;
                }
                //物品槽位
                int slot_index = snd;
                //此槽位物品数
                //槽位物品数少于所需则清空此槽
                if (int current_amount = fst.second; current_amount < total) {
                    auto the_one = player.getInventory().getItem(slot_index);
                    ItemData itemData;
                    if (the_one->hasItemMeta()) {
                        auto meta = the_one->getItemMeta();
                        itemData.item_id = the_one->getType().getKey();
                        itemData.item_num = current_amount;
                        itemData.item_meta = {meta->getLore(),meta->getDamage(),meta->getDisplayName(),EnchantToSimMap(meta->getEnchants())};
                    } else {
                        itemData.item_id = the_one->getType().getKey();
                        itemData.item_num = current_amount;
                    }
                    cleared_itemData.push_back(itemData);
                    player.getInventory().setItem(slot_index, nullptr);
                    total -= current_amount;
                }
                //槽位物品数大于等于所需则设置为减去所需的数量并结束
                else {
                    int amount = current_amount - total;
                    auto the_item = player.getInventory().getItem(slot_index);
                    //构造ItemData
                    ItemData itemData;
                    if (the_item->hasItemMeta()) {
                        auto meta = the_item->getItemMeta();
                        itemData.item_id = the_item->getType().getKey();
                        itemData.item_num = total;
                        itemData.item_meta = Meta_data{meta->getLore(),meta->getDamage(),meta->getDisplayName(),EnchantToSimMap(meta->getEnchants())};
                    } else {
                        itemData.item_id = the_item->getType().getKey();
                        itemData.item_num = total;
                    }
                    //将获取的数据返回去
                    cleared_itemData.push_back(itemData);
                    //构建减去的物品设置给玩家
                    endstone::ItemStack new_item(the_item->getType(),amount);
                    if (the_item->hasItemMeta()) {
                        auto meta = the_item->getItemMeta();
                        auto new_meta = meta->clone();
                        new_item.setItemMeta(new_meta.get());
                    }
                    player.getInventory().setItem(slot_index,&new_item);
                    return true;
                }
            }
            return true;
        }
        return false;
    }

    // 检查是否允许触发事件
    static bool canTriggerEvent(const string& playername) {
        const auto now = std::chrono::steady_clock::now();

        // 查找玩家的上次触发时间
        if (lastTriggerTime.contains(playername)) {
            const auto lastTime = lastTriggerTime[playername];

            // 如果时间差小于 0.2 秒，不允许触发
            if (const auto elapsedTime = std::chrono::duration<double>(now - lastTime).count(); elapsedTime < 0.2) {
                return false;
            }
        }

        // 更新玩家的上次触发时间为当前时间
        lastTriggerTime[playername] = now;
        return true;
    }

    //检查玩家的背包是否已满
    static bool check_player_inventory_full(const endstone::Player&player) {
        for (int i =0;i <= 35;i++) {
            if (!player.getInventory().getItem(i)) {
                return false;
            }
        }
        return true;
    }

private:
    endstone::Plugin &plugin_;
};


#endif //FREEMARKET_MARKET_CORE_H