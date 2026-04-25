//
// Created by yuhang on 2025/11/17.
//

#include "market_core.h"
#include "freemarket.h"
#include <fstream>
#include <iostream>
#include <nbt_tool.hpp>

// 检查插件存在
bool MarketCore::umoney_check_exists() const {
    if (plugin_.getServer().getPluginManager().getPlugin("umoney")) {
        return true;
    }
    return false;
}

//获取玩家资金
int MarketCore::umoney_get_player_money(const std::string& player_name) {
    std::ifstream f(dynamic_cast<FreeMarket&>(plugin_).getUmoneyFile());
    if (!f.is_open()) {
        std::cerr << "Error: Could not open file: " << dynamic_cast<FreeMarket&>(plugin_).getUmoneyFile() << std::endl;
        return -1;
    }

    try {
        if (json data = json::parse(f); data.contains(player_name)) {
            return data[player_name].get<int>();
        }
        std::cerr << "Warning: Player '" << player_name << "' not found in the data." << std::endl;
        return 0;
    } catch (const json::parse_error& e) {
        std::cerr << "Error: JSON parse error in file '" << dynamic_cast<FreeMarket&>(plugin_).getUmoneyFile() << "': " << e.what() << std::endl;
        return -1;
    }
}

// 接入 umoney 相关实现
bool MarketCore::umoney_change_player_money(const std::string& player_name, const int money) const {
    std::ifstream ifs(dynamic_cast<FreeMarket&>(plugin_).getUmoneyFile());
    if (!ifs.is_open()) {
        std::cerr << "Error: Could not open file for reading: " << dynamic_cast<FreeMarket&>(plugin_).getUmoneyFile() << std::endl;
        return false;
    }
    if (plugin_.getServer().getPluginManager().getPlugin("money_connect")) {
        string command = "myct umoney change \"" + player_name + "\" " + to_string(money);
        return plugin_.getServer().dispatchCommand(plugin_.getServer().getCommandSender(),command);
    }
    try {
        json data = json::parse(ifs);
        ifs.close();

        if (data.contains(player_name)) {
            data[player_name] = data[player_name].get<int>() + money;
        } else {
            data[player_name] = money;
        }

        std::ofstream ofs(dynamic_cast<FreeMarket&>(plugin_).getUmoneyFile());
        if (!ofs.is_open()) {
            std::cerr << "Error: Could not open file for writing: " << dynamic_cast<FreeMarket&>(plugin_).getUmoneyFile() << std::endl;
            return false;
        }
        ofs << data.dump(4);
        ofs.close();

        return true;

    } catch (const json::parse_error& e) {
        std::cerr << "Error: JSON parse error in file '" << dynamic_cast<FreeMarket&>(plugin_).getUmoneyFile() << "': " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error: An unexpected error occurred: " << e.what() << std::endl;
        return false;
    }
}

vector<ItemStackData> MarketCore::UserItemRead(const string& uuid) {
    vector<ItemStackData> vec_itemData;
    if (const auto user_data = dynamic_cast<FreeMarket&>(plugin_).getMarket().user_get(uuid); user_data.status) {
        for (const auto items = string_utils::split(user_data.item); const auto & one_item:items) {
            auto one_str = one_item.substr(1, one_item.size() - 2);
            if (one_str.empty()) {
                continue;
            }
            auto one_data = ItemSerializer::deserialize(one_str);
            vec_itemData.push_back(one_data);
        }
    }
    return vec_itemData;
}

pair<bool,string> MarketCore::GoodsDataToString(const Market_Action::Goods_data& goods_data) {
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

    return {true,string_goods};
}

pair<bool,Market_Action::Goods_data> MarketCore::StringToGoodsData(const string& string_goods_data) {
    const auto vec_goods_data = string_utils::split(string_goods_data);
    string goods_meta_str = vec_goods_data[6];
    if (goods_meta_str.length() >= 2 && goods_meta_str.front() == '{' && goods_meta_str.back() == '}') {
        goods_meta_str = goods_meta_str.substr(1, goods_meta_str.length() - 2);
    }
    Market_Action::Goods_data goods_data = {true,string_utils::to_int(vec_goods_data[0]),vec_goods_data[1],vec_goods_data[2],vec_goods_data[3],vec_goods_data[4]+","+vec_goods_data[5],goods_meta_str,vec_goods_data[7],string_utils::to_int(vec_goods_data[8]),vec_goods_data[9],vec_goods_data[10]};
    return {true,goods_data};
}

// 获取玩家资产
int MarketCore::get_player_money(const endstone::Player& player) {
    int buyer_money;
    if (dynamic_cast<FreeMarket&>(plugin_).getMoneyConfig() == "freemarket") {
        if (const auto buyer_data = dynamic_cast<FreeMarket&>(plugin_).getMarket().user_get(player.getUniqueId().str()); buyer_data.status) {
            buyer_money = buyer_data.money;
        } else {
            buyer_money = 0;
        }
    } else {
        buyer_money = umoney_get_player_money(player.getName());
    }
    return buyer_money;
}

// 通用转账系统
bool MarketCore::general_change_money(const string& uuid,const string& playername,int money) const {
    if (dynamic_cast<FreeMarket&>(plugin_).getMoneyConfig() == "freemarket") {
        if (const auto buyer_data = dynamic_cast<FreeMarket&>(plugin_).getMarket().user_get(uuid); buyer_data.status) {
            const auto [fst, snd] = dynamic_cast<FreeMarket&>(plugin_).getMarket().user_money(buyer_data.uuid,buyer_data.money + money);
            return fst;
        }
        return false;
    }
    return umoney_change_player_money(playername,money);
}

// 检查玩家快捷物品栏中资产并清除
bool MarketCore::check_player_inventory_and_clear(endstone::Player& player, const string& item_id, int item_num, vector<ItemStackData>& cleared_itemData) {
    vector<pair<pair<string, int>, int>> items;
    int player_own_num = 0;

    for (int i = 0; i <= 35; i++) {
        if (auto the_one = player.getInventory().getItem(i)) {
            if (the_one->getType().getId() == item_id) {
                player_own_num += the_one->getAmount();
                items.emplace_back(
                    std::make_pair(the_one->getType().getId(), the_one->getAmount()),
                    i
                );
            }
        }
    }

    if (player_own_num < item_num) {
        return false;
    }

    int total = item_num;

    for (const auto& [fst, snd] : items) {
        if (total <= 0) {
            return true;
        }

        int slot_index = snd;
        int current_amount = fst.second;

        auto the_item = player.getInventory().getItem(slot_index);
        if (!the_item.has_value()) continue;

        ItemStackData itemData;
        itemData.item_id = the_item->getType().getId();
        itemData.item_num = std::min(current_amount, total);

        if (the_item->hasItemMeta()) {
            auto meta = the_item->getItemMeta();
            itemData.item_meta = ItemMeta{
                meta->getLore(),
                meta->getDamage(),
                meta->getDisplayName(),
                ItemSerializer::enchantToSimMap(meta->getEnchants())
            };
        }

        if (auto nbt = the_item->getNbt(); !nbt.empty()) {
            itemData.nbt = nbt;
        }

        cleared_itemData.push_back(itemData);

        if (current_amount < total) {
            player.getInventory().setItem(slot_index, std::nullopt);
            total -= current_amount;
        } else {
            int remaining = current_amount - total;
            endstone::ItemStack new_item(the_item->getType(), remaining);

            if (the_item->hasItemMeta()) {
                auto meta = the_item->getItemMeta();
                auto new_meta = meta->clone();
                new_item.setItemMeta(new_meta.get());
            }
            if (auto nbt = the_item->getNbt(); !nbt.empty()) {
                new_item.setNbt(nbt);
            }

            player.getInventory().setItem(slot_index, new_item);
            return true;
        }
    }

    return true;
}

// 检查是否允许触发事件
bool MarketCore::canTriggerEvent(const string& playername) {
    const auto now = std::chrono::steady_clock::now();

    if (dynamic_cast<FreeMarket&>(plugin_).getLastTriggerTime().contains(playername)) {
        const auto lastTime = dynamic_cast<FreeMarket&>(plugin_).getLastTriggerTime()[playername];

        if (const auto elapsedTime = std::chrono::duration<double>(now - lastTime).count(); elapsedTime < 0.2) {
            return false;
        }
    }

    dynamic_cast<FreeMarket&>(plugin_).getLastTriggerTime()[playername] = now;
    return true;
}

// 检查玩家的背包是否已满
bool MarketCore::check_player_inventory_full(const endstone::Player&player) {
    for (int i =0;i <= 35;i++) {
        if (!player.getInventory().getItem(i)) {
            return false;
        }
    }
    return true;
}
