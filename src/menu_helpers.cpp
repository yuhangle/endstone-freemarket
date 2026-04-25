#include "menu_helpers.hpp"
#include "item_serializer.hpp"

std::string buildItemMetaDisplay(const ItemStackData& data, translate& translator) {
    std::string result;

    if (data.item_meta.has_value()) {
        const auto& [lore, damage, display_name, enchants] = data.item_meta.value();
        result += translator.tr("Item damage: ") + std::to_string(damage);
        if (!display_name.empty()) {
            result += "\n" + translator.tr("Name tag: ") + display_name;
        }
        result += "§r";
        if (!lore.empty()) {
            result += "\n lore: ";
            for (const auto& one_lore : lore) {
                result += one_lore + ", ";
            }
        }
        if (!enchants.empty()) {
            result += "\n" + translator.tr("Enchant: ");
            for (const auto& [fst, snd] : enchants) {
                result += fst + ":" + std::to_string(snd) + ", ";
            }
        }
        if (data.nbt.has_value() && !data.nbt->empty()) {
            result += "\n§7(+ Contains container data)§r";
        }
    } else if (data.nbt.has_value() && !data.nbt->empty()) {
        result = translator.tr("(Contains custom NBT data)");
    } else {
        result = translator.tr("No additional data");
    }

    return result;
}

bool giveItemToPlayer(endstone::Player& player, const ItemStackData& data,
                      endstone::ItemFactory& item_factory) {
    auto itemStack = ItemSerializer::toItemStack(data, item_factory);
    player.getInventory().addItem(itemStack);
    return true;
}

std::vector<ItemStackData> readPlayerHotbar(const endstone::Player& player) {
    std::vector<ItemStackData> result;
    result.reserve(9);
    for (int i = 0; i <= 8; i++) {
        if (const auto one_item = player.getInventory().getItem(i)) {
            result.push_back(ItemSerializer::fromItemStack(one_item.value()));
        } else {
            result.push_back({"None", 0, std::nullopt, std::nullopt});
        }
    }
    return result;
}

bool isPlayerInventoryFull(const endstone::Player& player) {
    for (int i = 0; i <= 35; i++) {
        if (!player.getInventory().getItem(i)) {
            return false;
        }
    }
    return true;
}

bool playerHasItemCount(const endstone::Player& player, const std::string& item_id, int count) {
    int total = 0;
    for (int i = 0; i <= 35; i++) {
        if (auto item = player.getInventory().getItem(i)) {
            if (item->getType().getId() == item_id) {
                total += item->getAmount();
                if (total >= count) return true;
            }
        }
    }
    return false;
}

std::vector<ItemStackData> takeItemsFromInventory(endstone::Player& player,
                                                   const std::string& item_id, int count) {
    std::vector<ItemStackData> result;
    int total_needed = count;

    for (int i = 0; i <= 35; i++) {
        if (total_needed <= 0) break;
        if (auto the_item = player.getInventory().getItem(i)) {
            if (the_item->getType().getId() != item_id) continue;

            int current_amount = the_item->getAmount();
            auto data = ItemSerializer::fromItemStack(the_item.value());
            data.item_num = std::min(current_amount, total_needed);
            result.push_back(data);

            if (current_amount <= total_needed) {
                player.getInventory().setItem(i, std::nullopt);
                total_needed -= current_amount;
            } else {
                int remaining = current_amount - total_needed;
                endstone::ItemStack new_item(the_item->getType(), remaining);
                if (the_item->hasItemMeta()) {
                    auto meta = the_item->getItemMeta();
                    auto new_meta = meta->clone();
                    new_item.setItemMeta(new_meta.get());
                }
                if (auto nbt = the_item->getNbt(); !nbt.empty()) {
                    new_item.setNbt(nbt);
                }
                player.getInventory().setItem(i, new_item);
                return result;
            }
        }
    }
    return result;
}

// ========== Presets ==========

const std::vector<AvatarPreset>& getAvatarPresets() {
    static const std::vector<AvatarPreset> presets = {
        {"Steve", "textures/ui/icon_steve"},
        {"Alex", "textures/ui/icon_alex"},
        {"Minions", "textures/ui/sidebar_icons/Minions_packicon_0"},
        {"Panda", "textures/ui/icon_panda"},
    };
    return presets;
}

const std::vector<CategoryPreset>& getCategoryImagePresets() {
    static const std::vector<CategoryPreset> presets = {
        {"Apple", "textures/ui/icon_apple", ""},
        {"Tools & Equipment", "textures/ui/icon_recipe_equipment", ""},
        {"Diamond", "textures/items/diamond", ""},
        {"Grass block", "textures/blocks/grass_side_carried", ""},
    };
    return presets;
}

const std::vector<MoneyTypePreset>& getMoneyTypePresets() {
    static const std::vector<MoneyTypePreset> presets = {
        {"money", "money"},
        {"minecraft:diamond", "minecraft:diamond"},
        {"minecraft:emerald", "minecraft:emerald"},
        {"minecraft:gold_ingot", "minecraft:gold_ingot"},
        {"minecraft:iron_ingot", "minecraft:iron_ingot"},
        {"Custom", "custom"},
    };
    return presets;
}

const std::vector<TagPreset>& getTagPresets() {
    static const std::vector<TagPreset> presets = {
        {"Food", "Food"},
        {"Tools & Equipment", "Tools & Equipment"},
        {"Ore", "Ore"},
        {"Block", "Block"},
        {"Other", "Other"},
    };
    return presets;
}
