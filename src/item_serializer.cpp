#include "item_serializer.hpp"
#include "string_utils.hpp"
#include "nbt_tool.hpp"

std::string ItemSerializer::serialize(const ItemStackData& data) {
    std::string result = data.item_id + "," + std::to_string(data.item_num);

    if (data.item_meta.has_value()) {
        const auto& meta = data.item_meta.value();
        std::string lore = meta.lore.empty()
            ? "None"
            : "{" + string_utils::join(meta.lore) + "}";
        std::string damage = std::to_string(meta.damage);
        std::string display_name = meta.display_name.empty() ? "None" : meta.display_name;
        std::string enchants = meta.enchants.empty()
            ? "None"
            : "{" + string_utils::format_enchants(meta.enchants) + "}";
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
        std::vector<std::string>(tokens.begin() + 2, tokens.end()));

    // Try as pure NBT first (starts with { or [)
    if (!remainder.empty() && (remainder.front() == '{' || remainder.front() == '[')) {
        auto nbt = NBTTools::stringToNbt(remainder);
        if (!nbt.empty()) {
            data.nbt = nbt;
            return data;
        }
        return {}; // NBT exists but parse failed -> corrupted
    }

    // Legacy format: first 4 tokens = meta, rest = NBT
    if (tokens.size() - 2 >= 4) {
        std::vector<std::string> meta_tokens(tokens.begin() + 2, tokens.begin() + 6);
        std::string nbt_str = string_utils::join(
            std::vector<std::string>(tokens.begin() + 6, tokens.end()));

        ItemMeta item_meta;
        auto meta_str = string_utils::join(meta_tokens);
        if (!meta_str.empty()) {
            auto meta_parts = string_utils::split(meta_str);
            if (!meta_parts.empty()) {
                std::string lore_str = meta_parts[0];
                if (lore_str != "None") {
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
                std::string ench_str = meta_parts[3];
                if (ench_str != "None") {
                    if (ench_str.front() == '{' && ench_str.back() == '}') {
                        ench_str = ench_str.substr(1, ench_str.size() - 2);
                    }
                    item_meta.enchants = string_utils::parse_enchants(ench_str);
                }
            }
            data.item_meta = item_meta;
        }

        if (!nbt_str.empty()) {
            auto nbt = NBTTools::stringToNbt(nbt_str);
            if (!nbt.empty()) {
                data.nbt = nbt;
            } else {
                return {};
            }
        }
    } else {
        // Less than 4 fields, treat all as meta
        ItemMeta item_meta;
        auto meta_str = string_utils::join(
            std::vector<std::string>(tokens.begin() + 2, tokens.end()));
        if (!meta_str.empty()) {
            auto meta_parts = string_utils::split(meta_str);
            if (!meta_parts.empty()) {
                std::string lore_str = meta_parts[0];
                if (lore_str != "None") {
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
                std::string ench_str = meta_parts[3];
                if (ench_str != "None") {
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
        std::vector<std::string>(tokens.begin() + 2, tokens.end()));
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

    auto nbt = item.getNbt();
    if (!nbt.empty()) {
        data.nbt = nbt;
    }

    return data;
}

endstone::ItemStack ItemSerializer::toItemStack(const ItemStackData& data,
                                                  endstone::ItemFactory& item_factory) {
    endstone::ItemStack item(data.item_id, data.item_num);

    if (data.nbt.has_value() && !data.nbt->empty()) {
        item.setNbt(data.nbt.value().get<endstone::CompoundTag>());
    } else if (data.item_meta.has_value()) {
        const auto& item_meta = data.item_meta.value();
        auto meta_ptr = item_factory.getItemMeta(item.getType());
        endstone::ItemMeta* meta = meta_ptr.get();

        if (!item_meta.lore.empty()) {
            meta->setLore(item_meta.lore);
        }
        meta->setDamage(item_meta.damage);
        if (!item_meta.display_name.empty()) {
            meta->setDisplayName(item_meta.display_name);
        }
        if (!item_meta.enchants.empty()) {
            for (const auto& [ench_id, level] : item_meta.enchants) {
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
        const auto& meta = data.item_meta.value();
        result += "Damage: " + std::to_string(meta.damage) + "\n";
        if (!meta.display_name.empty()) {
            result += "Display Name: " + meta.display_name + "\n";
        }
        if (!meta.lore.empty()) {
            result += "Lore: ";
            for (const auto& line : meta.lore) {
                result += line + " ";
            }
            result += "\n";
        }
        if (!meta.enchants.empty()) {
            result += "Enchants: ";
            for (const auto& [id, lvl] : meta.enchants) {
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
