//
// Created by yuhang on 2025/3/25.
//

#ifndef EPARTY_DATABASE_H
#define EPARTY_DATABASE_H

#include <iostream>
#include <sqlite3.h>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>
#include <map>

class DataBase {
public:
    // 构造时需要指定数据库文件名
    explicit DataBase(std::string  db_filename) : db_filename(std::move(db_filename)) {}


    // 函数用于检查文件是否存在
    static bool fileExists(const std::string& filename) {
        std::ifstream f(filename.c_str());
        return f.good();
    }

    // 初始化数据库
    int init_database() {
        sqlite3* db;
        int rc = sqlite3_open(db_filename.c_str(), &db);
        if (rc) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return rc;
        }

        // 创建 USER 表
        std::string create_user_table = "CREATE TABLE IF NOT EXISTS USER ("
                                        "uuid TEXT PRIMARY KEY,"
                                        "playername TEXT,"
                                        "username TEXT,"
                                        "avatar TEXT,"
                                        "item TEXT,"
                                        "money INTEGER"
                                        ");";
        rc = sqlite3_exec(db, create_user_table.c_str(), nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "创建 USER 表失败: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return rc;
        }

        // 创建 GOODS 表
        std::string create_goods_table = "CREATE TABLE IF NOT EXISTS GOODS ("
                                        "gid INTEGER PRIMARY KEY,"
                                        "uuid TEXT,"
                                        "name TEXT,"
                                        "text TEXT,"
                                        "item TEXT,"
                                        "data TEXT,"
                                        "image TEXT,"
                                        "price TEXT,"
                                        "money_type TEXT,"
                                        "tag TEXT"
                                        ");";
        rc = sqlite3_exec(db, create_goods_table.c_str(), nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "创建 GOODS 表失败: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return rc;
        }

        // 创建 COMMENT 表
        std::string create_comment_table = "CREATE TABLE IF NOT EXISTS COMMENT ("
                                           "cid INTEGER PRIMARY KEY AUTOINCREMENT,"
                                           "seller TEXT,"
                                           "uuid TEXT,"
                                           "time TEXT,"
                                           "message TEXT"
                                         ");";
        rc = sqlite3_exec(db, create_comment_table.c_str(), nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "创建 GOODS 表失败: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return rc;
        }

        sqlite3_close(db);
        return SQLITE_OK;
    }

    ///////////////////// 通用操作 /////////////////////

    // 通用执行 SQL 命令（用于添加、删除、修改操作）
    int executeSQL(const std::string &sql) {
        sqlite3* db;
        int rc = sqlite3_open(db_filename.c_str(), &db);
        if (rc) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return rc;
        }
        rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL 执行失败: " << sqlite3_errmsg(db) << std::endl;
        }
        sqlite3_close(db);
        return rc;
    }

// 通用查询 SQL（select查询使用回调函数返回结果为 vector<map<string, string>>）
    static int queryCallback(void* data, int argc, char** argv, char** azColName) {
        auto* result = static_cast<std::vector<std::map<std::string, std::string>>*>(data);
        std::map<std::string, std::string> row;
        for (int i = 0; i < argc; i++) {
            row[azColName[i]] = argv[i] ? argv[i] : "NULL";
        }
        result->push_back(row);
        return 0;
    }

    int querySQL(const std::string &sql, std::vector<std::map<std::string, std::string>> &result) {
        sqlite3* db;
        int rc = sqlite3_open(db_filename.c_str(), &db);
        if (rc) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return rc;
        }
        rc = sqlite3_exec(db, sql.c_str(), queryCallback, &result, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL 查询失败: " << sqlite3_errmsg(db) << std::endl;
        }
        sqlite3_close(db);
        return rc;
    }

// 批量查询 SQL（select查询使用回调函数返回结果为 vector<map<string, string>>）
    static int queryCallback_many_dict(void* data, int argc, char** argv, char** azColName) {
        auto* result = static_cast<std::vector<std::map<std::string, std::string>>*>(data);
        std::map<std::string, std::string> row;
        for (int i = 0; i < argc; i++) {
            row[azColName[i]] = argv[i] ? argv[i] : "NULL";
        }
        result->push_back(row);
        return 0;
    }

    int querySQL_many(const std::string &sql, std::vector<std::map<std::string, std::string>> &result) {
        sqlite3* db;
        int rc = sqlite3_open(db_filename.c_str(), &db);
        if (rc) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return rc;
        }
        rc = sqlite3_exec(db, sql.c_str(), queryCallback_many_dict, &result, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL 查询失败: " << sqlite3_errmsg(db) << std::endl;
        }
        sqlite3_close(db);
        return rc;
    }

    int updateSQL(const std::string &table, const std::string &set_clause, const std::string &where_clause) {
        sqlite3* db;
        int rc = sqlite3_open(db_filename.c_str(), &db);
        if (rc) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return rc;
        }

        std::string sql = "UPDATE " + table + " SET " + set_clause + " WHERE " + where_clause + ";";
        rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);

        if (rc != SQLITE_OK) {
            std::cerr << "SQL 更新失败: " << sqlite3_errmsg(db) << std::endl;
        }

        sqlite3_close(db);
        return rc;
    }

    // 查找指定表中是否存在指定值
    bool isValueExists(const std::string &tableName, const std::string &columnName, const std::string &value) {
        sqlite3* db;
        int rc = sqlite3_open(db_filename.c_str(), &db); // 打开数据库
        if (rc) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }
        std::string sql;
        if (columnName == "gid") {
            // 构造 SQL 查询语句
            sql = "SELECT COUNT(*) FROM " + tableName + " WHERE " + columnName + " = " + value + ";";
        } else {
            // 构造 SQL 查询语句
            sql = "SELECT COUNT(*) FROM " + tableName + " WHERE " + columnName + " = '" + value + "';";
        }
        sqlite3_stmt* stmt;
        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL 预处理失败: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return false;
        }

        // 执行查询并获取结果
        bool exists = false;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0); // 获取 COUNT(*) 的结果
            exists = (count > 0);
        }

        // 清理资源
        sqlite3_finalize(stmt);
        sqlite3_close(db);

        return exists;
    }

    // 修改指定表中的指定数据的指定值
    bool updateValue(const std::string &tableName,
                     const std::string &targetColumn,
                     const std::string &newValue,
                     const std::string &conditionColumn,
                     const std::string &conditionValue) {
        sqlite3* db;
        int rc = sqlite3_open(db_filename.c_str(), &db); // 打开数据库
        if (rc) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }
        std::string sql;
        if (conditionColumn == "gid") {
            sql = "UPDATE " + tableName +
                  " SET " + targetColumn + " = '" + newValue + "'" +
                  " WHERE " + conditionColumn + " = " + conditionValue + ";";
        }
        else {
            if (targetColumn == "money") {
                sql = "UPDATE " + tableName +
                      " SET " + targetColumn + " = " + newValue + "" +
                      " WHERE " + conditionColumn + " = '" + conditionValue + "';";
            }
            else {
                // 构造 SQL 更新语句
                sql = "UPDATE " + tableName +
                      " SET " + targetColumn + " = '" + newValue + "'" +
                      " WHERE " + conditionColumn + " = '" + conditionValue + "';";
            }
        }
        // 执行 SQL 语句
        rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL 更新失败: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return false;
        }

        // 关闭数据库连接
        sqlite3_close(db);

        return true;
    }


    ///////////////////// USER 表操作 /////////////////////

    int addUser(const std::string &uuid, const std::string &playername,
                const std::string &username, const std::string &avatar,const std::string& item, const int money) {
        std::string sql = "INSERT INTO USER (uuid, playername, username, avatar, item, money) VALUES ('" +
                          uuid + "', '" + playername + "', '" + username + "', '" + avatar +
                          "',' " + item + "',"+ std::to_string(money) +");";
        return executeSQL(sql);
    }

    int deleteUser(const std::string &uuid) {
        std::string sql = "DELETE FROM USER WHERE uuid = '" + uuid + "';";
        return executeSQL(sql);
    }

    int updateUser(const std::string &uuid, const std::string &playername,
                   const std::string &username, const std::string &avatar,const std::string& item, const int money) {
        std::string sql = "UPDATE USER SET playername = '" +
                          uuid + "', '" + playername + "', '" + username + "', '" + avatar +
                          "',' " + item + "',"+ std::to_string(money) +";";
        return executeSQL(sql);
    }

    int getUser(const std::string &uuid, std::vector<std::map<std::string, std::string>> &result) {
        std::string sql = "SELECT * FROM USER WHERE uuid = '" + uuid + "';";
        return querySQL(sql, result);
    }

    int getUser_by_playername(const std::string &playername, std::vector<std::map<std::string, std::string>> &result) {
        std::string sql = "SELECT * FROM USER WHERE playername = '" + playername + "';";
        return querySQL(sql, result);
    }

    ///////////////////// GOODS 表操作 /////////////////////

    int addGoods(const int& gid,const std::string &uuid, const std::string &name, const std::string &text,
                const std::string &item,const std::string &data,const std::string &image,const int price,const std::string &money_type,const std::string &tag) {
        std::string sql = "INSERT INTO GOODS (gid, uuid, name, text, item, data, image, price, money_type, tag) VALUES (" + std::to_string(gid) +",'" + uuid + "','" +
                          name + "', '" + text + "', '" + item + "','" + data + "','" + image + "'," + std::to_string(price) + ",'" + money_type + "','" + tag + "');";
        return executeSQL(sql);
    }

    int deleteGoods(int gid) {
        std::string sql = "DELETE FROM GOODS WHERE gid = " + std::to_string(gid) + ";";
        return executeSQL(sql);
    }

    int getGoodsByUuid(const std::string& uuid, std::vector<std::map<std::string, std::string>> &result) {
        std::string sql = "SELECT * FROM GOODS WHERE uuid = '" + uuid + "';";
        return querySQL_many(sql, result);
    }

    int getGoodsByGid(const int& gid, std::vector<std::map<std::string, std::string>> &result) {
        std::string sql = "SELECT * FROM GOODS WHERE gid = " + std::to_string(gid) + ";";
        return querySQL_many(sql, result);
    }

    int getAllGoods(std::vector<std::map<std::string, std::string>> &result) {
        // 构造 SQL 查询语句，从 GOODS 表中获取所有数据
        std::string sql = "SELECT * FROM GOODS;";

        // 调用 querySQL_many 函数执行查询，并将结果存储到 result 中
        return querySQL_many(sql, result);
    }

    int updateGoods(const int& gid, const std::string &name, const std::string &text, const std::string &image,const std::string& tag) {
        // 构造 SQL 更新语句
        std::string sql = "UPDATE GOODS SET name = '" + name + "', " +
                "text = '" + text + "', " +
                "image = '" + image + "', " +
                "tag = '" + tag + "' " +
                "WHERE gid = " + std::to_string(gid) + ";";
        // 执行 SQL 语句
        return executeSQL(sql);
    }

    int getLatestGid() {
        sqlite3* db;
        int rc = sqlite3_open(db_filename.c_str(), &db);
        if (rc) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return -1; // 默认返回 -1
        }

        std::string sql = "SELECT MAX(gid) FROM GOODS;";
        sqlite3_stmt* stmt;
        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            sqlite3_close(db);
            return -1;
        }

        int latestGoodId = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            latestGoodId = sqlite3_column_int(stmt, 0); // 获取第一列的值
        }

        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return latestGoodId;
    }

    int getGoodsCountByUuid(const std::string& uuid) {
        sqlite3* db;
        int rc = sqlite3_open(db_filename.c_str(), &db);
        if (rc) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return -1; // 数据库打开失败，返回 -1
        }

        // 构造 SQL 查询语句，统计指定 uuid 的商品数量
        std::string sql = "SELECT COUNT(*) FROM GOODS WHERE uuid = '" + uuid + "';";
        sqlite3_stmt* stmt;
        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL 语句准备失败: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return -1; // SQL 准备失败，返回 -1
        }

        int goodsCount = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            goodsCount = sqlite3_column_int(stmt, 0); // 获取第一列的值（COUNT(*) 的结果）
        } else {
            std::cerr << "查询结果为空或执行失败" << std::endl;
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return -1; // 查询结果异常，返回 -1
        }

        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return goodsCount; // 返回商品总数
    }

    ///////////////////// COMMENT 表操作 /////////////////////

    int addComment(int cid, const std::string &seller, const std::string &uuid, const std::string &time, const std::string &message) {
        std::string sql = "INSERT INTO COMMENT (cid, pid, uuid, time, message) VALUES ("+
                          std::to_string(cid) + ",'" +
                          seller + "', '" + uuid + "', '" + time + "', '" + message + "');";
        return executeSQL(sql);
    }

    int deleteComment(int cid) {
        std::string sql = "DELETE FROM COMMENT WHERE cid = " + std::to_string(cid) + ";";
        return executeSQL(sql);
    }

    int getComment(int cid, std::vector<std::map<std::string, std::string>> &result) {
        std::string sql = "SELECT * FROM COMMENT WHERE cid = " + std::to_string(cid) + ";";
        return querySQL(sql, result);
    }

    int getCommentBySeller(const std::string& seller_uuid, std::vector<std::map<std::string, std::string>> &result) {
        std::string sql = "SELECT * FROM COMMENT WHERE seller = '" + seller_uuid + "';";
        return querySQL_many(sql, result);
    }

    int getLatestCommentId() {
        sqlite3* db;
        int rc = sqlite3_open(db_filename.c_str(), &db);
        if (rc) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return -1; // 默认返回 -1
        }

        std::string sql = "SELECT MAX(cid) FROM COMMENT;";
        sqlite3_stmt* stmt;
        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            sqlite3_close(db);
            return -1;
        }

        int latestCommentId = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            latestCommentId = sqlite3_column_int(stmt, 0); // 获取第一列的值
        }

        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return latestCommentId;
    }

    //数据库工具

    //将逗号字符串分割为vector
    static std::vector<std::string> splitString(const std::string& input) {
        std::vector<std::string> result; // 用于存储分割后的结果
        std::string current;             // 当前正在构建的子字符串
        bool inBraces = false;           // 标记是否在 {} 内部

        for (char ch : input) {
            if (ch == '{') {
                // 遇到左大括号，标记进入 {} 内部
                inBraces = true;
                current += ch; // 将 { 添加到当前字符串
            } else if (ch == '}') {
                // 遇到右大括号，标记离开 {} 内部
                inBraces = false;
                current += ch; // 将 } 添加到当前字符串
            } else if (ch == ',' && !inBraces) {
                // 遇到逗号且不在 {} 内部时，分割字符串
                if (!current.empty()) {
                    result.push_back(current);
                    current.clear();
                }
            } else {
                // 其他情况，将字符添加到当前字符串
                current += ch;
            }
        }

        // 处理最后一个子字符串（如果非空）
        if (!current.empty()) {
            result.push_back(current);
        }

        return result;
    }

    //将数字字符逗号分割为vector
    static std::vector<int> splitStringInt(const std::string& input) {
        std::vector<int> result; // 用于存储分割后的结果
        std::stringstream ss(input);     // 使用 string stream 处理输入字符串
        std::string item;

        // 按逗号分割字符串
        while (std::getline(ss, item, ',')) {
            if (!item.empty()) {         // 如果分割出的元素非空，则添加到结果中
                std::stringstream sss(item);
                int groupid = 0;
                if (!(sss >> groupid)) {
                    return {}; // 如果解析失败，返回空
                }
                result.push_back(groupid);
            }
        }

        // 如果没有逗号且结果为空则返回空
        if (result.empty()) {
            return {};
        }

        return result;
    }

    //将vector(string)通过逗号分隔改为字符串
    static std::string vectorToString(const std::vector<std::string>& vec) {
        if (vec.empty()) {
            return ""; // 如果向量为空，返回空字符串
        }

        std::ostringstream oss; // 使用字符串流拼接结果
        for (size_t i = 0; i < vec.size(); ++i) {
            oss << vec[i]; // 添加当前元素
            if (i != vec.size() - 1 && !(vec[i].empty())) { // 如果不是最后一个元素且不为空，添加逗号
                oss << ",";
            }
        }

        return oss.str(); // 返回拼接后的字符串
    }

    //将vector(int)通过逗号分隔改为字符串
    static std::string IntVectorToString(const std::vector<int>& vec) {
        if (vec.empty()) {
            return ""; // 如果向量为空，返回空字符串
        }

        std::ostringstream oss; // 使用字符串流拼接结果
        for (size_t i = 0; i < vec.size(); ++i) {
            oss << vec[i]; // 添加当前元素
            if (i != vec.size() - 1) { // 如果不是最后一个元素，添加逗号
                oss << ",";
            }
        }

        return oss.str(); // 返回拼接后的字符串
    }

    //字符串改整数
    static int stringToInt(const std::string& str) {
        try {
            // 尝试将字符串转换为整数
            return std::stoi(str);
        } catch (const std::invalid_argument&) {
            // 捕获无效参数异常（例如无法解析为整数）
            return 0;
        } catch (const std::out_of_range&) {
            // 捕获超出范围异常（例如数值过大或过小）
            return 0;
        }
    }

private:
    std::string db_filename;
};

#endif // EPARTY_DATABASE_H

