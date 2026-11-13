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

#include "Archaeology.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "GameObject.h"
#include "Language.h"
#include "Map.h"
#include "Player.h"
#include "Random.h"

// use squared distances so we don't have to use the squareroot
enum DigSiteDist
{
    DIST_FAR            = 10000,    // 100 yards
    DIST_MED            = 900,      // 30  yards
    DIST_CLOSE          = 100,      // 10  yards
};

enum SurveyBot
{
    FAR_SURVEYBOT       = 206590,
    MED_SURVEYBOT       = 206589,
    CLOSE_SURVEYBOT     = 204272,
};

void Archaeology::LoadSitesFromDB(PreparedQueryResult result)
{
    if (!result)
        return;

    do
    {
        Field *fields = result->Fetch();
        uint32 site = fields[0].Get<uint8>();
        uint16 type = fields[1].Get<uint16>();
        uint8 finds = fields[2].Get<uint8>();
        if (site >= _site.size())
            continue;

        SetSite(site, type, finds);
    } while (result->NextRow());
}

void Archaeology::VerifySites()
{
    uint16 skillValue = _player->GetSkillValue(SKILL_ARCHAEOLOGY);

    if (skillValue >= SKILL_OUTLAND)
        _continentState[CONT_OUTLAND] = STATE_USE;
    else
        _continentState[CONT_OUTLAND] = STATE_NULL;

    if (skillValue >= SKILL_NORTHREND)
        _continentState[CONT_NORTHREND] = STATE_USE;
    else
        _continentState[CONT_NORTHREND] = STATE_NULL;

    if (skillValue >= SKILL_CATA)
    {
        _continentState[CONT_EASTERN] = STATE_EXT;
        _continentState[CONT_KALIMDOR] = STATE_EXT;
    }
    else
    {
        _continentState[CONT_EASTERN] = STATE_USE;
        _continentState[CONT_KALIMDOR] = STATE_USE;
    }

    RegenerateAllSites();
}

void Archaeology::UseSite()
{
    float dist = 0.0f;
    uint32 position = GetNearestSite(dist);
    uint32 surveyGoID = 0;

    if (0 == position)
        return;

    position--;

    float o = _player->GetOrientation();
    float x = _player->GetPositionX() + cos(o) * 2.0f;
    float y = _player->GetPositionY() + sin(o) * 2.0f;
    float z = _player->GetPositionZ();
    float ground = _player->GetMap()->GetHeight(_player->GetPhaseMask(), x, y, z + 5.0f);
    float angle = 0.0f;

    if (dist > float(DIST_CLOSE))
    {
        angle = std::atan2(_site[position].Y - _player->GetPositionY(), _site[position].X - _player->GetPositionX());
        static float const pi = float(M_PI);

        if (dist < float(DIST_MED))
        {
            surveyGoID = CLOSE_SURVEYBOT;     // within the med radius -> green light
            angle += frand(-pi / 12.f, pi / 12.f);
        }
        else if (dist < float(DIST_FAR))
        {
            surveyGoID = MED_SURVEYBOT;       // within the far radius -> yellow light
            angle += frand(-pi / 6.f, pi / 6.f);
        }
        else
        {
            surveyGoID = FAR_SURVEYBOT;       // outside far radius -> red light
            angle += frand(-pi / 3.f, pi / 3.f);
        }
    }

    if ((std::abs(z - ground) >= 5.0f) || !_player->IsWithinLOS(x, y, ground))
    {
        x = _player->GetPositionX();
        y = _player->GetPositionY();
        ground = z;
    }

    // Using survey bot
    if (surveyGoID)
    {
        _player->SummonGameObject(surveyGoID, x, y, ground, angle, 0.f, 0.f, std::sin(angle / 2.f), std::cos(angle / 2.f), 4);
    }
    else
    {
        // Found a artifact. Let's spawn it.
        uint32 goId = sArchaeologyMgr->GetSiteType(_site[position].Entry);
        angle = frand(0.0f, float(M_PI * 2));

        _player->SummonGameObject(goId, x, y, ground, o, 0.f, 0.f, std::sin(angle / 2.f), std::cos(angle / 2.f), MINUTE);
        (_site[position].State)++;

        if (_site[position].State >= DIGS_PER_SITE)
            RegeneratePosition(position, GetContinent());
        else
        {
            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_ARCHAEOLOGY_SITE_FINDS);
            stmt->SetData(0, uint8(_site[position].State));
            stmt->SetData(1, _player->GetGUID().GetCounter());
            stmt->SetData(2, uint8(position));
            CharacterDatabase.Execute(stmt);
            sArchaeologyMgr->SetSiteCoords(_site[position]);
        }
    }
}

Continent Archaeology::GetContinent()
{
    switch (_player->GetMapId())
    {
        case 0:
            return CONT_EASTERN;
        case 1:
            return CONT_KALIMDOR;
        case 530:
            return CONT_OUTLAND;
        case 571:
            return CONT_NORTHREND;
    }
    return COUNT_CONT;
}

uint32 Archaeology::GetNearestSite(float &distance)
{
    uint32 cont = GetContinent();

    if (COUNT_CONT == cont)
        return 0;

    uint32 position = cont * CONTINENT_SITES;

    float pX = _player->GetPositionX();
    float pY = _player->GetPositionY();

    float distSq = (pX - _site[position].X) * (pX - _site[position].X) + (pY - _site[position].Y) * (pY - _site[position].Y);

    for (uint32 i = cont * CONTINENT_SITES + 1; i < (cont + 1) * CONTINENT_SITES; ++i)
    {
        float distSq2 = (pX -_site[i].X) * (pX -_site[i].X) + (pY - _site[i].Y) * (pY - _site[i].Y);
        if (distSq2 < distSq)
        {
            position = i;
            distSq = distSq2;
        }
    }
    distance = distSq;

    return position + 1;
}

void Archaeology::SetSite(uint32 position, uint16 entry, uint32 state)
{
    _site[position].Entry = entry;
    _site[position].State = state;
    _player->SetUInt16Value(PLAYER_FIELD_RESEARCH_SITE_1 + (position / 2), position % 2, entry);

    if (entry == 0)
    {
        _site[position].X = 0.f;
        _site[position].Y = 0.f;
    }
    else
        sArchaeologyMgr->SetSiteCoords(_site[position]);
}

void Archaeology::RegeneratePosition(uint32 position, Continent continent)
{
    if (_site[position].Entry && sResearchSiteStore.LookupEntry(_site[position].Entry) && _site[position].State < DIGS_PER_SITE)
        return;

    uint16 entry = sArchaeologyMgr->GetNewSite(continent, _site, _continentState[continent] == STATE_EXT, _player->GetLevel());
    SetSite(position, entry);
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_CHAR_ARCHAEOLOGY_SITE);
    stmt->SetData(0, _player->GetGUID().GetCounter());
    stmt->SetData(1, uint8(position));
    stmt->SetData(2, entry);
    stmt->SetData(3, uint8(_site[position].State));
    CharacterDatabase.Execute(stmt);
}

void Archaeology::RegenerateContinent(Continent continent)
{
    if (_continentState[continent] == STATE_NULL)
    {
        uint32 position = continent * CONTINENT_SITES;

        for (int i = 0; i < CONTINENT_SITES; i++)
            SetSite(position + i, 0);
        return;
    }

    uint32 position = continent * CONTINENT_SITES;

    for (uint32 i = 0; i < CONTINENT_SITES; i++)
        RegeneratePosition(position + i, continent);
}

void Archaeology::RegenerateAllSites()
{
    for (uint32 i = 0; i < COUNT_CONT; i++)
        RegenerateContinent(Continent(i));
}
