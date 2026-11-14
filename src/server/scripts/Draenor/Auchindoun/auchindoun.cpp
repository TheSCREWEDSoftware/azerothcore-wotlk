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

#include "AreaTriggerScript.h"
#include "CreatureScript.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "TaskScheduler.h"
#include "auchindoun.h"

// Ported from TrinityCore master (src/server/scripts/Draenor/Auchindoun/auchindoun.cpp).
// Not ported: the Grimrail Depot scene (scene templates), the server side npc
// reaction area trigger 1725 and the phase change, which need systems this core
// does not have. Area trigger 10280 instead replaces the escorting Soulbinder
// Tuulani with the imprisoned one, which master spawns after the scene.

enum AuchindounSpells
{
    SPELL_HALO_HEAL                  = 157795,
    SPELL_TUULANI_UNLOCK_VISUAL      = 160415,
    SPELL_VOID_STASIS                = 154840
};

enum AuchindounActions
{
    ACTION_TUULANI_INTRO = 1,
    ACTION_TUULANI_BREAK_BARRIER,
    ACTION_AUCHENAI_DEFENDER_TALK
};

enum AuchindounTexts
{
    // Auchenai Defender
    SAY_NYAMI_AWAITS    = 0,

    // Soulbinder Tuulani
    SAY_WELCOME_1       = 0,
    SAY_WELCOME_2       = 1,
    SAY_WELCOME_3       = 2,
    SAY_BREAK_BARRIER_1 = 3,
    SAY_BREAK_BARRIER_2 = 4,
    SAY_HEROES_SOULS    = 5
};

enum AuchindounMisc
{
    // Waypoint
    PATH_BARRIER      = 7924800,
    PATH_MOVE_TO_BOSS = 7924801,

    POINT_TALK    = 2,
    POINT_BARRIER = 8,
    POINT_SOULS   = 5
};

Position const TuulaniImprisonedPosition = { 1859.0208740234375f, 3197.3203125f, 29.50753593444824218f, 0.019564647227525711f };

// 9973 - Areatrigger
class at_auchindoun_entrance : public OnlyOnceAreaTriggerScript
{
public:
    at_auchindoun_entrance() : OnlyOnceAreaTriggerScript("at_auchindoun_entrance") { }

    bool _OnTrigger(Player* player, AreaTrigger const* /*trigger*/) override
    {
        if (InstanceScript* instance = player->GetInstanceScript())
            if (Creature* tuulani = instance->GetCreature(DATA_SOULBINDER_TUULANI))
                tuulani->AI()->DoAction(ACTION_TUULANI_INTRO);

        return true;
    }
};

// 10072 - Areatrigger
class at_auchindoun_auchenai_defender_intro : public OnlyOnceAreaTriggerScript
{
public:
    at_auchindoun_auchenai_defender_intro() : OnlyOnceAreaTriggerScript("at_auchindoun_auchenai_defender_intro") { }

    bool _OnTrigger(Player* player, AreaTrigger const* /*trigger*/) override
    {
        // Master looks the defender up by its spawn StringId; this core has none, so the nearest one is used
        if (Creature* auchenaiDefender = player->FindNearestCreature(NPC_AUCHENAI_DEFENDER, 30.0f))
            auchenaiDefender->AI()->DoAction(ACTION_AUCHENAI_DEFENDER_TALK);

        return true;
    }
};

// 9974 - Areatrigger
// Not OnlyOnce because players can trigger this AT before Tuulani reaches the barrier
class at_auchindoun_barrier : public AreaTriggerScript
{
public:
    at_auchindoun_barrier() : AreaTriggerScript("at_auchindoun_barrier") { }

    bool OnTrigger(Player* player, AreaTrigger const* /*trigger*/) override
    {
        if (InstanceScript* instance = player->GetInstanceScript())
            if (Creature* tuulani = instance->GetCreature(DATA_SOULBINDER_TUULANI))
                tuulani->AI()->DoAction(ACTION_TUULANI_BREAK_BARRIER);

        return true;
    }
};

// 10280 - Areatrigger
class at_auchindoun_soulbinder_nyami_scene : public OnlyOnceAreaTriggerScript
{
public:
    at_auchindoun_soulbinder_nyami_scene() : OnlyOnceAreaTriggerScript("at_auchindoun_soulbinder_nyami_scene") { }

    bool _OnTrigger(Player* player, AreaTrigger const* /*trigger*/) override
    {
        InstanceScript* instance = player->GetInstanceScript();
        if (!instance)
            return true;

        if (Creature* tuulani = instance->GetCreature(DATA_SOULBINDER_TUULANI))
            tuulani->DespawnOrUnsummon();

        if (Creature* imprisoned = player->SummonCreature(NPC_SOULBINDER_TUULANI, TuulaniImprisonedPosition, TEMPSUMMON_MANUAL_DESPAWN))
        {
            imprisoned->SetReactState(REACT_PASSIVE);
            imprisoned->CastSpell(imprisoned, SPELL_VOID_STASIS, true);
        }

        return true;
    }
};

// 77693 - Auchenai Defender
struct npc_auchindoun_auchenai_defender : public ScriptedAI
{
    npc_auchindoun_auchenai_defender(Creature* creature) : ScriptedAI(creature), _talked(false) { }

    void DoAction(int32 action) override
    {
        if (action != ACTION_AUCHENAI_DEFENDER_TALK || _talked)
            return;

        _talked = true;
        me->SetStandState(UNIT_STAND_STATE_STAND);
        _scheduler.Schedule(1s + 300ms, [this](TaskContext /*task*/)
        {
            me->SetFacingTo(3.8194849f);
            Talk(SAY_NYAMI_AWAITS);
        });
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);

        if (!UpdateVictim())
            return;

        DoMeleeAttackIfReady();
    }

private:
    TaskScheduler _scheduler;
    bool _talked;
};

// 79248 - Soulbinder Tuulani
struct npc_auchindoun_soulbinder_tuulani : public ScriptedAI
{
    npc_auchindoun_soulbinder_tuulani(Creature* creature) : ScriptedAI(creature), _isAtBarrier(false), _actionStarted(false) { }

    void DoAction(int32 action) override
    {
        if (action == ACTION_TUULANI_INTRO)
        {
            Talk(SAY_WELCOME_1);
            _scheduler.Schedule(4s + 300ms, [this](TaskContext task)
            {
                Talk(SAY_WELCOME_2);

                task.Schedule(6s, [this](TaskContext /*task*/)
                {
                    me->GetMotionMaster()->MoveWaypoint(PATH_BARRIER, false);
                });
            });
        }
        else if (action == ACTION_TUULANI_BREAK_BARRIER)
        {
            if (!_isAtBarrier || _actionStarted)
                return;

            _actionStarted = true;

            Talk(SAY_BREAK_BARRIER_1);
            _scheduler.Schedule(3s, [this](TaskContext task)
            {
                DoCast(SPELL_TUULANI_UNLOCK_VISUAL);

                task.Schedule(4s, [this](TaskContext task)
                {
                    if (InstanceScript* instance = me->GetInstanceScript())
                        if (GameObject* holyBarrier = instance->GetGameObject(DATA_HOLY_BARRIER))
                            holyBarrier->SetGoState(GO_STATE_ACTIVE);

                    task.Schedule(2s + 300ms, [this](TaskContext task)
                    {
                        Talk(SAY_BREAK_BARRIER_2);

                        task.Schedule(1s + 300ms, [this](TaskContext /*task*/)
                        {
                            me->GetMotionMaster()->MoveWaypoint(PATH_MOVE_TO_BOSS, false);
                        });
                    });
                });
            });
        }
    }

    void WaypointReached(uint32 waypointId, uint32 pathId) override
    {
        if (pathId == PATH_BARRIER)
        {
            switch (waypointId)
            {
                case POINT_TALK:
                    _scheduler.Schedule(500ms, [this](TaskContext /*task*/)
                    {
                        Talk(SAY_WELCOME_3);
                    });
                    break;
                case POINT_BARRIER:
                    _isAtBarrier = true;
                    break;
                default:
                    break;
            }
        }
        else if (pathId == PATH_MOVE_TO_BOSS)
        {
            if (waypointId == POINT_SOULS)
            {
                _scheduler.Schedule(500ms, [this](TaskContext /*task*/)
                {
                    me->SetFacingTo(1.727875f);
                    Talk(SAY_HEROES_SOULS);
                });
            }
        }
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
    bool _isAtBarrier;
    bool _actionStarted;
};

// 157762 - Halo
class spell_auchindoun_halo : public SpellScript
{
    PrepareSpellScript(spell_auchindoun_halo);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_HALO_HEAL });
    }

    void HandleHit(SpellEffIndex /*effIndex*/)
    {
        if (GetHitUnit()->IsCreature())
            GetCaster()->CastSpell(GetHitUnit(), SPELL_HALO_HEAL, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_auchindoun_halo::HandleHit, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

void AddSC_auchindoun()
{
    new at_auchindoun_entrance();
    new at_auchindoun_auchenai_defender_intro();
    new at_auchindoun_barrier();
    new at_auchindoun_soulbinder_nyami_scene();
    RegisterAuchindounCreatureAI(npc_auchindoun_auchenai_defender);
    RegisterAuchindounCreatureAI(npc_auchindoun_soulbinder_tuulani);
    RegisterSpellScript(spell_auchindoun_halo);
}
