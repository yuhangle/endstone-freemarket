//
// Created by yuhang on 2026/3/9.
//

#ifndef DISABLE_SHUA_NBT_TOOL_H
#define DISABLE_SHUA_NBT_TOOL_H

#include <endstone/endstone.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <map>

using namespace nlohmann;

class NBTTools
{
public:
    static constexpr int TOOL_VERSION = 2;

    // ================= 1. 调试与打印 =================

    /**
     * @brief 以人类可读格式打印 NBT 到控制台
     */
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
                std::cout << pad << "(ByteArray) [size=" << arr.size() << "]\n";
                break;
            }
            case endstone::nbt::Type::IntArray: {
                const auto& arr = tag.get<endstone::IntArrayTag>();
                std::cout << pad << "(IntArray) [size=" << arr.size() << "]\n";
                break;
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
                // 注意：CompoundTag 迭代器可能无序，打印仅供参考
                for (const auto& [key, value] : comp) {
                    std::cout << pad << "  \"" << key << "\":\n";
                    dumpTag(value, indent + 2);
                }
                std::cout << pad << "}\n";
                break;
            }

            default:
                std::cout << pad << "(Unknown Type: " << static_cast<int>(tag.type()) << ")\n";
        }
    }

    // ================= 2. 核心转换 (NBT <-> JSON) =================

    /**
     * @brief NBT 标签转 JSON 对象 (内存结构)
     * 特点：Compound 键值对已自动排序，确保哈希一致性
     */
    static json nbtToJson(const endstone::nbt::Tag& tag) {
        switch (tag.type()) {
        case endstone::nbt::Type::Byte:
            return tag.get<endstone::ByteTag>().value();
        case endstone::nbt::Type::Short:
            return tag.get<endstone::ShortTag>().value();
        case endstone::nbt::Type::Int:
            return tag.get<endstone::IntTag>().value();
        case endstone::nbt::Type::Long:
            return tag.get<endstone::LongTag>().value();
        case endstone::nbt::Type::Float:
            return tag.get<endstone::FloatTag>().value();
        case endstone::nbt::Type::Double:
            return tag.get<endstone::DoubleTag>().value();
        case endstone::nbt::Type::String:
            return tag.get<endstone::StringTag>().value();
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
                for (const auto & i : list) {
                    j_list.push_back(nbtToJson(i));
                }
                return j_list;
        }
        case endstone::nbt::Type::Compound: {
                const auto& comp = tag.get<endstone::CompoundTag>();
                std::map<std::string, endstone::nbt::Tag> sortedMap;
                for (const auto& [key, value] : comp) {
                    sortedMap[key] = value;
                }
                json j_obj = json::object();
                for (const auto& [key, value] : sortedMap) {
                    j_obj[key] = nbtToJson(value);
                }
                return j_obj;
        }
        default:
            return nullptr;
        }
    }

    /**
     * @brief JSON 对象递归转换为 NBT 标签
     */
    static endstone::nbt::Tag jsonToNbt(const json& j, const std::string& parentKey = "", bool inEnchList = false) {
        if (j.is_null()) return {};

        // 布尔值 -> ByteTag (0/1)
        if (j.is_boolean()) {
            return {endstone::ByteTag(j.get<bool>() ? 1 : 0)};
        }

        // 数字处理
        if (j.is_number()) {
            if (j.is_number_integer()) {
                int64_t val = j.get<int64_t>();

                // 特例1: 在 ench 列表内，且字段名为 "id" 或 "lvl" -> 强制 ShortTag
                if (inEnchList && (parentKey == "id" || parentKey == "lvl")) {
                    if (val >= -32768 && val <= 32767) {
                        return {endstone::ShortTag(static_cast<int16_t>(val))};
                    }
                    // 若超出范围，降级为 Int（但理论上不会发生）
                }

                // 特例2: 常见物品字段按原规则处理
                if (parentKey == "Count" && val >= -128 && val <= 127) {
                    return {endstone::ByteTag(static_cast<int8_t>(val))};
                }
                if (parentKey == "Damage" && val >= -32768 && val <= 32767) {
                    return {endstone::ShortTag(static_cast<int16_t>(val))};
                }
                if (parentKey == "Slot" && val >= -128 && val <= 127) {
                    return {endstone::ByteTag(static_cast<int8_t>(val))};
                }
                if (parentKey == "WasPickedUp" && (val == 0 || val == 1)) {
                    return {endstone::ByteTag(static_cast<int8_t>(val))};
                }

                // 默认整数范围选择
                if (val >= -2147483648LL && val <= 2147483647LL) {
                    return {endstone::IntTag(static_cast<int32_t>(val))};
                } else {
                    return {endstone::LongTag(val)};
                }
            }
            // 浮点数 -> DoubleTag
            return {endstone::DoubleTag(j.get<double>())};
        }

        // 字符串 -> StringTag
        if (j.is_string()) {
            return {endstone::StringTag(j.get<std::string>())};
        }

        // 数组 -> ListTag
        if (j.is_array()) {
            endstone::ListTag list;
            for (const auto& item : j) {
                // 数组元素本身没有 parentKey，但保留 inEnchList 上下文（因为数组可能是 ench 列表）
                list.emplace_back(jsonToNbt(item, "", inEnchList));
            }
            return {list};
        }

        // 对象 -> CompoundTag
        if (j.is_object()) {
            endstone::CompoundTag comp;
            for (const auto& [key, value] : j.items()) {
                const bool nextInEnchList = inEnchList || (key == "ench" && value.is_array());
                comp.insert_or_assign(key, jsonToNbt(value, key, nextInEnchList));
            }
            return {comp};
        }

        return {};
    }

    // ================= 3. 字符串化工具 =================

    /**
     * @brief 将 JSON 对象转换为【纯一行、紧凑】的字符串
     * 用途：计算哈希、数据库存储、网络传输
     * 示例：{"a":1,"b":2}
     */
    static std::string jsonToCompactString(const json& j) {
        if (j.is_null()) return "";
        // dump(-1, ' ', false) -> 无缩进，无多余空格，不强制转义非ASCII
        return j.dump(-1, ' ', false);
    }

    /**
     * @brief 将 JSON 对象转换为【格式化、多行】的字符串
     * 用途：日志打印、调试查看
     * 示例：
     * {
     *   "a": 1,
     *   "b": 2
     * }
     */
    static std::string jsonToPrettyString(const json& j) {
        if (j.is_null()) return "";
        // dump(2) -> 缩进2个空格
        return j.dump(2);
    }

    /**
     * @brief 将【字符串】解析为 JSON 对象
     * @param jsonString 输入的 JSON 格式字符串
     * @return 解析后的 json 对象，如果解析失败返回 null
     */
    static json stringToJson(const std::string& jsonString) {
        try {
            return json::parse(jsonString);
        } catch (const json::parse_error& e) {
            std::cerr << "[NBTTools] JSON Parse Error: " << e.what() << std::endl;
            return nullptr;
        }
    }

    /**
     * @brief 直接将【字符串】转换为 NBT 标签
     * 流程：String -> JSON -> NBT
     */
    static endstone::nbt::Tag stringToNbt(const std::string& jsonString) {
        cout << "在stringToNbt函数中打印" << endl;
        cout << jsonString << endl;
        const json j = stringToJson(jsonString);
        if (j.is_null()) {
            return {}; // 返回空 Tag (EndTag)
        }
        return jsonToNbt(j);
    }

    // ================= 5. 业务逻辑工具 =================

    static std::pair<bool, std::vector<int>> findItem(const endstone::Inventory &inventory,
                                                       const std::string &itemID,
                                                       const bool key_search = false)
    {
        if (inventory.isEmpty()) {
            return {false, {}};
        }

        std::vector<int> found_index;
        found_index.reserve(inventory.getSize());

        for (int i = 0; i < inventory.getSize(); i++) {
            auto the_one = inventory.getItem(i);
            if (!the_one.has_value()) continue;

            const auto& item = the_one.value();
            bool isMatch = false;

            if (key_search) {
                isMatch = (item.getType().getId().getKey().find(itemID) != std::string::npos);
            } else {
                isMatch = (item.getType().getId() == itemID);
            }

            if (isMatch) {
                found_index.push_back(i);
            }
        }

        return {!found_index.empty(), found_index};
    }

    /**
     * @brief 检查潜影盒内是否存在附魔物品
     */
    static bool hasEnchantedItems(const endstone::ItemStack& shulkerBox) {
        const auto nbtTag = shulkerBox.getNbt();
        json j = nbtToJson(nbtTag);

        if (j.is_null()) {
            return false;
        }

        if (!j.contains("Items") || !j["Items"].is_array()) {
            return false;
        }

        const auto& items = j["Items"];
        return std::ranges::any_of(items, [](const auto& item) {
            if (!item.is_object()) return false;
            if (!item.contains("tag") || !item["tag"].is_object()) return false;

            const auto& tag = item["tag"];

            // 检查普通附魔
            if (tag.contains("ench") && tag["ench"].is_array() && !tag["ench"].empty()) {
                return true;
            }

            // 检查附魔书
            if (tag.contains("StoredEnchantments") && tag["StoredEnchantments"].is_array() && !tag["StoredEnchantments"].empty()) {
                return true;
            }

            return false;
        });
    }
};

#endif //DISABLE_SHUA_NBT_TOOL_H
