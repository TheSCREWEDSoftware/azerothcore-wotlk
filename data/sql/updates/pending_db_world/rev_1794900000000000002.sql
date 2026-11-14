-- WorldMapArea.dbc has 14 fields since 4.x (WorldMapAreaEntryfmt "xinxffffixxxxx"); the server side override table
-- still had the 11 columns of 3.3.5, so the worldserver stopped on its ASSERT as soon as the table had rows
ALTER TABLE IF EXISTS `worldmaparea_dbc`
    ADD COLUMN IF NOT EXISTS `Flags` int unsigned NOT NULL DEFAULT 0 AFTER `ParentWorldMapID`,
    ADD COLUMN IF NOT EXISTS `LevelRangeMin` int unsigned NOT NULL DEFAULT 0 AFTER `Flags`,
    ADD COLUMN IF NOT EXISTS `LevelRangeMax` int unsigned NOT NULL DEFAULT 0 AFTER `LevelRangeMin`;
