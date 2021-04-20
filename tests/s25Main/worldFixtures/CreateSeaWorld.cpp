// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CreateSeaWorld.h"
#include "RttrForeachPt.h"
#include "initGameRNG.hpp"
#include "lua/GameDataLoader.h"
#include "world/GameWorld.h"
#include "world/MapLoader.h"
#include "gameData/TerrainDesc.h"
#include <boost/test/test_tools.hpp>

CreateSeaWorld::CreateSeaWorld(const MapExtent& size) : size_(size) {}

namespace {
bool PlaceHarbor(MapPoint pt, GameWorldBase& world, std::vector<MapPoint>& harbors)
{
    // Get all points within a radius of 3 and place the harbor on the first possible place
    std::vector<MapPoint> pts = world.GetPointsInRadiusWithCenter(pt, 3);
    for(MapPoint curPt : pts)
    {
        // Harbor only at castles
        world.RecalcBQ(curPt);
        if(world.GetNode(curPt).bq != BuildingQuality::Castle)
            continue;
        // We must have a coast around
        for(const MapPoint posCoastPt : world.GetNeighbours(curPt))
        {
            // Coast must not be water
            if(world.IsWaterPoint(posCoastPt))
                continue; // LCOV_EXCL_LINE
            // But somewhere around must be a sea
            for(const MapPoint posSeaPt : world.GetNeighbours(posCoastPt))
            {
                if(world.IsSeaPoint(posSeaPt))
                {
                    harbors.push_back(curPt);
                    return true;
                }
            }
        }
    }
    return false;
}
} // namespace

bool CreateSeaWorld::operator()(GameWorld& world) const
{
    // For consistent results
    initGameRNG(0);

    loadGameData(world.GetDescriptionWriteable());
    world.Init(size_);
    // Set everything to water
    DescIdx<TerrainDesc> t(0);
    const WorldDescription& desc = world.GetDescription();
    for(; t.value < desc.terrain.size(); t.value++)
    {
        if(desc.get(t).Is(ETerrain::Shippable) && desc.get(t).kind == TerrainKind::Water) //-V807
            break;
    }
    RTTR_FOREACH_PT(MapPoint, size_)
    {
        MapNode& node = world.GetNodeWriteable(pt);
        node.t1 = node.t2 = t;
    }

    /* We create an outlined square of land in the water so it looks like:
     * WWWWWWWWWWWWWWWWWWWWWWW  Height of water: Offset
     * WWWWWWWWWWWWWWWWWWWWWWW
     * WWLLLLLLLLLLLLLLLLLLLWW  Height of land: landSize
     * WWLLLLLLLLLLLLLLLLLLLWW  Width of water (left and right): offset (on each side)
     * WWLLWWWWWWWWWWWWWWWLLWW  Width of land: landSize (on each side)
     * WWLLWWWWWWWWWWWWWWWLLWW
     * WWLLWWWWWWWWWWWWWWWLLWW
     * WWLLWWWWWWWWWWWWWWWLLWW
     * WWLLWWWWWWWWWWWWWWWLLWW
     * WWLLLLLLLLLLLLLLLLLLLWW
     * WWLLLLLLLLLLLLLLLLLLLWW
     * WWWWWWWWWWWWWWWWWWWWWWW  Height of water: Offset
     * WWWWWWWWWWWWWWWWWWWWWWW
     */
    // Init some land stripes of size 15 (a bit less than the HQ radius)
    const MapCoord offset = 7;
    const MapCoord landSize = 15;
    // We need the offset at each side, the land on each side
    // and at least the same amount of water between the land
    const MapCoord minSize = landSize * 3 + offset * 2;
    if(size_.x < minSize || size_.y < minSize)
        throw std::runtime_error("World to small"); // LCOV_EXCL_LINE
    t = DescIdx<TerrainDesc>(0);
    for(; t.value < desc.terrain.size(); t.value++)
    {
        if(desc.get(t).Is(ETerrain::Buildable) && desc.get(t).kind == TerrainKind::Land)
            break;
    }
    // Vertical
    for(MapPoint pt(offset, offset); pt.y < size_.y - offset; ++pt.y)
    {
        for(pt.x = offset; pt.x < offset + landSize; ++pt.x)
        {
            MapNode& node = world.GetNodeWriteable(pt);
            node.t1 = node.t2 = t;
        }
        for(pt.x = size_.x - offset - landSize; pt.x < size_.x - offset; ++pt.x)
        {
            MapNode& node = world.GetNodeWriteable(pt);
            node.t1 = node.t2 = t;
        }
    }
    // Horizontal
    for(MapPoint pt(offset, offset); pt.x < size_.x - offset; ++pt.x)
    {
        for(pt.y = offset; pt.y < offset + landSize; ++pt.y)
        {
            MapNode& node = world.GetNodeWriteable(pt);
            node.t1 = node.t2 = t;
        }
        for(pt.y = size_.y - offset - landSize; pt.y < size_.y - offset; ++pt.y)
        {
            MapNode& node = world.GetNodeWriteable(pt);
            node.t1 = node.t2 = t;
        }
    }

    // Place HQs at top, left, right, bottom
    std::vector<MapPoint> hqPositions;
    hqPositions.push_back(MapPoint(size_.x / 2, offset + landSize / 2));
    hqPositions.push_back(MapPoint(offset + landSize / 2, size_.y / 2));
    hqPositions.push_back(MapPoint(size_.x - offset - landSize / 2, size_.y / 2));
    hqPositions.push_back(MapPoint(size_.x / 2, size_.y - offset - landSize / 2));

    std::vector<MapPoint> harbors;
    // Place harbors
    for(MapPoint pt : hqPositions)
    {
        unsigned harborsPlaced = 0;
        if(PlaceHarbor(pt - MapPoint(landSize / 2, 0), world, harbors))
            ++harborsPlaced;
        if(PlaceHarbor(pt + MapPoint(landSize / 2, 0), world, harbors))
            ++harborsPlaced;
        if(PlaceHarbor(pt - MapPoint(0, landSize / 2), world, harbors))
            ++harborsPlaced;
        if(PlaceHarbor(pt + MapPoint(0, landSize / 2), world, harbors))
            ++harborsPlaced;
        // Exactly 2 harbors should be placed per HQ (left and right or above and below)
        RTTR_Assert(harborsPlaced == 2);
    }

    BOOST_TEST_REQUIRE(MapLoader::InitSeasAndHarbors(world, harbors));

    if(!MapLoader::PlaceHQs(world, hqPositions, false))
        return false; // LCOV_EXCL_LINE
    world.InitAfterLoad();

    /* The HQs and harbor(ids) are here: (H=HQ, 1-8=harbor)
     * WWWWWWWWWWWWWWWWWWWWWWW
     * WWWWWWWWWW1WWWWWWWWWWWW
     * WWLLLLLLLLHLLLLLLLLLLWW
     * WWLLLLLLLLLLLLLLLLLLLWW
     * WWLLWWWWWW2WWWWWWWWLLWW
     * WWLLWWWWWWWWWWWWWWWLLWW
     * W3HL4WWWWWWWWWWWWW5HL6W
     * WWLLWWWWWWWWWWWWWWWLLWW
     * WWLLWWWWWWW7WWWWWWWLLWW
     * WWLLLLLLLLLHLLLLLLLLLWW
     * WWLLLLLLLLLLLLLLLLLLLWW
     * WWWWWWWWWWW8WWWWWWWWWWW
     * WWWWWWWWWWWWWWWWWWWWWWW
     */
    return true;
}

CreateWaterWorld::CreateWaterWorld(const MapExtent& size) : size_(size) {}

bool CreateWaterWorld::operator()(GameWorld& world) const
{
    // Only 2 players supported
    RTTR_Assert(world.GetNumPlayers() <= 2u);
    loadGameData(world.GetDescriptionWriteable());
    world.Init(size_);
    // Set everything to water
    DescIdx<TerrainDesc> t(0);
    const WorldDescription& desc = world.GetDescription();
    for(; t.value < desc.terrain.size(); t.value++)
    {
        if(desc.get(t).Is(ETerrain::Shippable) && desc.get(t).kind == TerrainKind::Water) //-V807
            break;
    }
    RTTR_FOREACH_PT(MapPoint, size_)
    {
        MapNode& node = world.GetNodeWriteable(pt);
        node.t1 = node.t2 = t;
    }
    const unsigned landRadius = 8;
    std::vector<MapPoint> hqPositions;
    hqPositions.push_back(MapPoint(10, 10));
    if(world.GetNumPlayers() > 1)
        hqPositions.push_back(world.MakeMapPoint(hqPositions.front() + size_ / 2)); // LCOV_EXCL_LINE
    t = DescIdx<TerrainDesc>(0);
    for(; t.value < desc.terrain.size(); t.value++)
    {
        if(desc.get(t).Is(ETerrain::Buildable) && desc.get(t).kind == TerrainKind::Land)
            break;
    }
    for(MapPoint hqPos : hqPositions)
    {
        std::vector<MapPoint> pts = world.GetPointsInRadiusWithCenter(hqPos, landRadius);
        for(MapPoint curPt : pts)
        {
            MapNode& node = world.GetNodeWriteable(curPt);
            node.t1 = node.t2 = t;
        }
    }
    BOOST_TEST_REQUIRE(MapLoader::PlaceHQs(world, hqPositions, false));

    std::vector<MapPoint> harbors;
    for(MapPoint hqPos : hqPositions)
    {
        BOOST_TEST_REQUIRE(PlaceHarbor(world.MakeMapPoint(hqPos - Position(landRadius, 0)), world, harbors));
        BOOST_TEST_REQUIRE(PlaceHarbor(world.MakeMapPoint(hqPos - Position(0, landRadius)), world, harbors));
        BOOST_TEST_REQUIRE(PlaceHarbor(world.MakeMapPoint(hqPos + Position(landRadius, 0)), world, harbors));
        BOOST_TEST_REQUIRE(PlaceHarbor(world.MakeMapPoint(hqPos + Position(0, landRadius)), world, harbors));
    }
    RTTR_Assert(harbors.size() == hqPositions.size() * 4u);
    BOOST_TEST_REQUIRE(MapLoader::InitSeasAndHarbors(world, harbors));
    return true;
}
