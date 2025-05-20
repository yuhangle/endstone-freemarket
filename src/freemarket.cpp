//
// Created by yuhang on 2025/4/13.
//

#include "freemarket.h"

ENDSTONE_PLUGIN("freemarket", "0.1.0dev5", FreeMarket)
{
    description = "An Endstone plugin for player to use in free market trading";

    command("market")
            .description("Open market menu")
            .usages("/market",
                    "/market <register> <username: message> [avatar: message]",
                    "/market <money> <add|less> <player:target> <money: int>"
                    )
            .permissions("fm.command.member");

    permission("fm.command.member")
            .description("member command")
            .default_(endstone::PermissionDefault::True);

    permission("fm.command.op")
            .description("op command")
            .default_(endstone::PermissionDefault::Operator);
}