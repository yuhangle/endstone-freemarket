#pragma once
#include "item_data.hpp"
#include <string>
#include <vector>
#include <endstone/endstone.hpp>

class ItemSerializer {
public:
    // Serialize a single ItemStackData to string for DB storage
    static std::string serialize(const ItemStackData& data);

    // Deserialize a single ItemStackData from DB string
    static ItemStackData deserialize(const std::string& str);

    // Serialize ItemStackData and split into (goods_item, goods_data) for GOODS table
    // goods_item = "id,count", goods_data = rest (meta + nbt)
    static std::pair<std::string, std::string> toGoodsParts(const ItemStackData& data);

    // Extract ItemStackData from an in-game ItemStack
    static ItemStackData fromItemStack(const endstone::ItemStack& item);

    // Convert to in-game ItemStack (handles NBT/Meta exclusively)
    static endstone::ItemStack toItemStack(const ItemStackData& data,
                                           const endstone::ItemFactory& item_factory);

    // Build a human-readable display string from ItemStackData
    static std::string buildMetaDisplay(const ItemStackData& data);

    // Check if data represents a valid item
    static bool isValid(const ItemStackData& data) {
        return !data.item_id.empty() && data.item_num > 0;
    }

    // Convert Enchantment* map to string map
    static std::unordered_map<std::string, int>
    enchantToSimMap(const std::unordered_map<const endstone::Enchantment*, int>& enchants);
};
