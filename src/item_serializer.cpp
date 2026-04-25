#include "item_serializer.hpp"
#include "string_utils.hpp"
#include "nbt_tool.hpp"

std::string ItemSerializer::serialize(const ItemStackData& data) {
    std::string result = data.item_id + "," + std::to_string(data.item_num);

    if (data.item_meta.has_value()) {
        const auto& [lore_, damage_, display_name_, enchants_] = data.item_meta.value();
        const std::string lore = lore_.empty()
            ? "None"
            : "{" + string_utils::join(lore_) + "}";
        const std::string damage = std::to_string(damage_);
        const std::string display_name = display_name_.empty() ? "None" : display_name_;
        const std::string enchants = enchants_.empty()
            ? "None"
            : "{" + string_utils::format_enchants(enchants_) + "}";
        result += "," + lore + "," + damage + "," + display_name + "," + enchants;
    }

    if (data.nbt.has_value() && !data.nbt->empty()) {
        result += "," + NBTTools::nbtToJson(data.nbt.value()).dump(-1, ' ', false);
    }

    return result;
}

ItemStackData ItemSerializer::deserialize(const std::string& str) {
    ItemStackData data;
    auto tokens = string_utils::split(str);
    if (tokens.size() < 2) return data;

    data.item_id = tokens[0];
    data.item_num = string_utils::to_int(tokens[1]);

    if (tokens.size() == 2) return data;

    // Remaining tokens (meta + optional nbt)
    std::string remainder = string_utils::join(
        std::vector(tokens.begin() + 2, tokens.end()));

    // Try as pure NBT first (starts with { or [)
    if (!remainder.empty() && (remainder.front() == '{' || remainder.front() == '[')) {
        if (auto nbt = NBTTools::stringToNbt(remainder); !nbt.empty()) {
            data.nbt = nbt;
            return data;
        }
        return {}; // NBT exists but parse failed -> corrupted
    }

    // Legacy format: first 4 tokens = meta, rest = NBT
    if (tokens.size() - 2 >= 4) {
        std::vector meta_tokens(tokens.begin() + 2, tokens.begin() + 6);
        std::string nbt_str = string_utils::join(
            std::vector(tokens.begin() + 6, tokens.end()));

        if (auto meta_str = string_utils::join(meta_tokens); !meta_str.empty()) {
            ItemMeta item_meta;
            auto meta_parts = string_utils::split(meta_str);
            if (!meta_parts.empty()) {
                if (std::string lore_str = meta_parts[0]; lore_str != "None") {
                    if (lore_str.front() == '{' && lore_str.back() == '}') {
                        lore_str = lore_str.substr(1, lore_str.size() - 2);
                    }
                    item_meta.lore = string_utils::split(lore_str);
                }
            }
            if (meta_parts.size() > 1) {
                item_meta.damage = string_utils::to_int(meta_parts[1]);
            }
            if (meta_parts.size() > 2 && meta_parts[2] != "None") {
                item_meta.display_name = meta_parts[2];
            }
            if (meta_parts.size() > 3) {
                if (std::string ench_str = meta_parts[3]; ench_str != "None") {
                    if (ench_str.front() == '{' && ench_str.back() == '}') {
                        ench_str = ench_str.substr(1, ench_str.size() - 2);
                    }
                    item_meta.enchants = string_utils::parse_enchants(ench_str);
                }
            }
            data.item_meta = item_meta;
        }

        if (!nbt_str.empty()) {
            if (auto nbt = NBTTools::stringToNbt(nbt_str); !nbt.empty()) {
                data.nbt = nbt;
            } else {
                return {};
            }
        }
    } else {
        // Less than 4 fields, treat all as meta
        auto meta_str = string_utils::join(
            std::vector<std::string>(tokens.begin() + 2, tokens.end()));
        if (!meta_str.empty()) {
            ItemMeta item_meta;
            auto meta_parts = string_utils::split(meta_str);
            if (!meta_parts.empty()) {
                if (std::string lore_str = meta_parts[0]; lore_str != "None") {
                    if (lore_str.front() == '{' && lore_str.back() == '}') {
                        lore_str = lore_str.substr(1, lore_str.size() - 2);
                    }
                    item_meta.lore = string_utils::split(lore_str);
                }
            }
            if (meta_parts.size() > 1) {
                item_meta.damage = string_utils::to_int(meta_parts[1]);
            }
            if (meta_parts.size() > 2 && meta_parts[2] != "None") {
                item_meta.display_name = meta_parts[2];
            }
            if (meta_parts.size() > 3) {
                if (std::string ench_str = meta_parts[3]; ench_str != "None") {
                    if (ench_str.front() == '{' && ench_str.back() == '}') {
                        ench_str = ench_str.substr(1, ench_str.size() - 2);
                    }
                    item_meta.enchants = string_utils::parse_enchants(ench_str);
                }
            }
            data.item_meta = item_meta;
        }
    }

    return data;
}

std::pair<std::string, std::string> ItemSerializer::toGoodsParts(const ItemStackData& data) {
    std::string serialized = serialize(data);
    // Format: "id,count,lore,damage,name,enchants[,nbt]"
    // Need to split at the second comma to get (item_part, data_part)
    auto tokens = string_utils::split(serialized);
    if (tokens.size() < 2) {
        return {serialized, ""};
    }
    std::string item_part = tokens[0] + "," + tokens[1];
    // Remaining tokens are the data part
    if (tokens.size() == 2) {
        return {item_part, ""};
    }
    std::string data_part = string_utils::join(
        std::vector(tokens.begin() + 2, tokens.end()));
    return {item_part, data_part};
}

ItemStackData ItemSerializer::fromItemStack(const endstone::ItemStack& item) {
    ItemStackData data;
    data.item_id = std::string(item.getType().getId());
    data.item_num = item.getAmount();

    if (item.hasItemMeta()) {
        auto meta = item.getItemMeta();
        ItemMeta item_meta;
        item_meta.lore = meta->getLore();
        item_meta.damage = meta->getDamage();
        item_meta.display_name = meta->getDisplayName();

        if (meta->hasEnchants()) {
            item_meta.enchants = enchantToSimMap(meta->getEnchants());
        }

        data.item_meta = item_meta;
    }

    if (auto nbt = item.getNbt(); !nbt.empty()) {
        data.nbt = nbt;
    }

    return data;
}

endstone::ItemStack ItemSerializer::toItemStack(const ItemStackData& data,
                                                  const endstone::ItemFactory& item_factory) {
    endstone::ItemStack item(data.item_id, data.item_num);

    if (data.nbt.has_value() && !data.nbt->empty()) {
        item.setNbt(data.nbt.value().get<endstone::CompoundTag>());
    } else if (data.item_meta.has_value()) {
        const auto& [lore_, damage_, display_name_, enchants_] = data.item_meta.value();
        const auto meta_ptr = item_factory.getItemMeta(item.getType());
        endstone::ItemMeta* meta = meta_ptr.get();

        if (!lore_.empty()) {
            meta->setLore(lore_);
        }
        meta->setDamage(damage_);
        if (!display_name_.empty()) {
            meta->setDisplayName(display_name_);
        }
        if (!enchants_.empty()) {
            for (const auto& [ench_id, level] : enchants_) {
                (void)meta->addEnchant(endstone::EnchantmentId(ench_id), level, false);
            }
        }

        item.setItemMeta(meta);
    }

    return item;
}

std::string ItemSerializer::buildMetaDisplay(const ItemStackData& data) {
    std::string result;
    result += "Item ID: " + data.item_id + "\n";
    result += "Count: " + std::to_string(data.item_num) + "\n";

    if (data.item_meta.has_value()) {
        const auto& [lore, damage, display_name, enchants] = data.item_meta.value();
        result += "Damage: " + std::to_string(damage) + "\n";
        if (!display_name.empty()) {
            result += "Display Name: " + display_name + "\n";
        }
        if (!lore.empty()) {
            result += "Lore: ";
            for (const auto& line : lore) {
                result += line + " ";
            }
            result += "\n";
        }
        if (!enchants.empty()) {
            result += "Enchants: ";
            for (const auto& [id, lvl] : enchants) {
                result += id + ":" + std::to_string(lvl) + " ";
            }
            result += "\n";
        }
    }

    if (data.nbt.has_value() && !data.nbt->empty()) {
        result += "(Contains NBT data)";
    }

    return result;
}

std::unordered_map<std::string, int>
ItemSerializer::enchantToSimMap(const std::unordered_map<const endstone::Enchantment*, int>& enchants) {
    std::unordered_map<std::string, int> result;
    for (const auto& [enchantment, level] : enchants) {
        result[std::string(enchantment->getId())] = level;
    }
    return result;
}
