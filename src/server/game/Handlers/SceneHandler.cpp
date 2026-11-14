/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// 6.x scene events, from TrinityCore 6.x (SceneHandler.cpp)

#include "Log.h"
#include "ScenePackets.h"
#include "WorldSession.h"

void WorldSession::HandleSceneTriggerEvent(WorldPackets::Scenes::SceneTriggerEvent& sceneTriggerEvent)
{
    LOG_DEBUG("scenes", "HandleSceneTriggerEvent: SceneInstanceID: {} Event: {}", sceneTriggerEvent.SceneInstanceID, sceneTriggerEvent._Event);
}

void WorldSession::HandleScenePlaybackComplete(WorldPackets::Scenes::ScenePlaybackComplete& scenePlaybackComplete)
{
    LOG_DEBUG("scenes", "HandleScenePlaybackComplete: SceneInstanceID: {}", scenePlaybackComplete.SceneInstanceID);
}

void WorldSession::HandleScenePlaybackCanceled(WorldPackets::Scenes::ScenePlaybackCanceled& scenePlaybackCanceled)
{
    LOG_DEBUG("scenes", "HandleScenePlaybackCanceled: SceneInstanceID: {}", scenePlaybackCanceled.SceneInstanceID);
}
