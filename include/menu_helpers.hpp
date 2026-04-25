#pragma once
#include "item_data.hpp"
#include "translate.h"
#include <string>
#include <vector>
#include <endstone/endstone.hpp>

// ========== Item interaction helpers ==========

// Build a localized display string from ItemStackData for in-game menus
std::string buildItemMetaDisplay(const ItemStackData& data, translate& translator);

// Restore item to player's inventory, returns true if successful
bool giveItemToPlayer(endstone::Player& player, const ItemStackData& data,
                      endstone::ItemFactory& item_factory);

// Read player's hotbar (slots 0-8) as ItemStackData list (empty slots = invalid items)
std::vector<ItemStackData> readPlayerHotbar(const endstone::Player& player);

// Check if player's main inventory (slots 0-35) is completely full
bool isPlayerInventoryFull(const endstone::Player& player);

// Check if player has at least `count` of `item_id` in slots 0-35
bool playerHasItemCount(const endstone::Player& player, const std::string& item_id, int count);

// Take `count` of `item_id` from player inventory, return extracted ItemStackData list
std::vector<ItemStackData> takeItemsFromInventory(endstone::Player& player,
                                                   const std::string& item_id, int count);

// ========== Menu presets ==========

struct AvatarPreset {
    std::string name;
    std::string texture;
};
const std::vector<AvatarPreset>& getAvatarPresets();

struct CategoryPreset {
    std::string name;
    std::string texture;
    std::string tag;
};
const std::vector<CategoryPreset>& getCategoryImagePresets();

struct MoneyTypePreset {
    std::string name;
    std::string value;
};
const std::vector<MoneyTypePreset>& getMoneyTypePresets();

struct TagPreset {
    std::string name;
    std::string value;
};
const std::vector<TagPreset>& getTagPresets();
