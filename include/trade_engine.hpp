#pragma once
#include "item_data.hpp"
#include "market_action.h"
#include <string>
#include <endstone/endstone.hpp>

class MarketCore;
class translate;

struct TradeResult {
    bool success = false;
    std::string message_key;      // i18n key for status message
    std::string broadcast_key;    // i18n key for broadcast
    std::string broadcast_arg;    // goods name for broadcast
};

class TradeEngine {
public:
    TradeEngine(endstone::Plugin& plugin, MarketCore& market_core, translate& translator)
        : plugin_(plugin), market_core_(market_core), translator_(translator) {}

    // Execute a purchase: buyer buys goods_data from the market
    // Handles both money and item-exchange transactions
    TradeResult executePurchase(endstone::Player& buyer,
                                const Market_Action::Goods_data& goods_data);

private:
    // Money transaction path
    TradeResult purchaseWithMoney(endstone::Player& buyer,
                                   const Market_Action::Goods_data& goods_data,
                                   const Market_Action::User_data& seller_data);

    // Item-exchange transaction path
    TradeResult purchaseWithItems(endstone::Player& buyer,
                                   const Market_Action::Goods_data& goods_data,
                                   const Market_Action::User_data& seller_data);

    endstone::Plugin& plugin_;
    MarketCore& market_core_;
    translate& translator_;
};
