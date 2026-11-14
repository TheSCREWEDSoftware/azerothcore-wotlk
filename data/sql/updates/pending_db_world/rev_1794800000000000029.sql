-- Cataclysm and Pandaria content scripts from TrinityCore master (TC 6.x had Baradin Hold, Firelands,
-- Halls of Origination and the Stonecore; master added the rest). Bindings taken from TC master's world
-- updates (sql/old 4.3.4 .. 12.x); bosses whose names only exist in the TDB base are bound by entry.

DELETE FROM `instance_template` WHERE `map` IN (757, 671, 645, 669, 670, 643, 938, 940, 939, 720, 644, 755, 754, 657, 725);
INSERT INTO `instance_template` (`map`, `parent`, `script`, `allowMount`) VALUES
(757, 0, 'instance_baradin_hold', 0),
(671, 0, 'instance_bastion_of_twilight', 0),
(645, 0, 'instance_blackrock_caverns', 0),
(669, 0, 'instance_blackwing_descent', 0),
(670, 0, 'instance_grim_batol', 0),
(643, 0, 'instance_throne_of_the_tides', 0),
(938, 0, 'instance_end_time', 0),
(940, 0, 'instance_hour_of_twilight', 0),
(939, 0, 'instance_well_of_eternity', 0),
(720, 0, 'instance_firelands', 1),
(644, 0, 'instance_halls_of_origination', 0),
(755, 0, 'instance_lost_city_of_the_tolvir', 0),
(754, 0, 'instance_throne_of_the_four_winds', 0),
(657, 0, 'instance_vortex_pinnacle', 0),
(725, 0, 'instance_stonecore', 0);

UPDATE `creature_template` SET `ScriptName` = 'boss_alizabal' WHERE `entry` IN (55869);
UPDATE `creature_template` SET `ScriptName` = 'boss_anraphet' WHERE `entry` IN (39788);
UPDATE `creature_template` SET `ScriptName` = 'boss_argaloth' WHERE `entry` IN (47120);
UPDATE `creature_template` SET `ScriptName` = 'boss_ascendant_lord_obsidius' WHERE `entry` IN (39705);
UPDATE `creature_template` SET `ScriptName` = 'boss_baleroc' WHERE `entry` IN (53494);
UPDATE `creature_template` SET `ScriptName` = 'boss_beauty' WHERE `entry` IN (39700);
UPDATE `creature_template` SET `ScriptName` = 'boss_corborus' WHERE `entry` IN (43438);
UPDATE `creature_template` SET `ScriptName` = 'boss_corla' WHERE `entry` IN (39679);
UPDATE `creature_template` SET `ScriptName` = 'boss_earthrager_ptah' WHERE `entry` IN (39428);
UPDATE `creature_template` SET `ScriptName` = 'boss_general_husam' WHERE `entry` IN (44577);
UPDATE `creature_template` SET `ScriptName` = 'boss_high_priestess_azil' WHERE `entry` IN (42333);
UPDATE `creature_template` SET `ScriptName` = 'boss_karsh_steelbender' WHERE `entry` IN (39698);
UPDATE `creature_template` SET `ScriptName` = 'boss_lockmaw' WHERE `entry` IN (43614);
UPDATE `creature_template` SET `ScriptName` = 'boss_occuthar' WHERE `entry` IN (52363);
UPDATE `creature_template` SET `ScriptName` = 'boss_ozruk' WHERE `entry` IN (42188);
UPDATE `creature_template` SET `ScriptName` = 'boss_romogg_bonecrusher' WHERE `entry` IN (39665);
UPDATE `creature_template` SET `ScriptName` = 'boss_slabhide' WHERE `entry` IN (43214);
UPDATE `creature_template` SET `ScriptName` = 'boss_temple_guardian_anhuur' WHERE `entry` IN (39425);
UPDATE `creature_template` SET `ScriptName` = 'npc_alpha_beam' WHERE `entry` IN (41144);
UPDATE `creature_template` SET `ScriptName` = 'npc_amberleaf_scamp' WHERE `entry` IN (54130);
UPDATE `creature_template` SET `ScriptName` = 'npc_aysa_cloudsinger_cave_of_meditation' WHERE `entry` IN (59642);
UPDATE `creature_template` SET `ScriptName` = 'npc_aysa_cloudsinger_summon' WHERE `entry` IN (59652);
UPDATE `creature_template` SET `ScriptName` = 'npc_blazing_monstrosity' WHERE `entry` IN (53786, 53791);
UPDATE `creature_template` SET `ScriptName` = 'npc_brann_bronzebeard_anraphet' WHERE `entry` IN (39908);
UPDATE `creature_template` SET `ScriptName` = 'npc_chains_of_woe' WHERE `entry` IN (40447);
UPDATE `creature_template` SET `ScriptName` = 'npc_crazed_mage' WHERE `entry` IN (39982);
UPDATE `creature_template` SET `ScriptName` = 'npc_delivery_cart' WHERE `entry` IN (57208, 57740, 59496);
UPDATE `creature_template` SET `ScriptName` = 'npc_devout_follower' WHERE `entry` IN (42428);
UPDATE `creature_template` SET `ScriptName` = 'npc_egg_pile' WHERE `entry` IN (53795);
UPDATE `creature_template` SET `ScriptName` = 'npc_eyestalk' WHERE `entry` IN (52369);
UPDATE `creature_template` SET `ScriptName` = 'npc_fire_cyclone' WHERE `entry` IN (40164);
UPDATE `creature_template` SET `ScriptName` = 'npc_firelands_flame_archon' WHERE `entry` IN (54161);
UPDATE `creature_template` SET `ScriptName` = 'npc_firelands_magmakin' WHERE `entry` IN (54144);
UPDATE `creature_template` SET `ScriptName` = 'npc_firelands_molten_flamefather' WHERE `entry` IN (54143);
UPDATE `creature_template` SET `ScriptName` = 'npc_gravity_well' WHERE `entry` IN (42499);
UPDATE `creature_template` SET `ScriptName` = 'npc_harbinger_of_flame' WHERE `entry` IN (53793);
UPDATE `creature_template` SET `ScriptName` = 'npc_huojin_trainee' WHERE `entry` IN (54586, 65470);
UPDATE `creature_template` SET `ScriptName` = 'npc_husam_bad_intentions_target' WHERE `entry` IN (44586);
UPDATE `creature_template` SET `ScriptName` = 'npc_husam_shockwave_visual' WHERE `entry` IN (44712);
UPDATE `creature_template` SET `ScriptName` = 'npc_husam_tolvir_land_mine' WHERE `entry` IN (44796, 44798, 44840);
UPDATE `creature_template` SET `ScriptName` = 'npc_instructor_zhi' WHERE `entry` IN (61411);
UPDATE `creature_template` SET `ScriptName` = 'npc_jaomin_ro' WHERE `entry` IN (54611);
UPDATE `creature_template` SET `ScriptName` = 'npc_jaomin_ro_hawk' WHERE `entry` IN (57750);
UPDATE `creature_template` SET `ScriptName` = 'npc_lava_fissure' WHERE `entry` IN (43242);
UPDATE `creature_template` SET `ScriptName` = 'npc_li_fei' WHERE `entry` IN (54135);
UPDATE `creature_template` SET `ScriptName` = 'npc_li_fei_combat' WHERE `entry` IN (54734);
UPDATE `creature_template` SET `ScriptName` = 'npc_lockmaw_augh_add' WHERE `entry` IN (45378, 45379);
UPDATE `creature_template` SET `ScriptName` = 'npc_lockmaw_augh_boss' WHERE `entry` IN (49045);
UPDATE `creature_template` SET `ScriptName` = 'npc_lockmaw_frenzied_crocolisk' WHERE `entry` IN (43658);
UPDATE `creature_template` SET `ScriptName` = 'npc_mad_prisoner' WHERE `entry` IN (39985);
UPDATE `creature_template` SET `ScriptName` = 'npc_master_li_fei_summon' WHERE `entry` IN (54856);
UPDATE `creature_template` SET `ScriptName` = 'npc_min_dimwind_summon' WHERE `entry` IN (56503);
UPDATE `creature_template` SET `ScriptName` = 'npc_molten_barrage' WHERE `entry` IN (53787, 53788);
UPDATE `creature_template` SET `ScriptName` = 'npc_omega_stance' WHERE `entry` IN (41194);
UPDATE `creature_template` SET `ScriptName` = 'npc_rappelling_rope' WHERE `entry` IN (66640);
UPDATE `creature_template` SET `ScriptName` = 'npc_raz_the_crazed' WHERE `entry` IN (39670);
UPDATE `creature_template` SET `ScriptName` = 'npc_rock_borer' WHERE `entry` IN (43917);
UPDATE `creature_template` SET `ScriptName` = 'npc_sc_millhouse_manastorm' WHERE `entry` IN (43391);
UPDATE `creature_template` SET `ScriptName` = 'npc_seismic_shard' WHERE `entry` IN (42355);
UPDATE `creature_template` SET `ScriptName` = 'npc_shard_of_torment' WHERE `entry` IN (53495);
UPDATE `creature_template` SET `ScriptName` = 'npc_stalactite_trigger' WHERE `entry` IN (43159);
UPDATE `creature_template` SET `ScriptName` = 'npc_tushui_leading_trainee' WHERE `entry` IN (54587, 65471);
UPDATE `creature_template` SET `ScriptName` = 'npc_twilight_flame_caller' WHERE `entry` IN (39708);
UPDATE `creature_template` SET `ScriptName` = 'npc_twilight_sadist' WHERE `entry` IN (39980);
UPDATE `creature_template` SET `ScriptName` = 'npc_twilight_torturer' WHERE `entry` IN (39978);
UPDATE `creature_template` SET `ScriptName` = 'npc_vp_slipstream' WHERE `entry` IN (45455);
UPDATE `creature_template` SET `ScriptName` = 'npc_vp_slipstream_landing_zone' WHERE `entry` IN (45504);
UPDATE `gameobject_template` SET `ScriptName` = 'go_edict_of_temperance' WHERE `entry` IN (210986);
UPDATE `gameobject_template` SET `ScriptName` = 'go_hoo_the_makers_lift_controller' WHERE `entry` IN (207669);

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_alysrazor_aggro_closest', 'spell_alysrazor_cosmetic_egg_xplosion', 'spell_alysrazor_fieroblast', 'spell_alysrazor_turn_monstrosity', 'spell_anhuur_disable_beacon_beams', 'spell_anhuur_divine_reckoning', 'spell_anhuur_shield_of_light', 'spell_anraphet_alpha_beams', 'spell_anraphet_omega_stance_summon', 'spell_argaloth_consuming_darkness', 'spell_argaloth_fel_firestorm', 'spell_argaloth_fel_firestorm_forcecast', 'spell_baleroc_blades_of_baleroc', 'spell_baleroc_countdown', 'spell_baleroc_countdown_aoe_dummy', 'spell_baleroc_countdown_proximity_check', 'spell_baleroc_decimating_strike', 'spell_baleroc_decimation_blade', 'spell_baleroc_inferno_blade', 'spell_baleroc_shards_of_torment_target_search', 'spell_baleroc_torment', 'spell_baleroc_torment_target_search', 'spell_baleroc_tormented', 'spell_baleroc_tormented_spread', 'spell_baleroc_vital_flame', 'spell_baleroc_vital_spark', 'spell_barrel_explosion_reversecast', 'spell_cancel_blackout', 'spell_cannon_explosion_reversecast', 'spell_chains_of_woe_1', 'spell_chains_of_woe_4', 'spell_earthrager_ptah_explosion', 'spell_earthrager_ptah_flame_bolt', 'spell_elementium_spike_shield', 'spell_feet_of_fury', 'spell_fire_crash', 'spell_firelands_fiery_torment', 'spell_firelands_smouldering', 'spell_firelands_smouldering_aura', 'spell_flame_spout', 'spell_flying_shadow_kick', 'spell_force_of_earth', 'spell_force_summoner_to_ride_vehicle', 'spell_gravity_well_damage', 'spell_gravity_well_damage_nearby', 'spell_gravity_well_pull', 'spell_husam_bad_intentions', 'spell_husam_detonate_traps', 'spell_husam_hammer_fist', 'spell_husam_hurl', 'spell_husam_land_mine_player_search_effect', 'spell_husam_shockwave', 'spell_husam_shockwave_summon_search', 'spell_into_the_mists_scene_end', 'spell_into_the_mists_scene_jf', 'spell_lockmaw_scent_of_blood', 'spell_meditation_timer_bar', 'spell_nether_dragon_essence_1', 'spell_nether_dragon_essence_2', 'spell_occuthar_eyes_of_occuthar', 'spell_occuthar_eyes_of_occuthar_vehicle', 'spell_occuthar_focused_fire', 'spell_occuthar_occuthars_destruction', 'spell_rappelling_rope', 'spell_rappelling_rope_aura', 'spell_ride_drake', 'spell_rupture', 'spell_s81028_s80650_stalactite', 'spell_s81035_stalactite', 'spell_s92300_crystal_storm', 'spell_s92306_crystal_storm', 'spell_sc_quake', 'spell_sc_twilight_documents', 'spell_seismic_shard', 'spell_seismic_shard_change_seat', 'spell_spirit_totem', 'spell_stalactite_mod_dest_height', 'spell_summon_gunship_turret', 'spell_summon_wave_south', 'spell_summon_wave_west', 'spell_the_mission_scene_end', 'spell_the_mission_scene_jf', 'spell_vp_slipstream', 'spell_vp_slipstream_shortcut');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(74930, 'spell_anhuur_shield_of_light'),
(75437, 'spell_chains_of_woe_1'),
(75519, 'spell_earthrager_ptah_explosion'),
(75540, 'spell_earthrager_ptah_flame_bolt'),
(75592, 'spell_anhuur_divine_reckoning'),
(75649, 'spell_nether_dragon_essence_1'),
(75650, 'spell_nether_dragon_essence_2'),
(75653, 'spell_nether_dragon_essence_2'),
(75654, 'spell_nether_dragon_essence_2'),
(76573, 'spell_anhuur_shield_of_light'),
(76606, 'spell_anhuur_disable_beacon_beams'),
(76608, 'spell_anhuur_disable_beacon_beams'),
(76904, 'spell_anraphet_alpha_beams'),
(77106, 'spell_anraphet_omega_stance_summon'),
(78835, 'spell_elementium_spike_shield'),
(79015, 'spell_seismic_shard'),
(79196, 'spell_summon_wave_west'),
(79200, 'spell_summon_wave_south'),
(79249, 'spell_gravity_well_damage'),
(79251, 'spell_gravity_well_damage_nearby'),
(79332, 'spell_gravity_well_pull'),
(79614, 'spell_spirit_totem'),
(80643, 'spell_stalactite_mod_dest_height'),
(80647, 'spell_stalactite_mod_dest_height'),
(80650, 'spell_s81028_s80650_stalactite'),
(80654, 'spell_stalactite_mod_dest_height'),
(81008, 'spell_sc_quake'),
(81028, 'spell_s81028_s80650_stalactite'),
(81035, 'spell_s81035_stalactite'),
(81459, 'spell_force_of_earth'),
(81690, 'spell_lockmaw_scent_of_blood'),
(82189, 'spell_chains_of_woe_4'),
(83112, 'spell_husam_land_mine_player_search_effect'),
(83113, 'spell_husam_bad_intentions'),
(83236, 'spell_husam_hurl'),
(83445, 'spell_husam_shockwave'),
(83466, 'spell_husam_shockwave_summon_search'),
(83654, 'spell_husam_hammer_fist'),
(84978, 'spell_vp_slipstream'),
(84989, 'spell_vp_slipstream'),
(85017, 'spell_vp_slipstream'),
(85395, 'spell_vp_slipstream'),
(85396, 'spell_vp_slipstream'),
(86863, 'spell_seismic_shard_change_seat'),
(88954, 'spell_argaloth_consuming_darkness'),
(88972, 'spell_argaloth_fel_firestorm'),
(88987, 'spell_argaloth_fel_firestorm_forcecast'),
(89499, 'spell_vp_slipstream_shortcut'),
(89501, 'spell_vp_slipstream_shortcut'),
(89881, 'spell_earthrager_ptah_flame_bolt'),
(91263, 'spell_husam_detonate_traps'),
(92300, 'spell_s92300_crystal_storm'),
(92306, 'spell_s92306_crystal_storm'),
(92309, 'spell_stalactite_mod_dest_height'),
(92393, 'spell_rupture'),
(92429, 'spell_elementium_spike_shield'),
(92653, 'spell_stalactite_mod_dest_height'),
(93167, 'spell_sc_twilight_documents'),
(95173, 'spell_argaloth_consuming_darkness'),
(96872, 'spell_occuthar_focused_fire'),
(96931, 'spell_occuthar_eyes_of_occuthar'),
(96932, 'spell_occuthar_eyes_of_occuthar_vehicle'),
(96942, 'spell_occuthar_occuthars_destruction'),
(99253, 'spell_baleroc_torment_target_search'),
(99256, 'spell_baleroc_torment'),
(99257, 'spell_baleroc_tormented'),
(99259, 'spell_baleroc_shards_of_torment_target_search'),
(99262, 'spell_baleroc_vital_spark'),
(99263, 'spell_baleroc_vital_flame'),
(99342, 'spell_baleroc_blades_of_baleroc'),
(99350, 'spell_baleroc_inferno_blade'),
(99352, 'spell_baleroc_decimation_blade'),
(99353, 'spell_baleroc_decimating_strike'),
(99489, 'spell_baleroc_tormented_spread'),
(99515, 'spell_baleroc_countdown_aoe_dummy'),
(99516, 'spell_baleroc_countdown'),
(99517, 'spell_baleroc_countdown_proximity_check'),
(100076, 'spell_alysrazor_turn_monstrosity'),
(100078, 'spell_alysrazor_turn_monstrosity'),
(100080, 'spell_alysrazor_turn_monstrosity'),
(100082, 'spell_alysrazor_turn_monstrosity'),
(100089, 'spell_alysrazor_turn_monstrosity'),
(100090, 'spell_alysrazor_turn_monstrosity'),
(100091, 'spell_alysrazor_turn_monstrosity'),
(100092, 'spell_alysrazor_turn_monstrosity'),
(100094, 'spell_alysrazor_fieroblast'),
(100099, 'spell_alysrazor_cosmetic_egg_xplosion'),
(100462, 'spell_alysrazor_aggro_closest'),
(100799, 'spell_firelands_fiery_torment'),
(101009, 'spell_occuthar_occuthars_destruction'),
(101089, 'spell_firelands_smouldering'),
(101092, 'spell_firelands_smouldering'),
(101093, 'spell_firelands_smouldering_aura'),
(101223, 'spell_alysrazor_fieroblast'),
(101294, 'spell_alysrazor_fieroblast'),
(101295, 'spell_alysrazor_fieroblast'),
(101296, 'spell_alysrazor_fieroblast'),
(102499, 'spell_fire_crash'),
(108582, 'spell_ride_drake'),
(108583, 'spell_force_summoner_to_ride_vehicle'),
(108936, 'spell_flying_shadow_kick'),
(108958, 'spell_feet_of_fury'),
(114684, 'spell_flame_spout'),
(116421, 'spell_meditation_timer_bar'),
(121545, 'spell_into_the_mists_scene_jf'),
(130233, 'spell_cannon_explosion_reversecast'),
(130246, 'spell_barrel_explosion_reversecast'),
(130723, 'spell_into_the_mists_scene_end'),
(130812, 'spell_cancel_blackout'),
(130960, 'spell_rappelling_rope'),
(130970, 'spell_rappelling_rope_aura'),
(130996, 'spell_summon_gunship_turret'),
(130997, 'spell_summon_gunship_turret'),
(130998, 'spell_summon_gunship_turret'),
(131057, 'spell_the_mission_scene_jf'),
(131059, 'spell_the_mission_scene_end');

DELETE FROM `areatrigger_scripts` WHERE `entry` IN (6070, 6076, 6958, 6986, 6987, 7246, 7645, 7756);
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES
(6070, 'at_sc_slabhide_intro'),
(6076, 'at_sc_corborus_intro'),
(6958, 'at_min_dimwind_captured'),
(6986, 'at_singing_pools_transform_frog'),
(6987, 'at_singing_pools_transform_frog'),
(7246, 'at_alizabal_intro'),
(7645, 'at_inside_of_cave_of_meditation'),
(7756, 'at_cave_of_meditation');

-- Share the Pain (achievement 5830)
DELETE FROM `criteria_data` WHERE `criteria_id` = 17577 AND `type` = 11;
INSERT INTO `criteria_data` (`criteria_id`, `type`, `value1`, `value2`, `ScriptName`) VALUES
(17577, 11, 0, 0, 'achievement_share_the_pain');
