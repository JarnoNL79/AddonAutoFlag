// Copyright (C) 2017 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "mapGenerator/MapSettings.h"
#include "gameData/MaxPlayers.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(MapSettingsTests)

BOOST_AUTO_TEST_CASE(MakeValid_sets_default_name_and_author)
{
    MapSettings settings;
    settings.MakeValid();
    BOOST_TEST(settings.name == "Random");
    BOOST_TEST(settings.author == "AutoGenerated");
}

BOOST_AUTO_TEST_CASE(MakeValid_ensures_valid_number_of_players)
{
    MapSettings settings;
    settings.numPlayers = 0;
    settings.MakeValid();
    BOOST_TEST(settings.numPlayers == 1u);

    settings.numPlayers = MAX_PLAYERS + 1;
    settings.MakeValid();
    BOOST_TEST(settings.numPlayers == MAX_PLAYERS);
}

BOOST_AUTO_TEST_CASE(MakeValid_ensures_map_size_is_even)
{
    MapSettings settings;
    settings.size = MapExtent(17, 19);
    settings.MakeValid();
    BOOST_TEST(settings.size.x % 2u == 0u);
    BOOST_TEST(settings.size.y % 2u == 0u);
}

BOOST_AUTO_TEST_CASE(MakeValid_ensures_minimum_size_is_16_x_16)
{
    MapSettings settings;
    settings.size = MapExtent(14, 14);
    settings.MakeValid();
    BOOST_TEST(settings.size == MapExtent(16, 16));
}

BOOST_AUTO_TEST_CASE(MakeValid_ensures_total_ratio_for_mountain_resources_is_at_least_1)
{
    MapSettings settings;
    settings.ratioCoal = 0;
    settings.ratioGold = 0;
    settings.ratioIron = 0;
    settings.ratioGranite = 0;
    settings.MakeValid();
    BOOST_TEST(settings.ratioCoal + settings.ratioGold + settings.ratioIron + settings.ratioGranite > 0);
}

BOOST_AUTO_TEST_SUITE_END()
