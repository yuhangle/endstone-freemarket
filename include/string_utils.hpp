#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace string_utils {

// Split comma-separated string into vector, respecting brace depth
std::vector<std::string> split(const std::string& input);

// Join vector of strings with comma separator
std::string join(const std::vector<std::string>& vec);

// Convert string to int, returns 0 on failure
int to_int(const std::string& str);

// Format enchant map to string: "enchant_id:level,enchant_id:level"
std::string format_enchants(const std::unordered_map<std::string, int>& enchants);

// Parse enchant string back to map
std::unordered_map<std::string, int> parse_enchants(const std::string& str);

// Generate a UUID v4 string
std::string generate_uuid();

} // namespace string_utils
