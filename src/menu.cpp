//
// Created by yuhang on 2025/11/17.
//

#include "menu.h"
#include "freemarket.h"
#include "item_serializer.hpp"
#include "menu_helpers.hpp"
#include "string_utils.hpp"
#include "trade_engine.hpp"

//通知菜单
void Menu::notice_menu(endstone::Player& player,const string& msg,const std::function<void(endstone::Player&)>& yes_func) const
{
    endstone::MessageForm menu;
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Notice"));
    menu.setContent(msg);
    menu.setButton1(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Back"));
    menu.setButton2(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Exit"));
    menu.setOnSubmit([=, &player](endstone::Player*, const int test) {
        if (test == 0 && yes_func) {
            yes_func(player);
        } else {
            player.closeForm();
        }
    });
    player.sendForm(menu);
}

//主菜单
void Menu::main_menu(endstone::Player& player) {
    endstone::ActionForm menu;
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("FreeMarket Menu"));
    menu.setContent(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("§l§o§bWelcome! You can open the menu by entering the /market command or right-clicking with emerald"));

    if (auto userdata = dynamic_cast<FreeMarket&>(plugin_).getMarket().user_get(player.getUniqueId().str()); !userdata.status) {
        menu.addButton(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("§l§5Register Account"),"textures/ui/icon_steve",[this](endstone::Player *p) {
            register_menu(*p);});
    }
    else {
        menu.addButton(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("§l§5Account Information"),userdata.avatar,[this](endstone::Player *p) {
            account_menu(*p);});
        menu.addButton(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("§l§5Trading Market"),"textures/ui/teams_icon",[this](endstone::Player *p) {
            market_display_menu(*p);});
        menu.addButton(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("§l§5Add Items to Market"),"textures/ui/icon_blackfriday",[this](endstone::Player*p){
            goods_upload_menu(*p);
        });
        menu.addButton(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("§l§5Manage goods"),"textures/ui/icon_setting",[this](endstone::Player*p){
            choose_manage_goods_menu(*p);
        });
    }
    player.sendForm(menu);
}

//玩家注册菜单
void Menu::register_menu(endstone::Player& player) {
    endstone::ModalForm menu;
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Register menu"));
    endstone::Label label;
    label.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("You has not an freemarket account,please register in this menu."));
    endstone::TextInput input;
    input.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Input your username"));
    endstone::Dropdown dropdown;
    dropdown.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Select an avatar.\nThe menu only supports presets,you can change it to custom later"));

    std::vector<std::string> avatar_options;
    for (const auto& preset : getAvatarPresets()) {
        avatar_options.push_back(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(preset.name));
    }
    dropdown.setOptions(avatar_options);
    dropdown.setDefaultIndex(0);
    menu.addControl(label);
    menu.addControl(input);
    menu.addControl(dropdown);
    menu.setOnSubmit([=](const endstone::Player *p,const string& response)
                     {
                         auto parsedResponse = json::parse(response);
                         string username = parsedResponse[1];
                         int avatar_index = parsedResponse[2];
                         string avatar = getAvatarPresets()[avatar_index].texture;
                         const string command = "market register \"" + username + "\"" +" " + "\"" +avatar + "\"";
                         (void)p->performCommand(command);
                     });

    menu.setOnClose([this](endstone::Player* p){
        main_menu(*p);
    });
    player.sendForm(menu);
}

//账户信息主菜单
void Menu::account_menu(endstone::Player& player) {
    endstone::ActionForm menu;
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Account Info Menu"));
    const auto user_data = dynamic_cast<FreeMarket&>(plugin_).getMarket().user_get(player.getUniqueId().str());
    endstone::Button change_avatar;
    change_avatar.setIcon("textures/ui/icon_multiplayer");
    change_avatar.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Change account avatar"));
    change_avatar.setOnClick([this](endstone::Player *p){
        player_avatar_menu(*p);
    });
    endstone::Button rename;
    rename.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Rename account"));
    rename.setIcon("textures/ui/book_edit_default");
    rename.setOnClick([this](endstone::Player* p){
        player_rename_menu(*p);
    });
    endstone::Button get_payment;
    get_payment.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Withdraw the payment"));
    get_payment.setIcon("textures/ui/trade_icon");
    get_payment.setOnClick([this](endstone::Player* p){
        get_item_to_player_menu(*p);
    });
    endstone::Button record;
    record.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Transaction Records"));
    record.setIcon("textures/ui/icon_timer");
    record.setOnClick([this](endstone::Player* p){
        recordMainMenu(*p);
    });

    menu.setControls({change_avatar,rename,get_payment,record});

    menu.setOnClose([this](endstone::Player* p){
        main_menu(*p);
    });
    int money;
    if (dynamic_cast<FreeMarket&>(plugin_).getMoneyConfig() == "freemarket") {
        money = user_data.money;
    } else {
        money = market_core_.umoney_get_player_money(player.getName());
    }
    menu.setContent(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("User name: ") + user_data.username + "\n" + endstone::ColorFormat::Green +dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Money: ") + to_string(money));
    player.sendForm(menu);
}

//玩家头像更改菜单
void Menu::player_avatar_menu(endstone::Player& player) {
    endstone::ModalForm menu;
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Change Avatar Menu"));
    endstone::Label label;
    label.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Change the avatar via a dropdown box or an input box. \nIf text is entered in the input box, use the input box."));
    endstone::Dropdown dropdown;
    dropdown.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Select an avatar."));

    std::vector<std::string> avatar_options;
    for (const auto& preset : getAvatarPresets()) {
        avatar_options.push_back(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(preset.name));
    }
    dropdown.setOptions(avatar_options);
    dropdown.setDefaultIndex(0);
    endstone::TextInput textInput;
    textInput.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Custom Avatar"));
    textInput.setPlaceholder("textures/ui/icon_steve");
    menu.addControl(label);
    menu.addControl(dropdown);
    menu.addControl(textInput);
    menu.setOnSubmit([=,this](endstone::Player *p,const string& response) {
        auto parsedResponse = json::parse(response);
        string custom = parsedResponse[2];
        int avatar_index = parsedResponse[1];
        string avatar;
        if (custom.empty()) {
            avatar = getAvatarPresets()[avatar_index].texture;
        } else {
            avatar = custom;
        }
        if (auto [fst, snd] = dynamic_cast<FreeMarket&>(plugin_).getMarket().user_change_avatar(p->getUniqueId().str(),avatar); fst) {
            notice_menu(*p,dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(snd),[this](endstone::Player& pp) { account_menu(pp);});
        } else {
            notice_menu(*p,endstone::ColorFormat::Red+dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(snd),[this](endstone::Player& p) { this->account_menu(p);});
        }
    });

    menu.setOnClose([this](endstone::Player* p){
        account_menu(*p);
    });

    player.sendForm(menu);
}

//玩家重命名菜单
void Menu::player_rename_menu(endstone::Player& player) {
    endstone::ModalForm menu;
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Rename Menu"));
    endstone::Label label;
    label.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Change your user name"));
    endstone::TextInput textInput;
    textInput.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Input new name"));
    textInput.setDefaultValue(dynamic_cast<FreeMarket&>(plugin_).getMarket().GetUsername(player.getUniqueId().str()));
    menu.addControl(label);
    menu.addControl(textInput);
    menu.setOnSubmit([this](endstone::Player *p,const string& response) {
        auto parsedResponse = json::parse(response);
        const string name = parsedResponse[1];
        if (name.empty()) {
            return ;
        }
        if (const auto [fst, snd] = dynamic_cast<FreeMarket&>(plugin_).getMarket().user_rename(p->getUniqueId().str(),name); fst) {
            notice_menu(*p,dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(snd),[this](endstone::Player& pp) { account_menu(pp);});
        } else {
            notice_menu(*p,endstone::ColorFormat::Red+dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(snd),[this](endstone::Player& p) { this->account_menu(p);});
        }
    });

    menu.setOnClose([this](endstone::Player* p){
        account_menu(*p);
    });
    player.sendForm(menu);
}

//商品上传菜单
void Menu::goods_upload_menu(endstone::Player& player) {
    if (const int player_goods_amount = dynamic_cast<FreeMarket&>(plugin_).getDatabase().getGoodsCountByUuid(player.getUniqueId().str()); player_goods_amount > dynamic_cast<FreeMarket&>(plugin_).getPlayerMaxGoods() || player_goods_amount < 0) {
        notice_menu(player,dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("You have reached the maximum limit for your goods"),[this](endstone::Player& p) { this->main_menu(p);});
        return;
    }
    endstone::ModalForm menu;
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Add Goods Menu"));

    auto hotbar_items = readPlayerHotbar(player);
    std::vector<std::string> hotbar_names;
    for (int i = 0; i < 9; i++) {
        const auto& item = hotbar_items[i];
        if (ItemSerializer::isValid(item)) {
            hotbar_names.push_back(std::to_string(i+1) + ". " +
                dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Item ID: ") + item.item_id +
                dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Number: ") + std::to_string(item.item_num));
        } else {
            hotbar_names.push_back(std::to_string(i+1) + ". " + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("None"));
        }
    }
    endstone::Dropdown chose_item;
    chose_item.setOptions(hotbar_names);
    menu.addControl(chose_item);

    menu.setOnSubmit([this, hotbar_items](endstone::Player*p, const string& response){
        auto json_response = json::parse(response);
        const int item_index = json_response[0];
        const auto& the_item = hotbar_items[item_index];
        if (!ItemSerializer::isValid(the_item)) {
            notice_menu(*p,dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("You have not selected a valid item"),[this](endstone::Player& p) { this->main_menu(p);});
            return;
        }
        goods_upload_confirm_menu(*p, the_item, item_index);
    });
    menu.setOnClose([this](endstone::Player*p){
        main_menu(*p);
    });
    player.sendForm(menu);
}

void Menu::goods_upload_confirm_menu(endstone::Player& player, const ItemStackData& item_data, int quick_index) {
    endstone::ModalForm menu;
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Confirm to Add Goods Menu"));
    endstone::Label goods_info;
    goods_info.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Goods ID: ") + item_data.item_id + "\n" + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Number: ") + to_string(item_data.item_num));

    endstone::TextInput title;
    endstone::TextInput text;
    endstone::Dropdown money_type;
    endstone::TextInput custom_money;
    endstone::TextInput price_Input;
    endstone::Dropdown image_drop;
    endstone::Dropdown tag_drop;

    title.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Goods title"));
    title.setDefaultValue(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("One unit of goods"));
    text.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Goods description"));
    text.setDefaultValue(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("The seller didn't write a description"));

    // Money type options
    std::vector<std::string> money_values;
    std::vector<std::string> money_names;
    for (const auto& preset : getMoneyTypePresets()) {
        money_names.push_back(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(preset.name));
        money_values.push_back(preset.value);
    }
    money_type.setOptions(money_names);
    money_type.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Select the transaction settlement method"));
    money_type.setDefaultIndex(0);

    custom_money.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Custom settlement currency id"));
    custom_money.setPlaceholder(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("The value entered here will be used as the settlement currency when you select custom"));

    price_Input.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Enter a price"));
    price_Input.setPlaceholder("114514");

    // Cover image options
    std::vector<std::string> image_names;
    std::vector<std::string> image_values;
    for (const auto& preset : getCategoryImagePresets()) {
        image_names.push_back(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(preset.name));
        image_values.push_back(preset.texture);
    }
    image_drop.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Select a cover"));
    image_drop.setOptions(image_names);
    image_drop.setDefaultIndex(0);

    // Tag options
    std::vector<std::string> tag_names;
    std::vector<std::string> tag_values;
    for (const auto& preset : getTagPresets()) {
        tag_names.push_back(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(preset.name));
        tag_values.push_back(preset.value);
    }
    tag_drop.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Set goods tag"));
    tag_drop.setOptions(tag_names);
    tag_drop.setDefaultIndex(0);

    menu.setControls({title, text, money_type, custom_money, price_Input, image_drop, tag_drop});

    menu.setOnSubmit([this, item_data, quick_index, image_values, money_values, tag_values](endstone::Player*p, const string& response) {
        auto json_response = json::parse(response);
        string the_title = json_response[0];
        string the_text = json_response[1];
        int money_index = json_response[2];
        string the_custom_money = json_response[3];
        string price_str = json_response[4];
        int image_index = json_response[5];
        int tag_index = json_response[6];
        int price = string_utils::to_int(price_str);

        if (the_title.empty() || the_text.empty() || (money_index == 5 && the_custom_money.empty()) || price <= 0) {
            notice_menu(*p, endstone::ColorFormat::Red + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Title and description can not be empty,and when you select custom currency,custom currency can not be empty"),
                        [this](endstone::Player& p) { this->main_menu(p); });
            return;
        }

        // Unified serialization: split into goods.item and goods.data
        auto [goods_item, goods_data] = ItemSerializer::toGoodsParts(item_data);
        string money_type_val = (money_index == 5) ? the_custom_money : money_values[money_index];

        if (auto [fst, snd] = dynamic_cast<FreeMarket&>(plugin_).getMarket().goods_add(p->getUniqueId().str(), the_title, the_text,
                goods_item, goods_data, image_values[image_index], price, money_type_val, tag_values[tag_index]); fst) {
            notice_menu(*p, dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(snd), [this](endstone::Player& p) { this->main_menu(p); });
        } else {
            notice_menu(*p, endstone::ColorFormat::Red + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(snd), [this](endstone::Player& p) { this->main_menu(p); });
            return;
        }

        // Remove the item from player's inventory
        p->getInventory().setItem(quick_index, std::nullopt);
        plugin_.getServer().broadcastMessage("§l§2" + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("[Marketing Promotion] New goods have been listed: ") + "§r" + the_title);
    });
    menu.setOnClose([this](endstone::Player*p){
        goods_upload_menu(*p);
    });
    player.sendForm(menu);
}

// 市场展示主菜单
void Menu::market_display_menu(endstone::Player& player) {
    endstone::ActionForm menu;
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Market"));
    menu.addButton(" " + endstone::ColorFormat::MaterialDiamond + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("§lSearch") + " ",
                   std::nullopt, [this](endstone::Player* p) { search_goods_menu(*p); });
    if (auto goods_data = dynamic_cast<FreeMarket&>(plugin_).getMarket().goods_get_all(); goods_data.empty()) {
        menu.setContent(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("No goods are currently for sale"));
    } else {
        for (auto & goods : std::ranges::reverse_view(goods_data)) {
            menu.addButton(goods.name, goods.image,
                           [this, goods](endstone::Player* p) { goods_view_menu(*p, goods); });
        }
    }
    menu.setOnClose([this](endstone::Player* p) { main_menu(*p); });
    player.sendForm(menu);
}

// 商品查看菜单
void Menu::goods_view_menu(endstone::Player& player, const Market_Action::Goods_data& goods_data) {
    auto seller_data = dynamic_cast<FreeMarket&>(plugin_).getMarket().user_get(goods_data.uuid);
    int buyer_money = market_core_.get_player_money(player);

    std::string seller_info = dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Unknown");
    if (seller_data.status) {
        seller_info = seller_data.username + "(" + seller_data.playername + ")";
    }

    // Build item meta display string
    auto deserialized = ItemSerializer::deserialize(goods_data.item + "," + goods_data.data);
    std::string dis_goods_meta = buildItemMetaDisplay(deserialized, dynamic_cast<FreeMarket&>(plugin_).getTranslator());

    endstone::ActionForm menu;
    menu.setTitle(goods_data.name);

    endstone::Label goods_info;
    endstone::Label buyer_info;
    endstone::Button confirm_buy;

    goods_info.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Goods name: ") + goods_data.name + "\n" +
                       dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Goods description: ") + goods_data.text + "\n" +
                       dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Goods Info: ") + goods_data.item + "\n" + dis_goods_meta + "\n" +
                       dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Price: ") + std::to_string(goods_data.price) + "\n" +
                       dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Currency: ") + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(goods_data.money_type) + "\n" +
                       dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Seller info: ") + seller_info);

    buyer_info.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Your money: ") + std::to_string(buyer_money));

    confirm_buy.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Buy it"));
    confirm_buy.setIcon("textures/ui/confirm");
    confirm_buy.setOnClick([this, goods_data](endstone::Player* p) { confirm_to_buy_menu(*p, goods_data); });

    if (seller_data.status) {
        endstone::Button seller_home_page;
        seller_home_page.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Seller homepage"));
        seller_home_page.setIcon(seller_data.avatar);
        seller_home_page.setOnClick([this, seller_data](endstone::Player* p) { seller_homepage(*p, seller_data); });
        menu.setControls({seller_home_page, goods_info, buyer_info, confirm_buy});
    } else {
        menu.setControls({buyer_info, confirm_buy});
    }
    menu.setOnClose([this](endstone::Player* p) { market_display_menu(*p); });
    player.sendForm(menu);
}

// 确认购买菜单
void Menu::confirm_to_buy_menu(endstone::Player& player, const Market_Action::Goods_data& goods_data) {
    endstone::MessageForm menu;
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Confirm Menu"));
    menu.setButton1(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Yes,It will be mine"));
    menu.setButton2(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("No,I don't want to buy it"));
    menu.setContent(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Confirm to buy it?"));

    menu.setOnSubmit([this, goods_data](endstone::Player* p, int chose) {
        if (chose == 0) {
            TradeEngine trade_engine(plugin_, market_core_, dynamic_cast<FreeMarket&>(plugin_).getTranslator());
            auto result = trade_engine.executePurchase(*p, goods_data);
            if (result.success) {
                notice_menu(*p, dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(result.message_key),
                            [this](endstone::Player& p) { main_menu(p); });
            } else {
                notice_menu(*p, endstone::ColorFormat::Red + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(result.message_key),
                            [this](endstone::Player& p) { this->main_menu(p); });
            }
        } else {
            goods_view_menu(*p, goods_data);
        }
    });
    menu.setOnClose([this, goods_data](endstone::Player* p) { goods_view_menu(*p, goods_data); });
    player.sendForm(menu);
}

// 货款提现菜单
void Menu::get_item_to_player_menu(endstone::Player& player) {
    const auto user_all_item = market_core_.UserItemRead(player.getUniqueId().str());
    if (user_all_item.empty()) {
        notice_menu(player, endstone::ColorFormat::Red + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("You have no withdrawable payment"),
                    [this](endstone::Player& p) { account_menu(p); });
        return;
    }

    endstone::ModalForm menu;
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Withdraw the payment"));

    std::string display_info;
    for (const auto& item_data : user_all_item) {
        std::string dis_goods_meta = buildItemMetaDisplay(item_data, dynamic_cast<FreeMarket&>(plugin_).getTranslator());
        display_info.append(item_data.item_id)
            .append(" x ")
            .append(std::to_string(item_data.item_num))
            .append("\n")
            .append(dis_goods_meta)
            .append("\n--------------------\n");
    }

    endstone::Label context;
    context.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Your payment: ") + "\n" + display_info);
    menu.setControls({context});

    menu.setOnSubmit([this](endstone::Player* p, const std::string& response) {
        for (const auto user_items = market_core_.UserItemRead(p->getUniqueId().str()); const auto& item : user_items) {
            giveItemToPlayer(*p, item, plugin_.getServer().getItemFactory());
        }
        (void)dynamic_cast<FreeMarket&>(plugin_).getMarket().user_clear_item(p->getUniqueId().str());
        notice_menu(*p, dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("The withdrawal is complete"),
                    [this](endstone::Player& p) { account_menu(p); });
    });

    menu.setOnClose([this](endstone::Player *p) { account_menu(*p); });
    player.sendForm(menu);
}

// 选择管理商品菜单
void Menu::choose_manage_goods_menu(endstone::Player &player) {
    auto all_user_goods = dynamic_cast<FreeMarket&>(plugin_).getMarket().goods_get_by_uuid(player.getUniqueId().str());
    if (all_user_goods.empty()) {
        notice_menu(player, endstone::ColorFormat::Red + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("You have not goods"),
                    [this](endstone::Player& p) { main_menu(p); });
        return;
    }
    endstone::ModalForm menu;
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Manage Goods Menu"));
    endstone::Dropdown goods_list;
    std::vector<std::string> goods_name_list;
    int i = 0;
    for (const auto& one_goods : all_user_goods) {
        i++;
        goods_name_list.push_back(std::to_string(i) + ". " + one_goods.name);
    }
    goods_list.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Choose goods to manage"));
    goods_list.setOptions(goods_name_list);
    menu.setControls({goods_list});
    menu.setOnSubmit([this, all_user_goods](endstone::Player* p, const std::string& response) {
        auto json_response = json::parse(response);
        const int goods_index = json_response[0];
        const auto& the_goods_data = all_user_goods[goods_index];
        manage_goods_menu(*p, the_goods_data);
    });
    menu.setOnClose([this](endstone::Player* p) { main_menu(*p); });
    player.sendForm(menu);
}

// 管理商品子菜单
void Menu::manage_goods_menu(endstone::Player &player, const Market_Action::Goods_data& goods_data) {
    endstone::ActionForm menu;
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Manage Goods Menu"));
    endstone::Button edit_button;
    endstone::Button del_button;

    edit_button.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Edit goods info"));
    edit_button.setOnClick([this, goods_data](endstone::Player* p) { edit_goods_menu(*p, goods_data); });

    del_button.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Delete goods and return it"));
    del_button.setOnClick([this, goods_data](endstone::Player* p) { del_goods_menu(*p, goods_data); });

    menu.setOnClose([this](endstone::Player* p) { main_menu(*p); });
    menu.setControls({edit_button, del_button});
    player.sendForm(menu);
}

// 删除商品菜单
void Menu::del_goods_menu(endstone::Player& player, const Market_Action::Goods_data& goods_data) {
    if (isPlayerInventoryFull(player)) {
        notice_menu(player, endstone::ColorFormat::Red + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Your inventory is full, please clear some space and try again"),
                    [this](endstone::Player& p) { main_menu(p); });
        return;
    }
    endstone::MessageForm menu;
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Delete Goods Menu"));
    menu.setContent(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Are you sure to delete it?"));
    menu.setButton1(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Yes,delete it and return it"));
    menu.setButton2(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("No,I don't want to delete it"));
    menu.setOnSubmit([this, goods_data](endstone::Player* p, const int bt_i) {
        if (bt_i == 0) {
            const auto update_goods_data = dynamic_cast<FreeMarket&>(plugin_).getMarket().goods_get_by_gid(goods_data.gid);
            if (!update_goods_data.status) {
                notice_menu(*p, endstone::ColorFormat::Red + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("The goods not exist"),
                            [this](endstone::Player& p) { this->choose_manage_goods_menu(p); });
                return;
            }
            if (auto [fst, snd] = dynamic_cast<FreeMarket&>(plugin_).getMarket().goods_del(update_goods_data.gid); fst) {
                auto deserialized = ItemSerializer::deserialize(goods_data.item + "," + goods_data.data);
                giveItemToPlayer(*p, deserialized, plugin_.getServer().getItemFactory());
                notice_menu(*p, dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(snd),
                            [this](endstone::Player& p) { this->choose_manage_goods_menu(p); });
                plugin_.getServer().broadcastMessage("§l§2" + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("[Marketing Promotion] This good has been delisted: ") + "§r" + goods_data.name);
            } else {
                notice_menu(*p, endstone::ColorFormat::Red + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(snd),
                            [this](endstone::Player& p) { this->choose_manage_goods_menu(p); });
            }
        }
    });
    menu.setOnClose([this, goods_data](endstone::Player* p) { manage_goods_menu(*p, goods_data); });
    player.sendForm(menu);
}

// 编辑商品菜单
void Menu::edit_goods_menu(endstone::Player& player, const Market_Action::Goods_data& goods_data) {
    endstone::ModalForm menu;
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Edit Goods Menu"));

    endstone::TextInput title_input;
    endstone::TextInput text_input;
    endstone::Dropdown image_drop;
    endstone::TextInput image_custom;
    endstone::Dropdown tag_drop;

    title_input.setDefaultValue(goods_data.name);
    title_input.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Goods title"));

    text_input.setDefaultValue(goods_data.text);
    text_input.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Goods description"));

    std::vector<std::string> image_names;
    std::vector<std::string> image_values;
    for (const auto& preset : getCategoryImagePresets()) {
        image_names.push_back(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(preset.name));
        image_values.push_back(preset.texture);
    }
    image_drop.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Select a cover"));
    image_drop.setOptions(image_names);
    image_drop.setDefaultIndex(0);

    image_custom.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Set custom cover"));

    std::vector<std::string> tag_names;
    std::vector<std::string> tag_values;
    for (const auto& preset : getTagPresets()) {
        tag_names.push_back(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(preset.name));
        tag_values.push_back(preset.value);
    }
    tag_drop.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Set goods tag"));
    tag_drop.setOptions(tag_names);
    tag_drop.setDefaultIndex(0);

    menu.setControls({title_input, text_input, image_drop, image_custom, tag_drop});

    menu.setOnSubmit([this, goods_data, image_values, tag_values](endstone::Player* p, const std::string& response) {
        auto json_response = json::parse(response);
        std::string the_title = json_response[0];
        std::string the_text = json_response[1];
        int image_index = json_response[2];
        std::string custom_image = json_response[3];
        int tag_index = json_response[4];
        if (the_title.empty() || the_text.empty() || (image_index == 3 && custom_image.empty())) {
            notice_menu(*p, endstone::ColorFormat::Red + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Title and description can not be empty,and when you select custom cover,custom cover can not be empty"),
                        [this](endstone::Player& pp) { main_menu(pp); });
            return;
        }
        if (!dynamic_cast<FreeMarket&>(plugin_).getMarket().goods_exist(goods_data.gid)) {
            notice_menu(*p, endstone::ColorFormat::Red + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Goods not exists"),
                        [this](endstone::Player& p) { this->choose_manage_goods_menu(p); });
            return;
        }
        std::string image;
        if (image_index == 3) {
            image = custom_image;
        } else {
            image = image_values[image_index];
        }
        auto status = dynamic_cast<FreeMarket&>(plugin_).getMarket().goods_update(goods_data.gid, the_title, the_text, image, tag_values[tag_index]);
        if (status.first) {
            notice_menu(*p, dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(status.second),
                        [this](endstone::Player& p) { this->choose_manage_goods_menu(p); });
        } else {
            notice_menu(*p, endstone::ColorFormat::Red + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(status.second),
                        [this](endstone::Player& p) { this->choose_manage_goods_menu(p); });
        }
    });
    menu.setOnClose([this, goods_data](endstone::Player* p) { manage_goods_menu(*p, goods_data); });
    player.sendForm(menu);
}

// 显示商家主页
void Menu::seller_homepage(endstone::Player& player, Market_Action::User_data seller_data) {
    const auto seller_all_goods = dynamic_cast<FreeMarket&>(plugin_).getMarket().goods_get_by_uuid(seller_data.uuid);
    if (seller_all_goods.empty()) {
        notice_menu(player, endstone::ColorFormat::Red + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Seller has not goods"),
                    [this](endstone::Player& p) { this->market_display_menu(p); });
        return;
    }
    if (!seller_data.status) {
        notice_menu(player, endstone::ColorFormat::Red + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Seller not exists"),
                    [this](endstone::Player& p) { this->market_display_menu(p); });
        return;
    }
    endstone::ActionForm menu;
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Seller homepage"));
    menu.addHeader(seller_data.username);
    menu.addDivider();
    menu.addLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Seller goods: ") + "\n");
    for (const auto& one_goods : seller_all_goods) {
        menu.addButton(one_goods.name, one_goods.image,
                       [this, one_goods](endstone::Player* p) { goods_view_menu(*p, one_goods); });
    }
    menu.setOnClose([this](endstone::Player* p) { market_display_menu(*p); });
    player.sendForm(menu);
}

// 搜索菜单
void Menu::search_goods_menu(endstone::Player& player) {
    endstone::ModalForm menu;
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Search Goods Menu"));
    endstone::TextInput search_text_input;
    endstone::Dropdown tag_dropdown;

    search_text_input.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Search goods"));
    tag_dropdown.setLabel(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Tag"));
    std::vector<std::string> tag_names;
    std::vector<std::string> tag_values;
    for (const auto& preset : getTagPresets()) {
        tag_names.push_back(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(preset.name));
        tag_values.push_back(preset.value);
    }
    // Add "All" as first option
    tag_names.insert(tag_names.begin(), dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("All"));
    tag_values.insert(tag_values.begin(), "All");
    tag_dropdown.setOptions(tag_names);
    menu.setControls({search_text_input, tag_dropdown});
    menu.setOnSubmit([this, tag_values](endstone::Player* p, const std::string& response) {
        auto json_response = json::parse(response);
        const std::string search_text = json_response[0];
        const int tag_index = json_response[1];
        display_search_result_menu(*p, search_text, tag_values[tag_index]);
    });
    menu.setOnClose([this](endstone::Player* p) { market_display_menu(*p); });
    player.sendForm(menu);
}

// 显示搜索结果菜单
void Menu::display_search_result_menu(endstone::Player& player, const std::string& search_text, const std::string& tag) {
    if (search_text.empty() && tag == "All") {
        market_display_menu(player);
        return;
    }
    endstone::ActionForm menu;
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Search Result") + " : " + "\"" + search_text + "\"");
    if (search_text.empty()) {
        for (const auto all_goods = dynamic_cast<FreeMarket&>(plugin_).getMarket().goods_get_all(); const auto& one_goods : all_goods) {
            if (one_goods.tag == tag || tag == "All") {
                menu.addButton(one_goods.name, one_goods.image,
                               [this, one_goods](endstone::Player* p) { goods_view_menu(*p, one_goods); });
            }
        }
    } else {
        for (const auto all_goods = dynamic_cast<FreeMarket&>(plugin_).getMarket().goods_get_all(); const auto& one_goods : all_goods) {
            if ((one_goods.tag == tag || tag == "All") && one_goods.name.find(search_text) != std::string::npos) {
                menu.addButton(one_goods.name, one_goods.image,
                               [this, one_goods](endstone::Player* p) { goods_view_menu(*p, one_goods); });
            }
        }
    }
    menu.setOnClose([this](endstone::Player* p) { search_goods_menu(*p); });
    player.sendForm(menu);
}

// 事件处理：打开主菜单（右击绿宝石）
void Menu::onOpenMenu(const endstone::PlayerInteractEvent& event) {
    if (event.hasItem()) {
        if (event.getItem()->getType() != "minecraft:emerald") {
            return;
        }
    } else {
        return;
    }
    if (market_core_.canTriggerEvent(event.getPlayer().getName())) {
        main_menu(event.getPlayer());
    }
}

// 交易记录主菜单
void Menu::recordMainMenu(endstone::Player& player) {
    endstone::ActionForm menu;
    endstone::Button seller_record;
    endstone::Button buyer_record;

    seller_record.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Sell Records"));
    buyer_record.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Buy Records"));
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Transaction Records"));
    seller_record.setOnClick([this](endstone::Player* p) { recordSellerMenu(*p); });
    buyer_record.setOnClick([this](endstone::Player* p) { recordBuyerMenu(*p); });
    menu.setControls({seller_record, buyer_record});
    menu.setOnClose([this](endstone::Player *p) { account_menu(*p); });
    player.sendForm(menu);
}

// 显示记录详情菜单
void Menu::recordDisplayMenu(endstone::Player& player, const Market_Action::Record_data& record) {
    endstone::ActionForm menu;
    endstone::Label record_time;
    endstone::Label seller;
    endstone::Label buyer;
    endstone::Label goods_info;

    menu.setTitle(record.time);
    auto seller_data = dynamic_cast<FreeMarket&>(plugin_).getMarket().user_get(record.seller);
    auto buyer_data = dynamic_cast<FreeMarket&>(plugin_).getMarket().user_get(record.buyer);
    record_time.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Date: ") + record.time);
    seller.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Seller: ") + seller_data.username + "(" + seller_data.playername + ")");
    buyer.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Buyer: ") + buyer_data.username + "(" + buyer_data.playername + ")");
    auto goods_data = MarketCore::StringToGoodsData(record.goods).second;

    auto deserialized = ItemSerializer::deserialize(goods_data.item + "," + goods_data.data);
    std::string dis_goods_meta = buildItemMetaDisplay(deserialized, dynamic_cast<FreeMarket&>(plugin_).getTranslator());

    goods_info.setText(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Goods name: ") + goods_data.name + "\n" +
                       dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Goods description: ") + goods_data.text + "\n" +
                       dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Goods Info: ") + goods_data.item + "\n" + dis_goods_meta + "\n" +
                       dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Price: ") + std::to_string(goods_data.price) + "\n" +
                       dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Currency: ") + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal(goods_data.money_type) + "\n");

    menu.setControls({record_time, seller, buyer, goods_info});
    menu.setOnClose([this](endstone::Player* p) { recordMainMenu(*p); });
    player.sendForm(menu);
}

// 卖出记录显示菜单
void Menu::recordSellerMenu(endstone::Player& player) {
    auto records = dynamic_cast<FreeMarket&>(plugin_).getMarket().record_get_by_seller(player.getUniqueId().str());
    if (records.empty()) {
        notice_menu(player, endstone::ColorFormat::Red + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("No sell records"),
                    [this](endstone::Player& p) { this->recordMainMenu(p); });
        return;
    }
    endstone::ActionForm menu;
    for (const auto& record : std::ranges::reverse_view(records)) {
        menu.addButton(record.time, std::nullopt,
                       [this, record](endstone::Player* p) { recordDisplayMenu(*p, record); });
    }
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Sell Records"));
    menu.setOnClose([this](endstone::Player* p) { recordMainMenu(*p); });
    player.sendForm(menu);
}

// 买入记录显示菜单
void Menu::recordBuyerMenu(endstone::Player& player) {
    auto records = dynamic_cast<FreeMarket&>(plugin_).getMarket().record_get_by_buyer(player.getUniqueId().str());
    if (records.empty()) {
        notice_menu(player, endstone::ColorFormat::Red + dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("No buy records"),
                    [this](endstone::Player& p) { this->recordMainMenu(p); });
        return;
    }
    endstone::ActionForm menu;
    for (const auto& record : std::ranges::reverse_view(records)) {
        menu.addButton(record.time, std::nullopt,
                       [this, record](endstone::Player* p) { recordDisplayMenu(*p, record); });
    }
    menu.setTitle(dynamic_cast<FreeMarket&>(plugin_).getTranslator().getLocal("Buy Records"));
    menu.setOnClose([this](endstone::Player* p) { recordMainMenu(*p); });
    player.sendForm(menu);
}
