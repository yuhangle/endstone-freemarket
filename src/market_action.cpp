//
// Created by yuhang on 2025/4/14.
//

#include "../include/market_action.h"
#include <utility>

Market_Action::Market_Action(DataBase database) : Database(std::move(database)) {}

int Market_Action::goods_generate_id() const {
    auto latest_id = Database.getLatestGid();
    if (latest_id == -1) {
        cout << "商品号生成异常" << endl;
        return -1;
    } else {
        return latest_id+1;
    }
}

int Market_Action::comment_generate_id() const {
    auto latest_id = Database.getLatestCommentId();
    if (latest_id == -1) {
        cout << "评论号生成异常" << endl;
        return -1;
    } else {
        return latest_id+1;
    }
}

// 时间获取
std::string Market_Action::getFormattedCurrentTime() {
    // 获取系统当前时间
    auto now = std::chrono::system_clock::now();
    // 转换为 time_t 类型
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm local_tm{};
#if defined(_WIN32) || defined(_WIN64)
    // Windows 平台使用 thread-safe 的 localtime_s
    localtime_s(&local_tm, &t);
#else
    // POSIX 平台使用 localtime，注意它返回的是静态分配的 tm 对象
    local_tm = *std::localtime(&t);
#endif

    // 使用 std::put_time 格式化输出
    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y/%m/%d/%H/%M/%S");
    return oss.str();
}

// 时间转换
std::vector<int> Market_Action::parseTimeString(const std::string &timeStr) {
    std::vector<int> timeParts;
    std::istringstream iss(timeStr);
    std::string token;
    // 按 '/' 进行分割
    while (std::getline(iss, token, '/')) {
        try {
            int value = std::stoi(token);
            timeParts.push_back(value);
        } catch (const std::exception &e) {
            // 如果转换失败，可以输出错误信息，也可以根据需求处理错误
            std::cerr << e.what() << std::endl;
        }
    }
    return timeParts;
}

string Market_Action::GetTimeHuman() {
    auto the_time = parseTimeString(getFormattedCurrentTime());
    // 重新格式化时间字符串
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << the_time[0] << "-"  // 年
        << std::setw(2) << std::setfill('0') << the_time[1] << "-"  // 月
        << std::setw(2) << std::setfill('0') << the_time[2] << " "  // 日
        << std::setw(2) << std::setfill('0') << the_time[3] << ":"  // 时
        << std::setw(2) << std::setfill('0') << the_time[4] << ":"  // 分
        << std::setw(2) << std::setfill('0') << the_time[5];        // 秒
    return oss.str();
}

string Market_Action::VectorTimeHuman(const std::vector<int>& time) {
    // 重新格式化时间字符串
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << time[0] << "-"  // 年
        << std::setw(2) << std::setfill('0') << time[1] << "-"  // 月
        << std::setw(2) << std::setfill('0') << time[2] << " "  // 日
        << std::setw(2) << std::setfill('0') << time[3] << ":"  // 时
        << std::setw(2) << std::setfill('0') << time[4] << ":"  // 分
        << std::setw(2) << std::setfill('0') << time[5];        // 秒
    return oss.str();
}

pair<bool, string> Market_Action::user_add(const std::string &uuid, const std::string &playername,
                                           const std::string &username, const std::string &avatar,const std::string& item, int money) const {
    vector<map<string,string>> result;
    Database.getUser(uuid,result);
    if (result.empty()) {
        if (Database.addUser(uuid,playername,username,avatar,item,money)) {
            return {false,"Register failed"};
        } else {
            return {true,"Register successfully"};
        }
    } else {
        return {false,"The player already exists"};
    }
}

Market_Action::User_data Market_Action::user_get(const std::string &uuid) const {
    vector<map<string,string>> result;
    Database.getUser(uuid,result);
    if (result.empty()) {
        return {false};
    }
    else {
        auto data = result[0];
        return {true,data.at("uuid"),data.at("playername"),data.at("username"),data.at("avatar"),data.at("item"),DataBase::stringToInt(data.at("money"))};
    }
}

pair<bool, string> Market_Action::user_del(const std::string &uuid) const {
    //删除玩家
    vector<map<string,string>> result;
    Database.getUser(uuid,result);
    if (result.empty()) {
        return {false,"The player not exists"};
    } else {
        if (Database.deleteUser(uuid)) {
            return {false,"Delete failed"};
        } else {
            return {true,"Delete successfully"};
        }
    }
}

bool Market_Action::user_exist(const std::string &uuid) const {
    //用户检测
    return Database.isValueExists("USER","uuid",uuid);
}

pair<bool, string> Market_Action::user_change_avatar(const std::string &uuid, const std::string &avatar) const {
    if (user_exist(uuid)) {
        if (Database.updateValue("USER","avatar",avatar,"uuid",uuid)) {
            return {true,"Change successfully"};
        } else {
            return {false,"Change failed"};
        }
    } else {
        return {false,"The player not exists"};
    }
}

pair<bool, string> Market_Action::user_rename(const std::string &uuid, const std::string &name) const {
    if (user_exist(uuid)) {
        if (Database.updateValue("USER","username",name,"uuid",uuid)) {
            return {true,"Change successfully"};
        } else {
            return {false,"Change failed"};
        }
    } else {
        return {false,"The player not exists"};
    }
}

pair<bool, string> Market_Action::user_money(const std::string &uuid, int money) const {
    if (user_exist(uuid)) {
        if (Database.updateValue("USER","money",to_string(money),"uuid",uuid)) {
            return {true,"Change successfully"};
        } else {
            return {false,"Change failed"};
        }
    } else {
        return {false,"The player not exists"};
    }
}

Market_Action::User_data Market_Action::user_get_by_playername(const std::string &playername) const {
    vector<map<string,string>> result;
    Database.getUser_by_playername(playername,result);
    if (result.empty()) {
        return {false};
    }
    else {
        auto data = result[0];
        return {true,data.at("uuid"),data.at("playername"),data.at("username"),data.at("avatar"),data.at("item"),DataBase::stringToInt(data.at("money"))};
    }
}

bool Market_Action::user_add_item(const std::string &uuid, const std::string &item) const {
    if (user_exist(uuid)) {
        auto user_data = user_get(uuid);
        if (user_data.item.empty()) {
            return Database.updateValue("USER","item","{"+item+"}","uuid",uuid);
        } else {
            auto vec_item = DataBase::splitString(user_data.item);
            vec_item.push_back("{"+item+"}");
            return Database.updateValue("USER","item",DataBase::vectorToString(vec_item),"uuid",uuid);
        }
    }
    return false;
}

bool Market_Action::user_clear_item(const std::string &uuid) const {
    if (user_exist(uuid)) {
        return Database.updateValue("USER","item","","uuid",uuid);
    } else {
        return false;
    }
}

string Market_Action::GetUsername(const std::string &uuid) const {
    auto user_data = user_get(uuid);
    if (user_data.status) {
        return user_data.username;
    } else {
        return "Unknown";
    }
}

//商品操作
pair<bool, string> Market_Action::goods_add(const std::string &uuid, const std::string &name, const std::string &text,
                                           const std::string &item, const std::string &data, const std::string &image,
                                           const int price, const std::string &money_type, const std::string &tag) const {
    int gid = goods_generate_id();
    if (gid == -1){
        return {false,"Goods ID error"};
    }
    if (Database.addGoods(gid,uuid,name,text,item,data,image,price,money_type,tag)) {
        return {false,"Add failed"};
    } else {
        return {true,"Add successfully"};
    }
}

Market_Action::Goods_data Market_Action::goods_get_by_gid(int gid) const {
    vector<map<string,string>> result;
    Database.getGoodsByGid(gid,result);
    if (result.empty()) {
        return {false};
    }
    else {
        auto data = result[0];
        return {true,gid,data.at("uuid"),data.at("name"),data.at("text"),data.at("item"),data.at("data"),data.at("image"),DataBase::stringToInt(data.at("price")),data.at("money_type"),data.at("tag")};
    }
}

bool Market_Action::goods_exist(const int gid) const {
    return Database.isValueExists("GOODS","gid",to_string(gid));
}

pair<bool,string> Market_Action::goods_del(const int gid) const {
    if (!goods_exist(gid)) {
        return {false,"The goods not exists"};
    } else {
        if (Database.deleteGoods(gid)) {
            return {false,"Delete failed"};
        } else {
            return {true,"Delete successfully"};
        }
    }   
}

vector<Market_Action::Goods_data> Market_Action::goods_get_by_uuid(const string& uuid) const {
    if (!user_exist(uuid)) {
        return {};
    }
    std::vector<std::map<std::string, std::string>> result;
    Database.getGoodsByUuid(uuid,result);
    if (result.empty()) {
        return {};
    }
    vector<Market_Action::Goods_data> goods_data;
    for (const auto& data:result) {
        Market_Action::Goods_data one_goods = {true,DataBase::stringToInt(data.at("gid")),data.at("uuid"),data.at("name"),data.at("text"),data.at("item"),data.at("data"),data.at("image"),DataBase::stringToInt(data.at("price")),data.at("money_type"),data.at("tag")};
        goods_data.push_back(one_goods);
    }
    return goods_data;
}

pair<bool, string> Market_Action::goods_del_by_uuid(const std::string &uuid) const {
    auto goods_data = goods_get_by_uuid(uuid);
    if (goods_data.empty()) {
        return {false,"The player has not goods"};
    }
    int su_times = 0;
    int fail_times = 0;
    for (const auto& one_goods:goods_data) {
        auto status = goods_del(one_goods.gid);
        if (status.first) {
            su_times++;
        } else {
            fail_times++;
        }
    }
    if (fail_times == 0) {
        return {true, to_string(su_times) + "," + to_string(fail_times)};
    } else {
        return {false, to_string(su_times) + "," + to_string(fail_times)};
    }
}

vector<Market_Action::Goods_data> Market_Action::goods_get_all() const {
    vector<map<string,string>> result;
    Database.getAllGoods(result);
    if (result.empty()) {
        return {};
    }
    vector<Market_Action::Goods_data> goods_data;
    for (const auto& data:result) {
        Market_Action::Goods_data one_goods = {true,DataBase::stringToInt(data.at("gid")),data.at("uuid"),data.at("name"),data.at("text"),data.at("item"),data.at("data"),data.at("image"),DataBase::stringToInt(data.at("price")),data.at("money_type"),data.at("tag")};
        goods_data.push_back(one_goods);
    }
    return goods_data;
}

pair<bool,string> Market_Action::goods_update(int gid,const string& name,const string& text,const string&image,const string& tag) const {
    if (!goods_exist(gid)) {
        return {false,"Goods not exist"};
    }
    if (Database.updateGoods(gid,name,text,image,tag)) {
        return {false,"Update failed"};
    } else {
        return {true,"Update successfully"};
    }
}

pair<bool, string> Market_Action::comment_add(const std::string &uuid, const string& seller,
                                              const std::string &message) const {
    auto cid = comment_generate_id();
    if (cid == -1) {
        return {false,"Comment ID error"};
    }
    if (Database.addComment(cid,seller,uuid, getFormattedCurrentTime(),message)) {
        return {false,"Comment failed"};
    } else {
        return {true,"Comment successfully"};
    }
}

Market_Action::Comment_data Market_Action::comment_get_by_cid(const int& cid) const {
    vector<map<string,string>> result;
    Database.getComment(cid,result);
    if (result.empty()) {
        return {false};
    } else {
        auto data = result[0];
        return {true,cid,data.at("uuid"),data.at("seller"),data.at("time"),data.at("message")};
    }
}

bool Market_Action::comment_exist(const int cid) const {
    return Database.isValueExists("COMMENT","cid",to_string(cid));
}

pair<bool, string> Market_Action::comment_del(int cid) const {
    if (!comment_exist(cid)) {
        return {false,"Comment not exists"};
    }
    if (Database.deleteComment(cid)) {
        return {false,"Delete failed"};
    } else {
        return {true,"Delete successfully"};
    }
}

vector<Market_Action::Comment_data> Market_Action::comment_get_by_seller(const std::string &uuid) const {
    if (!user_exist(uuid)) {
        return {};
    }
    std::vector<std::map<std::string, std::string>> result;
    Database.getCommentBySeller(uuid,result);
    if (result.empty()) {
        return {};
    }
    vector<Market_Action::Comment_data> comment_data;
    for (const auto& data:result) {
        Market_Action::Comment_data one_comment = {true,DataBase::stringToInt(data.at("cid")),data.at("uuid"),data.at("seller"),data.at("time"),data.at("message")};
        comment_data.push_back(one_comment);
    }
    return comment_data;
}

pair<bool, string> Market_Action::comment_del_by_seller(const std::string &uuid) const {
    auto comment_data = comment_get_by_seller(uuid);
    if (comment_data.empty()) {
        return {false,"The seller has no user comments"};
    }
    int su_times = 0;
    int fail_times = 0;
    for (const auto& one_comment:comment_data) {
        auto status = goods_del(one_comment.cid);
        if (status.first) {
            su_times++;
        } else {
            fail_times++;
        }
    }
    if (fail_times == 0) {
        return {true, to_string(su_times) + "," + to_string(fail_times)};
    } else {
        return {false, to_string(su_times) + "," + to_string(fail_times)};
    }
}


//交易记录
pair<bool,string> Market_Action::record_add(const string& seller,const string& buyer,const string& goods) const {
    const auto uuid = DataBase::generate_uuid_v4();
    if (Database.addRecord(uuid,seller,buyer, getFormattedCurrentTime(),goods)) {
        return {false,"Record failed"};
    } else {
        return {true,"Record successfully"};
    }
};

Market_Action::Record_data Market_Action::record_get_by_uuid(const string& uuid) const {
    vector<map<string,string>> result;
    Database.getRecord(uuid,result);
    if (result.empty()) {
        return {false};
    } else {
        auto data = result[0];
        return {true,uuid,data.at("seller"),data.at("buyer"),data.at("time"),data.at("goods")};
    }
}

bool Market_Action::record_exist(const string& uuid) const {
    return Database.isValueExists("RECORD","uuid",uuid);
}

pair<bool,string> Market_Action::record_del(const string& uuid) const {
    if (!record_exist(uuid)) {
        return {false,"Record not exists"};
    }
    if (Database.deleteRecord(uuid)) {
        return {false,"Delete failed"};
    } else {
        return {true,"Delete successfully"};
    }
}

vector<Market_Action::Record_data> Market_Action::record_get_by_seller(const string& seller) const {
    if (!user_exist(seller)) {
        return {};
    }
    std::vector<std::map<std::string, std::string>> result;
    Database.getRecordBySeller(seller,result);
    if (result.empty()) {
        return {};
    }
    vector<Record_data> record_data;
    for (const auto& data:result) {
        Record_data one_record = {true,data.at("uuid"),data.at("seller"),data.at("buyer"),data.at("time"),data.at("goods")};
        record_data.push_back(one_record);
    }
    return record_data;
}

vector<Market_Action::Record_data> Market_Action::record_get_by_buyer(const string& buyer) const {
    if (!user_exist(buyer)) {
        return {};
    }
    std::vector<std::map<std::string, std::string>> result;
    Database.getRecordByBuyer(buyer,result);
    if (result.empty()) {
        return {};
    }
    vector<Record_data> record_data;
    for (const auto& data:result) {
        Record_data one_record = {true,data.at("uuid"),data.at("seller"),data.at("buyer"),data.at("time"),data.at("goods")};
        record_data.push_back(one_record);
    }
    return record_data;
}