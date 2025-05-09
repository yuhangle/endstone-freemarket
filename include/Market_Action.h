//
// Created by yuhang on 2025/4/14.
//

#ifndef TEST_MARKET_ACTION_H
#define TEST_MARKET_ACTION_H
#include <vector>
#include "DataBase.h"
#include <sstream>
#include <chrono>
#include <iomanip>
using namespace std;

class Market_Action {
public:
    explicit Market_Action(DataBase database);

    int goods_generate_id();
    int comment_generate_id();
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
    pair<bool,string> user_add(const std::string &uuid, const std::string &playername,
                                      const std::string &username, const std::string &avatar,const std::string& item, int money);

    //用户数据读取
    User_data user_get(const string& uuid);
    //删除玩家
    pair<bool,string> user_del(const string& uuid);
    //玩家存在
    bool user_exist(const string& uuid);
    //更新头像
    pair<bool,string> user_change_avatar(const string& uuid,const string& avatar);
    //重命名
    pair<bool,string> user_rename(const string& uuid,const string& name);
    //设置资金
    pair<bool,string> user_money(const string& uuid,int money);
    //添加玩家实体货币
    bool user_add_item(const string& uuid,const string& item);
    //清空玩家实体货币
    bool user_clear_item(const string& uuid);
    //根据玩家名查询玩家数据
    User_data user_get_by_playername(const string& playername);

    string GetUsername(const std::string &uuid);

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
    pair<bool,string> goods_add(const std::string &uuid, const std::string &name, const std::string &text,
                               const std::string &item,const std::string &data,const std::string &image,int price,const std::string &money_type,const std::string &tag);

    //获取商品数据
    Goods_data goods_get_by_gid(int gid);

    //检查商品存在
    bool goods_exist(int gid);

    //删除商品
    pair<bool,string> goods_del(int gid);

    //通过卖家的uuid批量获取商品数据
    vector<Goods_data> goods_get_by_uuid(const string& uuid);

    //根据卖家的uuid删除其全部商品
    pair<bool,string> goods_del_by_uuid(const string& uuid);

    //获取全部商品数据
    vector<Goods_data> goods_get_all();

    //update goods
    pair<bool,string> goods_update(int gid,const string& name,const string& text,const string&image,const string& tag);

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
    pair<bool,string> comment_add(const string& uuid,const string& seller,const string& message);

    //获取评论数据
    Comment_data comment_get_by_cid(const int& cid);

    //评论存在
    bool comment_exist(int cid);

    //删除评论
    pair<bool,string> comment_del(int cid);

    //通过商户的uuid批量获取评论数据
    vector<Comment_data> comment_get_by_seller(const string& uuid);

    //根据商户的uuid删除其全部评论
    pair<bool,string> comment_del_by_seller(const string& uuid);

private:
    DataBase Database;
};


#endif //TEST_MARKET_ACTION_H
