#include "trade_engine.hpp"
#include "freemarket.h"
#include "market_core.h"
#include "item_serializer.hpp"
#include "menu_helpers.hpp"

TradeResult TradeEngine::executePurchase(endstone::Player& buyer,
                                          const Market_Action::Goods_data& goods_data) const
{
    // Check goods still exists
    if (auto latest_goods = dynamic_cast<FreeMarket&>(plugin_).getMarket().goods_get_by_gid(goods_data.gid); !latest_goods.status) {
        return {false, "The item does not exist", "", ""};
    }

    // Check inventory space
    if (isPlayerInventoryFull(buyer)) {
        return {false, "Your inventory is full, please clear some space and try again", "", ""};
    }

    auto seller_data = dynamic_cast<FreeMarket&>(plugin_).getMarket().user_get(goods_data.uuid);
    if (!seller_data.status) {
        return {false, "The goods not exists", "", ""};
    }

    TradeResult result;
    if (goods_data.money_type == "money") {
        result = purchaseWithMoney(buyer, goods_data, seller_data);
    } else {
        result = purchaseWithItems(buyer, goods_data, seller_data);
    }

    if (result.success) {
        // Record the transaction
        auto [fst, snd] = MarketCore::GoodsDataToString(goods_data);
        (void)dynamic_cast<FreeMarket&>(plugin_).getMarket().record_add(seller_data.uuid, buyer.getUniqueId().str(), snd);

        // Notify seller
        if (auto seller_player = plugin_.getServer().getPlayer(seller_data.playername)) {
            seller_player->sendMessage(
                translator_.tr("Your goods have been purchased. Transaction details: ") + "\n" +
                translator_.tr("Buyer: ") + buyer.getName() + "\n" +
                translator_.tr("Goods info: ") + goods_data.name);
        }

        // Broadcast
        plugin_.getServer().broadcastMessage("§l§2" +
            translator_.tr("[Marketing Promotion] This good has been purchased: ") + "§r" +
            goods_data.name);
    }

    return result;
}

TradeResult TradeEngine::purchaseWithMoney(const endstone::Player& buyer,
                                            const Market_Action::Goods_data& goods_data,
                                            const Market_Action::User_data& seller_data) const
{
    if (const int buyer_money = market_core_.get_player_money(buyer); buyer_money < goods_data.price) {
        return {false, "You have not enough money", "", ""};
    }

    // Deduct from buyer, credit seller
    (void)market_core_.general_change_money(buyer.getUniqueId().str(), buyer.getName(), -goods_data.price);
    (void)market_core_.general_change_money(seller_data.uuid, seller_data.playername, goods_data.price);

    // Give item to buyer
    const auto deserialized = ItemSerializer::deserialize(
        goods_data.item + "," + goods_data.data);
    auto itemStack = ItemSerializer::toItemStack(deserialized,
        plugin_.getServer().getItemFactory());
    buyer.getInventory().addItem(itemStack);

    // Remove goods listing
    (void)dynamic_cast<FreeMarket&>(plugin_).getMarket().goods_del(goods_data.gid);

    return {true, "Successful purchase",
            "[Marketing Promotion] This good has been purchased: ", goods_data.name};
}

TradeResult TradeEngine::purchaseWithItems(const endstone::Player& buyer,
                                            const Market_Action::Goods_data& goods_data,
                                            const Market_Action::User_data& seller_data) const
{
    // Take payment items from buyer's inventory
    auto taken_items = takeItemsFromInventory(buyer, goods_data.money_type, goods_data.price);
    if (taken_items.empty()) {
        return {false, "You have not enough money", "", ""};
    }

    // Give purchased item to buyer
    const auto deserialized = ItemSerializer::deserialize(
        goods_data.item + "," + goods_data.data);
    auto itemStack = ItemSerializer::toItemStack(deserialized,
        plugin_.getServer().getItemFactory());
    buyer.getInventory().addItem(itemStack);

    // Store taken items as seller's withdrawable payments
    for (const auto& one_item : taken_items) {
        std::string string_itemData = ItemSerializer::serialize(one_item);
        (void)dynamic_cast<FreeMarket&>(plugin_).getMarket().user_add_item(seller_data.uuid, string_itemData);
    }

    // Remove goods listing
    (void)dynamic_cast<FreeMarket&>(plugin_).getMarket().goods_del(goods_data.gid);

    return {true, "Successful purchase",
            "[Marketing Promotion] This good has been purchased: ", goods_data.name};
}
