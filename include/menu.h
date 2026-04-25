//
// Created by yuhang on 2025/11/17.
//

#ifndef FREEMARKET_MENU_H
#define FREEMARKET_MENU_H

#include <endstone/endstone.hpp>
#include <market_core.h>

class MarketCore; // 前向声明

class Menu {
public:
    //explicit Menu(endstone::Plugin &plugin) : plugin_(plugin) {}
    explicit Menu(endstone::Plugin &plugin, MarketCore &market_core)
        : plugin_(plugin), market_core_(market_core) {}

    // 通知菜单
    void notice_menu(endstone::Player& player, const std::string& msg,
                     const std::function<void(endstone::Player&)>& yes_func);

    // 主菜单
    void main_menu(endstone::Player& player);

    // 玩家注册菜单
    void register_menu(endstone::Player& player);

    // 账户信息主菜单
    void account_menu(endstone::Player& player);

    // 玩家头像更改菜单
    void player_avatar_menu(endstone::Player& player);

    // 玩家重命名菜单
    void player_rename_menu(endstone::Player& player);

    // 商品上传菜单
    void goods_upload_menu(endstone::Player& player);

    // 商品上传确认菜单
    void goods_upload_confirm_menu(endstone::Player& player, const ItemStackData& item_data, int quick_index);

    // 市场展示主菜单
    void market_display_menu(endstone::Player& player);

    // 商品查看菜单
    void goods_view_menu(endstone::Player& player, const Market_Action::Goods_data& goods_data);

    // 确认购买菜单
    void confirm_to_buy_menu(endstone::Player& player, const Market_Action::Goods_data& goods_data);

    // 货款提现菜单
    void get_item_to_player_menu(endstone::Player& player);

    // 选择管理商品菜单
    void choose_manage_goods_menu(endstone::Player &player);

    // 管理商品子菜单
    void manage_goods_menu(endstone::Player &player, const Market_Action::Goods_data& goods_data);

    // 删除商品菜单
    void del_goods_menu(endstone::Player& player, const Market_Action::Goods_data& goods_data);

    // 编辑商品菜单
    void edit_goods_menu(endstone::Player& player, const Market_Action::Goods_data& goods_data);

    // 显示商家主页
    void seller_homepage(endstone::Player& player, Market_Action::User_data seller_data);

    // 搜索菜单
    void search_goods_menu(endstone::Player& player);

    // 显示搜索结果菜单
    void display_search_result_menu(endstone::Player& player, const std::string& search_text, const std::string& tag);

    // 事件处理：打开主菜单（右击绿宝石）
    void onOpenMenu(const endstone::PlayerInteractEvent& event);

    // 交易记录主菜单
    void recordMainMenu(endstone::Player& player);

    // 显示记录详情菜单
    void recordDisplayMenu(endstone::Player& player, const Market_Action::Record_data& record);

    // 卖出记录显示菜单
    void recordSellerMenu(endstone::Player& player);

    // 买入记录显示菜单
    void recordBuyerMenu(endstone::Player& player);

private:
    endstone::Plugin &plugin_;
    MarketCore &market_core_;
};

#endif //FREEMARKET_MENU_H