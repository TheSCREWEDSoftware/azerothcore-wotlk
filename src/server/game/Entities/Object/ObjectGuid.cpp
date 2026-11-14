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

#include "ObjectGuid.h"
#include "Errors.h"
#include "Log.h"
#include "Realm.h"
#include "TC9Sidecar.h"
#include "World.h"
#include <cstring>
#include <iomanip>
#include <sstream>

ObjectGuid const ObjectGuid::Empty = ObjectGuid();
ObjectGuid const ObjectGuid::TradeItem = ObjectGuid::Create<HighGuid::Uniq>(UI64LIT(10));

uint8& ObjectGuid::operator[](uint32 index)
{
    ASSERT(index < sizeof(uint64) * 2);
    return index < sizeof(uint64) ? reinterpret_cast<uint8*>(&_low)[index] : reinterpret_cast<uint8*>(&_high)[index - sizeof(uint64)];
}

uint8 const& ObjectGuid::operator[](uint32 index) const
{
    ASSERT(index < sizeof(uint64) * 2);
    return index < sizeof(uint64) ? reinterpret_cast<uint8 const*>(&_low)[index] : reinterpret_cast<uint8 const*>(&_high)[index - sizeof(uint64)];
}

char const* ObjectGuid::GetTypeName(HighGuid high)
{
    switch (high)
    {
        case HighGuid::Null: return "Null";
        case HighGuid::Uniq: return "Uniq";
        case HighGuid::Player: return "Player";
        case HighGuid::Item: return "Item";
        case HighGuid::WorldTransaction: return "WorldTransaction";
        case HighGuid::StaticDoor: return "StaticDoor";
        case HighGuid::Transport: return "Transport";
        case HighGuid::Conversation: return "Conversation";
        case HighGuid::Creature: return "Creature";
        case HighGuid::Vehicle: return "Vehicle";
        case HighGuid::Pet: return "Pet";
        case HighGuid::GameObject: return "GameObject";
        case HighGuid::DynamicObject: return "DynamicObject";
        case HighGuid::AreaTrigger: return "AreaTrigger";
        case HighGuid::Corpse: return "Corpse";
        case HighGuid::LootObject: return "LootObject";
        case HighGuid::SceneObject: return "SceneObject";
        case HighGuid::Scenario: return "Scenario";
        case HighGuid::AIGroup: return "AIGroup";
        case HighGuid::DynamicDoor: return "DynamicDoor";
        case HighGuid::ClientActor: return "ClientActor";
        case HighGuid::Vignette: return "Vignette";
        case HighGuid::CallForHelp: return "CallForHelp";
        case HighGuid::AIResource: return "AIResource";
        case HighGuid::AILock: return "AILock";
        case HighGuid::AILockTicket: return "AILockTicket";
        case HighGuid::ChatChannel: return "ChatChannel";
        case HighGuid::Party: return "Party";
        case HighGuid::Guild: return "Guild";
        case HighGuid::WowAccount: return "WowAccount";
        case HighGuid::BNetAccount: return "BNetAccount";
        case HighGuid::GMTask: return "GMTask";
        case HighGuid::MobileSession: return "MobileSession";
        case HighGuid::RaidGroup: return "RaidGroup";
        case HighGuid::Spell: return "Spell";
        case HighGuid::Mail: return "Mail";
        case HighGuid::WebObj: return "WebObj";
        case HighGuid::LFGObject: return "LFGObject";
        case HighGuid::LFGList: return "LFGList";
        case HighGuid::UserRouter: return "UserRouter";
        case HighGuid::PVPQueueGroup: return "PVPQueueGroup";
        case HighGuid::UserClient: return "UserClient";
        case HighGuid::PetBattle: return "PetBattle";
        case HighGuid::UniqUserClient: return "UniqUserClient";
        case HighGuid::BattlePet: return "BattlePet";
        case HighGuid::CommerceObj: return "CommerceObj";
        case HighGuid::ClientSession: return "ClientSession";
        default:
            return "<unknown>";
    }
}

std::string ObjectGuid::ToString() const
{
    std::ostringstream str;
    str << "GUID Full: 0x" << ToHexString();
    str << " Type: " << GetTypeName();
    if (HasEntry())
        str << (IsPet() ? " Pet number: " : " Entry: ") << GetEntry();

    str << " Low: " << GetCounter();
    return str.str();
}

std::string ObjectGuid::ToHexString() const
{
    std::ostringstream str;
    str << std::hex << std::uppercase << std::setw(16) << std::setfill('0') << _high << std::setw(16) << std::setfill('0') << _low;
    return str.str();
}

ObjectGuid ObjectGuid::Global(HighGuid type, LowType counter)
{
    return ObjectGuid(uint64(uint64(type) << 58), counter);
}

ObjectGuid ObjectGuid::RealmSpecific(HighGuid type, LowType counter)
{
    return RealmSpecific(type, uint16(realm.Id.Realm), counter);
}

ObjectGuid ObjectGuid::RealmSpecific(HighGuid type, uint16 realmId, LowType counter)
{
    return ObjectGuid(uint64(uint64(type) << 58 | uint64(realmId & 0x1FFF) << 42), counter);
}

ObjectGuid ObjectGuid::MapSpecific(HighGuid type, uint8 subType, uint16 mapId, uint32 serverId, uint32 entry, LowType counter)
{
    return ObjectGuid(uint64((uint64(type) << 58) | (uint64(realm.Id.Realm & 0x1FFF) << 42) | (uint64(mapId & 0x1FFF) << 29) | (uint64(entry & 0x7FFFFF) << 6) | (uint64(subType) & 0x3F)),
        uint64((uint64(serverId & 0xFFFFFF) << 40) | (counter & UI64LIT(0xFFFFFFFFFF))));
}

std::vector<uint8> ObjectGuid::GetRawValue() const
{
    std::vector<uint8> raw(16);
    std::memcpy(raw.data(), &_low, sizeof(_low));
    std::memcpy(raw.data() + sizeof(_low), &_high, sizeof(_high));
    return raw;
}

void ObjectGuid::SetRawValue(std::vector<uint8> const& guid)
{
    ASSERT(guid.size() == sizeof(uint64) * 2);
    std::memcpy(&_low, guid.data(), sizeof(_low));
    std::memcpy(&_high, guid.data() + sizeof(_low), sizeof(_high));
}

ObjectGuid ObjectGuid::FromRawValue(std::vector<uint8> const& raw)
{
    ObjectGuid guid;
    if (raw.size() == sizeof(uint64) * 2)
        guid.SetRawValue(raw);

    return guid;
}

void PackedGuid::Set(ObjectGuid const& guid)
{
    _packedGuid.clear();
    _packedGuid << guid;
}

ByteBuffer& operator<<(ByteBuffer& buf, ObjectGuid const& guid)
{
    uint8 lowMask = 0;
    uint8 highMask = 0;
    buf.FlushBits();    // flush any unwritten bits to make wpos return a meaningful value
    std::size_t pos = buf.wpos();
    buf << uint8(lowMask);
    buf << uint8(highMask);

    uint8 packed[8];
    if (std::size_t packedSize = ByteBuffer::PackUInt64(guid._low, &lowMask, packed))
        buf.append(packed, packedSize);
    if (std::size_t packedSize = ByteBuffer::PackUInt64(guid._high, &highMask, packed))
        buf.append(packed, packedSize);

    buf.put(pos, lowMask);
    buf.put(pos + 1, highMask);

    return buf;
}

ByteBuffer& operator>>(ByteBuffer& buf, ObjectGuid& guid)
{
    uint8 lowMask, highMask;
    buf >> lowMask >> highMask;
    buf.ReadPackedUInt64(lowMask, guid._low);
    buf.ReadPackedUInt64(highMask, guid._high);
    return buf;
}

ByteBuffer& operator<<(ByteBuffer& buf, PackedGuid const& guid)
{
    buf.append(guid._packedGuid);
    return buf;
}

ByteBuffer& operator>>(ByteBuffer& buf, PackedGuidReader const& guid)
{
    return buf >> guid.Guid;
}

std::ostream& operator<<(std::ostream& stream, ObjectGuid const& guid)
{
    stream << guid.ToHexString();
    return stream;
}

void ObjectGuidGeneratorBase::HandleCounterOverflow(HighGuid high)
{
    // StopNow only flags the shutdown, so Generate would keep handing out wrapped guids until the world loop exits.
    // For persisted guids those overwrite other rows on save (REPLACE INTO item_instance), so stop right here.
    if (high == HighGuid::Item || high == HighGuid::Player)
        ABORT("{} guid overflow!! Can't continue, renumber the guids before restarting.", ObjectGuid::GetTypeName(high));

    LOG_ERROR("entities.object", "{} guid overflow!! Can't continue, shutting down server. ", ObjectGuid::GetTypeName(high));
    World::StopNow(ERROR_EXIT_CODE);
}

bool ObjectGuidGeneratorBase::GetClusterGuid(HighGuid high, uint16 realmId, ObjectGuid::LowType& clusterGuid)
{
    if (!sToCloud9Sidecar->ClusterModeEnabled())
        return false;

    uint64 guid = 0;
    if (high == HighGuid::Player)
        guid = sToCloud9Sidecar->GenerateCharacterGuid(realmId);
    else if (high == HighGuid::Item)
        guid = sToCloud9Sidecar->GenerateItemGuid(realmId);
    else
        return false;

    // The sidecar packs the realm into bits 32-47, the counter is the low 32 bits
    clusterGuid = ObjectGuid::LowType(uint32(guid));
    if (clusterGuid >= ObjectGuid::GetMaxCounter(high) - 1)
        HandleCounterOverflow(high);

    return true;
}

#define GUID_TRAIT_INSTANTIATE_GUID( HIGH_GUID ) \
    template class ObjectGuidGenerator< HIGH_GUID >;

GUID_TRAIT_INSTANTIATE_GUID(HighGuid::Player)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::Item)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::Transport)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::Guild)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::Conversation)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::Creature)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::Vehicle)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::Pet)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::GameObject)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::DynamicObject)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::AreaTrigger)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::Corpse)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::LootObject)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::SceneObject)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::Scenario)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::AIGroup)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::DynamicDoor)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::Vignette)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::CallForHelp)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::AIResource)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::AILock)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::AILockTicket)
GUID_TRAIT_INSTANTIATE_GUID(HighGuid::Party)
