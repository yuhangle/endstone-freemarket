//
// Created by yuhang on 2025/4/13.
//

#include "freemarket.h"
#include "version.h"
#include <fstream>
#include <filesystem>

ENDSTONE_PLUGIN("freemarket", FREEMARKET_PLUGIN_VERSION, FreeMarket)
{
    description = "An Endstone plugin for player to use in free market trading";

    command("market")
            .description("Open market menu")
            .usages("/market",
                    "/market <register> <username: str> [avatar: str]",
                    "/market <money> <add|less> <player:target> <money: int>"
                    )
            .permissions("fm.command.member");

    permission("fm.command.member")
            .description("member command")
            .default_(endstone::PermissionDefault::True);

    permission("fm.command.op")
            .description("op command")
            .default_(endstone::PermissionDefault::Operator);
}

//实现逻辑从这里开始

//数据目录和配置文件检查
void FreeMarket::datafile_check() const {
    json df_config = {
            {"language","zh_CN"},
            {"money", "freemarket"},
            {"player_max_goods_", 10}
    };

    if (!(std::filesystem::exists(data_path_))) {
        getLogger().info(translator_.getLocal("No data path,auto create"));
        std::filesystem::create_directory(data_path_);
        if (!(std::filesystem::exists(config_path_))) {
            std::ofstream file(config_path_);
            if (file.is_open()) {
                file << df_config.dump(4);
                file.close();
                getLogger().info(translator_.getLocal("Config file created"));
            }
        }
    } else if (std::filesystem::exists(data_path_)) {
        if (!(std::filesystem::exists(config_path_))) {
            std::ofstream file(config_path_);
            if (file.is_open()) {
                file << df_config.dump(4);
                file.close();
                getLogger().info(translator_.getLocal("Config file created"));
            }
        } else {
            bool need_update = false;
            json loaded_config;

            // 加载现有配置文件
            std::ifstream file(config_path_);
            file >> loaded_config;

            // 检查配置完整性并更新
            for (auto& [key, value] : df_config.items()) {
                if (!loaded_config.contains(key)) {
                    loaded_config[key] = value;
                    getLogger().info(translator_.getLocal("Config '{}' has update with default config")+","+ key);
                    need_update = true;
                }
            }

            // 如果需要更新配置文件，则进行写入
            if (need_update) {
                std::ofstream outfile(config_path_);
                if (outfile.is_open()) {
                    outfile << loaded_config.dump(4);
                    outfile.close();
                    getLogger().info(translator_.getLocal("Config file update over"));
                }
            }
        }
    }
}

// 读取配置文件
json FreeMarket::read_config() const {
    std::ifstream i(config_path_);
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

void FreeMarket::onLoad()
{
    getLogger().info("onLoad is called");
    //初始化目录
    if (!(filesystem::exists(data_path_))) {
        getLogger().info("No data path,auto create");
        filesystem::create_directory(data_path_);
    }
    //加载语言
    language_file_ = data_path_ + "/language/" + getServer().getLanguage().getLocale();
    translator_ = translate(language_file_);
    const auto [fst, snd] = translator_.loadLanguage();
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
        // 使用完整路径初始化
        database_ = std::make_unique<DataBase>(finalPathStr);
        market_ = std::make_unique<Market_Action>(*database_);
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "General error: " << e.what() << std::endl;
    }
    #endif
}

void FreeMarket::onEnable()
{
    getLogger().info("onEnable is called");
    datafile_check();
    if (!DataBase::fileExists(db_file_)) {
        getLogger().info(endstone::ColorFormat::Yellow + translator_.getLocal("Database file not find,auto create"));
    }
    (void)database_->init_database();
    market_core_ = std::make_unique<MarketCore>(*this);
    menu_ = std::make_unique<Menu>(*this, *market_core_);
    registerEvent(&Menu::onOpenMenu,*menu_);
    //进行一个配置文件的读取
    json json_msg = read_config();
    string lang = "en_US";
    try {
        if (!json_msg.contains("error")) {
            money_config_ = json_msg["money"];
            player_max_goods_ = json_msg["player_max_goods"];
            lang = json_msg["language"];
            if (json_msg["money"] == "umoney") {
                if (!market_core_->umoney_check_exists()) {
                    money_config_ = "freemarket";
                    getLogger().error(translator_.getLocal("Umoney plugin not find,use freemarket economy system"));
                }
                else {
                    money_config_ = "umoney";
                    getLogger().info(translator_.getLocal("Use umoney plugin"));
                }
            } else {
                money_config_ = "freemarket";
                getLogger().info(endstone::ColorFormat::Yellow+translator_.getLocal("Use freemarket economy system"));
            }
        } else {
            getLogger().error(translator_.getLocal("Config file error!Use default config"));
            money_config_ = "freemarket";
            player_max_goods_ = 10;
        }
    } catch (const std::exception& e) {
        money_config_ = "freemarket";
        player_max_goods_ = 10;
        getLogger().error(translator_.getLocal("Config file error!Use default config"));
        getLogger().error(e.what());
    }
    language_file_ = data_path_ + "/language/" + lang + ".json";
    translator_ = translate(language_file_);
    translator_.loadLanguage();
    translate::checkLanguageCommon(language_file_,data_path_+"/language/lang.json");
    //显示启动消息
    getLogger().info(endstone::ColorFormat::Yellow + "FreeMarket 插件已启动。版本：" + getServer().getPluginManager().getPlugin("freemarket")->getDescription().getVersion());
}

void FreeMarket::onDisable()
{
    getLogger().info("onDisable is called");
}

bool FreeMarket::onCommand(endstone::CommandSender &sender, const endstone::Command &command, const std::vector<std::string> &args)
{
    if (command.getName() == "market")
    {
        //控制台命令
        if (!sender.asPlayer()) {
            getLogger().error(translator_.getLocal("The console cannot use this command"));
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
                        if (market_->user_exist(uuid)) {
                            sender.sendErrorMessage(translator_.getLocal("You have already registered and cannot register again"));
                            return false;
                        }
                        string avatar;
                        if (args[2].empty()) {
                            avatar = "textures/ui/icon_steve";
                        } else {
                            avatar = args[2];
                        }
                        const auto [fst, snd] = market_->user_add(uuid,playername,username,avatar,"",0);
                        if (fst) {
                            sender.sendMessage(translator_.getLocal(snd));
                            return true;
                        }
                        sender.sendErrorMessage(translator_.getLocal(snd));
                        return false;
                    }
                    sender.sendErrorMessage(translator_.getLocal("Missing parameters"));
                } catch (const std::exception &e) {
                    //返回错误给玩家
                    sender.sendErrorMessage(e.what());
                }
            }
            //自带经济系统
            else if (args[0] == "money") {
                try {
                    if (money_config_ != "freemarket") {
                        sender.sendErrorMessage(translator_.getLocal("The built-in economy system of freemarket not enable,you can not use this command"));
                        return false;
                    }
                    if (!sender.hasPermission("OP")) {
                        sender.sendErrorMessage(translator_.getLocal("You are not op,you can not use this command"));
                        return false;
                    }
                    if (!args[1].empty() && !args[2].empty() && !args[3].empty()) {
                        int money = string_utils::to_int(args[3]);
                        if (money == 0) {
                            sender.sendErrorMessage(translator_.getLocal("Invalid integer value"));
                            return false;
                        }
                        const auto userdata =market_->user_get_by_playername(args[2]);
                        if (!userdata.status) {
                            sender.sendErrorMessage(translator_.getLocal("This player is not registered"));
                            return false;
                        }
                        if (args[1] == "less") {
                            money = -money;
                        }
                        const auto [fst, snd] = market_->user_money(userdata.uuid,userdata.money + money);
                        if (fst) {
                            sender.sendMessage(translator_.getLocal(snd));
                            sender.sendMessage(translator_.getLocal("The player's current money is: ") + to_string(userdata.money + money));
                            return true;
                        }
                        sender.sendErrorMessage(translator_.getLocal(snd));
                        return false;
                    }
                    sender.sendErrorMessage(translator_.getLocal("Missing parameters"));
                }  catch (const std::exception &e) {
                    //返回错误给玩家
                    sender.sendErrorMessage(e.what());
                }
            }
        }
    }
    return true;
}
