//
// Created by yuhang on 2025/11/17.
//

#include "menu.h"

//通知菜单
void Menu::notice_menu(endstone::Player& player,const string& msg,const std::function<void(endstone::Player&)>& yes_func) {
    endstone::MessageForm menu;
    menu.setTitle(Tran.getLocal("Notice"));
    menu.setContent(msg);
    menu.setButton1(Tran.getLocal("Back"));
    menu.setButton2(Tran.getLocal("Exit"));
    // 设置回调
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
    menu.setTitle(Tran.getLocal("FreeMarket Menu"));
    menu.setContent(Tran.getLocal("§l§o§bWelcome! You can open the menu by entering the /market command or right-clicking with emerald"));

    if (auto userdata = market.user_get(player.getUniqueId().str()); !userdata.status) {
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
void Menu::register_menu(endstone::Player& player) {
    endstone::ModalForm menu;
    menu.setTitle(Tran.getLocal("Register menu"));
    endstone::Label label;
    label.setText(Tran.getLocal("You has not an freemarket account,please register in this menu."));
    endstone::TextInput input;
    input.setLabel(Tran.getLocal("Input your username"));
    endstone::Dropdown dropdown;
    dropdown.setLabel(Tran.getLocal("Select an avatar.\nThe menu only supports presets,you can change it to custom later"));
    dropdown.setOptions({Tran.getLocal("Steve"),Tran.getLocal("Alex"),Tran.getLocal("Minions"),Tran.getLocal("Panda")});
    dropdown.setDefaultIndex(0);
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
void Menu::account_menu(endstone::Player& player) {
    endstone::ActionForm menu;
    menu.setTitle(Tran.getLocal("Account Info Menu"));
    auto user_data = market.user_get(player.getUniqueId().str());
    endstone::Button change_avatar;
    change_avatar.setIcon("textures/ui/icon_multiplayer");
    change_avatar.setText(Tran.getLocal("Change account avatar"));
    change_avatar.setOnClick([this](endstone::Player *p){
        player_avatar_menu(*p);
    });
    endstone::Button rename;
    rename.setText(Tran.getLocal("Rename account"));
    rename.setIcon("textures/ui/book_edit_default");
    rename.setOnClick([this](endstone::Player* p){
        player_rename_menu(*p);
    });
    endstone::Button get_payment;
    get_payment.setText(Tran.getLocal("Withdraw the payment"));
    get_payment.setIcon("textures/ui/trade_icon");
    get_payment.setOnClick([this](endstone::Player* p){
        get_item_to_player_menu(*p);
    });
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
        money = MarketCore::umoney_get_player_money(player.getName());
    }
    menu.setContent(Tran.getLocal("User name: ") + user_data.username + "\n" + endstone::ColorFormat::Green +Tran.getLocal("Money: ") + to_string(money));
    player.sendForm(menu);
}

//玩家头像更改菜单
void Menu::player_avatar_menu(endstone::Player& player) {
    endstone::ModalForm menu;
    menu.setTitle(Tran.getLocal("Change Avatar Menu"));
    endstone::Label label;
    label.setText(Tran.getLocal("Change the avatar via a dropdown box or an input box. \nIf text is entered in the input box, use the input box."));
    endstone::Dropdown dropdown;
    dropdown.setLabel(Tran.getLocal("Select an avatar."));
    dropdown.setOptions({Tran.getLocal("Steve"),Tran.getLocal("Alex"),Tran.getLocal("Minions"),Tran.getLocal("Panda")});
    dropdown.setDefaultIndex(0);
    endstone::TextInput textInput;
    textInput.setLabel(Tran.getLocal("Custom Avatar"));
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
        if (auto [fst, snd] = market.user_change_avatar(p->getUniqueId().str(),avatar); fst) {
            notice_menu(*p,Tran.getLocal(snd),[this](endstone::Player& pp) { account_menu(pp);});
        } else {
            notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal(snd),[this](endstone::Player& p) { this->account_menu(p);});
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
    menu.setTitle(Tran.getLocal("Rename Menu"));
    endstone::Label label;
    label.setText(Tran.getLocal("Change your user name"));
    endstone::TextInput textInput;
    textInput.setLabel(Tran.getLocal("Input new name"));
    textInput.setDefaultValue(market.GetUsername(player.getUniqueId().str()));
    menu.addControl(label);
    menu.addControl(textInput);
    menu.setOnSubmit([this](endstone::Player *p,const string& response) {
        auto parsedResponse = json::parse(response);
        const string name = parsedResponse[1];
        if (name.empty()) {
            return ;
        }
        if (const auto [fst, snd] = market.user_rename(p->getUniqueId().str(),name); fst) {
            notice_menu(*p,Tran.getLocal(snd),[this](endstone::Player& pp) { account_menu(pp);});
        } else {
            notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal(snd),[this](endstone::Player& p) { this->account_menu(p);});
        }
    });

    menu.setOnClose([this](endstone::Player* p){
        account_menu(*p);
    });
    player.sendForm(menu);
}

//商品上传菜单
void Menu::goods_upload_menu(endstone::Player& player) {
    if (const int player_goods_amount = Database.getGoodsCountByUuid(player.getUniqueId().str()); player_goods_amount > player_max_goods || player_goods_amount < 0) {
        notice_menu(player,Tran.getLocal("You have reached the maximum limit for your goods"),[this](endstone::Player& p) { this->main_menu(p);});
        return;
    }
    endstone::ModalForm menu;
    menu.setTitle(Tran.getLocal("Add Goods Menu"));
    vector<ItemData> quick_items;
    vector<string> quick_items_name;
    for (int i = 0;i <= 8;i++) {
        if (const auto one_item = player.getInventory().getItem(i)) {
            const auto meta = one_item->getItemMeta();
            if (one_item->hasItemMeta()){
                if (meta->hasEnchants()) {
                    quick_items.push_back({string(one_item->getType().getId()),one_item->getAmount(), Meta_data{meta->getLore(),meta->getDamage(),meta->getDisplayName(),MarketCore::EnchantToSimMap(meta->getEnchants())}});
                    quick_items_name.push_back(to_string(i+1) + ". "+ Tran.getLocal("Item ID: ") + string(one_item->getType().getId()) + Tran.getLocal("Number: ") +
                                                       to_string(one_item->getAmount()));
                }
                else {
                    quick_items.push_back({string(one_item->getType().getId()),one_item->getAmount(), Meta_data{meta->getLore(),meta->getDamage(),meta->getDisplayName()}});
                    quick_items_name.push_back(to_string(i+1) + ". "+ Tran.getLocal("Item ID: ") + string(one_item->getType().getId()) + Tran.getLocal("Number: ") +
                                                       to_string(one_item->getAmount()));
                }
            }
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
        const int item_index = json_response[0];
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

void Menu::goods_upload_confirm_menu(endstone::Player& player,const ItemData& item_data,int quick_index) {
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
    text.setLabel(Tran.getLocal("Goods description"));
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
    vector image_name_opt = {Tran.getLocal("Apple"),Tran.getLocal("Tools & Equipment"),Tran.getLocal("Diamond"),Tran.getLocal("Grass block")};
    vector<string> image_opt = {"textures/ui/icon_apple","textures/ui/icon_recipe_equipment","textures/items/diamond","textures/blocks/grass_side_carried"};
    image_drop.setLabel(Tran.getLocal("Select a cover"));
    image_drop.setOptions(image_name_opt);
    image_drop.setDefaultIndex(0);

    tag_drop.setLabel(Tran.getLocal("Set goods tag"));
    vector<string> tag_opt = {"Food","Tools & Equipment","Ore","Block","Other"};
    vector tag_name_opt = {Tran.getLocal("Food"),Tran.getLocal("Tools & Equipment"),Tran.getLocal("Ore"),Tran.getLocal("Block"),Tran.getLocal("Other")};
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
        if (money_index != 5) {
            if (item_data.item_meta.has_value()) {
                auto string_meta = MarketCore::ItemMetaToString(item_data.item_meta.value());
                if (auto [fst, snd] = market.goods_add(p->getUniqueId().str(),the_title,the_text,item_data.item_id+","+to_string(item_data.item_num),string_meta,image_opt[image_index],price,money_list[money_index],tag_opt[tag_index]); fst) {
                    notice_menu(*p,Tran.getLocal(snd),[this](endstone::Player& p) { this->main_menu(p);});
                } else {
                    notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal(snd),[this](endstone::Player& p) { this->main_menu(p);});
                    return;
                }
            } else {
                if (auto status = market.goods_add(p->getUniqueId().str(),the_title,the_text,item_data.item_id+","+to_string(item_data.item_num),"",image_opt[image_index],price,money_list[money_index],tag_opt[tag_index]); status.first) {
                    notice_menu(*p,Tran.getLocal(status.second),[this](endstone::Player& p) { this->main_menu(p);});
                } else {
                    notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal(status.second),[this](endstone::Player& p) { this->main_menu(p);});
                    return;
                }
            }
        } else {
            if (item_data.item_meta.has_value()) {
                auto string_meta = MarketCore::ItemMetaToString(item_data.item_meta.value());
                if (auto [fst, snd] = market.goods_add(p->getUniqueId().str(),the_title,the_text,item_data.item_id+","+to_string(item_data.item_num),string_meta,image_opt[image_index],price,the_custom_money,tag_opt[tag_index]); fst) {
                    notice_menu(*p,Tran.getLocal(snd),[this](endstone::Player& p) { this->main_menu(p);});
                } else {
                    notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal(snd),[this](endstone::Player& p) { this->main_menu(p);});
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
        std::optional<endstone::ItemStack> air = std::nullopt;
        p->getInventory().setItem(quick_index,air);
        plugin_.getServer().broadcastMessage("§l§2"+Tran.getLocal("[Marketing Promotion] New goods have been listed: ")+"§r"+the_title);
    });
    menu.setOnClose([this](endstone::Player*p){
        goods_upload_menu(*p);
    });
    player.sendForm(menu);
}
