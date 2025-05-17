![header](https://capsule-render.vercel.app/api?type=waving&height=300&color=gradient&text=Freemarket)

> This plugin is still in the development phase. Some item data may be lost during transactions. Currently supported item data includes: item name, enchantments, lore, and item durability. Unsupported data includes shulker box contents, firework data, and more. Further support will depend on future API updates from Endstone.

[简体中文](README_zh-CN.md)

## Introduction

Freemarket is a plugin designed for free trading between players. Players can independently list their items for sale and browse other players' listings in the market interface for purchasing. It comes with its own built-in economy system and also supports using umoney as the economy system.

## How to Use

> Installation & Configuration

* Install Endstone

Please refer to the Endstone documentation for this step.

* Download & Install the Freemarket Plugin

> Windows Platform

Go to the Releases page to download the latest Windows version compressed package, and then extract the files in it to the server's plugins directory.

> Linux Platform

Go to the Releases page to download the latest Linux version compressed package, and then extract the files in it to the server's plugins directory.

* Configuration

After running the plugin for the first time, a "freemarket" folder will be automatically created in the plugins directory, containing the configuration file `config.json`.
The default configuration of the configuration file is as follows:

```bash
{
    "money": "freemarket",
    "player_max_goods": 10
}
```

`money` is the name of the economy system used. The default is its own economy system, "freemarket". If you need to switch to the umoney plugin as the economy system, please change this value to "umoney". For a better experience when using umoney as the economy system, please additionally install a "money\_connext" plugin for communication between freemarket and umoney.

`player_max_goods` is the maximum number of items a player can list for sale simultaneously. When the number of items a player is currently listing reaches this value, they cannot list any more items.

> Command Usage

**Command List**

```shell
/market
```

Opens the market menu.

```shell
/market register <username: message> [avatar: message]
```

Registers a market account. The "username: message" parameter is the market username to be used, and the "avatar: message" parameter is the file path of the user's avatar.

```shell
/market money (add|less) <player:target> <money: int>
```

Changes a player's funds through the market's built-in economy system. The "add|less" parameter is the operation to increase or decrease, "player:target" is the name of the player to be changed, and "money:int" is the amount of funds. Only administrators can use this command.
