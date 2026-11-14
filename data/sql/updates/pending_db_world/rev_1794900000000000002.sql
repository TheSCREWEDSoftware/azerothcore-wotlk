-- WorldMapArea.dbc has 14 fields since 4.x (WorldMapAreaEntryfmt "xinxffffixxxxx"); the server side override table
-- still had the 11 columns of 3.3.5, so the worldserver stopped on its ASSERT as soon as the table had rows.
-- MySQL has no ADD COLUMN IF NOT EXISTS: only alter when the table exists and the columns are missing.
SET @wma_alter := (
    SELECT IF(COUNT(*) = 1 AND NOT EXISTS (SELECT 1 FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'worldmaparea_dbc' AND COLUMN_NAME = 'Flags'),
        'ALTER TABLE `worldmaparea_dbc` ADD COLUMN `Flags` int unsigned NOT NULL DEFAULT 0 AFTER `ParentWorldMapID`, ADD COLUMN `LevelRangeMin` int unsigned NOT NULL DEFAULT 0 AFTER `Flags`, ADD COLUMN `LevelRangeMax` int unsigned NOT NULL DEFAULT 0 AFTER `LevelRangeMin`',
        'DO 0')
    FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'worldmaparea_dbc');
PREPARE wma_stmt FROM @wma_alter;
EXECUTE wma_stmt;
DEALLOCATE PREPARE wma_stmt;
