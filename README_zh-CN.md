![header](https://capsule-render.vercel.app/api?type=waving&height=300&color=gradient&text=Freemarket)

 [English](README.md)

## 介绍

Freemarket为一款用于玩家间自由交易的插件，玩家可自主上架商品，在市场界面浏览其他玩家的商品并购买，其自带一个经济系统，也支持使用umoney作为经济系统。

## 如何使用

> 安装&配置

* 安装Endstone

此步请查看endstone文档

* 下载&安装Freemarket插件

> Windows平台

前往Releases处下载最新的Windows版本的压缩包,然后解压其中的文件到服务端的plugins目录

> Linux平台

前往Releases处下载最新的Linux版本的压缩包,然后解压其中的文件到服务端的plugins目录

* 配置

首次运行插件后将自动在plugins目录创建freemarket文件夹,里面会生成配置文件config.json
配置文件的默认配置如下:

```bash
{
    "money": "freemarket",
    "player_max_goods": 10
}
```

`money` 为使用的经济系统名，默认为自身经济系统freemarket。若需更换为umoney插件作为经济系统，请更改此处值为umoney，为了在使用umoney作为经济系统时有更好的体验，请额外安装一个money_connext插件用于freemarket与umoney通信。



`player_max_goods` 为一个玩家能同时上架的最大商品量，当玩家当前正在上架商品的商品达到此值时，无法继续上架商品。

> 命令用法

**命令列表**

```shell
/market
```

打开市场菜单。

```shell
/market register <username: message> [avatar: message]
```

注册一个市场账号。"username: message" 参数为要使用的市场用户名，"avatar: message" 参数为用户使用的头像的文件位置。

```shell
/market money (add|less) <player:target> <money: int>
```

通过市场自带经济系统更改玩家资金。"add|less" 参数为增加操作或减少操作，"player:target" 为要更改的玩家名，money:int 为资金数额。仅管理员可用。
