from endstone.command import Command, CommandSender, CommandSenderWrapper
from endstone.plugin import Plugin
import json
import os
from endstone.form import ModalForm,Dropdown,Label,ActionForm,TextInput,Slider,MessageForm
from endstone.event import event_handler, PlayerInteractEvent
import re
import time
from endstone import ColorFormat,Player
# 插件初始化部分

fmdata = "plugins/freemarket"
moneydata = "plugins/freemarket/moneydata.json"
marketdata = "plugins/freemarket/marketdata.json"

# 创建数据文件夹
if not os.path.exists(fmdata):
    os.makedirs(fmdata)
# 创建货币数据文件    
if not os.path.exists(moneydata):
    with open(moneydata, 'w',encoding='utf-8') as file:
        json.dump({}, file)
# 创建市场数据文件    
if not os.path.exists(marketdata):
    with open(marketdata, 'w',encoding='utf-8') as file:
        json.dump({}, file)
# 定义货币名
moneyname = "信用点"


class freemarket(Plugin):
    api_version = "0.5"
    
    def on_load(self) -> None:
        self.logger.info("on_load is called!")

    def on_enable(self) -> None:
        self.logger.info("on_enable is called!")
        self.logger.info(f"自由市场贸易插件已启用! 版本0.0.4dev5")
        self.last_command_time = 0  # 记录上次执行命令的时间
        # 监听事件
        self.register_events(self)

    def on_disable(self) -> None:
        self.logger.info("on_disable is called!")

    # 分割线
    
    # 用于用户注册的函数
    def add_user(self,playername:str,username:str,money=0,otheritem=None,num=0):
        """
        注册用户信息
        playername: 玩家名
        username: 玩家注册的用户名
        money: 用户资金,默认为0
        otheritem: 其它交易介质
        num: 其它介质的数额,默认为0
        """
        # 读取现有的经济数据
        with open(moneydata, "r",encoding='utf-8') as f:
            data = json.load(f)
        # 注册用户信息
        data[playername] = {"username": username, "money": money, "otheritem": otheritem, "num": num}
        # 将修改后的数据写入文件
        with open(moneydata, "w",encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=4)
        msg = f"您已成功注册服务器交易账号,账号名为 {username}"
        return msg

    # 用于检测玩家是否注册过服务器交易账号的函数
    def testuser(self,playername:str):
        """
        检测玩家是否注册过服务器交易账号
        playername: 玩家名
        """  
        # 读取现有的经济数据
        with open(moneydata, "r",encoding='utf-8') as f:
            data = json.load(f)
        # 检查玩家是否注册
        if playername not in data:
            return False
        else:
            return True
        
    # 用于用户资金变动的函数
    def change_money(self,playername:str,action:str,money=0):
        """
        用户资金变动
        playername: 玩家名
        action: 对用户资金的操作,增加为add,减少为less
        money: 用户资金的更改额度,默认为0
        """
        # 读取现有的经济数据
        with open(moneydata, "r",encoding='utf-8') as f:
            data = json.load(f)
        # 获取玩家账户资金
        nowmoney = data[playername]["money"]
        # 对玩家资金进行运算
        if action == "add":
            aftermoney = nowmoney + money
        elif action == "less":
            aftermoney = nowmoney - money
        else:
            msg = "无经济操作"
            return msg
        # 更改玩家资金
        data[playername]["money"] = aftermoney
        # 将修改后的数据写入文件
        with open(moneydata, "w",encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=4)
        msg = f"玩家 {playername} 的账户已{"增加" if action == "add" else "减少"}了{money}{moneyname},现账户余额为{aftermoney}{moneyname}"
        return msg
    
    # 用于用户其它资金变动的函数
    def change_other_money(self,playername:str,action:str,item_name:str,num=0):
        """
        用户资金变动
        playername: 玩家名
        action: 对用户资金的操作,增加为add,减少为less
        otheritem: 用户其它资源
        num: 其它资源数目变动
        
        增加增加成功则返回True,物资出库则返回出库物资的字典值并删除账户内的物资
        """
        # 读取现有的经济数据
        with open(moneydata, "r",encoding='utf-8') as f:
            data = json.load(f)
        
        # 初始化items    
        if 'items' not in data[playername]:
            data[playername]['items'] = []
        
        items = data[playername]['items']
        
        # 入库操作
        if action == "add":
            # 检查是否已经存在相同的物品
            for item in items:
                if item['name'] == item_name:
                    item['num'] += num
                    break
            else:
                # 如果没有找到相同的物品，则添加新条目
                items.append({'name': item_name, 'num': num})
            msg = True
        # 出库操作
        elif action == "less":
            found = False
            for i, item in enumerate(items):
                if item['name'] == item_name and item['num'] >= num:
                    found = True
                    item['num'] -= num
                    if item['num'] <= 0:
                        del items[i]
                    msg = {
                        "name": item_name,
                        "num": num
                    }
                    break
            msg = "提现成功"
            if not found:
                msg = "物资不足或不存在"
        else:
            msg = "无经济操作"
            return msg
        # 将修改后的数据写入文件
        with open(moneydata, "w",encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=4)
        return msg

    # 用于用户重命名的函数
    def rename_account(self,playername:str,username:str):
        """
        用户重命名
        playername: 玩家名
        username: 新用户名
        """
        # 读取现有的经济数据
        with open(moneydata, "r",encoding='utf-8') as f:
            data = json.load(f)

        if data[playername]["username"] == username:
            msg = "新用户名与老用户名一致,无需更改"
            return msg
        elif username in data:
            msg = "该名称已存在!请换一个名字"
            return msg
        old_username = data[playername]["username"]
        # 更改玩家用户名
        data[playername]["username"] = username
        # 将修改后的数据写入文件
        with open(moneydata, "w",encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=4)
        msg = f"玩家 {playername} 的账户 {old_username} 已重命名为 {username}"
        return msg

    # 用于删除用户的函数
    def del_account(self,playername:str):
        """
        删除玩家账户
        playername: 玩家名
        """
        # 读取现有的经济数据
        with open(moneydata, "r",encoding='utf-8') as f:
            data = json.load(f)

        del data[playername]
        # 将修改后的数据写入文件
        with open(moneydata, "w",encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=4)
        msg = f"玩家 {playername} 的账户已被删除"
        return msg
    
    # 用于获取用户信息的函数
    def get_account_info(self,playername:str):
        """
        获取玩家账户信息,返回一个字典,其中提现物资也为字典
        playername: 玩家名
        """
        # 读取现有的经济数据
        with open(moneydata, "r",encoding='utf-8') as f:
            data = json.load(f)

        username = data[playername]["username"]
        money = data[playername]["money"]
        player_data = data[playername]
        msg = {
            "username": username,
            "money": money,
            "items": player_data.get("items", [])
        }
        return msg
    
    # 市场部分函数
    
    # 用于上架商品的函数
    def add_goods(self,playername:str,goodsname:str,goodsnum:int,needname:str,neednum:int,tips="商家没有留下商品介绍"):
        """
        上架商品信息
        playername: 玩家名
        goodsname: 上架的商品信息
        goodsnum: 上架的商品数量
        needname: 交易结算介质 可为货币也可为物品
        neednum: 交易结算介质需要数额
        tips: 商品介绍
        """
        # 读取现有的市场数据
        with open(marketdata, "r",encoding='utf-8') as f:
            data = json.load(f)
        
        # 当玩家没有商品时初始化玩家数据    
        if playername not in data:
            data[playername] = []

        # 上架商品信息
        #data[playername] = {"goodsname": goodsname, "goodsnum": goodsnum, "needname": needname, "neednum": neednum, "tips": tips}
        
        # 添加商品到玩家的商品列表
        item = {
            "goodsname": goodsname,
            "goodsnum": goodsnum,
            "needname": needname,
            "neednum": neednum,
            "tips": tips,
        }
        data[playername].append(item)
        
        # 将修改后的数据写入文件
        with open(marketdata, "w",encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=4)
        msg = f"您已成功上架{goodsnum}单位个{goodsname}"
        return msg

    # 用于检测玩家现在是否有商品在卖
    def testgoods(self,playername:str):
        """
        检测玩家是否有商品在卖,有商品返回true,没有返回false
        playername: 玩家名
        """  
        # 读取现有的经济数据
        with open(marketdata, "r",encoding='utf-8') as f:
            data = json.load(f)
        # 检查玩家是否注册
        if playername not in data:
            return False
        # 玩家商品数为0
        elif len(data[playername]) == 0:
            return False
        # 已注册且商品数不为0
        else:
            return True

    # 用于下架商品的函数
    def del_goods(self,playername:str,goodsname: str = None):
        """
        下架商品
        playername: 玩家名
        goodsname: 要下架的商品名
        """
        
        if goodsname == None:
            return "未指定下架商品!下架失败"
        
        # 读取现有的经济数据
        with open(marketdata, "r",encoding='utf-8') as f:
            data = json.load(f)

        player_goods = data[playername]
        
        # 只移除第一个匹配的商品
        for i, item in enumerate(player_goods):
            if item["goodsname"] == goodsname:
                del player_goods[i]
                msg = f"玩家 {playername} 的商品 {goodsname} 已下架"
                break
        
        if not data[playername]:  # 如果删除后列表为空，移除玩家数据
            del data[playername]
        msg = f"玩家 {playername} 的商品 {goodsname} 已下架"
        
        # 将修改后的数据写入文件
        with open(marketdata, "w",encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=4)
        msg = f"玩家 {playername} 的商品已下架"
        return msg
    
    # 用于获取商品信息的函数
    def get_goods_info(self,playername:str):
        """
        获取商品信息,返回一个列表值
        playername: 玩家名
        """
        # 读取现有的市场数据
        with open(marketdata, "r",encoding='utf-8') as f:
            data = json.load(f)

        # 遍历玩家的商品列表并构造消息
        goods_list = []
        for item in data[playername]:
            goods_info = {
                "goodsname": item.get("goodsname"),
                "goodsnum": item.get("goodsnum"),
                "needname": item.get("needname"),
                "neednum": item.get("neednum"),
                "tips": item.get("tips")
            }
            goods_list.append(goods_info)
            
        return goods_list
    # 检查买家是否具有购买条件的函数
    def check_shopping(self,buyer,needname,neednum):
        """
        检查买家是否具有购买条件
        buyer: 买家玩家名
        needname: 需要的交易媒介
        neednum: 交易媒介的数量
        """
        # 获取玩家背包
        all_item = []
        for item_num in range(36):
            item = self.server.get_player(buyer).inventory.get_item(item_num)
            if item:  # 如果槽位不为空
                item_name = item.type
                item_amount = item.amount
                all_item.append({'num': item_num, 'item': item_name, 'amount': item_amount})
        
        # 计算玩家背包中指定物品的总数量
        total_amount = 0
        for item_info in all_item:
            if item_info['item'] == needname:
                total_amount += item_info['amount']
        # 检查总数量是否满足价格
        if total_amount >= neednum:
            return True
        else:
            return False
            
    # 获取卖家快捷物品栏最右边的物品名称与数量的函数
    def get_seller_inventory(self,seller):
        """
        获取卖家快捷物品栏最右边的物品名称与数量
        seller: 卖家玩家名
        """
        try:
            # 获取玩家背包
            item = self.server.get_player(seller).inventory.get_item(8)
            if item:  # 如果槽位不为空
                goodsname = item.type
                goodsnum = item.amount
                return goodsname,goodsnum
            else:
                return None,None
        except:
            return None,None
    
    # 用于检查玩家上架商品数量上限的函数
    def check_if_goods_max(self,playername):
        """
        检查玩家上架商品数量是否达到上限 达到上限返回True,没有返回False
        playername: 玩家名
        """
        
        # 读取现有的市场数据
        with open(marketdata, "r", encoding="utf-8") as f:
            data = json.load(f)

        # 检查是否超过上架限制
        if len(data[playername]) >= 5:
            return True
        else:
            return False
    
    # 分割线
    commands = {
        "account": {
            "description": "对玩家账号进行操作 --用法 /account 玩家名 操作:[add less create rename del] 金额/用户名",
            "usages": ["/account [msg: message] [msg: message] [msg: message]"],
            "permissions": ["freemarket.command.changemoney"],
        },
        "market": {
            "description": "打开市场菜单",
            "usages": ["/market"],
            "permissions": ["freemarket.command.market"],
        }
    }
    permissions = {
        "freemarket.command.account": {
            "description": "对玩家账号进行操作",
            "default": "op", 
        },
        "freemarket.command.market": {
            "description": "打开市场菜单",
            "default": True, 
        }
    }

    def on_command(self, sender: CommandSender, command: Command, args: list[str]) -> bool:
        if command.name == "account":
            try:
                playername = args[0]
                action = args[1]
                # 检查玩家是否存在账号
                if self.testuser(playername) == False:
                    # 操作为create时创建默认账号
                    if action == "create":
                        msg = self.add_user(playername,username="默认昵称",money=0)
                    else:
                        sender.send_error_message(f"玩家 {playername} 没有服务器交易账号")
                        return
                else:
                    if action == "add":
                        msg = self.change_money(playername,action,money=float(args[2]))
                    elif action == "less":
                        msg = self.change_money(playername,action,money=float(args[2]))
                    elif action == "create":
                        sender.send_error_message("该玩家的账号已存在,无需创建")
                        return
                    elif action == "rename":
                        msg = self.rename_account(playername,username=args[2])
                    elif action == "del":
                        msg = self.del_account(playername)
            except:
                sender.send_error_message("格式错误")
                return
                
            sender.send_message(f"{msg}")
            
        elif command.name == "market":
            # 账户信息子菜单
            def account_info():
                def on_click(sender):
                    if self.testuser(sender.name) == True:
                        # 重命名按钮
                        rename_button = ActionForm.Button(text="§l§5重命名账户",icon="textures/ui/recap_glyph_color_2x",on_click=open_rename_sub())
                        get_sell_item_button = ActionForm.Button(text="§l§5货款提现",icon="textures/ui/trade_icon",on_click=open_get_sell_item_sub())
                        # 获取用户信息
                        account_info_msg = self.get_account_info(sender.name)
                        username = account_info_msg["username"]
                        money = account_info_msg["money"]
                        items = account_info_msg["items"]
                        all_item_info = ""
                        if items:
                            for item in items:
                                item_info = f" - {item['name']} {item['num']} 单位个\n"
                                all_item_info += item_info
                        else:
                            all_item_info = "无可提现货款"
                        # 用户信息展示
                        self.server.get_player(sender.name).send_form(ActionForm(
                            title=f"§l§5{sender.name}的用户信息",
                            content=f"§l§o§b用户名: {username}\n" + f"§l§o§b账户余额: {money}\n" + f"§l§o§b待提现货款:\n {all_item_info}",
                            buttons=[rename_button,get_sell_item_button]
                            )
                        )
                    else:
                        sender.send_error_message("§l您尚未拥有服务器交易账户,已为您自动创建")
                        sender.send_message(self.add_user(sender.name,username=f"§l默认昵称{sender.name}",money=0))
                return on_click
            # 账户重命名子菜单
            def open_rename_sub():
                def rename_sub(sender,json_str:str):
                    sender.send_message(self.rename_account(sender.name,json.loads(json_str)[0]))
                def on_click(sender):
                    self.server.get_player(sender.name).send_form(ModalForm(title="§4§l账户重命名界面",controls=[TextInput(label="§l新账户名",placeholder=self.get_account_info(sender.name)["username"],default_value=self.get_account_info(sender.name)["username"])],on_submit=rename_sub))
                return on_click
            
            # 货款提现子菜单
            def open_get_sell_item_sub():
                def get_sell_item_sub(sender,json_str:str):
                    action="less"
                    # 获取用户信息
                    account_info_msg = self.get_account_info(sender.name)
                    items = account_info_msg["items"]
                    if items:
                        for item in items:
                            item_name = item['name']
                            item_num = item['num']
                            self.server.dispatch_command(CommandSenderWrapper(self.server.command_sender), f'give "{sender.name}" {item_name} {item_num}')
                            self.change_other_money(sender.name,action,item_name,item_num)
                    
                    sender.send_message(f"§l您的货款已提现")
                def on_click(sender):
                    # 获取用户信息
                    account_info_msg = self.get_account_info(sender.name)
                    items = account_info_msg["items"]
                    all_item_info = ""
                    if items:
                        for item in items:
                            item_info = f" - {item['name']}: {item['num']} 单位个\n"
                            all_item_info += item_info
                        self.server.get_player(sender.name).send_form(ModalForm(title="§4§l提现界面菜单",controls=[Label(text=f"{ColorFormat.YELLOW}§l您的账户已收款:\n{all_item_info}\n点击下方按钮全部提现至您的物品栏中\n\n提现提示:\nminecraft:diamond为钻石\nminecraft:emerald为绿宝石\nminecraft:gold_ingot为金锭\nminecraft:iron_ingot为铁锭\nmoney为{moneyname}(服务器货币)")],on_submit=get_sell_item_sub))
                    else:
                        sender.send_error_message("您的账户暂无可提现的物品")
                        
                return on_click
            
            # 市场菜单
            def market_menu():
                def on_click(sender):
                    form = ActionForm(
                        title=f"{ColorFormat.DARK_PURPLE}§l交易市场",
                    )
                    with open(marketdata, "r", encoding='utf-8') as f:
                        goodsdata = json.load(f)
                    
                    # 动态添加按钮并设置回调
                    for playername, player_goods in goodsdata.items():
                        # 遍历每个玩家的商品列表
                        for item in player_goods:
                            # 从每个商品字典中提取信息
                            goodsname = item.get("goodsname")
                            goodsnum = item.get("goodsnum")
                            tips = item.get("tips")
                            needname = item.get("needname")
                            neednum = item.get("neednum")
                            username = self.get_account_info(playername)["username"]

                            # 创建回调函数
                            def create_callback(playername, username, goodsname, goodsnum, tips, needname, neednum):
                                def on_click(sender):
                                    def open_buy_sub(sender):
                                        # 购买确认子菜单逻辑
                                        def buy_sub(sender, json_str: str):
                                            buyer = sender.name
                                            # 使用货币付款
                                            if needname == "money" and neednum <= self.get_account_info(buyer)["money"]:
                                                #print(goodsname, goodsnum, needname, neednum, tips)
                                                # 买家扣款
                                                action = "less"
                                                #print(self.change_money(buyer, action, neednum))
                                                # 将货物给买家
                                                self.server.dispatch_command(CommandSenderWrapper(self.server.command_sender), f'give "{sender.name}" {goodsname} {goodsnum}')
                                                sender.send_message(f"{ColorFormat.YELLOW}§l付款成功")
                                                # 删除商品
                                                self.del_goods(playername,goodsname)
                                                # 给资金转给卖家
                                                action = "add"
                                                self.change_money(playername, action, neednum)
                                                self.server.broadcast_message(f"{ColorFormat.YELLOW}[市场推广] 商家【{username}】的商品【{tips}】已售空")
                                                # 通知卖家
                                                online_players = [p.name for p in self.server.online_players]
                                                if playername in online_players:
                                                    self.server.get_player(playername).send_message(f"{ColorFormat.YELLOW}§l玩家{sender.name}已购买了您的商品,请查看您的账户")
                                            # 使用其它物品付款
                                            elif self.check_shopping(buyer, needname, neednum) == True and not needname == "money":
                                                #print(goodsname, goodsnum, needname, neednum)
                                                # 买家扣款
                                                self.server.dispatch_command(CommandSenderWrapper(self.server.command_sender), f'clear "{sender.name}" {needname} 0 {neednum}')
                                                # 将货物给买家
                                                self.server.dispatch_command(CommandSenderWrapper(self.server.command_sender), f'give "{sender.name}" {goodsname} {goodsnum}')
                                                self.server.broadcast_message(f"§6§l[市场推广] §r§l商家【{username}】的商品【{tips}】已售空")
                                                # 将货款打到卖家账户上
                                                if self.change_other_money(playername, action="add", item_name=needname, num=neednum) == True:
                                                    sender.send_message(f"{ColorFormat.YELLOW}§l付款成功")
                                                    # 通知卖家
                                                    online_players = [p.name for p in self.server.online_players]
                                                    if playername in online_players:
                                                        self.server.get_player(playername).send_message(f"{ColorFormat.YELLOW}§l玩家{sender.name}已购买了您的商品,请查看您的账户")
                                                else:
                                                    sender.send_error_message("§l购买成功但是商家无法收款")

                                                self.del_goods(playername,goodsname)
                                            else:
                                                sender.send_error_message("§l您的资产不足,购买失败")

                                        self.server.get_player(sender.name).send_form(ModalForm(
                                            title=f"{ColorFormat.DARK_PURPLE}§l购买界面菜单",
                                            controls=[
                                                Label(text=f"§l§o§b店铺名:{username}的小店\n\n" + f"{ColorFormat.YELLOW}§l您将要购买的商品为 '{tips}'\n商家实名认证: {playername}\n物品ID为{goodsname}\n数量为 {goodsnum}\n价格为{needname} {neednum}单位.\n点击提交按钮确认交易,关闭窗口放弃交易\n\n交易提示:\nminecraft:diamond为钻石\nminecraft:emerald为绿宝石\nminecraft:gold_ingot为金锭\nminecraft:iron_ingot为铁锭\nmoney为{moneyname}(服务器货币)\n请勿交易各种附魔书、各种药水、不祥之瓶、附魔武器装备、药水箭、烟火及焰火炸药等带有特殊属性的物品,否则将导致属性丢失\n确保物品栏没有与货币物品同类且包含在上述禁止列表的物品,否则可能会被收错造成损失")
                                            ],
                                            on_submit=buy_sub
                                        ))

                                    open_buy_sub(sender)

                                return on_click

                            # 增加商品按钮
                            title = f"{ColorFormat.DARK_PURPLE}§l{tips}"
                            form.add_button(
                                title,
                                icon="textures/ui/sidebar_icons/Minions_packicon_0",
                                on_click=create_callback(playername, username, goodsname, goodsnum, tips, needname, neednum)
                            )
                    
                    self.server.get_player(sender.name).send_form(form)
                
                return on_click


            
            # 上架商品菜单
            def add_goods_menu():
                def on_click(sender):
                    form = ModalForm(
                        title="§l§b上架商品",
                        controls=[
                            Label(text=f"{ColorFormat.YELLOW}§l请将您要上架的商品放在快捷物品栏最右边,确认无误后点击上架按钮"),
                            Dropdown(label=f"§l选择交易结算方式(可选择钻石、绿宝石、黄金、铁锭、{moneyname}(服务器货币)或者自定义货币进行交易结算)",options=["钻石","绿宝石","黄金","铁锭",f"{moneyname}","自定义"]),
                            TextInput(label="§l当您选择自定义结算方式时,交易结算货币类型将以此处输入值为准,请确保该物品为Minecraft中的物品ID且无误,否则后果自负",placeholder="输入您的自定义结算物品ID",default_value="minecraft:emerald"),
                            Slider(label="§l选择结算货币数量(价格)",min=1,max=114,step=1)
                        ],
                        on_submit=open_add_sub
                    )
                    
                    if self.testuser(sender.name) == False:
                        sender.send_error_message(f"§l您还没有注册服务器交易账户,请点击账户信息注册")
                    # 先检查玩家是否有商品,有则测试是否上架超限
                    elif self.testgoods(sender.name) == True:
                        # 检查超限
                        if self.check_if_goods_max(sender.name) == True:
                            sender.send_error_message("§l您在卖的商品已达到上限,无法再上架新的商品")
                        # 没有超过
                        else:
                            self.server.get_player(sender.name).send_form(form)
                    else:
                        self.server.get_player(sender.name).send_form(form)
                return on_click
            
            # 上架确认子菜单
            def open_add_sub(player, json_str):
                seller = player.name  # 注意这里应该是使用传入的player参数，而非sender
                # 物品栏检查
                goodsname, goodsnum = self.get_seller_inventory(seller)
                if goodsname is None and goodsnum is None:
                    player.send_error_message("§l你的快捷物品栏最右边没有物品")
                    return

                # 获取商品信息
                goods_info_data = json.loads(json_str)

                # 当选择自定义时赋值为自定义值
                if goods_info_data[1] == 5:
                    needname = goods_info_data[2]
                else:
                    if goods_info_data[1] == 0:
                        needname = "minecraft:diamond"
                    elif goods_info_data[1] == 1:
                        needname = "minecraft:emerald"
                    elif goods_info_data[1] == 2:
                        needname = "minecraft:gold_ingot"
                    elif goods_info_data[1] == 3:
                        needname = "minecraft:iron_ingot"
                    elif goods_info_data[1] == 4:
                        needname = "money"
                neednum = int(goods_info_data[3])

                def add_sub(sender, json_str):
                    tips = json.loads(json_str)[1]
                    sender.send_message(self.add_goods(seller, goodsname, goodsnum, needname, neednum, tips))
                    self.server.dispatch_command(CommandSenderWrapper(self.server.command_sender), f'clear "{sender.name}" {goodsname} -1 {goodsnum}')
                    username = self.get_account_info(sender.name)["username"]
                    self.server.broadcast_message(f"§6§l[市场推广] §r§l商家【{username}】上架了商品【{tips}】")
                    

                # 创建新的 on_submit 回调函数，避免与外部函数冲突
                def submit_callback(sender):
                    form = ModalForm(
                        title="§4§l上架商品界面菜单",
                        controls=[
                            Label(text=f"{ColorFormat.YELLOW}§l您将要上架的商品为 {goodsname} 数量为 {goodsnum},价格为{needname} {neednum}单位.\n点击提交按钮确认交易,关闭窗口放弃交易\n\n上架提示:\n由于商品ID为游戏内的物品ID,玩家很难第一时间看懂商品名称,请在下方的输入框内输入商品名称和价格,以便顺利交易\n请勿交易各种附魔书、各种药水、不祥之瓶、附魔武器装备、药水箭、烟火及焰火炸药等带有特殊属性的物品,否则将导致属性丢失\n确保物品栏没有与上架物品同类且包含在上述禁止列表的物品,否则可能会被收错造成损失"),
                            TextInput(label=f"{ColorFormat.YELLOW}§l请输入商品信息", default_value="该商家很懒,没有留下商品信息")
                        ],
                        on_submit=add_sub
                    )
                    self.server.get_player(sender.name).send_form(form)

                submit_callback(player)  # 直接调用而不是返回
            
            # 下架商品菜单    
            def back_goods_menu():
                def back_goods(sender,json_str:str):
                    # 解析下架商品的索引
                    selected_index = int(json.loads(json_str)[0])  # 获取选中的商品索引
                    # 获取商品信息列表
                    try:
                        player_goods = self.get_goods_info(sender.name)
                    except: player_goods = []
                    # 排错
                    if not player_goods or selected_index >= len(player_goods):
                        sender.send_error_message("§l商品数据异常或商品不存在，无法下架")
                        return
                    # 获取商品信息
                    goods_to_remove = player_goods[selected_index]
                    goodsname = goods_to_remove["goodsname"]
                    goodsnum = goods_to_remove["goodsnum"]
                    tips =  goods_to_remove["tips"]
                    # 下架
                    self.server.dispatch_command(CommandSenderWrapper(self.server.command_sender), f'give "{sender.name}" {goodsname} {goodsnum}')
                    # 调用函数下架商品
                    self.del_goods(sender.name,goodsname)
                    sender.send_message(f"{ColorFormat.YELLOW}§l您的商品{goodsname}已下架并退还")
                    #self.server.broadcast_message(f"{ColorFormat.YELLOW}[市场推广] 商家【{self.get_account_info(sender.name)["username"]}】下架了商品【{tips}】")
                def on_click(sender):
                    if self.testuser(sender.name) == False:
                        sender.send_error_message(f"§l您还没有注册服务器交易账户,请点击账户信息注册")
                    elif self.testgoods(sender.name) == False:
                        sender.send_error_message("§l您没有物品在卖!")
                    else:
                        # 获取玩家商品信息
                        player_goods = self.get_goods_info(sender.name)
                        # 构建商品列表菜单
                        options = []
                        for idx, goods in enumerate(player_goods):
                            tips = goods["tips"]
                            goodsname = goods["goodsname"]
                            goodsnum = goods["goodsnum"]
                            options.append(f"商品 {idx + 1}: {tips} (ID: {goodsname}, 数量: {goodsnum})")
                        # 构建下架确认菜单
                        form = ModalForm(
                            title=f"{ColorFormat.DARK_PURPLE}§l下架商品菜单",
                            controls=[
                                #Label(text=f"{ColorFormat.YELLOW}§l您目前有'{self.get_goods_info(sender.name)["tips"]}' 物品ID为{self.get_goods_info(sender.name)["goodsname"]} {self.get_goods_info(sender.name)["goodsnum"]}单位在卖\n是否下架并退还该商品?点击下方按钮确认下架"),
                                Dropdown(label="§l选择您要下架的商品", options=options)
                            ],
                            on_submit=back_goods
                        )
                        self.server.get_player(sender.name).send_form(form)
                return on_click
            
            # 账户信息按钮
            button_account_info = ActionForm.Button(text="§l§5账户信息",icon="textures/ui/icon_steve",on_click=account_info())
            # 交易市场
            button_market = ActionForm.Button(text="§l§5交易市场",icon="textures/ui/teams_icon",on_click=market_menu())
            # 商品上架
            button_addgoods = ActionForm.Button(text="§l§5上架商品",icon="textures/ui/icon_blackfriday",on_click=add_goods_menu())
            # 商品下架并退款
            button_backgoods = ActionForm.Button(text="§l§5下架商品并退还商品",icon="textures/ui/icon_trash",on_click=back_goods_menu())
            # 市场主菜单
            form = ActionForm(
                title=f"{ColorFormat.DARK_PURPLE}§l市场菜单",
                content=f"§l§o§b欢迎光~临! 可以通过输入/market命令或者绿宝石点地打开菜单",
                buttons=[button_account_info,button_market,button_addgoods,button_backgoods]
            )
            self.server.get_player(sender.name).send_form(form)

        return True
    
    # 用于绿宝石开启菜单
    @event_handler
    def open_menu(self,event:PlayerInteractEvent):
        current_time = time.time()
        if current_time - self.last_command_time < 2:  # 2秒内不重复执行
            return
        item_str = str(event.item)
        item_name = r"ItemStack\(minecraft:emerald\s+x\s*\d+\)"
        if re.match(item_name, item_str):
            player = event.player
            player.perform_command("market")
            self.last_command_time = current_time  # 更新上次执行时间
            return