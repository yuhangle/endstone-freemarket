#pragma once
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <endstone/endstone.hpp>

struct ItemMeta {
    std::vector<std::string> lore;
    int damage = 0;
    std::string display_name;
    std::unordered_map<std::string, int> enchants; // enchant_id -> level
};

struct ItemStackData {
    std::string item_id;
    int item_num = 1;
    std::optional<ItemMeta> item_meta;
    std::optional<endstone::nbt::Tag> nbt;
};
