//
// Created by yuhang on 2025/11/17.
//

#include "market_core.h"
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

// 接入 umoney 相关实现
bool MarketCore::umoney_change_player_money(const std::string& player_name, const int money) const {
    std::ifstream ifs(umoney_file);
    if (!ifs.is_open()) {
        std::cerr << "Error: Could not open file for reading: " << umoney_file << std::endl;
        return false;
    }
    //优先使用 money_connect 插件的命令操作 umoney
    if (plugin_.getServer().getPluginManager().getPlugin("money_connect")) {
        string command = "myct umoney change \"" + player_name + "\" " + to_string(money);
        return plugin_.getServer().dispatchCommand(plugin_.getServer().getCommandSender(),command);
    }
    //未能使用 money_connect，直接数据操作
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

// 物品数据转换实现
string MarketCore::ItemMetaToString(const Meta_data& meta_data) {
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
    //lore 数据
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

// 反过来
Meta_data MarketCore::StringToItemMeta(const std::string& data) {
    auto vec = DataBase::splitString(data);
    std::vector<std::string> lore;
    std::unordered_map<std::string, int> enchants;
    std::string display_name;
    int damage = 0;

    // lore 数据 (vec[0])
    if (!vec.empty()) {
        if (std::string lore_str = vec[0]; lore_str != "None") {
            // 去除可能的花括号（旧数据中 lore 可能用花括号包裹）
            if (lore_str.front() == '{' && lore_str.back() == '}') {
                lore_str = lore_str.substr(1, lore_str.size() - 2);
            }
            // 按逗号分割 lore 行（原始逻辑）
            lore = DataBase::splitString(lore_str);
        }
    }

    // 耐久 (vec[1])
    if (vec.size() > 1) {
        try {
            damage = DataBase::stringToInt(vec[1]);
        } catch (...) {
            damage = 0;
        }
    }

    // 显示名称 (vec[2])
    if (vec.size() > 2 && vec[2] != "None") {
        display_name = vec[2];
    }

    // 附魔 (vec[3])
    if (vec.size() > 3) {
        if (std::string enchants_str = vec[3]; enchants_str != "None") {
            // 去除花括号
            if (enchants_str.front() == '{' && enchants_str.back() == '}') {
                enchants_str = enchants_str.substr(1, enchants_str.size() - 2);
            }
            // 使用原始附魔解析函数（假设存在）
            enchants = DataBase::stringToEnchants(enchants_str);
        }
    }
    Meta_data meta_data = {lore, damage, display_name, enchants};

    return meta_data;
}

// 还原回 ItemData
ItemData MarketCore::BackItemData(const string& item, const string& item_data) {
    // item 格式应为 "item_id,item_num"
    // item_data 是剩余的元数据和 NBT 部分（可能为空）
    string full_data = item;
    if (!item_data.empty()) {
        full_data += "," + item_data;   // 组合成完整字符串供 StringToItemData 解析
    }
    return StringToItemData(full_data);
}

// 将 ItemData 转换存入玩家账户的数据
std::string MarketCore::ItemDataToString(const ItemData& itemData) {
    std::stringstream string_data;
    bool has_meta = itemData.item_meta.has_value();
    bool has_nbt = itemData.nbt.has_value() && !itemData.nbt->empty();

    string_data << itemData.item_id << "," << std::to_string(itemData.item_num);

    if (has_meta) {
        string_data << "," << ItemMetaToString(itemData.item_meta.value());
    }
    if (has_nbt) {
        // 假设 NBTTools::nbtToJson 接受 const endstone::nbt::Tag&
        string_data << "," << NBTTools::nbtToJson(itemData.nbt.value()).dump();
    }
    return string_data.str();
}

// 反过来将字符串转换为 ItemData
ItemData MarketCore::StringToItemData(const std::string& string_data) {
    ItemData itemData;
    auto tokens = DataBase::splitString(string_data);
    if (tokens.size() < 2) return itemData;

    // 解析 ID 和数量
    itemData.item_id = tokens[0];
    try {
        itemData.item_num = DataBase::stringToInt(tokens[1]);
    } catch (...) {
        itemData.item_num = 0;
    }

    // 如果只有 ID 和数量，直接返回
    if (tokens.size() == 2) return itemData;

    // 剩余部分（可能包含 meta 和 nbt）
    std::string remainder = DataBase::vectorToString(std::vector<std::string>(tokens.begin() + 2, tokens.end()));

    // 优先尝试整体作为 NBT 解析（以 { 或 [ 开头）
    if (!remainder.empty() && (remainder.front() == '{' || remainder.front() == '[')) {
        auto nbt = NBTTools::stringToNbt(remainder);
        if (!nbt.empty()) {
            itemData.nbt = nbt;
            return itemData;  // NBT 已包含完整信息，meta 应为空
        } else {
            // NBT 存在但解析失败，说明数据损坏，返回空对象
            return {};
        }
    }

    // 旧格式兼容：尝试分割 meta 和 nbt（假设前4个为 meta，剩余为 nbt）
    if (tokens.size() - 2 >= 4) {
        std::vector<std::string> metaTokens(tokens.begin() + 2, tokens.begin() + 6);
        std::string nbtStr = DataBase::vectorToString(std::vector<std::string>(tokens.begin() + 6, tokens.end()));

        std::string metaStr = DataBase::vectorToString(metaTokens);
        if (!metaStr.empty()) {
            itemData.item_meta = StringToItemMeta(metaStr);
        }
        if (!nbtStr.empty()) {
            auto nbt = NBTTools::stringToNbt(nbtStr);
            if (!nbt.empty()) {
                itemData.nbt = nbt;
            } else {
                // NBT 存在但解析失败 → 数据损坏，返回空
                return {};
            }
        }
    } else {
        // 不足4个字段，全部作为 meta
        std::string metaStr = DataBase::vectorToString(std::vector<std::string>(tokens.begin() + 2, tokens.end()));
        if (!metaStr.empty()) {
            itemData.item_meta = StringToItemMeta(metaStr);
        }
    }
    return itemData;
}

vector<ItemData> MarketCore::UserItemRead(const string& uuid) {
    vector<ItemData> vec_itemData;
    if (const auto user_data = market.user_get(uuid); user_data.status) {
        for (const auto items = DataBase::splitString(user_data.item); const auto & one_item:items) {
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
    const auto vec_goods_data = DataBase::splitString(string_goods_data);
    string goods_meta_str = vec_goods_data[6];
    if (goods_meta_str.length() >= 2 && goods_meta_str.front() == '{' && goods_meta_str.back() == '}') {
        goods_meta_str = goods_meta_str.substr(1, goods_meta_str.length() - 2);
    }
    Market_Action::Goods_data goods_data = {true,DataBase::stringToInt(vec_goods_data[0]),vec_goods_data[1],vec_goods_data[2],vec_goods_data[3],vec_goods_data[4]+","+vec_goods_data[5],goods_meta_str,vec_goods_data[7],DataBase::stringToInt(vec_goods_data[8]),vec_goods_data[9],vec_goods_data[10]};
    return {true,goods_data};
}

std::unordered_map<std::string, int>
MarketCore::EnchantToSimMap(const std::unordered_map<const endstone::Enchantment*, int>& enchants) {
    std::unordered_map<std::string, int> enchant_sim_map;
    for (const auto& [fst, snd] : enchants) {
        std::string key{fst->getId()}; // fst is const Enchantment*
        enchant_sim_map[key] = snd;
    }
    return enchant_sim_map;
}

// 获取玩家资产
int MarketCore::get_player_money(const endstone::Player& player) {
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

// 通用转账系统
bool MarketCore::general_change_money(const string& uuid,const string& playername,int money) const {
    if (money_config == "freemarket") {
        if (const auto buyer_data = market.user_get(uuid); buyer_data.status) {
            const auto [fst, snd] = market.user_money(buyer_data.uuid,buyer_data.money + money);
            return fst;
        }
        return false;
    }
    return umoney_change_player_money(playername,money);
}

// 检查玩家快捷物品栏中资产并清除
bool MarketCore::check_player_inventory_and_clear(endstone::Player& player, const string& item_id, int item_num, vector<ItemData>& cleared_itemData) {
    vector<pair<pair<string, int>, int>> items;
    int player_own_num = 0;

    // 遍历玩家背包 (0-35 是快捷栏 + 背包，不包括盔甲栏和副手)
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

    int total = item_num;  // 还需要扣除的数量

    for (const auto& [fst, snd] : items) {
        if (total <= 0) {
            return true;
        }

        int slot_index = snd;
        int current_amount = fst.second;

        auto the_item = player.getInventory().getItem(slot_index);
        if (!the_item.has_value()) continue;

        // 构造被清除物品的 ItemData
        ItemData itemData;
        itemData.item_id = the_item->getType().getId();
        itemData.item_num = std::min(current_amount, total);

        // 提取 ItemMeta
        if (the_item->hasItemMeta()) {
            auto meta = the_item->getItemMeta();
            itemData.item_meta = Meta_data{
                meta->getLore(),
                meta->getDamage(),
                meta->getDisplayName(),
                EnchantToSimMap(meta->getEnchants())
            };
        }

        // 提取 NBT
        if (auto nbt = the_item->getNbt(); !nbt.empty()) {
            itemData.nbt = nbt;
        }

        cleared_itemData.push_back(itemData);

        if (current_amount < total) {
            // 情况1：清空整个槽位
            player.getInventory().setItem(slot_index, std::nullopt);
            total -= current_amount;
        } else {
            // 情况2：扣除部分，剩余放回
            int remaining = current_amount - total;
            endstone::ItemStack new_item(the_item->getType(), remaining);

            // 复制原物品的 ItemMeta 和 NBT 到新物品
            if (the_item->hasItemMeta()) {
                auto meta = the_item->getItemMeta();
                auto new_meta = meta->clone();
                new_item.setItemMeta(new_meta.get());
            }
            if (auto nbt = the_item->getNbt(); !nbt.empty()) {
                new_item.setNbt(nbt);
            }

            player.getInventory().setItem(slot_index, new_item);
            return true;  // 扣除完成
        }
    }

    return true;
}

// 检查是否允许触发事件
bool MarketCore::canTriggerEvent(const string& playername) {
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

// 检查玩家的背包是否已满
bool MarketCore::check_player_inventory_full(const endstone::Player&player) {
    for (int i =0;i <= 35;i++) {
        if (!player.getInventory().getItem(i)) {
            return false;
        }
    }
    return true;
}
