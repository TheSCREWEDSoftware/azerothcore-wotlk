-- Update Existing Teles
-- Standardisation and String Corrections
UPDATE `game_tele` SET  `name` = 'Onyxia' WHERE `id` = 699 AND `name` = 'onyxia';
UPDATE `game_tele` SET  `name` = 'TheUnderbog' WHERE `id` = 1262 AND `name` = 'underbog';
UPDATE `game_tele` SET  `name` = 'GoldBug' WHERE `id` = 1430 AND `name` = 'goldbug';
UPDATE `game_tele` SET  `name` = 'MineK' WHERE `id` = 1431 AND `name` = 'minek';
UPDATE `game_tele` SET  `name` = 'Box' WHERE `id` = 1432 AND `name` = 'box';
UPDATE `game_tele` SET  `name` = 'DKZone' WHERE `id` = 1433 AND `name` = 'DKzone';
UPDATE `game_tele` SET  `name` = 'Prison' WHERE `id` = 1434 AND `name` = 'prison';
UPDATE `game_tele` SET  `name` = 'BorgorokOutpost' WHERE `id` = 1394 AND `name` = 'Bor''gorokOutpost'; 
UPDATE `game_tele` SET  `name` = 'Unupe' WHERE `id` = 1396 AND `name` = 'Unu''pe'; 
UPDATE `game_tele` SET  `name` = 'TaunkaleVillage' WHERE `id` = 1397 AND `name` = 'Taunka''leVillage'; 
UPDATE `game_tele` SET  `name` = 'ZulDrak' WHERE `id` = 1403 AND `name` = 'Zul''Drak';
UPDATE `game_tele` SET  `name` = 'TheStormPeaks' WHERE `id` = 1405 AND `name` = 'StormPeaks';

-- Changes to locations of existing Teles
UPDATE `game_tele` SET `position_x` = 9214.63, `position_y` = -1110.82, `position_z` = 1216.12, `orientation` = 0.000729084, `map` = 571 WHERE `id` = 1406 AND `name` = 'Ulduar';
UPDATE `game_tele` SET `position_x` = 5807.98, `position_y` = 588.487, `position_z` = 660.94, `orientation` = 1.66594, `map` = 571 WHERE `id` = 1398 AND `name` = 'Dalaran';
UPDATE `game_tele` SET `position_x` = 5258.39, `position_y` = 156.958, `position_z` = 191.697, `orientation` = 6.03807, `map` = 571 WHERE `id` = 1490 AND `name` = 'CrystalsongForest';
UPDATE `game_tele` SET `position_x` = 4857.14, `position_y` = 5529.11, `position_z` = -55.5802, `orientation` = 6.2518, `map` = 571 WHERE `id` = 1413 AND `name` = 'SholazarBasin';
UPDATE `game_tele` SET `position_x` = 7374.96, `position_y` = 1991.1, `position_z` = 622.229, `orientation` = 2.79045, `map` = 571 WHERE `id` = 1407 AND `name` = 'Icecrown';
UPDATE `game_tele` SET `position_x` = 7701.6, `position_y` = -696.182, `position_z` = 1196.38, `orientation` = 4.49486, `map` = 571 WHERE `id` = 1405 AND `name` = 'TheStormPeaks';

-- Adding New Teles
DELETE FROM `game_tele` WHERE `id` IN (1774,1773,1772,1771,1770,1769,1768,1767,1766,1765,1764,1763,1762,1761,1760,1759,1758,1757,1756,1755,1753,1752,1751,1750,1749,1748,1747,1746,1745,1744,1743,1742,1741,1740,1739,1738,1737,1736,1735,1734,1733,1732,1731,1730,1729,1728,1727,1726,1725,1724,1723,1722,1721,1720,1719,1718,1717,1716,1715,1714,1713,1712,1711,1710,1709,1708,1707,1706,1705,1704,1703,1702,1701,1700,1699,1698,1697,1696,1695,1694,1693,1692,1691,1690,1689,1688,1687,1686,1685,1684,1683,1682,1681,1680,1679,1678,1677,1676,1675,1674,1673,1672,1671,1670,1669,1665,1664,1663,1662,1661,1660,1659,1658,1657,1656,1655,1654,1653,1652,1651,1650,1649,1648,1646,1645,1644,1643,1642,1641,1640,1639,1638,1637,1636,1635,1634,1633,1632,1631,1630,1629,1628,1627,1626,1625,1624,1623,1622,1621,1620,1619,1618,1617,1616,1615,1614,1613,1612,1611,1610,1609,1608,1607,1606,1605,1604,1603,1602,1601,1600,1599,1598,1597,1596,1595,1594,1593,1592,1591,1590,1589,1588,1587,1586,1585,1584,1583,1582,1581,1580,1579,1578,1577,1576,1575,1574,1573,1572,1571,1570,1569,1568,1567,1566,1565,1564,1563,1562,1561,1560,1559,1558,1557,1556,1555,1554,1553,1552,1551,1550,1549,1548,1547,1546,1545,1544,1543,1542,1541,1540,1539,1538,1537,1536,1535,1534,1533,1532,1531,1529,1528,1527,1526,1525,1524,1523,1522,1520,1519,1518,1517,1516,1515,1514,1513,1512,1511,1510,1509,1508,1507,1506,1505,1504,1503,1502,1501,1500,1499,1498,1497,1496,1495,1494,1493,1492);
INSERT INTO `game_tele` (`id`, `position_x`, `position_y`, `position_z`, `orientation`, `map`, `name`) VALUES
-- Crystalsong Forest
(1768, 5595.35, -704.415, 206.626, 1.06257, 571, 'SunreaversCommand'),
(1767, 5057.03, -560.349, 220.132, 5.96739, 571, 'WindrunnersOverlook'),
(1766, 5291.85, -729.117, 162.904, 6.27369, 571, 'RuinsOfShandaral'),
(1765, 5508, -384.533, 170.492, 5.15056, 571, 'TheUnboundThicket'),
(1764, 5842.22, 785.206, 149.071, 2.04037, 571, 'TheTwilightRivulet'),
(1763, 5728.56, 205.439, 181.593, 2.41343, 571, 'ForlornWoods'),
(1762, 5522.5, 115.187, 148.671, 2.78651, 571, 'TheMirrorOfTwilight'),
(1761, 5576.85, 730.087, 158.475, 3.04179, 571, 'TheAzureFront'),
(1760, 5744.35, 1017.14, 174.48, 3.39524, 571, 'VioletStand'),
(1759, 5818.74, 1089.68, 198.347, 6.2266, 571, 'TheGreatTree'),
(1758, 6158.07, 913.136, 148.881, 0.885881, 571, 'TheDecrepitFlow'),
-- Crytalsong Forest (Dalaran)
(1757, 5761.53, 736.624, 641.698, 2.63982, 571, 'SistersSorcerous'),
(1756, 5922.4, 566.631, 609.825, 5.58114, 571, 'TheBlackMarket'),
(1755, 5804.89, 640.08, 609.887, 1.00618, 571, 'CircleOfWills'),
(1753, 5650.68, 856.348, 570.438, 2.4749, 571, 'TheUnderbellyExit'),
(1752, 5772.59, 705.993, 618.721, 2.46311, 571, 'CantripsCrows'),
(1751, 5804.89, 640.08, 609.887, 1.00618, 571, 'DalaranSewer'),
(1750, 5814.48, 769.885, 639.241, 1.74838, 571, 'TheUnderbellyWest'),
(1749, 5798.26, 546.654, 648.213, 4.05745, 571, 'TheUnderbellyEast'),
(1748, 5795.23, 770.997, 661.279, 1.47744, 571, 'TheVioletCitadel'),
(1747, 5700.74, 761.688, 647.664, 5.589, 571, 'TheSilverEnclave'),
(1746, 5637.25, 740.106, 641.684, 5.81284, 571, 'TheBeerGarden'),
(1745, 5726.92, 682.248, 646.564, 2.4317, 571, 'AHerosWelcome'),
(1744, 5654.86, 682.2, 652.006, 2.69087, 571, 'DalaranMerchantsBank'),
(1743, 5666.24, 645.034, 647.98, 3.47627, 571, 'TheMilitantMystic'),
(1742, 5669.53, 628.231, 648.106, 3.67262, 571, 'LangromsLeatherLinks'),
(1741, 5682.33, 615.55, 648.237, 3.76687, 571, 'GloriousGoods'),
(1740, 5700.25, 641.774, 646.28, 3.51554, 571, 'TheEventide'),
(1739, 5726.38, 602.7, 648.54, 4.06925, 571, 'TheArsernalAbsolute'),
(1738, 5769.84, 632.804, 661.076, 3.18961, 571, 'CuriositiesMoore'),
(1737, 5805.09, 676.243, 658.032, 1.75626, 571, 'TheThreadsOfFate'),
(1736, 5812.08, 689.954, 647.046, 4.45411, 571, 'TheWonderworks'),
(1735, 5840.61, 647.844, 658.404, 0.0912267, 571, 'TheLedgerdemainLounge'),
(1734, 5757.44, 621.777, 650.091, 0.460362, 571, 'DalaranVisitorCenter'),
(1733, 5809.02, 579.559, 652.756, 1.66201, 571, 'TheVioletGate'),
(1732, 5778.72, 556.013, 651.639, 4.02756, 571, 'TheHuntersReach'),
(1731, 5813.84, 449.122, 658.752, 1.16086, 571, 'KrasusLanding'),
(1730, 5848.54, 566.409, 652.657, 5.62583, 571, 'MagicalMenagerie'),
(1729, 5886.87, 514.771, 641.57, 4.93077, 571, 'TheFilthyAnimal'),
(1728, 5908.21, 540.531, 649.94, 2.47248, 571, 'SunreaversSanctuary'),
(1727, 5885.77, 606.631, 650.294, 5.618, 571, 'OneMoreGlass'),
(1726, 5890.33, 621.467, 649.383, 5.62193, 571, 'Barbershop'),
(1725, 5958.08, 617.286, 650.627, 5.90074, 571, 'TheBankOfDalaran'),
(1724, 5947.99, 664.679, 641.227, 1.21196, 571, 'AntonidasMemorial'),
(1723, 5905.42, 680.836, 643.496, 5.53165, 571, 'TanksForEverything'),
(1722, 5860.97, 702.897, 643.276, 0.956697, 571, 'TheScribesSacellum'),
(1721, 5874.33, 719.983, 643.116, 4.97401, 571, 'CartierCoFineJewelry'),
(1720, 5890.16, 703.427, 643.239, 4.01976, 571, 'TheAgronomicalApothecary'),
(1719, 5881.62, 747.392, 640.372, 1.85205, 571, 'TalismanicTextiles'),
(1718, 5903.79, 752.506, 641.048, 1.33761, 571, 'LegendaryLeathers'),
(1717, 5922.9, 728.071, 642.135, 6.22279, 571, 'LikeClockwork'),
(1716, 5921.77, 709.744, 642.515, 6.22279, 571, 'AllThatGlittersProspectingCo'),
(1715, 5915.89, 694.852, 642.426, 5.73584, 571, 'ForgeOfFate'),
(1714, 5894.9, 719.209, 640.544, 0.854603, 571, 'MagusCommerceExchange'),
(1713, 5804.04, 657.739, 648.141, 4.85228, 571, 'RuneweaverSquare'),
-- Zul'Drak
(1712, 5754.5, -2939.17, 286.276, 5.17592, 571, 'AmphitheaterOfAnguish'),
(1711, 5506.12, -2773.37, 274.379, 4.81857, 571, 'DrakAgal'),
(1710, 6084, -2080.68, 422.947, 0.583579, 571, 'Voltarus'),
(1709, 6071.19, -1945.42, 235.752, 5.5209, 571, 'ReliquaryOfPain'),
(1708, 5950.3, -1826.16, 248.309, 4.6766, 571, 'DargathsDemise'),
(1707, 5620.84, -1661.04, 242.363, 5.07322, 571, 'TheDeadFields'),
(1706, 5653.61, -1402.88, 234.446, 1.03628, 571, 'ThrymsEnd'),
(1705, 5228.59, -1328.17, 242.364, 2.37145, 571, 'EbonWatch'),
(1704, 5273.84, -1694.88, 429.14, 2.9173, 571, 'Zeramas'),
(1703, 5217.57, -1588.56, 239.176, 4.29569, 571, 'ReliquaryOfAgony'),
(1702, 4768.23, -2438.55, 235.542, 5.49402, 571, 'RageclawLake'),
(1701, 4977.99, -2299.54, 245.837, 3.76615, 571, 'RageclawDen'),
(1700, 5154.52, -2188.33, 236.538, 4.48478, 571, 'LightsBreach'),
(1699, 5203.09, -3199.7, 273.816, 2.34064, 571, 'DrakSotra'),
(1698, 5031.48, -3608.56, 299.963, 5.13666, 571, 'Kolramas'),
(1697, 4976.34, -2666.44, 289.707, 5.51365, 571, 'DrakSotraFields'),
(1696, 5277.93, -2433.96, 290.05, 3.14174, 571, 'ZimAbwa'),
(1695, 5450.38, -2422.65, 292.419, 4.6929, 571, 'TheArgentStand'),
(1694, 5675.91, -2507.03, 287.433, 2.49378, 571, 'PoolsOfZhaJin'),
(1693, 5929.82, -2414.55, 293.403, 1.57487, 571, 'HebValok'),
(1692, 6205.11, -2620.46, 293.168, 6.27548, 571, 'AltarOfSseratus'),
(1691, 6276.62, -3264.6, 376.07, 0.0276335, 571, 'AltarOfRhunok'),
(1690, 6071.54, -3369.12, 350.617, 1.69268, 571, 'DrakMabwa'),
(1689, 6909.43, -4111.03, 467.354, 2.22675, 571, 'DubraJin'),
(1688, 6461.61, -3900.62, 482.288, 6.27547, 571, 'ZolMazStronghold'),
(1687, 6087.96, -3469.78, 365.552, 5.50187, 571, 'ZimRhuk'),
(1686, 5889.7, -3782.53, 361.962, 5.53329, 571, 'HebDrakkar'),
(1685, 5757.21, -3528.22, 388.185, 4.71648, 571, 'ZimTorga'),
(1684, 5569.49, -3447.85, 350.442, 3.95464, 571, 'PoolsOfJinAlai'),
(1683, 5376.1, -3732.46, 360.095, 3.91143, 571, 'AltarOfHarkoa'),
(1682, 5716.33, -4302.82, 373.996, 4.73609, 571, 'AltarOfQuetzlun'),
(1681, 6096.32, -4244.25, 319.05, 2.99251, 571, 'MamtothCrater'),
(1680, 6310.73, -4249.27, 447.474, 3.12603, 571, 'AltarOfMamtoth'),
(1679, 6499.12, -4481.15, 454.953, 3.02393, 571, 'ZolHeb'),
-- Wintergrasp
(1678, 4372.54, 1791.25, 356.766, 3.11312, 571, 'TheCauldronOfFlames'),
(1677, 4774.84, 2015.98, 424.402, 6.27041, 571, 'GlacialFalls'),
(1676, 4693.02, 3309.75, 374.64, 5.99946, 571, 'WesternBridge'),
(1675, 4607.97, 2846.56, 396.896, 0.179658, 571, 'CentralBridge'),
(1674, 4970.23, 2425.01, 320.791, 2.39449, 571, 'TheSunkenRing'),
(1673, 4607.46, 2368.2, 379.028, 0.434927, 571, 'EasternBridge'),
(1672, 4447.06, 2015, 434.694, 0.804061, 571, 'FlamewatchTower'),
(1671, 4403.72, 2380.28, 376.318, 5.73244, 571, 'EastsparkWorkshop'),
(1670, 4437.6, 2833.36, 405.225, 5.93272, 571, 'WintersEdgeTower'),
(1669, 4396.97, 3320.53, 372.424, 5.17873, 571, 'WestsparkWorkshop'),
(1665, 4589.59, 3560.4, 392.277, 4.29124, 571, 'ShadowsightTower'),
(1664, 4457.95, 3927.66, 411.536, 6.03089, 571, 'TheSteppeOfLife'),
(1663, 4803.77, 3801.29, 356.158, 5.84632, 571, 'TheForestOfShadows'),
(1662, 4991.69, 3333.74, 376.877, 3.54903, 571, 'TheBrokenTemple'),
(1661, 5178.08, 3420.28, 356.525, 4.18128, 571, 'TheChilledQuagmire'),
(1660, 5348.02, 2839.27, 409.239, 0.038305, 571, 'WintergraspFortress'),
-- The Storm Peaks
(1774, 6657, -1441, 682.935, 1.60449, 571, 'SnowblindTerrace'),
(1773, 6463, -1292, 466.597, 2.93967, 571, 'SnowblindHills'),
(1772, 7096, -918, 1067, 2.48807, 571, 'TheBroodmothersNext'),
(1771, 7843.9, -796.826, 1183.4, 4.79714, 571, 'GromarshCrashSite'),
(1659, 6327.28, -1576.64, 425.267, 5.7436, 571, 'Garm'),
(1658, 6327.91, -1320.87, 428.324, 4.6951, 571, 'GarmsBane'),
(1657, 6114.87, -581.335, 383.725, 3.86257, 571, 'SparksocketMinefield'),
(1656, 6472.49, -1040.14, 433.549, 0.0180354, 571, 'CrystalwebCavern'),
(1655, 6091.97, -932.899, 387.132, 4.44377, 571, 'RicketsFolly'),
(1654, 6123.7, -1059.19, 402.624, 4.65975, 571, 'K3'),
(1653, 8618.31, -622.963, 926.204, 2.47396, 571, 'TempleOfWisdom'),
(1652, 8186.44, -1934.66, 1738.48, 5.2896, 571, 'TempleOfOrder'),
(1651, 7528.82, -1906.84, 1473.62, 2.94125, 571, 'TempleOfWinter'),
(1650, 7890.03, -1418.43, 1534.06, 2.45823, 571, 'TempleOfInvention'),
(1649, 7771.56, -1772.27, 1279.5, 5.09325, 571, 'TerraceOfTheMakers'),
(1648, 8095.7, -2242.73, 1206.33, 1.27074, 571, 'HallOfTheShaper'),
(1646, 7934.5, -2582.04, 1154.45, 5.46475, 571, 'PlainOfEchoes'),
(1645, 8060.57, -2917.89, 1140.24, 5.67679, 571, 'TheHowlingHollow'),
(1644, 7977.41, -2680.05, 1138, 4.96209, 571, 'TempleOfLife'),
(1643, 7808.82, -2949.29, 1257.53, 4.11778, 571, 'CampTunkalo'),
(1642, 7776.83, -3202.63, 868.409, 6.05771, 571, 'Thunderfall'),
(1641, 7210.23, -3379.79, 845.316, 4.68326, 571, 'FjornsAnvil'),
(1640, 7393.51, -3247.05, 850.774, 1.29822, 571, 'FrostfieldLake'),
(1639, 7165.42, -2729.01, 776.77, 0.347884, 571, 'DunNiffelem'),
(1638, 7152.8, -2134.07, 761.915, 0.253633, 571, 'HibernalCavern'),
(1637, 7111.28, -2059.48, 766.21, 4.54192, 571, 'ValleyOfAncientWinters'),
(1636, 7056.37, -1698, 821.595, 3.61516, 571, 'BrunnhildarVillage'),
(1635, 6962.97, -1655.72, 810.822, 3.57196, 571, 'ThePitOfTheFang'),
(1634, 6933.2, -1351.58, 831.134, 1.70664, 571, 'TheForlomMine'),
(1633, 6840.09, -1061.41, 804.305, 2.93971, 571, 'SifreldarVillage'),
(1632, 7521.46, -899.843, 909.642, 4.72649, 571, 'EngineOfTheMakers'),
(1631, 7154.05, -754.211, 890.867, 5.41765, 571, 'AbandonedCamp'),
(1630, 6725.91, -783.269, 730.959, 0.163331, 571, 'BorsBreath'),
(1629, 7411.71, -531.622, 1896.85, 6.18734, 571, 'TempleOfStorms'),
(1628, 7243.53, -461.666, 916.621, 0.00232267, 571, 'Uldis'),
(1627, 6666.43, -211.341, 947.816, 1.56526, 571, 'Frosthold'),
(1626, 6768.53, -58.3224, 740.45, 1.49458, 571, 'RohemdalPass'),
(1625, 7393.04, 167.78, 770.688, 0.277205, 571, 'TheBlightedPool'),
(1624, 7335.99, -62.7787, 772.723, 2.46849, 571, 'Valkyrion'),
(1623, 7843.14, -110.96, 882.857, 2.24857, 571, 'TheFrozenMine'),
(1622, 7767.64, 29.2474, 1009.34, 0.120142, 571, 'Nidavelir'),
(1621, 7988.62, -291.5, 847.385, 5.69254, 571, 'NarvirsCradle'),
(1620, 8360.21, -139.844, 827.823, 6.25804, 571, 'SnowdriftPlains'),
-- Icecrown
(1770, 6319.63, 1740.14, 525, 0.0847461, 571, 'MalykrissTheVileHold'),
(1769, 6204, 2606, 570, 1.95007, 571, 'CathedralOfDarkness'),
(1619, 6216.68, -2.72999, 410.165, 0.391102, 571, 'TheArgentVanguard'),
(1618, 6244.26, 225.228, 387.66, 5.3116, 571, 'ValleyOfEchoes'),
(1617, 6447.27, 239.528, 397.736, 3.65834, 571, 'TheBreach'),
(1616, 6362.92, 1012.17, 327.467, 3.5798, 571, 'IronwallDam'),
(1615, 6873.3, 1174.87, 283.413, 3.61907, 571, 'TheBrokenFront'),
(1614, 7594.92, 1350.4, 333.679, 1.70663, 571, 'FirstLegionFowardCamp'),
(1613, 7678.03, 1372.25, 345.455, 2.71194, 571, 'TheValleyOfLostHope'),
(1612, 7497.14, 2598.56, 534.671, 3.43842, 571, 'IronwallRampart'),
(1611, 7233.18, 2971.8, 433.708, 5.13096, 571, 'IcecrownGlacier'),
(1610, 7841.6, 2274.98, 378.063, 3.79579, 571, 'TheValleyOfFallenHeroes'),
(1609, 7705.46, 1818.86, 354.901, 4.15315, 571, 'TheBombardment'),
(1608, 6749.04, 2544.51, 426.1, 0.383241, 571, 'TheConflagration'),
(1607, 6286.64, 2294.73, 481.885, 3.50913, 571, 'TheCourtOfBones'),
(1606, 8130.79, 437.999, 574.684, 0.30234, 571, 'SilentVigil'),
(1605, 10406.6, 915.842, 123.528, 1.88887, 571, 'HrothgarsLanding'),
(1604, 8103, 785.057, 481.016, 5.99883, 571, 'QuelDelarsRest'),
(1603, 7819.14, 1017.74, 449.338, 1.52599, 571, 'SindragosasFall'),
(1602, 7185.36, 609.466, 495.828, 1.16863, 571, 'ThePitOfFiends'),
(1601, 6809.66, 427.36, 465.349, 2.99863, 571, 'Scourgeholme'),
(1600, 6974.72, 1216.42, 415.465, 6.10879, 571, 'MordretharTheDeathGate'),
(1599, 7932.5, 2057.79, 500.268, 3.15567, 571, 'AldurtharTheDesolationGate'),
(1598, 7025.57, 1884.51, 532.252, 1.78908, 571, 'SaroniteMines'),
(1597, 7058.78, 1784.73, 527.87, 3.85076, 571, 'Ymirheim'),
(1596, 6514, 2563.77, 651.57, 0.752369, 571, 'CorpretharTheHorrorGate'),
(1595, 6635.72, 3210.13, 668.535, 4.1767, 571, 'SanctumOfReanimation'),
(1594, 6490.04, 3454.59, 595.554, 5.3862, 571, 'TheFleshwerks'),
(1593, 6955.48, 4514.21, 601.093, 4.98564, 571, 'Jotunheim'),
(1592, 7057.58, 4254.91, 676.829, 1.32961, 571, 'BalargardeFortress'),
(1591, 6869.59, 3878.78, 628.466, 4.16098, 571, 'KulgalarKeep'),
(1590, 7799.59, 3711.14, 653.015, 4.38089, 571, 'NjorndarVillage'),
(1589, 8428.12, 2894.52, 606.259, 1.60058, 571, 'UfrangsHall'),
(1588, 8454.85, 3137.16, 588.142, 3.88217, 571, 'SavageLedge'),
(1587, 7946.45, 3254.79, 676.137, 0.795563, 571, 'TheUnderhalls'),
(1586, 7865.1, 3350.07, 652.318, 2.33494, 571, 'TheCircleOfSuffering'),
(1585, 7994.08, 3303.67, 632.396, 0.815185, 571, 'HallOfHorrors'),
(1584, 8125.72, 3438.16, 672.373, 0.771992, 571, 'Valhalas'),
(1583, 7185.49, 3909.44, 591.7, 5.68466, 571, 'HallsOfTheAncestors'),
(1582, 7787.65, 5012.76, 4.37668, 3.1321, 571, 'OnslaughtHarbor'),
(1581, 7547.36, 4848.95, 52.4912, 3.15958, 571, 'TheCrimsonCathedral'),
-- Sholazar Basin 
(1580, 6352, 5812.13, 56.8665, 3.32617, 571, 'TheStormwrightsShelf'),
(1579, 6200.55, 4732.6, 224.713, 1.43729, 571, 'TheGlimmeringPillar'),
(1578, 6077.61, 4509.06, -80.9205, 0.32209, 571, 'DeathsHandEncampment'),
(1577, 6514.13, 4434.68, -49.7989, 3.6993, 571, 'TheBonefields'),
(1576, 6587.91, 4882.84, -42.9031, 3.11024, 571, 'TheSavageThicket'),
(1575, 6622.02, 5177.81, -41.6024, 4.14697, 571, 'SpearbornEncampment'),
(1574, 5196.96, 5504.35, -86.8983, 5.1601, 571, 'SparktouchedHaven'),
(1573, 5108.43, 5365.51, 224.995, 4.48466, 571, 'TheMosslightPillar'),
(1572, 4928.71, 5192.91, -92.8246, 3.52253, 571, 'Waygate'),
(1571, 4991.56, 4569.96, 148.897, 1.9753, 571, 'TheSkyreachPillar'),
(1570, 4997.3, 4324.79, -84.737, 1.97922, 571, 'SapphireHive'),
(1569, 5272.1, 4524.57, -83.0975, 3.81705, 571, 'FrenzyheartHill'),
(1568, 5540.35, 4104.43, 43.2335, 4.64957, 571, 'TheLifebloodPillar'),
(1567, 5588.53, 3860.26, -97.1207, 4.22938, 571, 'TheSunderedShard'),
(1566, 5702.6, 3456.26, 300.842, 4.83806, 571, 'MakersOverlook'),
(1565, 5800.24, 3556, -7.71123, 2.30908, 571, 'MosswalkerVillage'),
(1564, 5819.48, 3900.97, -57.8919, 2.6036, 571, 'TheLostLands'),
(1563, 6200.41, 3754.96, 167.473, 2.58396, 571, 'TheAvalanche'),
(1562, 5732.95, 4385.3, -136.637, 2.47012, 571, 'RainspeakerRapids'),
(1561, 5438.16, 4621.39, -142.395, 5.72167, 571, 'FrenzyheartRiver'),
(1560, 5653.33, 4565.95, -134.441, 1.00929, 571, 'RainspeakerCanopy'),
(1559, 5368.05, 4843.58, -197.195, 5.93375, 571, 'RiversHeart'),
(1558, 5501.47, 4733, -194.376, 2.29344, 571, 'LakesideLanding'),
(1557, 5660.99, 5098.01, -133.723, 5.82774, 571, 'WildgrowthMangal'),
(1556, 5794.79, 5479.6, 203.391, 3.75822, 571, 'TheSuntouchedPillar'),
(1555, 5923.1, 5326, -95.5351, 2.48981, 571, 'SwindlegrinsDig'),
(1554, 6092.06, 5317.29, -95.0416, 5.62353, 571, 'BittertideLake'),
(1553, 6118.86, 5662.16, 5.1509, 0.730512, 571, 'TheMakersPerch'),
(1552, 5561.69, 5748.65, -76.2847, 1.51591, 571, 'NesingwaryBaseCamp'),
(1551, 5505.3, 5990.77, -40.6402, 0.435986, 571, 'TheSeabreachFlow'),
(1550, 4874.82, 5901.06, -40.5268, 1.68084, 571, 'KartaksHold'),
-- Borean Tundra 
(1549, 4155.55, 6962.69, 164.671, 3.40091, 571, 'Coldarra'),
(1548, 3954.58, 6045.14, 23.8309, 3.93498, 571, 'TheWestrift'),
(1547, 3580.99, 6654.6, 195.569, 6.0948, 571, 'TransitusShield'),
(1546, 4130.43, 6262.65, 28.9689, 2.5307, 571, 'WinterfinCaverns'),
(1545, 4202.91, 6294, 11.929, 5.96681, 571, 'WinterfinVillage'),
(1544, 4497.91, 6358.62, 24.8969, 3.65774, 571, 'GlimmerBay'),
(1543, 4730.28, 5581.22, 6.86127, 4.8162, 571, 'TheLostPassage'),
(1542, 2803.56, 4696.17, 1.8599, 3.375, 571, 'NjordsBreathBay'),
(1541, 3998.88, 4818.12, 25.1371, 1.42266, 571, 'FizzcrankPumpingStation'),
(1540, 4018.28, 4921.16, -12.9943, 5.08653, 571, 'MidPointStation'),
(1539, 4210.47, 4797.77, -12.9951, 0.994613, 571, 'NorthPointStation'),
(1538, 4127.8, 5085.07, -0.181869, 2.57327, 571, 'WestPointStation'),
(1537, 3793.5, 4818.49, -12.9668, 0.282474, 571, 'SouthPointStation'),
(1536, 3814.71, 4668.49, -8.99847, 1.02861, 571, 'TheGeyserFields'),
(1535, 3577.14, 4821.72, 0.984557, 5.45434, 571, 'ScaldingPools'),
(1534, 3704.34, 4336.71, 13.5571, 1.77082, 571, 'SteamSprings'),
(1533, 3335.1, 3689.79, 32.4327, 3.15905, 571, 'TheWailingZiggurat'),
(1532, 3180.09, 3923.03, 31.1576, 0.897093, 571, 'FrozenReach'),
(1531, 3107.09, 3843.45, 22.3012, 4.7691, 571, 'DeathsStand'),
(1529, 3567.48, 3470.22, 50.2653, 2.13238, 571, 'Transborea'),
(1528, 4057.56, 3713.47, 92.6897, 0.400578, 571, 'SpireOfDecay'),
(1527, 3810.97, 3465.01, 82.904, 4.27652, 571, 'SpireOfPain'),
(1526, 4053.27, 3517.44, 115.678, 5.76875, 571, 'SpireOfBlood'),
(1525, 3694.55, 3724.94, 47.8466, 5.80018, 571, 'TempleCityofEnkilah'),
(1524, 4199.34, 4196.99, 88.6181, 5.40748, 571, 'DensOfDying'),
(1523, 4314.41, 4609.35, 26.678, 5.6235, 571, 'Talramas'),
(1522, 4357.79, 5470.73, 48.1204, 5.66279, 571, 'Magmoth'),
(1520, 4548.21, 6004.51, 74.8061, 1.57091, 571, 'RuinsOfEldranath'),
(1519, 4392.41, 6129.05, 0.116559, 2.6626, 571, 'WinterfinRetreat'),
(1518, 4323.22, 5859.49, 57.4084, 0.561667, 571, 'BlisteringPool'),
(1517, 3942.05, 5703.13, 63.3935, 5.57644, 571, 'SteeljawsCaravan'),
(1516, 3639.94, 5592.77, 32.8598, 5.58588, 571, 'ColdrockQuarry'),
(1515, 2913.33, 6502.97, 79.3253, 4.12898, 571, 'TorpsFarm'),
(1514, 2788.62, 6559.41, 49.4538, 3.34358, 571, 'WarsongGranary'),
(1513, 2647.44, 6674.29, 24.8536, 2.68779, 571, 'CoastOfEchoes'),
(1512, 2481.1, 6542.99, 41.0985, 4.26645, 571, 'PlainsOfNasam'),
(1511, 2797.04, 6997.76, 26.7684, 4.48636, 571, 'WarsongJetty'),
(1510, 2425.16, 6822.39, 4.41998, 5.57807, 571, 'Palea'),
(1509, 2987.91, 6817.53, 4.5871, 3.59886, 571, 'GarroshsLanding'),
(1508, 3101.81, 6326.78, 91.8155, 2.6446, 571, 'WarsongSlaughterhouse'),
(1507, 3458.82, 6218.68, 73.441, 3.67738, 571, 'BerylPoint'),
(1506, 3601.73, 5941.81, 136.218, 1.37617, 571, 'AmberLedge'),
(1505, 3179.06, 5735.6, 49.8884, 5.0911, 571, 'LakeKumuya'),
(1504, 2865.25, 5576.93, 56.1496, 1.14841, 571, 'BloodsporePlains'),
(1503, 3226.48, 5273.91, 47.0191, 2.8645, 571, 'DEHTAEncampment'),
(1502, 2953.95, 5029.19, 32.5341, 1.29836, 571, 'TheFloodPlains'),
(1501, 2562.73, 5082.13, 8.78205, 0.697533, 571, 'TheAbandonedReach'),
(1500, 2452.49, 5149.73, 30.2057, 0.575801, 571, 'FarshireLighthouse'),
(1499, 2709.2, 5295.72, 31.4857, 5.84583, 571, 'FarshireMine'),
(1498, 2601.02, 5276.52, 36.8806, 2.46075, 571, 'Farshire'),
(1497, 2104.41, 5514.25, 0.336683, 5.60234, 571, 'SandsOfNasam'),
(1496, 1471.34, 5802.38, 3.89735, 4.01848, 571, 'RiplashRuins'),
(1495, 1818.7, 5813.67, 0.434314, 6.06444, 571, 'RiplashStrand'),
(1494, 2535.97, 6126.06, 77.0848, 6.23719, 571, 'MightstoneQuarry'),
-- Dun Morogh
(1493, -6165.16, 383.46, 399.154, 6.27642, 0, 'Anvilmar'),
(1492, -6022.06, 60.0566, 406.509, 5.72273, 0, 'ColdridgePass');