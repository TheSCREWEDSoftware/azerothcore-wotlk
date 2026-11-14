-- Realm 1 for the 4.3.4 client: build_info row, realm build and a name that shows the version
-- (build_info values from TrinityCore's auth_database.sql where it has the build)
DELETE FROM `build_info` WHERE `build` = 15595;
INSERT INTO `build_info` (`build`, `majorVersion`, `minorVersion`, `bugfixVersion`, `hotfixVersion`, `winAuthSeed`, `win64AuthSeed`, `mac64AuthSeed`, `winChecksumSeed`, `macChecksumSeed`) VALUES (15595, 4, 3, 4, NULL, NULL, NULL, NULL, NULL, NULL);

ALTER TABLE `realmlist` ALTER COLUMN `gamebuild` SET DEFAULT 15595;
UPDATE `realmlist` SET `gamebuild` = 15595 WHERE `id` = 1;
UPDATE `realmlist` SET `name` = 'AzerothCore (4.3.4)' WHERE `id` = 1 AND `name` IN ('AzerothCore', 'AzerothCore 4.3.4');
