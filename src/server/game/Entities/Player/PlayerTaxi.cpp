/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ObjectMgr.h"
#include "Player.h"
#include "StringConvert.h"
#include "TaxiPackets.h"
#include "Tokenize.h"

void PlayerTaxi::InitTaxiNodesForLevel(uint32 race, uint32 chrClass, uint8 level)
{
    // class specific initial known nodes
    TaxiMask const& factionMask = Player::TeamIdForRace(race) == TEAM_HORDE ? sHordeTaxiNodesMask : sAllianceTaxiNodesMask;
    switch (chrClass)
    {
        case CLASS_DEATH_KNIGHT:
        {
            for (uint8 i = 0; i < TaxiMaskSize; ++i)
                m_taximask[i] |= sOldContinentsNodesMask[i] & factionMask[i];
            break;
        }
    }

    // race specific initial known nodes: capital and taxi hub masks
    switch (race)
    {
        case RACE_HUMAN:
        case RACE_DWARF:
        case RACE_NIGHTELF:
        case RACE_GNOME:
        case RACE_DRAENEI:
        case RACE_WORGEN:
        case RACE_PANDAREN_ALLIANCE:
            SetTaximaskNode(2);     // Stormwind, Elwynn
            SetTaximaskNode(6);     // Ironforge, Dun Morogh
            SetTaximaskNode(26);    // Lor'danel, Darkshore
            SetTaximaskNode(27);    // Rut'theran Village, Teldrassil
            SetTaximaskNode(49);    // Moonglade (Alliance)
            SetTaximaskNode(94);    // The Exodar
            SetTaximaskNode(456);   // Dolanaar, Teldrassil
            SetTaximaskNode(457);   // Darnassus, Teldrassil
            SetTaximaskNode(582);   // Goldshire, Elwynn
            SetTaximaskNode(589);   // Eastvale Logging Camp, Elwynn
            SetTaximaskNode(619);   // Kharanos, Dun Morogh
            SetTaximaskNode(620);   // Gol'Bolar Quarry, Dun Morogh
            SetTaximaskNode(624);   // Azure Watch, Azuremyst Isle
            break;
        case RACE_ORC:
        case RACE_UNDEAD_PLAYER:
        case RACE_TAUREN:
        case RACE_TROLL:
        case RACE_BLOODELF:
        case RACE_GOBLIN:
        case RACE_PANDAREN_HORDE:
            SetTaximaskNode(11);    // Undercity, Tirisfal
            SetTaximaskNode(22);    // Thunder Bluff, Mulgore
            SetTaximaskNode(23);    // Orgrimmar, Durotar
            SetTaximaskNode(69);    // Moonglade (Horde)
            SetTaximaskNode(82);    // Silvermoon City
            SetTaximaskNode(384);   // The Bulwark, Tirisfal
            SetTaximaskNode(402);   // Bloodhoof Village, Mulgore
            SetTaximaskNode(460);   // Brill, Tirisfal Glades
            SetTaximaskNode(536);   // Sen'jin Village, Durotar
            SetTaximaskNode(537);   // Razor Hill, Durotar
            SetTaximaskNode(625);   // Fairbreeze Village, Eversong Woods
            SetTaximaskNode(631);   // Falconwing Square, Eversong Woods
            break;
    }

    // new continent starting masks (It will be accessible only at new map)
    switch (Player::TeamIdForRace(race))
    {
        case TEAM_ALLIANCE: SetTaximaskNode(100); break;
        case TEAM_HORDE:    SetTaximaskNode(99);  break;
    }

    // level dependent taxi hubs
    if (level >= 68)
        SetTaximaskNode(213);                               //Shattered Sun Staging Area
}

bool PlayerTaxi::LoadTaxiMask(std::string_view data)
{
    bool warn = false;
    std::vector<std::string_view> tokens = Acore::Tokenize(data, ' ', false);

    // 6.x saves one byte per token; older characters saved 32-bit words, which are split into 4 bytes
    bool const legacyWords = tokens.size() != TaxiMaskSize;
    std::size_t const bytesPerToken = legacyWords ? 4 : 1;

    m_taximask.fill(0);
    for (std::size_t token = 0; token < tokens.size(); ++token)
    {
        Optional<uint32> value = Acore::StringTo<uint32>(tokens[token]);
        if (!value)
        {
            warn = true;
            continue;
        }

        for (std::size_t b = 0; b < bytesPerToken; ++b)
        {
            std::size_t index = token * bytesPerToken + b;
            uint8 mask = uint8(*value >> (b * 8));
            if (index >= TaxiMaskSize)
            {
                if (mask)
                    warn = true;
                continue;
            }

            // load and set bits only for existing taxi nodes
            m_taximask[index] = sTaxiNodesMask[index] & mask;
            if (m_taximask[index] != mask)
                warn = true;
        }
    }

    return !warn;
}

void PlayerTaxi::AppendTaximaskTo(WorldPackets::Taxi::ShowTaxiNodes& data, bool all)
{
    if (all)
        data.Nodes = &sTaxiNodesMask;              // all existed nodes
    else
        data.Nodes = &m_taximask;                  // known nodes
}

bool PlayerTaxi::LoadTaxiDestinationsFromString(std::string const& values, TeamId teamId)
{
    ClearTaxiDestinations();

    std::vector<std::string_view> tokens = Acore::Tokenize(values, ' ', false);
    auto itr = tokens.begin();
    if (itr != tokens.end())
    {
        if (Optional<uint32> faction = Acore::StringTo<uint32>(*itr))
        {
            m_flightMasterFactionId = *faction;
        }
        else
        {
            return false;
        }
    }
    else
        return false;

    while ((++itr) != tokens.end())
    {
        if (Optional<uint32> node = Acore::StringTo<uint32>(*itr))
        {
            AddTaxiDestination(*node);
        }
        else
        {
            return false;
        }
    }

    if (m_TaxiDestinations.empty())
    {
        return true;
    }

    // Check integrity
    if (m_TaxiDestinations.size() < 2)
    {
        return false;
    }

    for (std::size_t i = 1; i < m_TaxiDestinations.size(); ++i)
    {
        uint32 cost;
        uint32 path;
        sObjectMgr->GetTaxiPath(m_TaxiDestinations[i - 1], m_TaxiDestinations[i], path, cost);
        if (!path)
        {
            return false;
        }
    }

    // can't load taxi path without mount set (quest taxi path?)
    if (!sObjectMgr->GetTaxiMountDisplayId(GetTaxiSource(), teamId, true))
    {
        return false;
    }

    return true;
}

std::string PlayerTaxi::SaveTaxiDestinationsToString()
{
    if (m_TaxiDestinations.empty())
    {
        return "";
    }

    ASSERT(m_TaxiDestinations.size() >= 2);

    std::ostringstream ss;
    ss << m_flightMasterFactionId << ' ';

    for (std::size_t i = 0; i < m_TaxiDestinations.size(); ++i)
    {
        ss << m_TaxiDestinations[i] << ' ';
    }

    return ss.str();
}

uint32 PlayerTaxi::GetCurrentTaxiPath() const
{
    if (m_TaxiDestinations.size() < 2)
    {
        return 0;
    }

    uint32 path;
    uint32 cost;

    sObjectMgr->GetTaxiPath(m_TaxiDestinations[0], m_TaxiDestinations[1], path, cost);

    return path;
}

std::ostringstream& operator<< (std::ostringstream& ss, PlayerTaxi const& taxi)
{
    for (uint8 i = 0; i < TaxiMaskSize; ++i)
        ss << uint32(taxi.m_taximask[i]) << ' ';
    return ss;
}

bool PlayerTaxi::RequestEarlyLanding()
{
    if (m_TaxiDestinations.size() <= 2)
        return false;

    // start from first destination - m_TaxiDestinations[0] is the current starting node
    for (std::deque<uint32>::iterator it = ++m_TaxiDestinations.begin(); it != m_TaxiDestinations.end(); ++it)
    {
        if (IsTaximaskNodeKnown(*it))
        {
            if (++it == m_TaxiDestinations.end())
                return false;   // if we are left with only 1 known node on the path don't change the spline, its our final destination anyway

            m_TaxiDestinations.erase(it, m_TaxiDestinations.end());
            return true;
        }
    }

    return false;
}

FactionTemplateEntry const* PlayerTaxi::GetFlightMasterFactionTemplate() const
{
    return sFactionTemplateStore.LookupEntry(m_flightMasterFactionId);
}
