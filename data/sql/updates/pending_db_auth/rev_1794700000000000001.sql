-- 5.4.8 18414 client
DELETE FROM `build_info` WHERE `build` = 18414;
INSERT INTO `build_info`
    (`build`, `majorVersion`, `minorVersion`, `bugfixVersion`, `hotfixVersion`, `winAuthSeed`, `win64AuthSeed`, `mac64AuthSeed`, `winChecksumSeed`, `macChecksumSeed`)
VALUES
    (18414, 5, 4, 8, NULL, NULL, NULL, NULL, NULL, NULL);

ALTER TABLE `realmlist` ALTER COLUMN `gamebuild` SET DEFAULT 18414;
UPDATE `realmlist` SET `gamebuild` = 18414 WHERE `gamebuild` = 15595;
