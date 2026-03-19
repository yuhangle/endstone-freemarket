//
// Created by yuhang on 2026/3/9.
//

#ifndef DISABLE_SHUA_NBT_TOOL_H
#define DISABLE_SHUA_NBT_TOOL_H

#include <endstone/endstone.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <map>
#include <vector>

using namespace nlohmann;

class NBTTools
{
public:
    static constexpr int TOOL_VERSION = 3;

    // ================= 1. 调试与打印 =================

    static void dumpTag(const endstone::nbt::Tag& tag, const int indent = 0) {
        const std::string pad(indent * 2, ' ');
        switch (tag.type()) {
            case endstone::nbt::Type::Byte:
                std::cout << pad << "(Byte) " << static_cast<int>(tag.get<endstone::ByteTag>().value()) << "\n"; break;
            case endstone::nbt::Type::Short:
                std::cout << pad << "(Short) " << tag.get<endstone::ShortTag>().value() << "\n"; break;
            case endstone::nbt::Type::Int:
                std::cout << pad << "(Int) " << tag.get<endstone::IntTag>().value() << "\n"; break;
            case endstone::nbt::Type::Long:
                std::cout << pad << "(Long) " << tag.get<endstone::LongTag>().value() << "\n"; break;
            case endstone::nbt::Type::Float:
                std::cout << pad << "(Float) " << tag.get<endstone::FloatTag>().value() << "\n"; break;
            case endstone::nbt::Type::Double:
                std::cout << pad << "(Double) " << tag.get<endstone::DoubleTag>().value() << "\n"; break;
            case endstone::nbt::Type::String:
                std::cout << pad << "(String) \"" << tag.get<endstone::StringTag>().value() << "\"\n"; break;
            case endstone::nbt::Type::ByteArray: {
                const auto& arr = tag.get<endstone::ByteArrayTag>();
                std::cout << pad << "(ByteArray) [size=" << arr.size() << "]\n"; break;
            }
            case endstone::nbt::Type::IntArray: {
                const auto& arr = tag.get<endstone::IntArrayTag>();
                std::cout << pad << "(IntArray) [size=" << arr.size() << "]\n"; break;
            }
            case endstone::nbt::Type::List: {
                const auto& list = tag.get<endstone::ListTag>();
                std::cout << pad << "(List) [size=" << list.size() << "]\n";
                for (size_t i = 0; i < list.size(); ++i) {
                    std::cout << pad << "  [" << i << "]:\n";
                    dumpTag(list[i], indent + 2);
                }
                break;
            }
            case endstone::nbt::Type::Compound: {
                const auto& comp = tag.get<endstone::CompoundTag>();
                std::cout << pad << "(Compound) {\n";
                for (const auto& [key, value] : comp) {
                    std::cout << pad << "  \"" << key << "\":\n";
                    dumpTag(value, indent + 2);
                }
                std::cout << pad << "}\n"; break;
            }
            default:
                std::cout << pad << "(Unknown Type: " << static_cast<int>(tag.type()) << ")\n";
        }
    }

    // ================= 2. 核心转换 (NBT <-> JSON) =================

    static json nbtToJson(const endstone::nbt::Tag& tag) {
        switch (tag.type()) {
        case endstone::nbt::Type::Byte: return tag.get<endstone::ByteTag>().value();
        case endstone::nbt::Type::Short: return tag.get<endstone::ShortTag>().value();
        case endstone::nbt::Type::Int: return tag.get<endstone::IntTag>().value();
        case endstone::nbt::Type::Long: return tag.get<endstone::LongTag>().value();
        case endstone::nbt::Type::Float: return tag.get<endstone::FloatTag>().value();
        case endstone::nbt::Type::Double: return tag.get<endstone::DoubleTag>().value();
        case endstone::nbt::Type::String: return tag.get<endstone::StringTag>().value();
        case endstone::nbt::Type::ByteArray: {
            const auto& arr = tag.get<endstone::ByteArrayTag>();
            json j_arr = json::array();
            for (const auto v : arr) j_arr.push_back(static_cast<int>(v));
            return j_arr;
        }
        case endstone::nbt::Type::IntArray: {
            const auto& arr = tag.get<endstone::IntArrayTag>();
            json j_arr = json::array();
            for (auto v : arr) j_arr.push_back(v);
            return j_arr;
        }
        case endstone::nbt::Type::List: {
            const auto& list = tag.get<endstone::ListTag>();
            json j_list = json::array();
            for (const auto & i : list) j_list.push_back(nbtToJson(i));
            return j_list;
        }
        case endstone::nbt::Type::Compound: {
            const auto& comp = tag.get<endstone::CompoundTag>();
            std::map<std::string, endstone::nbt::Tag> sortedMap;
            for (const auto& [key, value] : comp) sortedMap[key] = value;
            json j_obj = json::object();
            for (const auto& [key, value] : sortedMap) j_obj[key] = nbtToJson(value);
            return j_obj;
        }
        default: return nullptr;
        }
    }

    /**
     * @brief JSON 转 NBT (已修复 Damage 类型问题)
     *
     * 严格规则：
     * 1. Damage, RepairCost, CustomModelData -> 强制 IntTag
     * 2. ench 列表内的 id, lvl -> 强制 ShortTag
     * 3. Count, Slot -> 优先 ByteTag (若范围允许)
     * 4. 其他整数 -> 默认 IntTag (若范围允许)
     */
    static endstone::nbt::Tag jsonToNbt(const json& j, const std::string& parentKey = "", bool inEnchList = false) {
        if (j.is_null()) return {};

        if (j.is_boolean()) {
            return {endstone::ByteTag(j.get<bool>() ? 1 : 0)};
        }

        if (j.is_number()) {
            if (j.is_number_integer()) {
                int64_t val = j.get<int64_t>();

                // --- 【特例组 A】：附魔列表内部 (必须是 Short) ---
                if (inEnchList && (parentKey == "id" || parentKey == "lvl")) {
                    if (val >= -32768 && val <= 32767) {
                        return {endstone::ShortTag(static_cast<int16_t>(val))};
                    }
                    // 超出 Short 范围则降级为 Int，防止溢出
                    return {endstone::IntTag(static_cast<int32_t>(val))};
                }

                // --- 【特例组 B】：必须强制为 Int 的物品属性 (修复核心问题) ---
                // 即使值为 0，也必须是 IntTag，不能是 ShortTag 或 ByteTag
                if (parentKey == "Damage" ||
                    parentKey == "RepairCost" ||
                    parentKey == "CustomModelData" ||
                    parentKey == "Unbreakable") { // Unbreakable 通常是 Int(1) 或 Byte(1)，但 Int 更安全
                    return {endstone::IntTag(static_cast<int32_t>(val))};
                }

                // --- 【特例组 C】：适合用小类型的字段 ---
                if (parentKey == "Count" && val >= -128 && val <= 127) {
                    return {endstone::ByteTag(static_cast<int8_t>(val))};
                }
                if (parentKey == "Slot" && val >= -128 && val <= 127) {
                    return {endstone::ByteTag(static_cast<int8_t>(val))};
                }
                if (parentKey == "WasPickedUp" && (val == 0 || val == 1)) {
                    return {endstone::ByteTag(static_cast<int8_t>(val))};
                }

                // --- 【默认规则】：普通整数 ---
                if (val >= -2147483648LL && val <= 2147483647LL) {
                    return {endstone::IntTag(static_cast<int32_t>(val))};
                } else {
                    return {endstone::LongTag(val)};
                }
            }
            // 浮点数
            return {endstone::DoubleTag(j.get<double>())};
        }

        if (j.is_string()) {
            return {endstone::StringTag(j.get<std::string>())};
        }

        // --- 数组处理 ---
        if (j.is_array()) {
            // 尝试构建 ListTag
            // 注意：这里依赖 endstone::ListTag 能够接受 push_back 或 emplace_back
            // 如果之前编译报错，请确保 endstone 版本支持此操作，或使用之前的 dump/load 方案
            endstone::ListTag list;
            for (const auto& item : j) {
                // 递归调用，保持上下文
                list.emplace_back(jsonToNbt(item, "", inEnchList));
            }
            return {list};
        }

        // --- 对象处理 ---
        if (j.is_object()) {
            endstone::CompoundTag comp;
            for (const auto& [key, value] : j.items()) {
                // 检测是否进入 ench 列表
                bool nextInEnchList = inEnchList;
                if ((key == "ench" || key == "StoredEnchantments") && value.is_array()) {
                    nextInEnchList = true;
                }

                comp.insert_or_assign(key, jsonToNbt(value, key, nextInEnchList));
            }
            return {comp};
        }

        return {};
    }

    // ================= 3. 字符串化工具 =================

    static std::string jsonToCompactString(const json& j) {
        if (j.is_null()) return "";
        return j.dump(-1, ' ', false);
    }

    static std::string jsonToPrettyString(const json& j) {
        if (j.is_null()) return "";
        return j.dump(2);
    }

    static json stringToJson(const std::string& jsonString) {
        try {
            return json::parse(jsonString);
        } catch (const json::parse_error& e) {
            std::cerr << "[NBTTools] JSON Parse Error: " << e.what() << std::endl;
            return nullptr;
        }
    }

    static endstone::nbt::Tag stringToNbt(const std::string& jsonString) {
        const json j = stringToJson(jsonString);
        if (j.is_null()) return {};
        return jsonToNbt(j);
    }

    // ================= 4. 业务逻辑工具 =================

    static std::pair<bool, std::vector<int>> findItem(const endstone::Inventory &inventory,
                                                       const std::string &itemID,
                                                       const bool key_search = false) {
        if (inventory.isEmpty()) return {false, {}};
        std::vector<int> found_index;
        found_index.reserve(inventory.getSize());
        for (int i = 0; i < inventory.getSize(); i++) {
            auto the_one = inventory.getItem(i);
            if (!the_one.has_value()) continue;
            const auto& item = the_one.value();
            bool isMatch = key_search ?
                (item.getType().getId().getKey().find(itemID) != std::string::npos) :
                (item.getType().getId() == itemID);
            if (isMatch) found_index.push_back(i);
        }
        return {!found_index.empty(), found_index};
    }

    static bool hasEnchantedItems(const endstone::ItemStack& shulkerBox) {
        const auto nbtTag = shulkerBox.getNbt();
        json j = nbtToJson(nbtTag);
        if (j.is_null() || !j.contains("Items") || !j["Items"].is_array()) return false;

        return std::ranges::any_of(j["Items"], [](const auto& item) {
            if (!item.is_object() || !item.contains("tag") || !item["tag"].is_object()) return false;
            const auto& tag = item["tag"];
            return (tag.contains("ench") && tag["ench"].is_array() && !tag["ench"].empty()) ||
                   (tag.contains("StoredEnchantments") && tag["StoredEnchantments"].is_array() && !tag["StoredEnchantments"].empty());
        });
    }

    // ================= 5. 自动修复工具 (新增) =================

    /**
     * @brief 检查并修复物品 NBT 中的类型错误
     * 专门用于修复被错误逻辑污染的物品 (如 Damage 变为 Short)
     * @param item 需要修复的物品引用
     * @return 如果进行了修复返回 true，否则返回 false
     */
    static bool fixItemNbtTypes(endstone::ItemStack& item) {


        auto comp =  item.getNbt();
        bool modified = false;

        // 修复 Damage
        if (comp.contains("Damage")) {
            if (auto& tag = comp.at("Damage"); tag.type() != endstone::nbt::Type::Int) {
                int32_t val = 0;
                // 提取当前值
                if (tag.type() == endstone::nbt::Type::Short) val = tag.get<endstone::ShortTag>().value();
                else if (tag.type() == endstone::nbt::Type::Byte) val = tag.get<endstone::ByteTag>().value();
                else if (tag.type() == endstone::nbt::Type::Int) val = tag.get<endstone::IntTag>().value();
                else if (tag.type() == endstone::nbt::Type::Long) val = static_cast<int32_t>(tag.get<endstone::LongTag>().value());

                comp.erase("Damage");
                comp.insert_or_assign("Damage", endstone::IntTag(val));
                modified = true;
            }
        }

        // 修复 RepairCost
        if (comp.contains("RepairCost")) {
            if (auto& tag = comp.at("RepairCost"); tag.type() != endstone::nbt::Type::Int) {
                int32_t val = 0;
                if (tag.type() == endstone::nbt::Type::Short) val = tag.get<endstone::ShortTag>().value();
                else if (tag.type() == endstone::nbt::Type::Byte) val = tag.get<endstone::ByteTag>().value();
                else if (tag.type() == endstone::nbt::Type::Int) val = tag.get<endstone::IntTag>().value();

                comp.erase("RepairCost");
                comp.insert_or_assign("RepairCost", endstone::IntTag(val));
                modified = true;
            }
        }

        if (modified) {
            item.setNbt(comp);
        }

        return modified;
    }
};

#endif // DISABLE_SHUA_NBT_TOOL_H