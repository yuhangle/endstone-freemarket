//
// Created by yuhang on 2025/4/13.
//
#ifndef FREEMARKET_H
#define FREEMARKET_H

#include "market_action.h"
#include "global.h"
#include "menu.h"
#include "market_core.h"

class FreeMarket : public endstone::Plugin {
public:

    //数据目录和配置文件检查
    void datafile_check() const {
        json df_config = {
                {"language","zh_CN"},
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

    void onLoad() override
    {
        getLogger().info("onLoad is called");
        //初始化目录
        if (!(filesystem::exists(data_path))) {
            getLogger().info("No data path,auto create");
            filesystem::create_directory(data_path);
        }
        //加载语言
        language_file = data_path + "/language/" + getServer().getLanguage().getLocale();
        const auto [fst, snd] = Tran.loadLanguage();
        getLogger().info(snd);
        #ifdef __linux__
        namespace fs = std::filesystem;
        try {
            // 获取当前路径
            const fs::path currentPath = fs::current_path();

            // 子目录路径
            const fs::path subdir = "plugins/freemarket/data.db";

            // 拼接路径
            const fs::path fullPath = currentPath / subdir;

            // 如果需要将最终路径转换为 string 类型
            const std::string finalPathStr = fullPath.string();
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
        string lang = "en_US";
        try {
            if (!json_msg.contains("error")) {
                money_config = json_msg["money"];
                player_max_goods = json_msg["player_max_goods"];
                lang = json_msg["language"];
                if (json_msg["money"] == "umoney") {
                    if (!market_core_->umoney_check_exists()) {
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
        language_file = data_path + "/language/" + lang + ".json";
        Tran = translate(language_file);
        Tran.loadLanguage();
        translate::checkLanguageCommon(language_file,data_path+"/language/lang.json");
        menu_ = std::make_unique<Menu>(*this);
        market_core_ = std::make_unique<MarketCore>(*this);
        registerEvent(&Menu::onOpenMenu,*menu_);
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
                    const auto player = getServer().getPlayer(sender.getName());
                    menu_->main_menu(*player);
                }
                //注册
                else if (args[0] == "register") {
                    try {
                        if (!args[1].empty()) {
                            const string& username = args[1];
                            const string playername = sender.getName();
                            const string uuid = getServer().getPlayer(playername)->getUniqueId().str();
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
                            const auto [fst, snd] = market.user_add(uuid,playername,username,avatar,"",0);
                            if (fst) {
                                sender.sendMessage(Tran.getLocal(snd));
                                return true;
                            }
                            sender.sendErrorMessage(Tran.getLocal(snd));
                            return false;
                        }
                        sender.sendErrorMessage(Tran.getLocal("Missing parameters"));
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
                            const auto userdata =market.user_get_by_playername(args[2]);
                            if (!userdata.status) {
                                sender.sendErrorMessage(Tran.getLocal("This player is not registered"));
                                return false;
                            }
                            if (args[1] == "less") {
                                money = -money;
                            }
                            const auto [fst, snd] = market.user_money(userdata.uuid,userdata.money + money);
                            if (fst) {
                                sender.sendMessage(Tran.getLocal(snd));
                                sender.sendMessage(Tran.getLocal("The player's current money is: ") + to_string(userdata.money + money));
                                return true;
                            }
                            sender.sendErrorMessage(Tran.getLocal(snd));
                            return false;
                        }
                        sender.sendErrorMessage(Tran.getLocal("Missing parameters"));
                    }  catch (const std::exception &e) {
                        //返回错误给玩家
                        sender.sendErrorMessage(e.what());
                    }
                }
            }
        }
        return true;
    }

private:
    std::unique_ptr<Menu> menu_;
};
#endif