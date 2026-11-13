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
#include "ArchaeologyPackets.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "GameTime.h"
#include "Log.h"
#include "Player.h"

void Archaeology::LoadCompletedProjectsFromDB(PreparedQueryResult result)
{
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint16 project = fields[0].Get<uint16>();
        int32 time = int32(fields[1].Get<uint32>());
        int32 count = int32(fields[2].Get<uint32>());

        _completedProjects.emplace(project, std::make_pair(time, count));
    } while (result->NextRow());
}

void Archaeology::InitBranches()
{
    for (ResearchBranchEntry const* branch : sResearchBranchStore)
    {
        if (branch->CurrencyID)
        {
            BranchData& branchData = _branches[uint8(branch->ID)];
            branchData.CompletedRares = 0;
            branchData.Project = 0;
        }
    }
}

void Archaeology::LoadCurrentProjectsFromDB(PreparedQueryResult result)
{
    InitBranches();

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint8 branch = fields[0].Get<uint8>();
        uint16 project = fields[1].Get<uint16>();

        if (uint16 old_project = _branches[branch].Project)
        {
            LOG_ERROR("player.skills", "Archaeology: Tried to assign project {} to branch {}, which already holds project {}!", project, branch, old_project);
            continue;
        }

        BranchData& branchData = _branches[branch];
        branchData.Project = project;
    } while (result->NextRow());
}

void Archaeology::VisualizeBranch(uint8 position, uint16 project)
{
    if (position < MAX_PROJECTS)
        _player->SetUInt16Value(PLAYER_FIELD_RESEARCH_PROJECT_1 + (position / 2), position % 2, project);
}

void Archaeology::RegenerateBranch(uint8 branch)
{
    uint16 project = sArchaeologyMgr->GetNewProject(branch, &_branches[branch], &_completedProjects, _player->GetSkillValue(SKILL_ARCHAEOLOGY));
    _branches[branch].Project = project;

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_CHAR_ARCHAEOLOGY_PROJECT);
    stmt->SetData(0, _player->GetGUID().GetCounter());
    stmt->SetData(1, branch);
    stmt->SetData(2, project);
    CharacterDatabase.Execute(stmt);
}

void Archaeology::VerifyProjects()
{
    for (std::map<uint8, BranchData>::iterator itr = _branches.begin(); itr != _branches.end(); ++itr)
    {
        if (BranchEntry const* branch = sArchaeologyMgr->GetBranchEntry(itr->first))
        {
            uint16 project = itr->second.Project;
            if (project)
            {
                if (_completedProjects.find(project) != _completedProjects.end() && sArchaeologyMgr->IsRareProject(project))
                    RegenerateBranch(itr->first);
            }
            else if (_player->GetCurrency(branch->currencyId, false))
                RegenerateBranch(itr->first);
            else
                continue;

            VisualizeBranch(branch->fieldId, _branches[itr->first].Project);
        }
    }
}

void Archaeology::CleanProjects()
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ARCHAEOLOGY_PROJECTS);
    stmt->SetData(0, _player->GetGUID().GetCounter());
    CharacterDatabase.Execute(stmt);

    for (uint8 i = 0; i < MAX_PROJECTS; ++i)
        VisualizeBranch(i, 0);

    _branches.clear();
}

void Archaeology::ActivateBranch(uint8 branchId, bool force)
{
    if (_branches[branchId].Project && !force)
        return;

    RegenerateBranch(branchId);
    VisualizeBranch(sArchaeologyMgr->GetBranchEntry(branchId)->fieldId, _branches[branchId].Project);
}

bool Archaeology::ProjectExists(uint16 projectId)
{
    if (ProjectEntry const* project = sArchaeologyMgr->GetProjectEntry(projectId))
        return (_branches[project->branch].Project == projectId);

    return false;
}

bool Archaeology::ProjectCompleteable(uint16 projectId)
{
    if (!_archData)
        return false;

    ProjectEntry const* project = sArchaeologyMgr->GetProjectEntry(projectId);
    if (!project)
        return false;

    if (_branches[project->branch].Project != projectId)
        return false;

    BranchEntry const* branchEntry = sArchaeologyMgr->GetBranchEntry(project->branch);
    if (!branchEntry)
        return false;

    // Check Ids
    uint32 currencyId = branchEntry->currencyId;
    uint32 keystoneId = branchEntry->keystoneId;
    if (currencyId != _archData->FragId || (_archData->KeyId && _archData->KeyId != keystoneId))
        return false;

    // Project can have keystones ?
    if (project->keystone < _archData->KeyCount)
        return false;

    // Check if the given fragment/keystone usage would be enough
    if (_archData->FragCount + _archData->KeyCount * 12 != project->fragments)
        return false;

    // Check if the player has the keystones
    if (_player->GetCurrency(_archData->FragId, false) < _archData->FragCount)
        return false;

    // Check if the player has the fragments
    return (!_archData->KeyId || _player->GetItemCount(_archData->KeyId) >= _archData->KeyCount);
}

void Archaeology::CompleteProject(uint16 projectId)
{
    ProjectEntry const* project = sArchaeologyMgr->GetProjectEntry(projectId);
    ASSERT(project);
    ASSERT(_branches[project->branch].Project == projectId);
    ASSERT(_archData);

    _player->ModifyCurrency(_archData->FragId, -int32(_archData->FragCount));
    _player->DestroyItemCount(_archData->KeyId, _archData->KeyCount, true, false);

    int32 completionTime = GameTime::GetGameTime().count();

    WorldPackets::Archaeology::ResearchComplete packet;
    packet.Research.CompletionCount = 1;
    packet.Research.FirstCompleted = completionTime;
    packet.Research.ProjectID = projectId;
    _player->SendDirectMessage(packet.Write());

    auto [itr, inserted] = _completedProjects.try_emplace(projectId, completionTime, 0);
    ++itr->second.second;

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_CHAR_ARCHAEOLOGY_COMPLETED);
    stmt->SetData(0, _player->GetGUID().GetCounter());
    stmt->SetData(1, projectId);
    stmt->SetData(2, uint32(itr->second.first));
    stmt->SetData(3, uint32(itr->second.second));
    CharacterDatabase.Execute(stmt);

    _player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ARCHAEOLOGY_PROJECTS, projectId, 1);

    _archData.reset();
    ActivateBranch(project->branch, true);
}

void Archaeology::SetArchData(ArchData const& data)
{
    _archData = std::make_unique<ArchData>(data);
}

void Archaeology::SendResearchHistory()
{
    WorldPackets::Archaeology::SetupResearchHistory packet;
    for (std::pair<uint16, std::pair<int32, int32>> itr : _completedProjects)
        packet.History.emplace_back(itr.first, itr.second.second, itr.second.first);

    _player->SendDirectMessage(packet.Write());
}
