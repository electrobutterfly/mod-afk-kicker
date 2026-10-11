/*
 * This file is part of the AFK Kicker Module subsystem for AzerothCore.
 *
 * Copyright (C) 2026 Klaus Simon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "Config.h"
#include "Log.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Timer.h"
#include "WorldSession.h"
#include "WorldSessionMgr.h"

#include <string>
#include <unordered_map>

class AFKKicker_WorldScript : public WorldScript
{
public:
    AFKKicker_WorldScript()
        : WorldScript("AFKKicker_WorldScript",
                      { WORLDHOOK_ON_UPDATE, WORLDHOOK_ON_BEFORE_WORLD_INITIALIZED })
    { }

    // OnStartup would be the natural place for this, but this fork never
    // dispatches WORLDHOOK_ON_STARTUP, so use OnBeforeWorldInitialized.
    void OnBeforeWorldInitialized() override
    {
        LOG_INFO("server.loading",
            "AFK Kicker module loaded. Enabled: {}, Check interval: {} ms, Kick time: {} ms, Kick GMs: {}",
            sConfigMgr->GetOption<bool>("AFK.Enabled", true),
            sConfigMgr->GetOption<uint32>("AFK.CheckInterval", 30000),
            sConfigMgr->GetOption<uint32>("AFK.KickTime", 1800000),
            sConfigMgr->GetOption<bool>("AFK.KickGM", false));
    }

    void OnUpdate(uint32 diff) override
    {
        if (!sConfigMgr->GetOption<bool>("AFK.Enabled", true))
            return;

        _updateTimer += diff;

        uint32 checkInterval = sConfigMgr->GetOption<uint32>("AFK.CheckInterval", 30000);
        if (_updateTimer < checkInterval)
            return;

        _updateTimer = 0;

        uint32 kickTime = sConfigMgr->GetOption<uint32>("AFK.KickTime", 1800000);
        bool   kickGMs  = sConfigMgr->GetOption<bool>("AFK.KickGM", false);

        LOG_INFO("server.loading",
            "AFK check tick -- kickTime={}ms kickGMs={}", kickTime, kickGMs);

        WorldSessionMgr::SessionMap const& sessions = sWorldSessionMgr->GetAllSessions();

        LOG_INFO("server.loading", "AFK session count this tick: {}", sessions.size());

        for (auto const& sessionPair : sessions)
        {
            WorldSession* session = sessionPair.second;
            if (!session)
                continue;

            Player* player = session->GetPlayer();
            if (!player)
                continue;

            LOG_INFO("server.loading",
                "AFK: player='{}' guid={} isAFK={} security={}",
                player->GetName(),
                player->GetGUID().GetCounter(),
                player->isAFK(),
                static_cast<uint32>(session->GetSecurity()));

            if (!kickGMs && session->GetSecurity() > SEC_PLAYER)
                continue;

            ObjectGuid guid = player->GetGUID();

            if (player->isAFK())
            {
                auto it = _afkStartTimes.find(guid);
                if (it == _afkStartTimes.end())
                {
                    _afkStartTimes[guid] = getMSTime();
                    LOG_INFO("server.loading",
                        "AFK: started AFK timer for '{}' at {}ms",
                        player->GetName(), _afkStartTimes[guid]);
                }
                else
                {
                    uint32 afkDuration = getMSTimeDiff(it->second, getMSTime());
                    LOG_INFO("server.loading",
                        "AFK: duration for '{}' = {}ms (kick at {}ms)",
                        player->GetName(), afkDuration, kickTime);

                    if (afkDuration >= kickTime)
                    {
                        LOG_INFO("server.loading",
                            "AFK: Kicking player {} (GUID: {}) for being AFK for {} minutes.",
                            player->GetName(), guid.GetCounter(), afkDuration / 60000);

                        std::string kickReason =
                            "AFK for " + std::to_string(afkDuration / 60000) + " minutes";
                        session->KickPlayer(kickReason);
                        _afkStartTimes.erase(it);
                    }
                }
            }
            else
            {
                auto it = _afkStartTimes.find(guid);
                if (it != _afkStartTimes.end())
                    _afkStartTimes.erase(it);
            }
        }
    }

private:
    uint32 _updateTimer = 0;
    std::unordered_map<ObjectGuid, uint32> _afkStartTimes;
};

void Addmod_afk_kickerScripts()
{
    new AFKKicker_WorldScript();
}
