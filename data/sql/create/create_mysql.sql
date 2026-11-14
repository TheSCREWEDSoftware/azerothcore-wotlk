DROP USER IF EXISTS 'acore_6x'@'localhost';
CREATE USER 'acore_6x'@'localhost' IDENTIFIED BY 'acore_6x' WITH MAX_QUERIES_PER_HOUR 0 MAX_CONNECTIONS_PER_HOUR 0 MAX_UPDATES_PER_HOUR 0;

-- Create databases for AzerothCore, only if they do not exist
CREATE DATABASE IF NOT EXISTS `acore_6x_world` DEFAULT CHARACTER SET UTF8MB4 COLLATE utf8mb4_unicode_ci;

CREATE DATABASE IF NOT EXISTS `acore_6x_characters` DEFAULT CHARACTER SET UTF8MB4 COLLATE utf8mb4_unicode_ci;

CREATE DATABASE IF NOT EXISTS `acore_6x_auth` DEFAULT CHARACTER SET UTF8MB4 COLLATE utf8mb4_unicode_ci;

CREATE DATABASE IF NOT EXISTS `acore_6x_hotfixes` DEFAULT CHARACTER SET UTF8MB4 COLLATE utf8mb4_unicode_ci;

GRANT ALL PRIVILEGES ON `acore_6x_world` . * TO 'acore_6x'@'localhost' WITH GRANT OPTION;

GRANT ALL PRIVILEGES ON `acore_6x_characters` . * TO 'acore_6x'@'localhost' WITH GRANT OPTION;

GRANT ALL PRIVILEGES ON `acore_6x_auth` . * TO 'acore_6x'@'localhost' WITH GRANT OPTION;

GRANT ALL PRIVILEGES ON `acore_6x_hotfixes` . * TO 'acore_6x'@'localhost' WITH GRANT OPTION;
