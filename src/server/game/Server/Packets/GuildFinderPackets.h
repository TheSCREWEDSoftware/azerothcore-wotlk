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

#ifndef GuildFinderPackets_h__
#define GuildFinderPackets_h__

#include "GuildFinderMgr.h"
#include "ObjectGuid.h"
#include "Packet.h"
#include <vector>

namespace WorldPackets
{
    namespace GuildFinder
    {
        // CMSG_LF_GUILD_ADD_RECRUIT - a guildless player files a membership request against a guild.
        class AddRecruit final : public ClientPacket
        {
        public:
            AddRecruit(WorldPacket&& packet) : ClientPacket(CMSG_LF_GUILD_ADD_RECRUIT, std::move(packet)) { }

            void Read() override;

            ObjectGuid GuildGuid;
            uint32 ClassRoles = 0;
            uint32 Interests = 0;
            uint32 Availability = 0;
            std::string Comment;
            // The client also sends its own name here; the real 4.3.4 server never uses it (the
            // applicant is always the sender of the packet), so it's read and discarded, exactly
            // like upstream TrinityCore's HandleGuildFinderAddRecruit.
        };

        // CMSG_LF_GUILD_BROWSE - a guildless player searches for guilds matching their own criteria.
        class Browse final : public ClientPacket
        {
        public:
            Browse(WorldPacket&& packet) : ClientPacket(CMSG_LF_GUILD_BROWSE, std::move(packet)) { }

            void Read() override;

            uint32 ClassRoles = 0;
            uint32 Availability = 0;
            uint32 Interests = 0;
            uint32 PlayerLevel = 0;
        };

        // SMSG_LF_GUILD_BROWSE_UPDATED - response to Browse, one entry per matching guild.
        struct BrowseEntry
        {
            ObjectGuid GuildGuid;
            std::string GuildName;
            std::string Comment;
            uint32 EmblemStyle = 0;
            uint32 EmblemColor = 0;
            uint32 BorderStyle = 0;
            uint32 BorderColor = 0;
            uint32 BackgroundColor = 0;
            uint32 GuildLevel = 0;
            uint32 MemberCount = 0;
            uint32 AchievementPoints = 0;
            uint32 Interests = 0;
            uint32 Availability = 0;
            uint32 ClassRoles = 0;
            bool RequestPending = false;
        };

        class BrowseUpdated final : public ServerPacket
        {
        public:
            BrowseUpdated() : ServerPacket(SMSG_LF_GUILD_BROWSE_UPDATED, 4) { }

            WorldPacket const* Write() override;

            std::vector<BrowseEntry> Guilds;
        };

        // CMSG_LF_GUILD_DECLINE_RECRUIT - a guild officer/leader declines one applicant.
        class DeclineRecruit final : public ClientPacket
        {
        public:
            DeclineRecruit(WorldPacket&& packet) : ClientPacket(CMSG_LF_GUILD_DECLINE_RECRUIT, std::move(packet)) { }

            void Read() override;

            ObjectGuid PlayerGuid;
        };

        // CMSG_LF_GUILD_GET_APPLICATIONS - a guildless player requests their own pending applications.
        class GetApplications final : public ClientPacket
        {
        public:
            GetApplications(WorldPacket&& packet) : ClientPacket(CMSG_LF_GUILD_GET_APPLICATIONS, std::move(packet)) { }

            void Read() override { }
        };

        // SMSG_LF_GUILD_MEMBERSHIP_LIST_UPDATED - response to GetApplications, one entry per pending
        // application the requesting player has filed.
        struct MembershipApplication
        {
            ObjectGuid GuildGuid;
            std::string GuildName;
            std::string Comment;
            uint32 Availability = 0;
            uint32 ClassRoles = 0;
            uint32 Interests = 0;
            uint32 SecondsSinceSubmit = 0;
            uint32 SecondsUntilExpiry = 0;
        };

        class MembershipListUpdated final : public ServerPacket
        {
        public:
            MembershipListUpdated() : ServerPacket(SMSG_LF_GUILD_MEMBERSHIP_LIST_UPDATED, 8) { }

            WorldPacket const* Write() override;

            std::vector<MembershipApplication> Applications;
            uint32 ApplicationsLeft = 0;
        };

        // CMSG_LF_GUILD_GET_RECRUITS - a guild officer/leader requests the guild's pending applicant list.
        class GetRecruits final : public ClientPacket
        {
        public:
            GetRecruits(WorldPacket&& packet) : ClientPacket(CMSG_LF_GUILD_GET_RECRUITS, std::move(packet)) { }

            void Read() override;

            uint32 UnkTime = 0;
        };

        // SMSG_LF_GUILD_RECRUIT_LIST_UPDATED - response to GetRecruits, one entry per applicant.
        struct RecruitEntry
        {
            ObjectGuid PlayerGuid;
            std::string Name;
            std::string Comment;
            bool Available = false; // Application has not yet expired
            int32 Level = 0;
            int32 Class = 0;
            uint32 SecondsSinceSubmit = 0;
            uint32 SecondsUntilExpiry = 0;
            uint32 Availability = 0;
            uint32 ClassRoles = 0;
            uint32 Interests = 0;
        };

        class RecruitListUpdated final : public ServerPacket
        {
        public:
            RecruitListUpdated() : ServerPacket(SMSG_LF_GUILD_RECRUIT_LIST_UPDATED, 8) { }

            WorldPacket const* Write() override;

            std::vector<RecruitEntry> Recruits;
        };

        // CMSG_LF_GUILD_POST_REQUEST - a guild member opens the Guild Finder tab of their own guild.
        class PostRequest final : public ClientPacket
        {
        public:
            PostRequest(WorldPacket&& packet) : ClientPacket(CMSG_LF_GUILD_POST_REQUEST, std::move(packet)) { }

            void Read() override { }
        };

        // SMSG_LF_GUILD_POST_UPDATED - response to PostRequest (and echoed after SetGuildPost changes).
        class PostUpdated final : public ServerPacket
        {
        public:
            PostUpdated() : ServerPacket(SMSG_LF_GUILD_POST_UPDATED, 5) { }

            WorldPacket const* Write() override;

            // Only the requester being the guild master/an officer matters client-side; when false
            // the client is shown a read-only "not permitted to edit" view and none of the fields
            // below are sent (mirrors upstream TrinityCore's HandleGuildFinderPostRequest exactly).
            bool IsGuildMaster = false;
            bool Listed = false;
            uint32 Level = 0;
            std::string Comment;
            uint32 Availability = 0;
            uint32 ClassRoles = 0;
            uint32 Interests = 0;
        };

        // CMSG_LF_GUILD_REMOVE_RECRUIT - an applicant withdraws their own pending application.
        class RemoveRecruit final : public ClientPacket
        {
        public:
            RemoveRecruit(WorldPacket&& packet) : ClientPacket(CMSG_LF_GUILD_REMOVE_RECRUIT, std::move(packet)) { }

            void Read() override;

            ObjectGuid GuildGuid;
        };

        // CMSG_LF_GUILD_SET_GUILD_POST - a guild master edits and/or (un)lists their guild's recruitment post.
        class SetGuildPost final : public ClientPacket
        {
        public:
            SetGuildPost(WorldPacket&& packet) : ClientPacket(CMSG_LF_GUILD_SET_GUILD_POST, std::move(packet)) { }

            void Read() override;

            uint32 Level = 0;
            uint32 Availability = 0;
            uint32 Interests = 0;
            uint32 ClassRoles = 0;
            std::string Comment;
            bool Listed = false;
        };

        // SMSG_LF_GUILD_APPLICANT_LIST_UPDATED - empty notification: a guild's applicant list changed
        // (sent to the guild master and officers).
        class ApplicantListUpdated final : public ServerPacket
        {
        public:
            ApplicantListUpdated() : ServerPacket(SMSG_LF_GUILD_APPLICANT_LIST_UPDATED, 0) { }

            WorldPacket const* Write() override { return &_worldPacket; }
        };

        // SMSG_LF_GUILD_APPLICATIONS_LIST_CHANGED - empty notification: one of a player's own
        // membership requests changed (added/removed).
        class ApplicationsListChanged final : public ServerPacket
        {
        public:
            ApplicationsListChanged() : ServerPacket(SMSG_LF_GUILD_APPLICATIONS_LIST_CHANGED, 0) { }

            WorldPacket const* Write() override { return &_worldPacket; }
        };

        // SMSG_LF_GUILD_COMMAND_RESULT is reserved in Opcodes.h (matching real Cata's protocol) but,
        // like upstream TrinityCore, is never actually sent anywhere by this subsystem - nothing
        // server-side needs to report a generic Guild Finder error/result code to the client, since
        // every failure path here is either a silent no-op (bad bitmask, not in a guild, not the
        // guild master) or surfaced through one of the *Updated packets above. No packet class is
        // defined for it, exactly as TrinityCore leaves it: an opcode with a value and zero senders.
    }
}

#endif // GuildFinderPackets_h__
