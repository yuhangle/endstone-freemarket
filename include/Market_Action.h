//
// Created by yuhang on 2025/4/14.
//

#ifndef TEST_MARKET_ACTION_H
#define TEST_MARKET_ACTION_H
#include <vector>
#include "DataBase.h"
#include <iomanip>  // 👈 Windows需要
using namespace std;

class Market_Action {
public:
    explicit Market_Action(DataBase database);

    [[nodiscard]] int goods_generate_id() const;
    [[nodiscard]] int comment_generate_id() const;
    static std::string getFormattedCurrentTime();
    static std::vector<int> parseTimeString(const std::string &timeStr);
    static string GetTimeHuman();
    static string VectorTimeHuman(const std::vector<int>& time);

    //用户数据操作

    //用户数据
    struct User_data {
        bool status;
        string uuid;
        string playername;
        string username;
        string avatar;
        string item;
        int money;
    };
    //用户创建
    [[nodiscard]] pair<bool,string> user_add(const std::string &uuid, const std::string &playername,
                                      const std::string &username, const std::string &avatar,const std::string& item, int money) const;

    //用户数据读取
    [[nodiscard]] User_data user_get(const string& uuid) const;
    //删除玩家
    [[nodiscard]] pair<bool,string> user_del(const string& uuid) const;
    //玩家存在
    [[nodiscard]] bool user_exist(const string& uuid) const;
    //更新头像
    [[nodiscard]] pair<bool,string> user_change_avatar(const string& uuid,const string& avatar) const;
    //重命名
    [[nodiscard]] pair<bool,string> user_rename(const string& uuid,const string& name) const;
    //设置资金
    [[nodiscard]] pair<bool,string> user_money(const string& uuid,int money) const;
    //添加玩家实体货币
    [[nodiscard]] bool user_add_item(const string& uuid,const string& item) const;
    //清空玩家实体货币
    [[nodiscard]] bool user_clear_item(const string& uuid) const;
    //根据玩家名查询玩家数据
    [[nodiscard]] User_data user_get_by_playername(const string& playername) const;

    [[nodiscard]] string GetUsername(const std::string &uuid) const;

    //商品操作

    //商品数据
    struct Goods_data {
        bool status;
        int gid;
        string uuid;
        string name;
        string text;
        string item;
        string data;
        string image;
        int price;
        string money_type;
        string tag;
    };

    //上架商品
    [[nodiscard]] pair<bool,string> goods_add(const std::string &uuid, const std::string &name, const std::string &text,
                               const std::string &item,const std::string &data,const std::string &image,int price,const std::string &money_type,const std::string &tag) const;

    //获取商品数据
    [[nodiscard]] Goods_data goods_get_by_gid(int gid) const;

    //检查商品存在
    [[nodiscard]] bool goods_exist(int gid) const;

    //删除商品
    [[nodiscard]] pair<bool,string> goods_del(int gid) const;

    //通过卖家的uuid批量获取商品数据
    [[nodiscard]] vector<Goods_data> goods_get_by_uuid(const string& uuid) const;

    //根据卖家的uuid删除其全部商品
    [[nodiscard]] pair<bool,string> goods_del_by_uuid(const string& uuid) const;

    //获取全部商品数据
    [[nodiscard]] vector<Goods_data> goods_get_all() const;

    //update goods
    [[nodiscard]] pair<bool,string> goods_update(int gid,const string& name,const string& text,const string&image,const string& tag) const;

    //评论部分

    //评论数据
    struct Comment_data {
        bool status;
        int cid;
        string uuid;
        string seller;
        string time;
        string message;
    };

    //添加评论
    [[nodiscard]] pair<bool,string> comment_add(const string& uuid,const string& seller,const string& message) const;

    //获取评论数据
    [[nodiscard]] Comment_data comment_get_by_cid(const int& cid) const;

    //评论存在
    [[nodiscard]] bool comment_exist(int cid) const;

    //删除评论
    [[nodiscard]] pair<bool,string> comment_del(int cid) const;

    //通过商户的uuid批量获取评论数据
    [[nodiscard]] vector<Comment_data> comment_get_by_seller(const string& uuid) const;

    //根据商户的uuid删除其全部评论
    [[nodiscard]] pair<bool,string> comment_del_by_seller(const string& uuid) const;

    //交易记录部分

    struct Record_data {
        bool status;
        string uuid;
        string seller;
        string buyer;
        string time;
        string goods;
    };

    //记录交易
    [[nodiscard]] pair<bool,string> record_add(const string& seller,const string& buyer,const string& goods) const;

    //获取交易数据
    [[nodiscard]] Record_data record_get_by_uuid(const string& uuid) const;

    //交易存在
    [[nodiscard]] bool record_exist(const string& uuid) const;

    //删除交易记录
    [[nodiscard]] pair<bool,string> record_del(const string& uuid) const;

    //通过商户的uuid批量获取交易数据
    [[nodiscard]] vector<Record_data> record_get_by_seller(const string& seller) const;

    //通过买家的uuid批量获取交易数据
    [[nodiscard]] vector<Record_data> record_get_by_buyer(const string& buyer) const;
private:
    DataBase Database;
};


#endif //TEST_MARKET_ACTION_H
