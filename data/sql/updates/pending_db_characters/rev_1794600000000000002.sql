-- 4.3.4 has nine glyph slots: three prime, three major and three minor
ALTER TABLE `character_glyphs`
  ADD COLUMN `glyph7` smallint unsigned DEFAULT '0' AFTER `glyph6`,
  ADD COLUMN `glyph8` smallint unsigned DEFAULT '0' AFTER `glyph7`,
  ADD COLUMN `glyph9` smallint unsigned DEFAULT '0' AFTER `glyph8`;
