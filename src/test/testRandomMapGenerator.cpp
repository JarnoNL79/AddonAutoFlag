// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "rttrDefines.h" // IWYU pragma: keep
#include "PointOutput.h"
#include "boost/interprocess/smart_ptr/unique_ptr.hpp"
#include "helpers/Deleter.h"
#include "mapGenerator/RandomConfig.h"
#include "mapGenerator/RandomMapGenerator.h"
#include "mapGenerator/VertexUtility.h"
#include "gameData/MaxPlayers.h"
#include <boost/test/unit_test.hpp>
#include <vector>

BOOST_AUTO_TEST_SUITE(RandomMapGeneratorTest)

/**
 * Tests the RandomMapGenerator.Create method. The generated map must have the
 * same width and height as defined inside of the settings.
 */
BOOST_AUTO_TEST_CASE(Create_CorrectSize)
{
    RandomConfig config(MapStyle::Random, 0x1337);
    MapSettings settings;
    settings.size = MapExtent(38, 30);
    settings.players = 1;
    settings.type = LT_GREENLAND;
    settings.minPlayerRadius = 0.2;
    settings.maxPlayerRadius = 0.3;

    RandomMapGenerator generator(config);
    boost::interprocess::unique_ptr<Map, Deleter<Map> > map(generator.Create(settings));

    BOOST_REQUIRE_EQUAL(map->size, settings.size);
}

/**
 * Tests the RandomMapGenerator.Create method. The generated map must contain the
 * the same number of headquarters as the number of players in the settings.
 */
BOOST_AUTO_TEST_CASE(Create_Headquarters)
{
    RandomConfig config(MapStyle::Random, 0x1337);
    MapSettings settings;
    settings.size = MapExtent(30u, 32u);
    settings.players = 2;
    settings.type = LT_GREENLAND;
    settings.minPlayerRadius = 0.2;
    settings.maxPlayerRadius = 0.3;

    RandomMapGenerator generator(config);

    boost::interprocess::unique_ptr<Map, Deleter<Map> > map(generator.Create(settings));
    BOOST_REQUIRE_EQUAL(map->players, settings.players);

    unsigned minSize = std::min(map->size.x, map->size.y) / 2;
    for(unsigned i = 0; i < settings.players; i++)
    {
        Point<uint16_t> p = map->positions[i];
        BOOST_REQUIRE_NE(p.x, 0xFF);
        BOOST_REQUIRE_NE(p.y, 0xFF);

        BOOST_REQUIRE_EQUAL(map->objectType[p.y * settings.size.x + p.x], i);
        BOOST_REQUIRE_EQUAL(map->objectInfo[p.y * settings.size.x + p.x], libsiedler2::OI_HeadquarterMask);

        double distance = VertexUtility::Distance(Position(p), Position(settings.size / 2), map->size);
        BOOST_REQUIRE_GE(distance, settings.minPlayerRadius * minSize);
        BOOST_REQUIRE_LE(distance, settings.maxPlayerRadius * minSize);
    }

    for(unsigned i = settings.players; i < MAX_PLAYERS; i++)
    {
        BOOST_REQUIRE_EQUAL(map->positions[i].x, 0xFF);
        BOOST_REQUIRE_EQUAL(map->positions[i].y, 0xFF);
    }
}

BOOST_AUTO_TEST_CASE(InvalidConfig)
{
    RandomConfig config(MapStyle::Random, 0x1337);
    RandomMapGenerator generator(config);

    MapSettings settings;
    settings.size = MapExtent(30u, 20u);
    settings.players = 0;
    settings.type = LT_WASTELAND;
    settings.minPlayerRadius = 0.2;
    settings.maxPlayerRadius = 0.3;

    boost::interprocess::unique_ptr<Map, Deleter<Map> > map(generator.Create(settings));
    BOOST_REQUIRE_GE(map->players, 1);

    settings.players = 99;
    map.reset(generator.Create(settings));
    BOOST_REQUIRE_LT(map->players, 99);

    settings.players = 2;
    settings.minPlayerRadius = -1;
    settings.maxPlayerRadius = 100;
    map.reset(generator.Create(settings));
    BOOST_REQUIRE_NE(map->positions[1].x, 0xFF);

    settings.minPlayerRadius = 1;
    settings.maxPlayerRadius = 1;
    map.reset(generator.Create(settings));
    BOOST_REQUIRE_NE(map->positions[1].x, 0xFF);

    settings.minPlayerRadius = 1;
    settings.maxPlayerRadius = 0.5;
    map.reset(generator.Create(settings));
    BOOST_REQUIRE_NE(map->positions[1].x, 0xFF);

    settings.ratioCoal = 0;
    settings.ratioGold = 0;
    settings.ratioGranite = 0;
    settings.ratioIron = 0;
    map.reset(generator.Create(settings));

    settings.size = MapExtent(33, 35);
    map.reset(generator.Create(settings));
    BOOST_REQUIRE_EQUAL(map->size, MapExtent(32, 34));
}

BOOST_AUTO_TEST_SUITE_END()
