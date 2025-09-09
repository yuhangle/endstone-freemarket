//
// Created by yuhang on 2025/4/13.
//
#ifndef FREEMARKET_H
#define FREEMARKET_H

#include <endstone/plugin/plugin.h>
#include <endstone/inventory/meta/item_meta.h>
#include <iostream>
#include <fstream>
#include <ranges>
#include <string>
#include <filesystem>
#include "endstone/endstone.hpp"
#include "Market_Action.h"
#include "translate.h"
#include <nlohmann/json.hpp>
#include <endstone/form/form.h>
#include <endstone/form/action_form.h>
#include <endstone/player.h>
#include <endstone/form/modal_form.h>
#include <endstone/server.h>
#include <endstone/event/player/player_interact_event.h>

using json = nlohmann::json;

const string data_path = "plugins/freemarket";
const string db_file = "plugins/freemarket/data.db";
const std::string config_path = "plugins/freemarket/config.json";
const std::string umoney_file = "plugins/umoney/money.json";

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
    std::optional<std::vector<std::string>> lore;//lore数据
    int damage{};//耐久损耗
    std::optional<std::string> display_name;//物品命名
    std::optional<std::unordered_map< std::string, int >> enchants;//物品附魔
};

//物品数据
struct ItemData{
    string item_id;
    int item_num{};
    std::optional<Meta_data> item_meta;
};

class FreeMarket : public endstone::Plugin {
public:

    //数据目录和配置文件检查
    void datafile_check() const {
        json df_config = {
                {"money", "freemarket"},
                {"player_max_goods", 10}
        };

        if (!(std::filesystem::exists(data_path))) {
            getLogger().info(Tran.getLocal("No data path,auto create"));
            std::filesystem::create_directory(data_path);
            if (!(std::filesystem::exists(config_path))) {
                std::ofstream file(config_path);
                if (file.is_open()) {
                    file << df_config.dump(4);
                    file.close();
                    getLogger().info(Tran.getLocal("Config file created"));
                }
            }
        } else if (std::filesystem::exists(data_path)) {
            if (!(std::filesystem::exists(config_path))) {
                std::ofstream file(config_path);
                if (file.is_open()) {
                    file << df_config.dump(4);
                    file.close();
                    getLogger().info(Tran.getLocal("Config file created"));
                }
            } else {
                bool need_update = false;
                json loaded_config;

                // 加载现有配置文件
                std::ifstream file(config_path);
                file >> loaded_config;

                // 检查配置完整性并更新
                for (auto& [key, value] : df_config.items()) {
                    if (!loaded_config.contains(key)) {
                        loaded_config[key] = value;
                        getLogger().info(Tran.getLocal("Config '{}' has update with default config")+","+ key);
                        need_update = true;
                    }
                }

                // 如果需要更新配置文件，则进行写入
                if (need_update) {
                    std::ofstream outfile(config_path);
                    if (outfile.is_open()) {
                        outfile << loaded_config.dump(4);
                        outfile.close();
                        getLogger().info(Tran.getLocal("Config file update over"));
                    }
                }
            }
        }
    }

    // 读取配置文件
    [[nodiscard]] json read_config() const {
        std::ifstream i(config_path);
        try {
            json j;
            i >> j;
            return j;
        } catch (json::parse_error& ex) { // 捕获解析错误
            getLogger().error( ex.what());
            json error_value = {
                    {"error","error"}
            };
            return error_value;
        }
    }

    //接入umoney

    //检查插件存在
    [[nodiscard]] bool umoney_check_exists() const {
        if (getServer().getPluginManager().getPlugin("umoney")) {
            return true;
        } else {
            return false;
        }
    }
    //获取玩家资金
    static int umoney_get_player_money(const std::string& player_name) {
        std::ifstream f(umoney_file);
        if (!f.is_open()) {
            std::cerr << "Error: Could not open file: " << umoney_file << std::endl;
            return -1; // 返回一个错误值表示文件无法打开
        }

        try {
            json data = json::parse(f);
            if (data.contains(player_name)) {
                return data[player_name].get<int>();
            } else {
                std::cerr << "Warning: Player '" << player_name << "' not found in the data." << std::endl;
                return 0; // 返回 0 表示玩家不存在或没有资金记录
            }
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
        if (getServer().getPluginManager().getPlugin("money_connect")) {
            string command = "myct umoney change \"" + player_name + "\" " + to_string(money);
            return getServer().dispatchCommand(getServer().getCommandSender(),command);
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
        string damage = to_string(meta_data.damage);
        string display_name;
        string enchants;
        //名字
        if (meta_data.display_name.has_value()) {
            display_name = meta_data.display_name.value();
        } else {
            display_name = "None";
        }
        //lore数据
        if (meta_data.lore.has_value()) {
            lore = "{" + DataBase::vectorToString(meta_data.lore.value()) + "}";
        }else {
            lore = "None";
        }
        //附魔数据
        if (meta_data.enchants.has_value()) {
            enchants = "{" + DataBase::enchantsToString(meta_data.enchants.value()) + "}";
        } else {
            enchants = "None";
        }
        return lore+","+damage+","+display_name+","+enchants;
    }

    //反过来
    static Meta_data StringToItemMeta(const string& data) {
        auto vec = DataBase::splitString(data);
        std::optional<std::vector<std::string>> lore;
        optional<std::unordered_map<std::string, int>> enchants;
        std::optional<string> display_name;

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

    //获取玩家资产
    static int get_player_money(const endstone::Player& player) {
        int buyer_money;
        if (money_config == "freemarket") {
            auto buyer_data = market.user_get(player.getUniqueId().str());
            if (buyer_data.status) {
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
            auto buyer_data = market.user_get(uuid);
            if (buyer_data.status) {
                auto status = market.user_money(buyer_data.uuid,buyer_data.money + money);
                return status.first;
            } else {
                return false;
            }
        } else {
            return umoney_change_player_money(playername,money);
        }
    }

    //检查玩家快捷物品栏中资产并清除
    static bool check_player_inventory_and_clear(endstone::Player&player,const string& item_id,int item_num,vector<ItemData> &cleared_itemData) {
        vector<pair<pair<string,int>,int>> items;
        int player_own_num = 0;
        for (int i = 0;i <= 35;i++) {
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
        if (player_own_num >= item_num) {
            int total = item_num;
            for (const auto& item_info : items) {
                //剩余总数小于等于0直接返回true(理论上不会执行)
                if (total <= 0) {
                    return true;
                }
                //物品槽位
                int slot_index = item_info.second;
                //此槽位物品数
                int current_amount = item_info.first.second;
                //槽位物品数少于所需则清空此槽
                if (current_amount < total) {
                    auto the_one = player.getInventory().getItem(slot_index);
                    ItemData itemData;
                    if (the_one->hasItemMeta()) {
                        auto meta = the_one->getItemMeta();
                        itemData.item_id = the_one->getType().getId();
                        itemData.item_num = current_amount;
                        itemData.item_meta = {meta->getLore(),meta->getDamage(),meta->getDisplayName(),meta->getEnchants()};
                    } else {
                        itemData.item_id = the_one->getType().getId();
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
                        itemData.item_id = the_item->getType().getId();
                        itemData.item_num = total;
                        itemData.item_meta = {meta->getLore(),meta->getDamage(),meta->getDisplayName(),meta->getEnchants()};
                    } else {
                        itemData.item_id = the_item->getType().getId();
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
        auto now = std::chrono::steady_clock::now();

        // 查找玩家的上次触发时间
        if (lastTriggerTime.contains(playername)) {
            auto lastTime = lastTriggerTime[playername];
            auto elapsedTime = std::chrono::duration<double>(now - lastTime).count(); // 计算时间差

            // 如果时间差小于 0.2 秒，不允许触发
            if (elapsedTime < 0.2) {
                return false;
            }
        }

        // 更新玩家的上次触发时间为当前时间
        lastTriggerTime[playername] = now;
        return true;
    }

    void onLoad() override
    {
        getLogger().info("onLoad is called");
        //初始化目录
        if (!(filesystem::exists(data_path))) {
            getLogger().info("No data path,auto create");
            filesystem::create_directory(data_path);
        }
        //加载语言
        pair lang_status = Tran.loadLanguage();
        getLogger().info(lang_status.second);
        #ifdef __linux__
        namespace fs = std::filesystem;
        try {
            // 获取当前路径
            const fs::path currentPath = fs::current_path();
            //std::cout << "Current path is: " << currentPath << std::endl;

            // 子目录路径
            const fs::path subdir = "plugins/freemarket/data.db";

            // 拼接路径
            const fs::path fullPath = currentPath / subdir;
            //std::cout << "Full path is: " << fullPath << std::endl;

            // 如果需要将最终路径转换为 string 类型
            const std::string finalPathStr = fullPath.string();
            //std::cout << "Final path as string is: " << finalPathStr << std::endl;
            // 使用完整路径重新初始化Database
            Database = DataBase(finalPathStr);
            market = Market_Action(Database);
        }
        catch (const fs::filesystem_error& e) {
            std::cerr << "Filesystem error: " << e.what() << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "General error: " << e.what() << std::endl;
        }
        #endif
    }

    void onEnable() override
    {
        getLogger().info("onEnable is called");
        datafile_check();
        if (!DataBase::fileExists(db_file)) {
            getLogger().info(endstone::ColorFormat::Yellow + Tran.getLocal("Database file not find,auto create"));
        }
        (void)Database.init_database();
        //进行一个配置文件的读取
        json json_msg = read_config();
        try {
            if (!json_msg.contains("error")) {
                money_config = json_msg["money"];
                player_max_goods = json_msg["player_max_goods"];
                if (json_msg["money"] == "umoney") {
                    if (!umoney_check_exists()) {
                        money_config = "freemarket";
                        getLogger().error(Tran.getLocal("Umoney plugin not find,use freemarket economy system"));
                    }
                    else {
                        money_config = "umoney";
                        getLogger().info(Tran.getLocal("Use umoney plugin"));
                    }
                } else {
                    money_config = "freemarket";
                    getLogger().info(endstone::ColorFormat::Yellow+Tran.getLocal("Use freemarket economy system"));
                }
            } else {
                getLogger().error(Tran.getLocal("Config file error!Use default config"));
                money_config = "freemarket";
                player_max_goods = 10;
            }
        } catch (const std::exception& e) {
            money_config = "freemarket";
            player_max_goods = 10;
            getLogger().error(Tran.getLocal("Config file error!Use default config"));
            getLogger().error(e.what());
        }
        registerEvent(&FreeMarket::onOpenMenu,*this);
        //显示启动消息
        getLogger().info(endstone::ColorFormat::Yellow + "FreeMarket插件已启动。版本: " + getServer().getPluginManager().getPlugin("freemarket")->getDescription().getVersion());
    }

    void onDisable() override
    {
        getLogger().info("onDisable is called");
    }

    bool onCommand(endstone::CommandSender &sender, const endstone::Command &command, const std::vector<std::string> &args) override
    {
        if (command.getName() == "market")
        {
            //控制台命令
            if (!sender.asPlayer()) {
                getLogger().error(Tran.getLocal("The console cannot use this command"));
                return false;
            }
            //玩家命令
            else {
                if (args.empty()) {
                    auto player = getServer().getPlayer(sender.getName());
                    main_menu(*player);
                }
                //注册
                else if (args[0] == "register") {
                    try {
                        if (!args[1].empty()) {
                            const string& username = args[1];
                            string playername = sender.getName();
                            string uuid = getServer().getPlayer(playername)->getUniqueId().str();
                            if (market.user_exist(uuid)) {
                                sender.sendErrorMessage(Tran.getLocal("You have already registered and cannot register again"));
                                return false;
                            }
                            string avatar;
                            if (args[2].empty()) {
                                avatar = "textures/ui/icon_steve";
                            } else {
                                avatar = args[2];
                            }
                            auto status = market.user_add(uuid,playername,username,avatar,"",0);
                            if (status.first) {
                                sender.sendMessage(Tran.getLocal(status.second));
                                return true;
                            } else {
                                sender.sendErrorMessage(Tran.getLocal(status.second));
                                return false;
                            }
                        } else {sender.sendErrorMessage(Tran.getLocal("Missing parameters"));}
                    } catch (const std::exception &e) {
                        //返回错误给玩家
                        sender.sendErrorMessage(e.what());
                    }
                }
                //自带经济系统
                else if (args[0] == "money") {
                    try {
                        if (money_config != "freemarket") {
                            sender.sendErrorMessage(Tran.getLocal("The built-in economy system of freemarket not enable,you can not use this command"));
                            return false;
                        }
                        if (!sender.hasPermission("OP")) {
                            sender.sendErrorMessage(Tran.getLocal("You are not op,you can not use this command"));
                            return false;
                        }
                        if (!args[1].empty() && !args[2].empty() && !args[3].empty()) {
                            int money = DataBase::stringToInt(args[3]);
                            if (money == 0) {
                                sender.sendErrorMessage(Tran.getLocal("Invalid integer value"));
                                return false;
                            }
                            auto userdata =market.user_get_by_playername(args[2]);
                            if (!userdata.status) {
                                sender.sendErrorMessage(Tran.getLocal("This player is not registered"));
                                return false;
                            }
                            if (args[1] == "less") {
                                money = -money;
                            }
                            auto status = market.user_money(userdata.uuid,userdata.money + money);
                            if (status.first) {
                                sender.sendMessage(Tran.getLocal(status.second));
                                sender.sendMessage(Tran.getLocal("The player's current money is: ") + to_string(userdata.money + money));
                                return true;
                            } else {
                                sender.sendErrorMessage(Tran.getLocal(status.second));
                                return false;
                            }
                        } else {sender.sendErrorMessage(Tran.getLocal("Missing parameters"));}
                    }  catch (const std::exception &e) {
                        //返回错误给玩家
                        sender.sendErrorMessage(e.what());
                    }
                }
            }
        }
        return true;
    }

    //菜单部分

    //通知菜单
    static void notice_menu(endstone::Player& player,const string& msg,const std::function<void(endstone::Player&)>& yes_func) {
        endstone::MessageForm menu;
        menu.setTitle(Tran.getLocal("Notice"));
        menu.setContent(msg);
        menu.setButton1(Tran.getLocal("Back"));
        menu.setButton2(Tran.getLocal("Exit"));
        // 设置按钮回调
        menu.setOnSubmit([=, &player](endstone::Player*, int test) { // 使用捕获列表捕获 player
            if (test == 0 && yes_func) { // Yes 被点击
                yes_func(player);
            } else { // No 被点击
                player.closeForm();
            }
        });
        player.sendForm(menu);
    }

    //主菜单
    void main_menu(endstone::Player& player) {
        endstone::ActionForm menu;
        menu.setTitle(Tran.getLocal("FreeMarket Menu"));
        menu.setContent(Tran.getLocal("§l§o§bWelcome! You can open the menu by entering the /market command or right-clicking with emerald"));

        auto userdata = market.user_get(player.getUniqueId().str());
        if (!userdata.status) {
            menu.addButton(Tran.getLocal("§l§5Register Account"),"textures/ui/icon_steve",[this](endstone::Player *p) {
                register_menu(*p);});
        }
        else {
            menu.addButton(Tran.getLocal("§l§5Account Information"),userdata.avatar,[this](endstone::Player *p) {
                account_menu(*p);});
            menu.addButton(Tran.getLocal("§l§5Trading Market"),"textures/ui/teams_icon",[this](endstone::Player *p) {
                market_display_menu(*p);});
            menu.addButton(Tran.getLocal("§l§5Add Items to Market"),"textures/ui/icon_blackfriday",[this](endstone::Player*p){
                goods_upload_menu(*p);
            });
            menu.addButton(Tran.getLocal("§l§5Manage goods"),"textures/ui/icon_setting",[this](endstone::Player*p){
                choose_manage_goods_menu(*p);
            });
        }
        player.sendForm(menu);
    }

    //玩家注册菜单
    void register_menu(endstone::Player& player) {
        endstone::ModalForm menu;
        menu.setTitle(Tran.getLocal("Register menu"));
        //标签
        endstone::Label label;
        label.setText(Tran.getLocal("You has not an freemarket account,please register in this menu."));
        //输入用户名
        endstone::TextInput input;
        input.setLabel(Tran.getLocal("Input your username"));
        //选择头像
        endstone::Dropdown dropdown;
        dropdown.setLabel(Tran.getLocal("Select an avatar.\nThe menu only supports presets,you can change it to custom later"));
        dropdown.setOptions({Tran.getLocal("Steve"),Tran.getLocal("Alex"),Tran.getLocal("Minions"),Tran.getLocal("Panda")});
        dropdown.setDefaultIndex(0);
        //注册功能
        menu.addControl(label);
        menu.addControl(input);
        menu.addControl(dropdown);
        menu.setOnSubmit([=](const endstone::Player *p,const string& response)
                         {
                             auto parsedResponse = json::parse(response);
                             string username = parsedResponse[1];
                             int avatar_index = parsedResponse[2];
                             string avatar;
                             if (avatar_index == 0) {
                                 avatar = "textures/ui/icon_steve";
                             } else if (avatar_index == 1) {
                                 avatar = "textures/ui/icon_alex";
                             } else if (avatar_index == 2) {
                                 avatar = "textures/ui/sidebar_icons/Minions_packicon_0";
                             } else if (avatar_index == 3) {
                                 avatar = "textures/ui/icon_panda";
                             } else {
                                 avatar = "texture/ui/icon_steve";
                             }
                             const string command = "market register \"" + username + "\"" +" " + "\"" +avatar + "\"";
                             (void)p->performCommand(command);
                         });

        menu.setOnClose([this](endstone::Player* p){
            main_menu(*p);
        });
        player.sendForm(menu);
    }

    //账户信息主菜单
    void account_menu(endstone::Player& player) {
        endstone::ActionForm menu;
        menu.setTitle(Tran.getLocal("Account Info Menu"));
        auto user_data = market.user_get(player.getUniqueId().str());
        //头像按钮
        endstone::Button change_avatar;
        change_avatar.setIcon("textures/ui/icon_multiplayer");
        change_avatar.setText(Tran.getLocal("Change account avatar"));
        change_avatar.setOnClick([this](endstone::Player *p){
            player_avatar_menu(*p);
        });
        //重命名
        endstone::Button rename;
        rename.setText(Tran.getLocal("Rename account"));
        rename.setIcon("textures/ui/book_edit_default");
        rename.setOnClick([this](endstone::Player* p){
            player_rename_menu(*p);
        });
        //货款提现
        endstone::Button get_payment;
        get_payment.setText(Tran.getLocal("Withdraw the payment"));
        get_payment.setIcon("textures/ui/trade_icon");
        get_payment.setOnClick([this](endstone::Player* p){
            get_item_to_player_menu(*p);
        });
        //交易记录
        endstone::Button record;
        record.setText(Tran.getLocal("Transaction Records"));
        record.setIcon("textures/ui/icon_timer");
        record.setOnClick([this](endstone::Player* p){
            recordMainMenu(*p);
        });

        menu.setControls({change_avatar,rename,get_payment,record});

        menu.setOnClose([this](endstone::Player* p){
            main_menu(*p);
        });
        int money;
        if (money_config == "freemarket") {
            money = user_data.money;
        } else {
            money = umoney_get_player_money(player.getName());
        }
        menu.setContent(Tran.getLocal("User name: ") + user_data.username + "\n" + endstone::ColorFormat::Green +Tran.getLocal("Money: ") + to_string(money));
        player.sendForm(menu);
    }
    //玩家头像更改菜单
    void player_avatar_menu(endstone::Player& player) {
        endstone::ModalForm menu;
        menu.setTitle(Tran.getLocal("Change Avatar Menu"));
        //标签
        endstone::Label label;
        label.setText(Tran.getLocal("Change the avatar via a dropdown box or an input box. \nIf text is entered in the input box, use the input box."));
        //下拉栏
        endstone::Dropdown dropdown;
        dropdown.setLabel(Tran.getLocal("Select an avatar."));
        dropdown.setOptions({Tran.getLocal("Steve"),Tran.getLocal("Alex"),Tran.getLocal("Minions"),Tran.getLocal("Panda")});
        dropdown.setDefaultIndex(0);
        //输入框
        endstone::TextInput textInput;
        textInput.setLabel(Tran.getLocal("Custom Avatar"));
        textInput.setPlaceholder("textures/ui/icon_steve");
        //构建菜单
        menu.addControl(label);
        menu.addControl(dropdown);
        menu.addControl(textInput);
        //修改逻辑
        menu.setOnSubmit([=,this](endstone::Player *p,const string& response) {
            auto parsedResponse = json::parse(response);
            string custom = parsedResponse[2];
            int avatar_index = parsedResponse[1];
            string avatar;
            if (custom.empty()) {
                if (avatar_index == 0) {
                    avatar = "textures/ui/icon_steve";
                } else if (avatar_index == 1) {
                    avatar = "textures/ui/icon_alex";
                } else if (avatar_index == 2) {
                    avatar = "textures/ui/sidebar_icons/Minions_packicon_0";
                } else if (avatar_index == 3) {
                    avatar = "textures/ui/icon_panda";
                } else {
                    avatar = "texture/ui/icon_steve";
                }
            } else {
                avatar = custom;
            }
            auto status = market.user_change_avatar(p->getUniqueId().str(),avatar);
            if (status.first) {
                notice_menu(*p,Tran.getLocal(status.second),[this](endstone::Player& pp) { account_menu(pp);});
            } else {
                notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal(status.second),[this](endstone::Player& p) { this->account_menu(p);});
            }
        });

        menu.setOnClose([this](endstone::Player* p){
            account_menu(*p);
        });

        player.sendForm(menu);
    }

    //玩家重命名菜单
    void player_rename_menu(endstone::Player& player) {
        endstone::ModalForm menu;
        menu.setTitle(Tran.getLocal("Rename Menu"));
        //标签
        endstone::Label label;
        label.setText(Tran.getLocal("Change your user name"));
        //输入框
        endstone::TextInput textInput;
        textInput.setLabel(Tran.getLocal("Input new name"));
        textInput.setDefaultValue(market.GetUsername(player.getUniqueId().str()));
        //构建菜单
        menu.addControl(label);
        menu.addControl(textInput);
        //修改逻辑
        menu.setOnSubmit([this](endstone::Player *p,const string& response) {
            auto parsedResponse = json::parse(response);
            string name = parsedResponse[1];
            if (name.empty()) {
                return ;
            }
            auto status = market.user_rename(p->getUniqueId().str(),name);
            if (status.first) {
                notice_menu(*p,Tran.getLocal(status.second),[this](endstone::Player& pp) { account_menu(pp);});
            } else {
                notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal(status.second),[this](endstone::Player& p) { this->account_menu(p);});
            }
        });

        menu.setOnClose([this](endstone::Player* p){
            account_menu(*p);
        });
        player.sendForm(menu);
    }


    //商品上传菜单
    void goods_upload_menu(endstone::Player& player) {
        int player_goods_amount = Database.getGoodsCountByUuid(player.getUniqueId().str());
        if (player_goods_amount > player_max_goods || player_goods_amount < 0) {
            notice_menu(player,Tran.getLocal("You have reached the maximum limit for your goods"),[this](endstone::Player& p) { this->main_menu(p);});
            return;
        }
        endstone::ModalForm menu;
        menu.setTitle(Tran.getLocal("Add Goods Menu"));
        vector<ItemData> quick_items;
        vector<string> quick_items_name;
        for (int i = 0;i <= 8;i++) {
            if (auto one_item = player.getInventory().getItem(i)) {
                auto meta = one_item->getItemMeta();
                //存在meta数据
                if (one_item->hasItemMeta()){
                    //附魔情况
                    if (meta->hasEnchants()) {
                        quick_items.push_back({string(one_item->getType().getId()),one_item->getAmount(), Meta_data{meta->getLore(),meta->getDamage(),meta->getDisplayName(),meta->getEnchants()}});
                        quick_items_name.push_back(to_string(i+1) + ". "+ Tran.getLocal("Item ID: ") + string(one_item->getType().getId()) + Tran.getLocal("Number: ") +
                                                           to_string(one_item->getAmount()));
                    }
                    //无附魔
                    else {
                        quick_items.push_back({string(one_item->getType().getId()),one_item->getAmount(), Meta_data{meta->getLore(),meta->getDamage(),meta->getDisplayName()}});
                        quick_items_name.push_back(to_string(i+1) + ". "+ Tran.getLocal("Item ID: ") + string(one_item->getType().getId()) + Tran.getLocal("Number: ") +
                                                           to_string(one_item->getAmount()));
                    }
                }
                //不存在meta数据
                else {
                    quick_items.push_back({string(one_item->getType().getId()),one_item->getAmount(), nullopt});
                    quick_items_name.push_back(to_string(i+1) + ". " + Tran.getLocal("Item ID: ") + string(one_item->getType().getId()) + Tran.getLocal("Number: ") +
                                                                            to_string(one_item->getAmount()));
                }
            } else {
                quick_items.push_back({"None"});
                quick_items_name.emplace_back(to_string(i+1) + ". " + Tran.getLocal("None"));
            }
        }
        endstone::Dropdown chose_item;
        chose_item.setOptions(quick_items_name);
        menu.addControl(chose_item);

        menu.setOnSubmit([this, quick_items](endstone::Player*p,const string& response){
            auto json_response = json::parse(response);
            int item_index = json_response[0];
            const auto& the_item = quick_items[item_index];
            if (the_item.item_id == "None") {
                notice_menu(*p,Tran.getLocal("You have not selected a valid item"),[this](endstone::Player& p) { this->main_menu(p);});
                return;
            }
            goods_upload_confirm_menu(*p,the_item,item_index);
        });
        menu.setOnClose([this](endstone::Player*p){
            main_menu(*p);
        });
        player.sendForm(menu);
    }

    void goods_upload_confirm_menu(endstone::Player& player,const ItemData& item_data,int quick_index) {
        endstone::ModalForm menu;
        menu.setTitle(Tran.getLocal("Confirm to Add Goods Menu"));
        endstone::Label goods_info;
        goods_info.setText(Tran.getLocal("Goods ID: ") + item_data.item_id + "\n" + Tran.getLocal("Number: ") + to_string(item_data.item_num));

        endstone::TextInput title;
        endstone::TextInput text;
        endstone::Dropdown money_type;
        endstone::TextInput custom_money;
        endstone::TextInput price_Input;
        endstone::Dropdown image_drop;
        endstone::Dropdown tag_drop;

        title.setLabel(Tran.getLocal("Goods title"));
        title.setDefaultValue(Tran.getLocal("One unit of goods"));
        text.setLabel(Tran.getLocal("Goods description")),
        text.setDefaultValue(Tran.getLocal("The seller didn't write a description"));
        vector<string> money_list = {"money","minecraft:diamond","minecraft:emerald","minecraft:gold_ingot","minecraft:iron_ingot","Custom"};
        vector<string> money_name_list = {Tran.getLocal("money"),Tran.getLocal("minecraft:diamond"),Tran.getLocal("minecraft:emerald"),Tran.getLocal("minecraft:gold_ingot"),Tran.getLocal("minecraft:iron_ingot"),Tran.getLocal("Custom")};
        money_type.setOptions(money_name_list);
        money_type.setLabel(Tran.getLocal("Select the transaction settlement method"));
        money_type.setDefaultIndex(0);

        custom_money.setLabel(Tran.getLocal("Custom settlement currency id"));
        custom_money.setPlaceholder(Tran.getLocal("The value entered here will be used as the settlement currency when you select custom"));

        price_Input.setLabel(Tran.getLocal("Enter a price"));
        price_Input.setPlaceholder("114514");
        vector<string> image_name_opt = {Tran.getLocal("Apple"),Tran.getLocal("Tools & Equipment"),Tran.getLocal("Diamond"),Tran.getLocal("Grass block")};
        vector<string> image_opt = {"textures/ui/icon_apple","textures/ui/icon_recipe_equipment","textures/items/diamond","textures/blocks/grass_side_carried"};
        image_drop.setLabel(Tran.getLocal("Select a cover"));
        image_drop.setOptions(image_name_opt);
        image_drop.setDefaultIndex(0);

        tag_drop.setLabel(Tran.getLocal("Set goods tag"));
        vector<string> tag_opt = {"Food","Tools & Equipment","Ore","Block","Other"};
        vector<string> tag_name_opt = {Tran.getLocal("Food"),Tran.getLocal("Tools & Equipment"),Tran.getLocal("Ore"),Tran.getLocal("Block"),Tran.getLocal("Other")};
        tag_drop.setOptions(tag_name_opt);
        tag_drop.setDefaultIndex(0);

        menu.setControls({title,text,money_type,custom_money,price_Input,image_drop,tag_drop});

        menu.setOnSubmit([this, item_data, quick_index, image_opt, money_list, tag_opt](endstone::Player*p,const string& response){
            auto json_response = json::parse(response);
            string the_title = json_response[0];
            string the_text = json_response[1];
            int money_index = json_response[2];
            string the_custom_money = json_response[3];
            string price_str = json_response[4];
            int image_index = json_response[5];
            int tag_index = json_response[6];
            int price = DataBase::stringToInt(price_str);
            if (the_title.empty()||the_text.empty()||(money_index == 5&&the_custom_money.empty())||price <= 0) {
                notice_menu(*p,endstone::ColorFormat::Red + Tran.getLocal("Title and description can not be empty,and when you select custom currency,custom currency can not be empty"),[this](endstone::Player& p) { this->main_menu(p);});
                return;
            }
                //不使用自定义货币
            if (money_index != 5) {
                if (item_data.item_meta.has_value()) {
                    auto string_meta = ItemMetaToString(item_data.item_meta.value());
                    auto status = market.goods_add(p->getUniqueId().str(),the_title,the_text,item_data.item_id+","+to_string(item_data.item_num),string_meta,image_opt[image_index],price,money_list[money_index],tag_opt[tag_index]);
                    if (status.first) {
                        notice_menu(*p,Tran.getLocal(status.second),[this](endstone::Player& p) { this->main_menu(p);});
                    } else {
                        notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal(status.second),[this](endstone::Player& p) { this->main_menu(p);});
                        return;
                    }
                } else {
                    auto status = market.goods_add(p->getUniqueId().str(),the_title,the_text,item_data.item_id+","+to_string(item_data.item_num),"",image_opt[image_index],price,money_list[money_index],tag_opt[tag_index]);
                    if (status.first) {
                        notice_menu(*p,Tran.getLocal(status.second),[this](endstone::Player& p) { this->main_menu(p);});
                    } else {
                        notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal(status.second),[this](endstone::Player& p) { this->main_menu(p);});
                        return;
                    }
                }
            } else {
                //使用自定义货币
                if (item_data.item_meta.has_value()) {
                    auto string_meta = ItemMetaToString(item_data.item_meta.value());
                    auto status = market.goods_add(p->getUniqueId().str(),the_title,the_text,item_data.item_id+","+to_string(item_data.item_num),string_meta,image_opt[image_index],price,the_custom_money,tag_opt[tag_index]);
                    if (status.first) {
                        notice_menu(*p,Tran.getLocal(status.second),[this](endstone::Player& p) { this->main_menu(p);});
                    } else {
                        notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal(status.second),[this](endstone::Player& p) { this->main_menu(p);});
                        return;
                    }
                } else {
                    auto status = market.goods_add(p->getUniqueId().str(),the_title,the_text,item_data.item_id+","+to_string(item_data.item_num),"",image_opt[image_index],price,the_custom_money,tag_opt[tag_index]);
                    if (status.first) {
                        notice_menu(*p,Tran.getLocal(status.second),[this](endstone::Player& p) { this->main_menu(p);});
                    } else {
                        notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal(status.second),[this](endstone::Player& p) { this->main_menu(p);});
                        return;
                    }
                }
            }
            const endstone::ItemStack *air = nullptr;
            p->getInventory().setItem(quick_index,air);
            getServer().broadcastMessage("§l§2"+Tran.getLocal("[Marketing Promotion] New goods have been listed: ")+"§r"+the_title);
        });
        menu.setOnClose([this](endstone::Player*p){
            goods_upload_menu(*p);
        });
        player.sendForm(menu);
    }

    //市场展示主菜单
    void market_display_menu(endstone::Player& player) {
        endstone::ActionForm menu;
        menu.setTitle(Tran.getLocal("Market"));
        menu.addButton("\uE021 " + endstone::ColorFormat::MaterialDiamond+Tran.getLocal("§lSearch")+" \uE021",nullopt,[this](endstone::Player* p){
            search_goods_menu(*p);
        });
        auto goods_data = market.goods_get_all();
        if (goods_data.empty()) {
            menu.setContent(Tran.getLocal("No goods are currently for sale"));
        } else {
            for (auto & goods : std::ranges::reverse_view(goods_data)) { // 使用反向迭代器
                menu.addButton(goods.name, goods.image, [this, goods](endstone::Player* p) {
                    goods_view_menu(*p, goods);
                });
            }
        }
        menu.setOnClose([this](endstone::Player* p){
            main_menu(*p);
        });
        player.sendForm(menu);
    }

    //商品查看菜单
    void goods_view_menu(endstone::Player& player,const Market_Action::Goods_data& goods_data) {
        auto seller_data = market.user_get(goods_data.uuid);
        int buyer_money;
        if (money_config == "freemarket") {
            auto buyer_data = market.user_get(player.getUniqueId().str());
            if (buyer_data.status) {
                buyer_money = buyer_data.money;
            } else {
                buyer_money = 0;
            }
        } else {
            buyer_money = umoney_get_player_money(player.getName());
        }
        string seller_info = Tran.getLocal("Unknown");
        if (seller_data.status) {
            seller_info = seller_data.username + "("+seller_data.playername+")";
        }
        endstone::ActionForm menu;
        menu.setTitle(goods_data.name);

        endstone::Label goods_info;
        endstone::Label buyer_info;
        endstone::Button confirm_buy;
        //物品数据
        string dis_goods_meta;
        if (!goods_data.data.empty()) {
            Meta_data goods_meta = StringToItemMeta(goods_data.data);
            dis_goods_meta += Tran.getLocal("Item damage: ")+to_string(goods_meta.damage);
            if (goods_meta.display_name.has_value()) {
                dis_goods_meta += "\n" + Tran.getLocal("Name tag: ")+goods_meta.display_name.value();
            }
            dis_goods_meta += "§r";
            if (goods_meta.lore.has_value()) {
                dis_goods_meta += "\n lore: ";
                for (const auto& one_lore:goods_meta.lore.value()) {
                    dis_goods_meta += one_lore+", ";
                }
            }
            if (goods_meta.enchants.has_value()) {
                dis_goods_meta += "\n" + Tran.getLocal("Enchant: ");
                for (const auto&[fst, snd]:goods_meta.enchants.value()) {
                    dis_goods_meta += fst + ":" + to_string(snd)+", ";
                }
            }
        }

        goods_info.setText(Tran.getLocal("Goods name: ")+goods_data.name+"\n"+Tran.getLocal("Goods description: ")+goods_data.text+"\n"+Tran.getLocal("Goods Info: ")+goods_data.item+"\n"+dis_goods_meta+"\n"
                            +Tran.getLocal("Price: ")+to_string(goods_data.price)+"\n"+Tran.getLocal("Currency: ")+Tran.getLocal(goods_data.money_type)+"\n"+
                            Tran.getLocal("Seller info: ")+seller_info);

        buyer_info.setText(Tran.getLocal(Tran.getLocal("Your money: ") + to_string(buyer_money)));

        confirm_buy.setText(Tran.getLocal("Buy it"));
        confirm_buy.setIcon("textures/ui/confirm");
        confirm_buy.setOnClick([this, goods_data](endstone::Player*p){
            confirm_to_buy_menu(*p,goods_data);
        });

        if (seller_data.status) {
            endstone::Button seller_home_page;
            seller_home_page.setText(Tran.getLocal("Seller homepage"));
            seller_home_page.setIcon(seller_data.avatar);
            seller_home_page.setOnClick([this, seller_data](endstone::Player* p){
                seller_homepage(*p,seller_data);
            });
            menu.setControls({seller_home_page,goods_info,buyer_info,confirm_buy});
        } else {
            menu.setControls({buyer_info,confirm_buy});
        }
        menu.setOnClose([this](endstone::Player*p){
            market_display_menu(*p);
        });
        player.sendForm(menu);
    }

    //确认购买
    void confirm_to_buy_menu(endstone::Player& player,const Market_Action::Goods_data& goods_data) {
        endstone::MessageForm menu;
        menu.setTitle(Tran.getLocal("Confirm Menu"));
        menu.setButton1(Tran.getLocal("Yes,It will be mine"));
        menu.setButton2(Tran.getLocal("No,I don't want to buy it"));
        menu.setContent(Tran.getLocal("Confirm to buy it?"));

        menu.setOnSubmit([this, goods_data](endstone::Player* p,int chose){
            if (chose == 0) {
                //确认购买

                //最后确认一遍商品存在
                if (!market.goods_exist(goods_data.gid)) {
                    notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal("The item does not exist"),[this](endstone::Player& p) { this->main_menu(p);});
                    return;
                }

                auto seller_data = market.user_get(goods_data.uuid);
                //使用数字货币
                if (goods_data.money_type == "money") {
                    //进行一个资产的判断
                    if (int buyer_money = get_player_money(*p); buyer_money >= goods_data.price) {
                        (void)general_change_money(p->getUniqueId().str(),p->getName(),-goods_data.price);
                        auto itemData = BackItemData(goods_data.item,goods_data.data);
                        //构建ItemStack
                        endstone::ItemStack new_item(itemData.item_id,itemData.item_num);
                        auto meta_ptr = getServer().getItemFactory().getItemMeta(new_item.getType());
                        endstone::ItemMeta* meta = meta_ptr.get();
                        // 设置 ItemMeta 属性
                        meta->setLore(itemData.item_meta->lore);
                        meta->setDamage(itemData.item_meta->damage);
                        meta->setDisplayName(itemData.item_meta->display_name);
                        //有附魔
                        if (itemData.item_meta->enchants.has_value()) {
                            for (const auto &one_enchant:itemData.item_meta->enchants.value()) {
                                meta->addEnchant(one_enchant.first, one_enchant.second,false);
                            }
                        }
                        // 将 meta 应用到 new_item
                        new_item.setItemMeta(nullptr);
                        new_item.setItemMeta(meta);
                        // 将物品添加到玩家的背包
                        p->getInventory().addItem(new_item);
                        auto delGoodsStatus = market.goods_del(goods_data.gid);
                        (void)general_change_money(seller_data.uuid,seller_data.playername,goods_data.price);
                        if (auto get_seller = getServer().getPlayer(seller_data.playername)) {
                            get_seller->sendMessage(Tran.getLocal("Your goods have been purchased. Transaction details: ")+"\n" + Tran.getLocal("Buyer: ") + p->getName() +"\n"+
                                                                Tran.getLocal("Goods info: ") + goods_data.name);
                        }
                        notice_menu(*p,Tran.getLocal("Successful purchase"),[this](endstone::Player& p){ main_menu(p);});
                        getServer().broadcastMessage("§l§2"+Tran.getLocal("[Marketing Promotion] This good has been purchased: ")+"§r"+goods_data.name);
                        //记录此次交易
                        auto [fst, snd] = GoodsDataToString(goods_data);
                        (void)market.record_add(seller_data.uuid,p->getUniqueId().str(),snd);
                    } else {
                        notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal("You have not enough money"),[this,goods_data](endstone::Player& p){ goods_view_menu(p,goods_data);});
                    }
                }
                //使用实体货币
                else {
                    auto itemData = BackItemData(goods_data.item,goods_data.data);
                    if (vector<ItemData> cleared_itemData; check_player_inventory_and_clear(*p,goods_data.money_type,goods_data.price,cleared_itemData)) {
                        //构建ItemStack
                        endstone::ItemStack new_item(itemData.item_id,itemData.item_num);
                        auto meta_ptr = getServer().getItemFactory().getItemMeta(new_item.getType());
                        endstone::ItemMeta* meta = meta_ptr.get();
                        // 设置 ItemMeta 属性
                        meta->setLore(itemData.item_meta->lore);
                        meta->setDamage(itemData.item_meta->damage);
                        meta->setDisplayName(itemData.item_meta->display_name);
                        //有附魔
                        if (itemData.item_meta->enchants.has_value()) {
                            for (const auto &one_enchant:itemData.item_meta->enchants.value()) {
                                meta->addEnchant(one_enchant.first, one_enchant.second,false);
                            }
                        }
                        // 将 meta 应用到 new_item
                        new_item.setItemMeta(meta);
                        // 将物品添加到玩家的背包
                        p->getInventory().addItem(new_item);
                        //删除货品
                        auto delGoodsStatus = market.goods_del(goods_data.gid);
                        //通知卖家(如果在线)
                        if (auto get_seller = getServer().getPlayer(seller_data.playername)) {
                            get_seller->sendMessage(Tran.getLocal("Your goods have been purchased. Transaction details: ")+"\n" + Tran.getLocal("Buyer: ") + p->getName() +"\n"+
                                                    Tran.getLocal("Goods info: ") + goods_data.name);
                        }
                        //把物品打到商家帐上
                        for(const auto& one_item:cleared_itemData) {
                            string string_itemData = ItemDataToString(one_item);
                            (void)market.user_add_item(seller_data.uuid,string_itemData);
                        }
                        notice_menu(*p,Tran.getLocal("Successful purchase"),[this](endstone::Player& p) { this->main_menu(p);});
                        getServer().broadcastMessage("§l§2"+Tran.getLocal("[Marketing Promotion] This good has been purchased: ")+"§r"+goods_data.name);
                        //记录此次交易
                        auto [fst, snd] = GoodsDataToString(goods_data);
                        (void)market.record_add(seller_data.uuid,p->getUniqueId().str(),snd);
                    } else {
                        notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal("You have not enough money"),[this](endstone::Player& p) { this->main_menu(p);});
                        return;
                    }
                }
            } else {
                goods_view_menu(*p,goods_data);
            }
        });
        menu.setOnClose([this, goods_data](endstone::Player*p){
            goods_view_menu(*p,goods_data);
        });
        player.sendForm(menu);
    }

    //货款提现菜单
    void get_item_to_player_menu(endstone::Player& player) {
        const auto user_all_item = UserItemRead(player.getUniqueId().str());
        if (user_all_item.empty()) {
            notice_menu(player,endstone::ColorFormat::Red+Tran.getLocal("You have no withdrawable payment"),[this](endstone::Player& p){ account_menu(p);});
            return;
        }
        endstone::ModalForm menu;
        menu.setTitle(Tran.getLocal("Withdraw the payment"));
        string display_info;
        for (const auto& one_item:user_all_item) {

            //物品数据
            string dis_goods_meta;
            if (one_item.item_meta.has_value()) {
                Meta_data goods_meta = one_item.item_meta.value();
                dis_goods_meta += Tran.getLocal("Item damage: ")+to_string(goods_meta.damage);
                if (goods_meta.display_name.has_value()) {
                    dis_goods_meta += "\n" + Tran.getLocal("Name tag: ")+goods_meta.display_name.value();
                }
                dis_goods_meta += "§r";
                if (goods_meta.lore.has_value()) {
                    dis_goods_meta += "\n lore: ";
                    for (const auto& one_lore:goods_meta.lore.value()) {
                        dis_goods_meta += one_lore+", ";
                    }
                }
                if (goods_meta.enchants.has_value()) {
                    dis_goods_meta += "\n" + Tran.getLocal("Enchant: ");
                    for (const auto&[fst, snd]:goods_meta.enchants.value()) {
                        dis_goods_meta += fst + ":" + to_string(snd)+", ";
                    }
                }
            }
            display_info += one_item.item_id + " x " + to_string(one_item.item_num) + "\n" + dis_goods_meta + "\n --------------------\n";
        }
        endstone::Label context;
        context.setText(Tran.getLocal("Your payment: ") +"\n"+display_info);
        menu.setControls({context});
        menu.setOnSubmit([this](endstone::Player* p,const string& response){
            const auto update_user_items = UserItemRead(p->getUniqueId().str());
            for (const auto& one_user_item:update_user_items) {
                endstone::ItemStack itemStack(one_user_item.item_id,one_user_item.item_num);
                if (one_user_item.item_meta.has_value()) {
                    auto meta_ptr = getServer().getItemFactory().getItemMeta(itemStack.getType());
                    endstone::ItemMeta* meta = meta_ptr.get();
                    const auto meta_value = one_user_item.item_meta;
                    meta->setLore(meta_value->lore);
                    meta->setDamage(meta_value->damage);
                    meta->setDisplayName(meta_value->display_name);
                    //有附魔
                    if (meta_value->enchants.has_value()) {
                        for (const auto& one_enchant:meta_value->enchants.value()) {
                            meta->addEnchant(one_enchant.first, one_enchant.second,false);
                        }
                    }
                    itemStack.setItemMeta(meta);
                    p->getInventory().addItem(itemStack);
                } else {
                    p->getInventory().addItem(itemStack);
                }
            }
            (void)market.user_clear_item(p->getUniqueId().str());
            notice_menu(*p,Tran.getLocal("The withdrawal is complete"),[this](endstone::Player& p){ account_menu(p);});
        });
        menu.setOnClose([this](endstone::Player *p){
            account_menu(*p);
        });
        player.sendForm(menu);
    }

    //选择管理商品菜单
    void choose_manage_goods_menu(endstone::Player &player){
        auto all_user_goods = market.goods_get_by_uuid(player.getUniqueId().str());
        if (all_user_goods.empty()) {
            notice_menu(player,endstone::ColorFormat::Red+Tran.getLocal("You have not goods"),[this](endstone::Player& p){ main_menu(p);});
            return;
        }
        endstone::ModalForm menu;
        menu.setTitle(Tran.getLocal("Manage Goods Menu"));
        endstone::Dropdown goods_list;
        vector<string> goods_name_list;
        int i=0;
        for(const auto& one_goods:all_user_goods) {
            i++;
            goods_name_list.push_back(to_string(i)+". "+one_goods.name);
        }
        goods_list.setLabel(Tran.getLocal("Choose goods to manage"));
        goods_list.setOptions(goods_name_list);
        menu.setControls({goods_list});
        menu.setOnSubmit([this, all_user_goods](endstone::Player* p,const string& response){
            auto json_response = json::parse(response);
            int goods_index = json_response[0];
            const auto& the_goods_data = all_user_goods[goods_index];
            manage_goods_menu(*p,the_goods_data);
        });
        menu.setOnClose([this](endstone::Player*p){
            main_menu(*p);
        });
        player.sendForm(menu);
    }

    //管理商品子菜单
    void manage_goods_menu(endstone::Player &player,const Market_Action::Goods_data& goods_data) {
        endstone::ActionForm menu;
        menu.setTitle(Tran.getLocal("Manage Goods Menu"));
        endstone::Button edit_button;
        endstone::Button del_button;

        edit_button.setText(Tran.getLocal("Edit goods info"));
        edit_button.setOnClick([this, goods_data](endstone::Player*p){
            edit_goods_menu(*p,goods_data);
        });

        del_button.setText(Tran.getLocal("Delete goods and return it"));
        del_button.setOnClick([this, goods_data](endstone::Player* p){
            del_goods_menu(*p,goods_data);
        });
        menu.setOnClose([this](endstone::Player*p){
            main_menu(*p);
        });
        menu.setControls({edit_button,del_button});
        player.sendForm(menu);
    }

    //删除商品菜单
    void del_goods_menu(endstone::Player& player,const Market_Action::Goods_data& goods_data){
        endstone::MessageForm menu;
        menu.setTitle(Tran.getLocal("Delete Goods Menu"));
        menu.setContent(Tran.getLocal("Are you sure to delete it?"));
        menu.setButton1(Tran.getLocal("Yes,delete it and return it"));
        menu.setButton2(Tran.getLocal("No,I don't want to delete it"));
        menu.setOnSubmit([this, goods_data](endstone::Player* p,int bt_i){
            if (bt_i == 0) {
                auto update_goods_data = market.goods_get_by_gid(goods_data.gid);
                if (!update_goods_data.status) {
                    notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal("The goods not exist"),[this](endstone::Player& p) { this->choose_manage_goods_menu(p);});
                    return;
                }
                auto status = market.goods_del(update_goods_data.gid);
                    if (status.first) {
                        auto itemData = BackItemData(goods_data.item,goods_data.data);
                        endstone::ItemStack itemStack(itemData.item_id,itemData.item_num);
                        if (itemData.item_meta.has_value()) {
                            auto meta_ptr = getServer().getItemFactory().getItemMeta(itemStack.getType());
                            endstone::ItemMeta* meta = meta_ptr.get();
                            meta->setLore(itemData.item_meta->lore);
                            meta->setDamage(itemData.item_meta->damage);
                            meta->setDisplayName(itemData.item_meta->display_name);
                            //附魔
                            if (itemData.item_meta->enchants.has_value()) {
                                for (const auto &one_enchant: itemData.item_meta->enchants.value()) {
                                    meta->addEnchant(one_enchant.first,one_enchant.second,false);
                                }
                            }
                            itemStack.setItemMeta(meta);
                        }
                        p->getInventory().addItem(itemStack);
                        notice_menu(*p,Tran.getLocal(status.second),[this](endstone::Player& p) { this->choose_manage_goods_menu(p);});
                        getServer().broadcastMessage("§l§2"+Tran.getLocal("[Marketing Promotion] This good has been delisted: ")+"§r"+goods_data.name);
                    } else {
                        notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal(status.second),[this](endstone::Player& p) { this->choose_manage_goods_menu(p);});
                    }
            }
        });
        menu.setOnClose([this, goods_data](endstone::Player*p){
            manage_goods_menu(*p,goods_data);
        });
        player.sendForm(menu);
    }

    //编辑商品菜单
    void edit_goods_menu(endstone::Player& player,const Market_Action::Goods_data& goods_data) {
        endstone::ModalForm menu;
        menu.setTitle(Tran.getLocal("Edit Goods Menu"));

        endstone::TextInput title_input;
        endstone::TextInput text_input;
        endstone::Dropdown image_drop;
        endstone::TextInput image_custom;
        endstone::Dropdown tag_drop;

        title_input.setDefaultValue(goods_data.name);
        title_input.setLabel(Tran.getLocal("Goods title"));

        text_input.setDefaultValue(goods_data.text);
        text_input.setLabel(Tran.getLocal("Goods description"));

        vector<string> image_name_opt = {Tran.getLocal("Apple"),Tran.getLocal("Tools & Equipment"),Tran.getLocal("Diamond"),Tran.getLocal("Grass block")};
        vector<string> image_opt = {"textures/ui/icon_apple","textures/ui/icon_recipe_equipment","textures/items/diamond","textures/blocks/grass_side_carried"};
        image_drop.setLabel(Tran.getLocal("Select a cover"));
        image_drop.setOptions(image_name_opt);
        image_drop.setDefaultIndex(0);

        image_custom.setLabel(Tran.getLocal("Set custom cover"));

        tag_drop.setLabel(Tran.getLocal("Set goods tag"));
        vector<string> tag_opt = {"Food","Tools & Equipment","Ore","Block","Other"};
        vector<string> tag_name_opt = {Tran.getLocal("Food"),Tran.getLocal("Tools & Equipment"),Tran.getLocal("Ore"),Tran.getLocal("Block"),Tran.getLocal("Other")};
        tag_drop.setOptions(tag_name_opt);
        tag_drop.setDefaultIndex(0);

        menu.setControls({title_input,text_input,image_drop,image_custom,tag_drop});

        menu.setOnSubmit([this, goods_data, image_opt, tag_opt](endstone::Player*p,const string& response){
            auto json_response = json::parse(response);
            string the_title = json_response[0];
            string the_text = json_response[1];
            int image_index = json_response[2];
            string custom_image = json_response[3];
            int tag_index = json_response[4];
            if (the_title.empty()||the_text.empty()||(image_index == 3 && custom_image.empty())) {
                notice_menu(*p,endstone::ColorFormat::Red + Tran.getLocal("Title and description can not be empty,and when you select custom cover,custom cover can not be empty"),[this](endstone::Player& pp) { main_menu(pp);});
                return;
            }
            //check_goods_exist
            if (!market.goods_exist(goods_data.gid)) {
                notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal("Goods not exists"),[this](endstone::Player& p) { this->choose_manage_goods_menu(p);});
                return;
            }
            string image;
            if (image_index == 3) {
                image = custom_image;
            } else {
                image = image_opt[image_index];
            }
            auto status = market.goods_update(goods_data.gid,the_title,the_text,image,tag_opt[tag_index]);
            if (status.first) {
                notice_menu(*p,Tran.getLocal(status.second),[this](endstone::Player& p) { this->choose_manage_goods_menu(p);});
            } else {
                notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal(status.second),[this](endstone::Player& p) { this->choose_manage_goods_menu(p);});
                }
        });
        menu.setOnClose([this, goods_data](endstone::Player*p){
            manage_goods_menu(*p,goods_data);
        });
        player.sendForm(menu);
    }

    //显示商家主页
    void seller_homepage(endstone::Player& player,Market_Action::User_data seller_data) {
        auto seller_all_goods = market.goods_get_by_uuid(seller_data.uuid);
        if (seller_all_goods.empty()) {
            notice_menu(player,endstone::ColorFormat::Red+Tran.getLocal("Seller has not goods"),[this](endstone::Player& p) { this->market_display_menu(p);});
            return;
        }
        if (!seller_data.status) {
            notice_menu(player,endstone::ColorFormat::Red+Tran.getLocal("Seller not exists"),[this](endstone::Player& p) { this->market_display_menu(p);});
            return;
        }
        endstone::ActionForm menu;
        menu.setTitle(Tran.getLocal("Seller homepage"));
        menu.addHeader(seller_data.username);
        //TODO
        //menu.addButton(Tran.getLocal("Customer reviews"),nullopt,[=](endstone::Player*p){
        //    cout << 1 << endl;;
        //});
        menu.addDivider();
        menu.addLabel(Tran.getLocal("Seller goods: ")+"\n");
        for (const auto& one_goods:seller_all_goods) {
            menu.addButton(one_goods.name,one_goods.image,[this, one_goods](endstone::Player*p){
                goods_view_menu(*p,one_goods);
            });
        }
        menu.setOnClose([this](endstone::Player*p){
            market_display_menu(*p);
        });
        player.sendForm(menu);
    }

    //搜索菜单
    void search_goods_menu(endstone::Player& player) {
        endstone::ModalForm menu;
        menu.setTitle(Tran.getLocal("Search Goods Menu"));
        endstone::TextInput search_text_input;
        endstone::Dropdown tag_dropdown;

        search_text_input.setLabel(Tran.getLocal("Search goods"));
        tag_dropdown.setLabel(Tran.getLocal("Tag"));
        vector<string> tag_opt = {"All","Food","Tools & Equipment","Ore","Block","Other"};
        vector<string> tag_name_opt = {Tran.getLocal("All"),Tran.getLocal("Food"),Tran.getLocal("Tools & Equipment"),Tran.getLocal("Ore"),Tran.getLocal("Block"),Tran.getLocal("Other")};
        tag_dropdown.setOptions(tag_name_opt);
        menu.setControls({search_text_input,tag_dropdown});
        menu.setOnSubmit([this, tag_opt](endstone::Player* p,const string& response){
            auto json_response = json::parse(response);
            string search_text = json_response[0];
            int tag_index = json_response[1];
            display_search_result_menu(*p,search_text,tag_opt[tag_index]);
        });
        menu.setOnClose([this](endstone::Player*p){
            market_display_menu(*p);
        });
        player.sendForm(menu);
    }

    //显示搜索结果菜单
    void display_search_result_menu(endstone::Player& player,const string& search_text,const string& tag) {
        if (search_text.empty()&&tag == "All") {
            market_display_menu(player);
            return;
        }
        endstone::ActionForm menu;
        menu.setTitle(Tran.getLocal("Search Result") + " : " +"\""+ search_text+"\"");
        //搜索词空
        if (search_text.empty()) {
            auto all_goods = market.goods_get_all();
            for (const auto& one_goods:all_goods) {
                if (one_goods.tag == tag || tag == "All") {
                    menu.addButton(one_goods.name,one_goods.image,[this, one_goods](endstone::Player* p){
                        goods_view_menu(*p,one_goods);
                    });
                }
            }
        }
        //有搜索词
        else {
            auto all_goods = market.goods_get_all();
            for (const auto& one_goods:all_goods) {
                if ((one_goods.tag == tag || tag == "All") && one_goods.name.find(search_text) != string::npos) {
                    menu.addButton(one_goods.name,one_goods.image,[this, one_goods](endstone::Player* p){
                        goods_view_menu(*p,one_goods);
                    });
                }
            }
        }
        menu.setOnClose([this](endstone::Player*p){
            search_goods_menu(*p);
        });
        player.sendForm(menu);
    }

    void onOpenMenu(const endstone::PlayerInteractEvent& event) {
        if (event.hasItem()) {
            if (event.getItem()->getType() != "minecraft:emerald") {
                return;
            }
        } else {
            return;
        }
        if (canTriggerEvent(event.getPlayer().getName())) {
            main_menu(event.getPlayer());
        }
    }

    //交易记录菜单
    void recordMainMenu(endstone::Player& player) {
        endstone::ActionForm menu;
        endstone::Button seller_record;
        endstone::Button buyer_record;

        seller_record.setText(Tran.getLocal("Sell Records"));
        buyer_record.setText(Tran.getLocal("Buy Records"));
        menu.setTitle(Tran.getLocal("Transaction Records"));
        seller_record.setOnClick([this](endstone::Player* p) {
            recordSellerMenu(*p);
        });
        buyer_record.setOnClick([this](endstone::Player* p) {
            recordBuyerMenu(*p);
        });
        menu.setControls({seller_record,buyer_record});
        menu.setOnClose([this](endstone::Player *p) {
            account_menu(*p);
        } );
        player.sendForm(menu);
    }

    //显示记录详情菜单
    void recordDisplayMenu(endstone::Player& player,const Market_Action::Record_data& record) {
        endstone::ActionForm menu;
        endstone::Label record_time;
        endstone::Label seller;
        endstone::Label buyer;
        endstone::Label goods_info;

        menu.setTitle(record.time);
        auto seller_data = market.user_get(record.seller);
        auto buyer_data = market.user_get(record.buyer);
        record_time.setText(Tran.getLocal("Date: ") + record.time);
        seller.setText(Tran.getLocal("Seller: ") + seller_data.username + "(" + seller_data.playername + ")");
        buyer.setText(Tran.getLocal("Buyer: ") + buyer_data.username + "(" + buyer_data.playername + ")");
        auto goods_data = StringToGoodsData(record.goods).second;

        //物品数据
        string dis_goods_meta;
        if (goods_data.data != "None") {
            Meta_data goods_meta = StringToItemMeta(goods_data.data);
            dis_goods_meta += Tran.getLocal("Item damage: ")+to_string(goods_meta.damage);
            if (goods_meta.display_name.has_value()) {
                dis_goods_meta += "\n" + Tran.getLocal("Name tag: ")+goods_meta.display_name.value();
            }
            dis_goods_meta += "§r";
            if (goods_meta.lore.has_value()) {
                dis_goods_meta += "\n lore: ";
                for (const auto& one_lore:goods_meta.lore.value()) {
                    dis_goods_meta += one_lore+", ";
                }
            }
            if (goods_meta.enchants.has_value()) {
                dis_goods_meta += "\n" + Tran.getLocal("Enchant: ");
                for (const auto&[fst, snd]:goods_meta.enchants.value()) {
                    dis_goods_meta += fst + ":" + to_string(snd)+", ";
                }
            }
        }

        goods_info.setText(Tran.getLocal("Goods name: ")+goods_data.name+"\n"+Tran.getLocal("Goods description: ")+goods_data.text+"\n"+Tran.getLocal("Goods Info: ")+goods_data.item+"\n"+dis_goods_meta+"\n"
                            +Tran.getLocal("Price: ")+to_string(goods_data.price)+"\n"+Tran.getLocal("Currency: ")+Tran.getLocal(goods_data.money_type)+"\n");

        menu.setControls({record_time,seller,buyer,goods_info});
        menu.setOnClose([this](endstone::Player* p) {
            recordMainMenu(*p);
        });
        player.sendForm(menu);
    }

    //卖出记录显示菜单
    void recordSellerMenu(endstone::Player& player) {
        auto records = market.record_get_by_seller(player.getUniqueId().str());
        if (records.empty()) {
            notice_menu(player,endstone::ColorFormat::Red+Tran.getLocal("No sell records"),[this](endstone::Player& p) { this->recordMainMenu(p);});
            return;
        }
        endstone::ActionForm menu;
        for (const auto& record : std::ranges::reverse_view(records)) {
            menu.addButton(record.time,nullopt,[this, record](endstone::Player* p) {
                recordDisplayMenu(*p,record);
            });
        }
        menu.setTitle(Tran.getLocal("Sell Records"));
        menu.setOnClose([this](endstone::Player* p) {
            recordSellerMenu(*p);
        });
        player.sendForm(menu);
    }

    //买入记录显示菜单
    void recordBuyerMenu(endstone::Player& player) {
        auto records = market.record_get_by_buyer(player.getUniqueId().str());
        if (records.empty()) {
            notice_menu(player,endstone::ColorFormat::Red+Tran.getLocal("No buy records"),[this](endstone::Player& p) { this->recordMainMenu(p);});
            return;
        }
        endstone::ActionForm menu;
        for (const auto& record : std::ranges::reverse_view(records)) {
            menu.addButton(record.time,nullopt,[this, record](endstone::Player* p) {
                recordDisplayMenu(*p,record);
            });
        }
        menu.setTitle(Tran.getLocal("Buy Records"));
        menu.setOnClose([this](endstone::Player* p) {
            recordSellerMenu(*p);
        });
        player.sendForm(menu);
    }

    //TODO
    //User comment seller menu
};
#endif