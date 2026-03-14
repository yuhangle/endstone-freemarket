//
// Created by yuhang on 2025/11/17.
//

#include "menu.h"

#include "nbt_tool.hpp"

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
    const auto user_data = market.user_get(player.getUniqueId().str());
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
    for (int i = 0; i <= 8; i++) {
        if (const auto one_item = player.getInventory().getItem(i)) {
            const auto meta = one_item->getItemMeta();

            if (auto nbt = one_item->getNbt(); !nbt.empty()) {
                // 物品含有 NBT
                if (one_item->hasItemMeta()) {
                    // 有 Meta，按原有逻辑处理
                    if (meta->hasEnchants()) {
                        quick_items.push_back({
                            string(one_item->getType().getId()),
                            one_item->getAmount(),
                            Meta_data{meta->getLore(), meta->getDamage(), meta->getDisplayName(),
                                      MarketCore::EnchantToSimMap(meta->getEnchants())},
                            nbt
                        });
                    } else {
                        quick_items.push_back({
                            string(one_item->getType().getId()),
                            one_item->getAmount(),
                            Meta_data{meta->getLore(), meta->getDamage(), meta->getDisplayName()},
                            nbt
                        });
                    }
                } else {
                    // 有 NBT 但无 Meta（例如某些只存 NBT 数据的物品）
                    quick_items.push_back({
                        string(one_item->getType().getId()),
                        one_item->getAmount(),
                        nullopt,   // Meta 为空
                        nbt
                    });
                }
                // 统一添加显示名称
                quick_items_name.push_back(to_string(i+1) + ". " + Tran.getLocal("Item ID: ") +
                                           string(one_item->getType().getId()) + Tran.getLocal("Number: ") +
                                           to_string(one_item->getAmount()));
            } else {
                // 无 NBT
                quick_items.push_back({string(one_item->getType().getId()), one_item->getAmount(), nullopt});
                quick_items_name.push_back(to_string(i+1) + ". " + Tran.getLocal("Item ID: ") +
                                           string(one_item->getType().getId()) + Tran.getLocal("Number: ") +
                                           to_string(one_item->getAmount()));
            }
        } else {
            // 空槽位
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
    vector money_name_list = {Tran.getLocal("money"),Tran.getLocal("minecraft:diamond"),Tran.getLocal("minecraft:emerald"),Tran.getLocal("minecraft:gold_ingot"),Tran.getLocal("minecraft:iron_ingot"),Tran.getLocal("Custom")};
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
        string nbt;
        if (item_data.nbt)
        {
            auto nbt_1 = NBTTools::nbtToJson(item_data.nbt.value());
            nbt = NBTTools::jsonToCompactString(nbt_1);
        }
        if (money_index != 5) {
            if (item_data.item_meta.has_value()) {
                auto string_meta = MarketCore::ItemMetaToString(item_data.item_meta.value());
                if (item_data.nbt)
                {
                    string_meta += "," + nbt;
                }
                if (auto [fst, snd] = market.goods_add(p->getUniqueId().str(),the_title,the_text,item_data.item_id+","+to_string(item_data.item_num),string_meta,image_opt[image_index],price,money_list[money_index],tag_opt[tag_index]); fst) {
                    notice_menu(*p,Tran.getLocal(snd),[this](endstone::Player& p_2) { this->main_menu(p_2);});
                } else {
                    notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal(snd),[this](endstone::Player& p) { this->main_menu(p);});
                    return;
                }
            } else {
                if (auto [fst, snd] = market.goods_add(p->getUniqueId().str(),the_title,the_text,item_data.item_id+","+to_string(item_data.item_num),nbt,image_opt[image_index],price,money_list[money_index],tag_opt[tag_index]); fst) {
                    notice_menu(*p,Tran.getLocal(snd),[this](endstone::Player& p_3) { this->main_menu(p_3);});
                } else {
                    notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal(snd),[this](endstone::Player& p_4) { this->main_menu(p_4);});
                    return;
                }
            }
        } else {
            if (item_data.item_meta.has_value()) {
                auto string_meta = MarketCore::ItemMetaToString(item_data.item_meta.value());
                if (item_data.nbt)
                {
                    string_meta += "," + nbt;
                }
                if (auto [fst, snd] = market.goods_add(p->getUniqueId().str(),the_title,the_text,item_data.item_id+","+to_string(item_data.item_num),string_meta,image_opt[image_index],price,the_custom_money,tag_opt[tag_index]); fst) {
                    notice_menu(*p,Tran.getLocal(snd),[this](endstone::Player& p_5) { this->main_menu(p_5);});
                } else {
                    notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal(snd),[this](endstone::Player& p_6) { this->main_menu(p_6);});
                    return;
                }
            } else {
                if (auto [fst, snd] = market.goods_add(p->getUniqueId().str(),the_title,the_text,item_data.item_id+","+to_string(item_data.item_num),nbt,image_opt[image_index],price,the_custom_money,tag_opt[tag_index]); fst) {
                    notice_menu(*p,Tran.getLocal(snd),[this](endstone::Player& p_7) { this->main_menu(p_7);});
                } else {
                    notice_menu(*p,endstone::ColorFormat::Red+Tran.getLocal(snd),[this](endstone::Player& p_8) { this->main_menu(p_8);});
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

// 市场展示主菜单
void Menu::market_display_menu(endstone::Player& player) {
    endstone::ActionForm menu;
    menu.setTitle(Tran.getLocal("Market"));
    menu.addButton("\uE021 " + endstone::ColorFormat::MaterialDiamond + Tran.getLocal("§lSearch") + " \uE021",
                   std::nullopt, [this](endstone::Player* p) { search_goods_menu(*p); });
    if (auto goods_data = market.goods_get_all(); goods_data.empty()) {
        menu.setContent(Tran.getLocal("No goods are currently for sale"));
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
    auto seller_data = market.user_get(goods_data.uuid);
    int buyer_money;
    if (money_config == "freemarket") {
        if (auto buyer_data = market.user_get(player.getUniqueId().str()); buyer_data.status) {
            buyer_money = buyer_data.money;
        } else {
            buyer_money = 0;
        }
    } else {
        buyer_money = MarketCore::umoney_get_player_money(player.getName());
    }
    std::string seller_info = Tran.getLocal("Unknown");
    if (seller_data.status) {
        seller_info = seller_data.username + "(" + seller_data.playername + ")";
    }
    endstone::ActionForm menu;
    menu.setTitle(goods_data.name);

    endstone::Label goods_info;
    endstone::Label buyer_info;
    endstone::Button confirm_buy;

    std::string dis_goods_meta;
    if (!goods_data.data.empty()) {
        // 使用 BackItemData 获取完整的物品数据

        if (auto [item_id, item_num, item_meta, item_nbt] = MarketCore::BackItemData(goods_data.item, goods_data.data); item_meta.has_value()) {
            const auto& [lore, damage, display_name, enchants] = item_meta.value();
            dis_goods_meta += Tran.getLocal("Item damage: ") + std::to_string(damage);
            if (!display_name.empty()) {
                dis_goods_meta += "\n" + Tran.getLocal("Name tag: ") + display_name;
            }
            dis_goods_meta += "§r";
            if (!lore.empty()) {
                dis_goods_meta += "\n lore: ";
                for (const auto& one_lore : lore) {
                    dis_goods_meta += one_lore + ", ";
                }
            }
            if (!enchants.empty()) {
                dis_goods_meta += "\n" + Tran.getLocal("Enchant: ");
                for (const auto& [fst, snd] : enchants) {
                    dis_goods_meta += fst + ":" + std::to_string(snd) + ", ";
                }
            }
            // 如果同时有 NBT 数据，附加提示
            if (item_nbt.has_value() && !item_nbt->empty()) {
                dis_goods_meta += "\n§7(+ Contains container data)§r";
            }
        } else if (item_nbt.has_value() && !item_nbt->empty()) {
            // 只有 NBT 的情况
            dis_goods_meta = Tran.getLocal("(Contains custom NBT data)");
        } else {
            dis_goods_meta = Tran.getLocal("No additional data");
        }
    } else {
        dis_goods_meta = Tran.getLocal("No data");
    }

    goods_info.setText(Tran.getLocal("Goods name: ") + goods_data.name + "\n" +
                       Tran.getLocal("Goods description: ") + goods_data.text + "\n" +
                       Tran.getLocal("Goods Info: ") + goods_data.item + "\n" + dis_goods_meta + "\n" +
                       Tran.getLocal("Price: ") + std::to_string(goods_data.price) + "\n" +
                       Tran.getLocal("Currency: ") + Tran.getLocal(goods_data.money_type) + "\n" +
                       Tran.getLocal("Seller info: ") + seller_info);

    buyer_info.setText(Tran.getLocal("Your money: ") + std::to_string(buyer_money));

    confirm_buy.setText(Tran.getLocal("Buy it"));
    confirm_buy.setIcon("textures/ui/confirm");
    confirm_buy.setOnClick([this, goods_data](endstone::Player* p) { confirm_to_buy_menu(*p, goods_data); });

    if (seller_data.status) {
        endstone::Button seller_home_page;
        seller_home_page.setText(Tran.getLocal("Seller homepage"));
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
    if (MarketCore::check_player_inventory_full(player)) {
        notice_menu(player, endstone::ColorFormat::Red + Tran.getLocal("Your inventory is full, please clear some space and try again"),
                    [this](endstone::Player& p) { main_menu(p); });
        return;
    }
    endstone::MessageForm menu;
    menu.setTitle(Tran.getLocal("Confirm Menu"));
    menu.setButton1(Tran.getLocal("Yes,It will be mine"));
    menu.setButton2(Tran.getLocal("No,I don't want to buy it"));
    menu.setContent(Tran.getLocal("Confirm to buy it?"));

    menu.setOnSubmit([this, goods_data](endstone::Player* p, int chose) {
        if (chose == 0) {
            if (!market.goods_exist(goods_data.gid)) {
                notice_menu(*p, endstone::ColorFormat::Red + Tran.getLocal("The item does not exist"),
                            [this](endstone::Player& p) { this->main_menu(p); });
                return;
            }
            auto seller_data = market.user_get(goods_data.uuid);

            if (goods_data.money_type == "money") {
                if (int buyer_money = MarketCore::get_player_money(*p); buyer_money >= goods_data.price) {
                    (void)market_core_->general_change_money(p->getUniqueId().str(), p->getName(), -goods_data.price);

                    // 解析物品数据
                    auto [item_id, item_num, item_meta, item_nbt] = MarketCore::BackItemData(goods_data.item, goods_data.data);

                    endstone::ItemStack new_item(item_id, item_num);
                    auto meta_ptr = plugin_.getServer().getItemFactory().getItemMeta(new_item.getType());

                    // 判断是否使用 NBT
                    if (item_nbt.has_value() && !item_nbt->empty()) {
                        // 使用 NBT 构建物品
                        new_item.setNbt(item_nbt.value().get<endstone::CompoundTag>());
                    } else {
                        // 使用 Meta 构建物品（如果存在）
                        if (item_meta.has_value()) {
                            endstone::ItemMeta* meta = meta_ptr.get();
                            const auto& [lore, damage, display_name, enchants] = item_meta.value();
                            meta->setLore(lore);
                            meta->setDamage(damage);
                            meta->setDisplayName(display_name);
                            if (!enchants.empty()) {
                                for (const auto &[fst, snd] : enchants) {
                                    auto enchantID = endstone::EnchantmentId(fst);
                                    (void)meta->addEnchant(enchantID, snd, true);
                                }
                            }
                            new_item.setItemMeta(meta);
                        }
                    }

                    p->getInventory().addItem(new_item);
                    auto delGoodsStatus = market.goods_del(goods_data.gid);
                    (void)market_core_->general_change_money(seller_data.uuid, seller_data.playername, goods_data.price);
                    if (auto get_seller = plugin_.getServer().getPlayer(seller_data.playername)) {
                        get_seller->sendMessage(Tran.getLocal("Your goods have been purchased. Transaction details: ") + "\n" +
                                                Tran.getLocal("Buyer: ") + p->getName() + "\n" +
                                                Tran.getLocal("Goods info: ") + goods_data.name);
                    }
                    notice_menu(*p, Tran.getLocal("Successful purchase"),
                                [this](endstone::Player& p) { main_menu(p); });
                    plugin_.getServer().broadcastMessage("§l§2" + Tran.getLocal("[Marketing Promotion] This good has been purchased: ") + "§r" + goods_data.name);
                    auto [fst, snd] = MarketCore::GoodsDataToString(goods_data);
                    (void)market.record_add(seller_data.uuid, p->getUniqueId().str(), snd);
                } else {
                    notice_menu(*p, endstone::ColorFormat::Red + Tran.getLocal("You have not enough money"),
                                [this, goods_data](endstone::Player& p_2) { goods_view_menu(p_2, goods_data); });
                }
            } else {
                // 非货币交易（物品换物品）
                auto [item_id, item_num, item_meta, item_nbt] = MarketCore::BackItemData(goods_data.item, goods_data.data);

                if (std::vector<ItemData> cleared_itemData; MarketCore::check_player_inventory_and_clear(*p, goods_data.money_type, goods_data.price, cleared_itemData)) {
                    endstone::ItemStack new_item(item_id, item_num);
                    auto meta_ptr = plugin_.getServer().getItemFactory().getItemMeta(new_item.getType());

                    // 判断是否使用 NBT
                    if (item_nbt.has_value() && !item_nbt->empty()) {
                        new_item.setNbt(item_nbt.value().get<endstone::CompoundTag>());
                        NBTTools::dumpTag(new_item.getNbt());
                    } else {
                        if (item_meta.has_value()) {
                            endstone::ItemMeta* meta = meta_ptr.get();
                            const auto& meta_val = item_meta.value();
                            meta->setLore(meta_val.lore);
                            meta->setDamage(meta_val.damage);
                            meta->setDisplayName(meta_val.display_name);
                            if (!meta_val.enchants.empty()) {
                                for (const auto &[fst, snd] : meta_val.enchants) {
                                    auto enchantID = endstone::EnchantmentId(fst);
                                    (void)meta->addEnchant(enchantID, snd, true);
                                }
                            }
                            new_item.setItemMeta(meta);
                        }
                    }

                    p->getInventory().addItem(new_item);
                    auto delGoodsStatus = market.goods_del(goods_data.gid);
                    if (auto get_seller = plugin_.getServer().getPlayer(seller_data.playername)) {
                        get_seller->sendMessage(Tran.getLocal("Your goods have been purchased. Transaction details: ") + "\n" +
                                                Tran.getLocal("Buyer: ") + p->getName() + "\n" +
                                                Tran.getLocal("Goods info: ") + goods_data.name);
                    }
                    // 在 confirm_to_buy_menu 的非货币交易分支中，将物品存入卖家账户时添加调试
                    for (const auto& one_item : cleared_itemData) {
                        std::string string_itemData = MarketCore::ItemDataToString(one_item);
                        (void)market.user_add_item(seller_data.uuid, string_itemData);
                    }
                    notice_menu(*p, Tran.getLocal("Successful purchase"),
                                [this](endstone::Player& p) { this->main_menu(p); });
                    plugin_.getServer().broadcastMessage("§l§2" + Tran.getLocal("[Marketing Promotion] This good has been purchased: ") + "§r" + goods_data.name);
                    auto [fst, snd] = MarketCore::GoodsDataToString(goods_data);
                    (void)market.record_add(seller_data.uuid, p->getUniqueId().str(), snd);
                } else {
                    notice_menu(*p, endstone::ColorFormat::Red + Tran.getLocal("You have not enough money"),
                                [this](endstone::Player& p) { this->main_menu(p); });
                }
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
    const auto user_all_item = MarketCore::UserItemRead(player.getUniqueId().str());
    if (user_all_item.empty()) {
        notice_menu(player, endstone::ColorFormat::Red + Tran.getLocal("You have no withdrawable payment"),
                    [this](endstone::Player& p) { account_menu(p); });
        return;
    }

    endstone::ModalForm menu;
    menu.setTitle(Tran.getLocal("Withdraw the payment"));

    std::string display_info;
    for (const auto& [item_id, item_num, item_meta, item_nbt] : user_all_item) {
        std::string dis_goods_meta;
        if (item_meta.has_value()) {
            const auto& [lore, damage, display_name, enchants] = item_meta.value();
            dis_goods_meta += Tran.getLocal("Item damage: ") + std::to_string(damage);
            if (!display_name.empty()) {
                dis_goods_meta += "\n" + Tran.getLocal("Name tag: ") + display_name;
            }
            dis_goods_meta += "§r";
            if (!lore.empty()) {
                dis_goods_meta += "\n lore: ";
                for (const auto& one_lore : lore) {
                    dis_goods_meta += one_lore + ", ";
                }
            }
            if (!enchants.empty()) {
                dis_goods_meta += "\n" + Tran.getLocal("Enchant: ");
                for (const auto& [fst, snd] : enchants) {
                    dis_goods_meta += fst + ":" + std::to_string(snd) + ", ";
                }
            }
        } else if (item_nbt.has_value()) {
            dis_goods_meta = "(Contains NBT data)";
        }

        display_info.append(item_id)
            .append(" x ")
            .append(std::to_string(item_num))
            .append("\n")
            .append(dis_goods_meta)
            .append("\n--------------------\n");
    }

    endstone::Label context;
    context.setText(Tran.getLocal("Your payment: ") + "\n" + display_info);
    menu.setControls({context});

    menu.setOnSubmit([this](endstone::Player* p, const std::string& response) {
        // 读取最新数据（防止提交前被修改）
        for (const auto user_items = MarketCore::UserItemRead(p->getUniqueId().str()); const auto& item : user_items) {
            const auto& [item_id, item_num, item_meta, item_nbt] = item;

            endstone::ItemStack itemStack(item_id, item_num);

            // 优先使用 NBT
            if (item_nbt.has_value() && !item_nbt->empty()) {
                itemStack.setNbt(item_nbt.value().get<endstone::CompoundTag>());
            }
            // 其次使用 Meta
            else if (item_meta.has_value()) {
                auto meta_ptr = plugin_.getServer().getItemFactory().getItemMeta(itemStack.getType());
                endstone::ItemMeta* meta = meta_ptr.get();
                const auto& [lore, damage, display_name, enchants] = item_meta.value();
                meta->setLore(lore);
                meta->setDamage(damage);
                meta->setDisplayName(display_name);
                if (!enchants.empty()) {
                    for (const auto& [fst, snd] : enchants) {
                        const auto enchantID = endstone::EnchantmentId(fst);
                        (void)meta->addEnchant(enchantID, snd, true);
                    }
                }
                itemStack.setItemMeta(meta);
            }

            p->getInventory().addItem(itemStack);
        }

        // 清空提现代币（假设已实现）
        (void)market.user_clear_item(p->getUniqueId().str());

        notice_menu(*p, Tran.getLocal("The withdrawal is complete"),
                    [this](endstone::Player& p) { account_menu(p); });
    });

    menu.setOnClose([this](endstone::Player *p) { account_menu(*p); });
    player.sendForm(menu);
}

// 选择管理商品菜单
void Menu::choose_manage_goods_menu(endstone::Player &player) {
    auto all_user_goods = market.goods_get_by_uuid(player.getUniqueId().str());
    if (all_user_goods.empty()) {
        notice_menu(player, endstone::ColorFormat::Red + Tran.getLocal("You have not goods"),
                    [this](endstone::Player& p) { main_menu(p); });
        return;
    }
    endstone::ModalForm menu;
    menu.setTitle(Tran.getLocal("Manage Goods Menu"));
    endstone::Dropdown goods_list;
    std::vector<std::string> goods_name_list;
    int i = 0;
    for (const auto& one_goods : all_user_goods) {
        i++;
        goods_name_list.push_back(std::to_string(i) + ". " + one_goods.name);
    }
    goods_list.setLabel(Tran.getLocal("Choose goods to manage"));
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
    menu.setTitle(Tran.getLocal("Manage Goods Menu"));
    endstone::Button edit_button;
    endstone::Button del_button;

    edit_button.setText(Tran.getLocal("Edit goods info"));
    edit_button.setOnClick([this, goods_data](endstone::Player* p) { edit_goods_menu(*p, goods_data); });

    del_button.setText(Tran.getLocal("Delete goods and return it"));
    del_button.setOnClick([this, goods_data](endstone::Player* p) { del_goods_menu(*p, goods_data); });

    menu.setOnClose([this](endstone::Player* p) { main_menu(*p); });
    menu.setControls({edit_button, del_button});
    player.sendForm(menu);
}

// 删除商品菜单
void Menu::del_goods_menu(endstone::Player& player, const Market_Action::Goods_data& goods_data) {
    if (MarketCore::check_player_inventory_full(player)) {
        notice_menu(player, endstone::ColorFormat::Red + Tran.getLocal("Your inventory is full, please clear some space and try again"),
                    [this](endstone::Player& p) { main_menu(p); });
        return;
    }
    endstone::MessageForm menu;
    menu.setTitle(Tran.getLocal("Delete Goods Menu"));
    menu.setContent(Tran.getLocal("Are you sure to delete it?"));
    menu.setButton1(Tran.getLocal("Yes,delete it and return it"));
    menu.setButton2(Tran.getLocal("No,I don't want to delete it"));
    menu.setOnSubmit([this, goods_data](endstone::Player* p, const int bt_i) {
        if (bt_i == 0) {
            const auto update_goods_data = market.goods_get_by_gid(goods_data.gid);
            if (!update_goods_data.status) {
                notice_menu(*p, endstone::ColorFormat::Red + Tran.getLocal("The goods not exist"),
                            [this](endstone::Player& p) { this->choose_manage_goods_menu(p); });
                return;
            }
            if (auto [fst, snd] = market.goods_del(update_goods_data.gid); fst) {
                auto [item_id, item_num, item_meta, item_nbt] = MarketCore::BackItemData(goods_data.item, goods_data.data);
                endstone::ItemStack itemStack(item_id, item_num);
                if (item_nbt.has_value()) {
                    itemStack.setNbt(item_nbt.value().get<endstone::CompoundTag>());
                }
                if (item_meta.has_value()) {
                    if (!item_nbt.has_value())
                    {
                        const auto meta_ptr = plugin_.getServer().getItemFactory().getItemMeta(itemStack.getType());
                        endstone::ItemMeta* meta = meta_ptr.get();
                        meta->setLore(item_meta->lore);
                        meta->setDamage(item_meta->damage);
                        meta->setDisplayName(item_meta->display_name);
                        if (!item_meta->enchants.empty()) {
                            for (const auto &[fst_2, snd_2] : item_meta->enchants) {
                                const auto enchantID = endstone::EnchantmentId(fst_2);
                                (void)meta->addEnchant(enchantID, snd_2, false);
                            }
                        }
                        itemStack.setItemMeta(meta);
                        }
                    }
                p->getInventory().addItem(itemStack);
                notice_menu(*p, Tran.getLocal(snd),
                            [this](endstone::Player& p) { this->choose_manage_goods_menu(p); });
                plugin_.getServer().broadcastMessage("§l§2" + Tran.getLocal("[Marketing Promotion] This good has been delisted: ") + "§r" + goods_data.name);
            } else {
                notice_menu(*p, endstone::ColorFormat::Red + Tran.getLocal(snd),
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

    const std::vector image_name_opt = {Tran.getLocal("Apple"), Tran.getLocal("Tools & Equipment"),
                                         Tran.getLocal("Diamond"), Tran.getLocal("Grass block")};
    std::vector<std::string> image_opt = {"textures/ui/icon_apple", "textures/ui/icon_recipe_equipment",
                                           "textures/items/diamond", "textures/blocks/grass_side_carried"};
    image_drop.setLabel(Tran.getLocal("Select a cover"));
    image_drop.setOptions(image_name_opt);
    image_drop.setDefaultIndex(0);

    image_custom.setLabel(Tran.getLocal("Set custom cover"));

    tag_drop.setLabel(Tran.getLocal("Set goods tag"));
    std::vector<std::string> tag_opt = {"Food", "Tools & Equipment", "Ore", "Block", "Other"};
    const std::vector tag_name_opt = {Tran.getLocal("Food"), Tran.getLocal("Tools & Equipment"),
                                       Tran.getLocal("Ore"), Tran.getLocal("Block"), Tran.getLocal("Other")};
    tag_drop.setOptions(tag_name_opt);
    tag_drop.setDefaultIndex(0);

    menu.setControls({title_input, text_input, image_drop, image_custom, tag_drop});

    menu.setOnSubmit([this, goods_data, image_opt, tag_opt](endstone::Player* p, const std::string& response) {
        auto json_response = json::parse(response);
        std::string the_title = json_response[0];
        std::string the_text = json_response[1];
        int image_index = json_response[2];
        std::string custom_image = json_response[3];
        int tag_index = json_response[4];
        if (the_title.empty() || the_text.empty() || (image_index == 3 && custom_image.empty())) {
            notice_menu(*p, endstone::ColorFormat::Red + Tran.getLocal("Title and description can not be empty,and when you select custom cover,custom cover can not be empty"),
                        [this](endstone::Player& pp) { main_menu(pp); });
            return;
        }
        if (!market.goods_exist(goods_data.gid)) {
            notice_menu(*p, endstone::ColorFormat::Red + Tran.getLocal("Goods not exists"),
                        [this](endstone::Player& p_2) { this->choose_manage_goods_menu(p_2); });
            return;
        }
        std::string image;
        if (image_index == 3) {
            image = custom_image;
        } else {
            image = image_opt[image_index];
        }
        auto status = market.goods_update(goods_data.gid, the_title, the_text, image, tag_opt[tag_index]);
        if (status.first) {
            notice_menu(*p, Tran.getLocal(status.second),
                        [this](endstone::Player& p) { this->choose_manage_goods_menu(p); });
        } else {
            notice_menu(*p, endstone::ColorFormat::Red + Tran.getLocal(status.second),
                        [this](endstone::Player& p) { this->choose_manage_goods_menu(p); });
        }
    });
    menu.setOnClose([this, goods_data](endstone::Player* p) { manage_goods_menu(*p, goods_data); });
    player.sendForm(menu);
}

// 显示商家主页
void Menu::seller_homepage(endstone::Player& player, Market_Action::User_data seller_data) {
    const auto seller_all_goods = market.goods_get_by_uuid(seller_data.uuid);
    if (seller_all_goods.empty()) {
        notice_menu(player, endstone::ColorFormat::Red + Tran.getLocal("Seller has not goods"),
                    [this](endstone::Player& p) { this->market_display_menu(p); });
        return;
    }
    if (!seller_data.status) {
        notice_menu(player, endstone::ColorFormat::Red + Tran.getLocal("Seller not exists"),
                    [this](endstone::Player& p) { this->market_display_menu(p); });
        return;
    }
    endstone::ActionForm menu;
    menu.setTitle(Tran.getLocal("Seller homepage"));
    menu.addHeader(seller_data.username);
    menu.addDivider();
    menu.addLabel(Tran.getLocal("Seller goods: ") + "\n");
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
    menu.setTitle(Tran.getLocal("Search Goods Menu"));
    endstone::TextInput search_text_input;
    endstone::Dropdown tag_dropdown;

    search_text_input.setLabel(Tran.getLocal("Search goods"));
    tag_dropdown.setLabel(Tran.getLocal("Tag"));
    std::vector<std::string> tag_opt = {"All", "Food", "Tools & Equipment", "Ore", "Block", "Other"};
    const std::vector tag_name_opt = {Tran.getLocal("All"), Tran.getLocal("Food"), Tran.getLocal("Tools & Equipment"),
                                       Tran.getLocal("Ore"), Tran.getLocal("Block"), Tran.getLocal("Other")};
    tag_dropdown.setOptions(tag_name_opt);
    menu.setControls({search_text_input, tag_dropdown});
    menu.setOnSubmit([this, tag_opt](endstone::Player* p, const std::string& response) {
        auto json_response = json::parse(response);
        const std::string search_text = json_response[0];
        const int tag_index = json_response[1];
        display_search_result_menu(*p, search_text, tag_opt[tag_index]);
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
    menu.setTitle(Tran.getLocal("Search Result") + " : " + "\"" + search_text + "\"");
    if (search_text.empty()) {
        for (const auto all_goods = market.goods_get_all(); const auto& one_goods : all_goods) {
            if (one_goods.tag == tag || tag == "All") {
                menu.addButton(one_goods.name, one_goods.image,
                               [this, one_goods](endstone::Player* p) { goods_view_menu(*p, one_goods); });
            }
        }
    } else {
        for (const auto all_goods = market.goods_get_all(); const auto& one_goods : all_goods) {
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
    if (MarketCore::canTriggerEvent(event.getPlayer().getName())) {
        main_menu(event.getPlayer());
    }
}

// 交易记录主菜单
void Menu::recordMainMenu(endstone::Player& player) {
    endstone::ActionForm menu;
    endstone::Button seller_record;
    endstone::Button buyer_record;

    seller_record.setText(Tran.getLocal("Sell Records"));
    buyer_record.setText(Tran.getLocal("Buy Records"));
    menu.setTitle(Tran.getLocal("Transaction Records"));
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
    auto seller_data = market.user_get(record.seller);
    auto buyer_data = market.user_get(record.buyer);
    record_time.setText(Tran.getLocal("Date: ") + record.time);
    seller.setText(Tran.getLocal("Seller: ") + seller_data.username + "(" + seller_data.playername + ")");
    buyer.setText(Tran.getLocal("Buyer: ") + buyer_data.username + "(" + buyer_data.playername + ")");
    auto goods_data = MarketCore::StringToGoodsData(record.goods).second;

    std::string dis_goods_meta;
    if (goods_data.data != "None") {
        Meta_data goods_meta = MarketCore::StringToItemMeta(goods_data.data);
        dis_goods_meta += Tran.getLocal("Item damage: ") + std::to_string(goods_meta.damage);
        if (!goods_meta.display_name.empty()) {
            dis_goods_meta += "\n" + Tran.getLocal("Name tag: ") + goods_meta.display_name;
        }
        dis_goods_meta += "§r";
        if (!goods_meta.lore.empty()) {
            dis_goods_meta += "\n lore: ";
            for (const auto& one_lore : goods_meta.lore) {
                dis_goods_meta += one_lore + ", ";
            }
        }
        if (!goods_meta.enchants.empty()) {
            dis_goods_meta += "\n" + Tran.getLocal("Enchant: ");
            for (const auto& [fst, snd] : goods_meta.enchants) {
                dis_goods_meta += fst + ":" + std::to_string(snd) + ", ";
            }
        }
    }

    goods_info.setText(Tran.getLocal("Goods name: ") + goods_data.name + "\n" +
                       Tran.getLocal("Goods description: ") + goods_data.text + "\n" +
                       Tran.getLocal("Goods Info: ") + goods_data.item + "\n" + dis_goods_meta + "\n" +
                       Tran.getLocal("Price: ") + std::to_string(goods_data.price) + "\n" +
                       Tran.getLocal("Currency: ") + Tran.getLocal(goods_data.money_type) + "\n");

    menu.setControls({record_time, seller, buyer, goods_info});
    menu.setOnClose([this](endstone::Player* p) { recordMainMenu(*p); });
    player.sendForm(menu);
}

// 卖出记录显示菜单
void Menu::recordSellerMenu(endstone::Player& player) {
    auto records = market.record_get_by_seller(player.getUniqueId().str());
    if (records.empty()) {
        notice_menu(player, endstone::ColorFormat::Red + Tran.getLocal("No sell records"),
                    [this](endstone::Player& p) { this->recordMainMenu(p); });
        return;
    }
    endstone::ActionForm menu;
    for (const auto& record : std::ranges::reverse_view(records)) {
        menu.addButton(record.time, std::nullopt,
                       [this, record](endstone::Player* p) { recordDisplayMenu(*p, record); });
    }
    menu.setTitle(Tran.getLocal("Sell Records"));
    menu.setOnClose([this](endstone::Player* p) { recordMainMenu(*p); });
    player.sendForm(menu);
}

// 买入记录显示菜单
void Menu::recordBuyerMenu(endstone::Player& player) {
    auto records = market.record_get_by_buyer(player.getUniqueId().str());
    if (records.empty()) {
        notice_menu(player, endstone::ColorFormat::Red + Tran.getLocal("No buy records"),
                    [this](endstone::Player& p) { this->recordMainMenu(p); });
        return;
    }
    endstone::ActionForm menu;
    for (const auto& record : std::ranges::reverse_view(records)) {
        menu.addButton(record.time, std::nullopt,
                       [this, record](endstone::Player* p) { recordDisplayMenu(*p, record); });
    }
    menu.setTitle(Tran.getLocal("Buy Records"));
    menu.setOnClose([this](endstone::Player* p) { recordMainMenu(*p); });
    player.sendForm(menu);
}