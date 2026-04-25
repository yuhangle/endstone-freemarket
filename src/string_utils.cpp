#include "string_utils.hpp"
#include <sstream>
#include <random>

namespace string_utils {

std::vector<std::string> split(const std::string& input) {
    std::vector<std::string> result;
    std::string current;
    int depth = 0;

    for (const char ch : input) {
        if (ch == '{' || ch == '[') {
            depth++;
            current += ch;
        } else if (ch == '}' || ch == ']') {
            depth--;
            current += ch;
        } else if (ch == ',' && depth == 0) {
            if (!current.empty()) {
                result.push_back(current);
                current.clear();
            }
        } else {
            current += ch;
        }
    }
    if (!current.empty()) {
        result.push_back(current);
    }
    return result;
}

std::string join(const std::vector<std::string>& vec) {
    if (vec.empty()) {
        return "";
    }

    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];
        if (i != vec.size() - 1 && !vec[i].empty()) {
            oss << ",";
        }
    }
    return oss.str();
}

int to_int(const std::string& str) {
    try {
        return std::stoi(str);
    } catch (const std::invalid_argument&) {
        return 0;
    } catch (const std::out_of_range&) {
        return 0;
    }
}

std::string format_enchants(const std::unordered_map<std::string, int>& enchants) {
    if (enchants.empty()) {
        return "";
    }

    std::ostringstream oss;
    for (auto it = enchants.begin(); it != enchants.end(); ++it) {
        oss << it->first << ":" << it->second;
        if (std::next(it) != enchants.end()) {
            oss << ",";
        }
    }
    return oss.str();
}

std::unordered_map<std::string, int> parse_enchants(const std::string& str) {
    std::unordered_map<std::string, int> result;
    std::istringstream ss(str);
    std::string pairStr;

    while (std::getline(ss, pairStr, ',')) {
        if (size_t colonPos = pairStr.find_last_of(':'); colonPos != std::string::npos) {
            std::string key = pairStr.substr(0, colonPos);
            std::string valueStr = pairStr.substr(colonPos + 1);
            try {
                int value = std::stoi(valueStr);
                result[key] = value;
            } catch (...) {
            }
        }
    }
    return result;
}

std::string generate_uuid() {
    static thread_local std::mt19937 gen{std::random_device{}()};

    std::uniform_int_distribution<int> dis(0, 15);

    std::stringstream ss;
    ss << std::hex;

    for (int i = 0; i < 8; ++i) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; ++i) ss << dis(gen);
    ss << "-";

    ss << "4";
    for (int i = 0; i < 3; ++i) ss << dis(gen);
    ss << "-";

    ss << (dis(gen) & 0x3 | 0x8);
    for (int i = 0; i < 3; ++i) ss << dis(gen);
    ss << "-";

    for (int i = 0; i < 12; ++i) ss << dis(gen);

    return ss.str();
}

} // namespace string_utils
