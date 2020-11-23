// Created by Modar Nasser on 12/11/2020.

#include <iostream>

#include "LDtkLoader/Layer.hpp"
#include "LDtkLoader/Utils.hpp"
#include "LDtkLoader/World.hpp"

using namespace ldtk;

Layer::Layer(const nlohmann::json& j, const World* w) :
type(getLayerTypeFromString(j["__type"].get<std::string>())),
name(j["__identifier"].get<std::string>()),
grid_size({j["__cWid"].get<unsigned int>(), j["__cHei"].get<unsigned int>()}),
cell_size(j["__gridSize"].get<unsigned int>()),
m_total_offset({j["__pxTotalOffsetX"].get<int>(), j["__pxTotalOffsetY"].get<int>()}),
m_opacity(j["__opacity"].get<float>()),
m_definition(&w->getLayerDef(name))
{
    std::string key = "gridTiles";
    int d_offset = 0;
    if (type == LayerType::IntGrid || type == LayerType::AutoLayer) {
        key = "autoLayerTiles";
        d_offset = 1;
    }
    for (const auto& tile : j[key]) {
        Tile new_tile;
        new_tile.coordId = tile["d"].get<std::vector<unsigned int>>()[d_offset];
        new_tile.position.x = tile["px"].get<std::vector<unsigned int>>()[0];
        new_tile.position.y = tile["px"].get<std::vector<unsigned int>>()[1];

        new_tile.tileId = tile["d"].get<std::vector<unsigned int>>()[d_offset+1];
        new_tile.texture_position.x = tile["src"].get<std::vector<unsigned int>>()[0];
        new_tile.texture_position.y = tile["src"].get<std::vector<unsigned int>>()[1];

        auto flip = tile["f"].get<unsigned int>();
        new_tile.flipX = flip & 1u;
        new_tile.flipY = (flip>>1u) & 1u;

        updateTileVertices(new_tile);

        m_tiles.push_back(new_tile);
        auto& last_tile = m_tiles[m_tiles.size()-1];
        m_tiles_map[last_tile.coordId] = &(last_tile);
    }

    for (const auto& ent : j["entityInstances"]) {
        Entity new_ent{ent, w};
        m_entities[new_ent.getName()].push_back(std::move(new_ent));
    }
}

Layer::Layer(Layer&& other) noexcept :
type(other.type),
name(other.name),
grid_size(other.grid_size),
cell_size(other.cell_size),
m_definition(other.m_definition),
m_total_offset(other.m_total_offset),
m_opacity(other.m_opacity),
m_tiles(std::move(other.m_tiles)),
m_entities(std::move(other.m_entities))
{
  for (auto& tile : m_tiles)
      m_tiles_map[tile.coordId] = &tile;
}

auto Layer::getOffset() const -> const IntPoint& {
    return m_total_offset;
}

void Layer::setOffset(const IntPoint& offset) {
    m_total_offset = offset;
}

auto Layer::getOpacity() const -> float {
    return m_opacity;
}

void Layer::setOpacity(float opacity) {
    m_opacity = std::min(1.f, std::max(0.f, opacity));
}

auto Layer::hasTileset() const -> bool {
    return m_definition->m_tileset != nullptr;
}

auto Layer::getTileset() const -> const Tileset& {
    return m_definition->getTileset();
}

auto Layer::allTiles() const -> const std::vector<Tile>& {
    return m_tiles;
}

auto Layer::getTile(unsigned int grid_x, unsigned int grid_y) const -> const Tile& {
    auto id = grid_x + grid_size.x*grid_y;
    if (m_tiles_map.count(id) > 0)
        return *(m_tiles_map.at(id));

    throw std::invalid_argument("Layer "+name+" does not have a tile at position ("+std::to_string(grid_x)+", "+std::to_string(grid_y)+")");
}

auto Layer::hasEntity(const std::string& entity_name) const -> bool {
    return m_entities.count(entity_name) > 0;
}

auto Layer::getEntities(const std::string& entity_name) const -> const std::vector<Entity>& {
    if (m_entities.count(entity_name) > 0)
        return m_entities.at(entity_name);
    throw std::invalid_argument("Layer "+name+" does not have Entity named "+entity_name);
}

void Layer::updateTileVertices(Tile& tile) const {
    auto& verts = tile.vertices;
    verts[0].pos.x = static_cast<float>(tile.position.x);           verts[0].pos.y = static_cast<float>(tile.position.y);
    verts[1].pos.x = static_cast<float>(tile.position.x+cell_size); verts[1].pos.y = static_cast<float>(tile.position.y);
    verts[2].pos.x = static_cast<float>(tile.position.x+cell_size); verts[2].pos.y = static_cast<float>(tile.position.y+cell_size);
    verts[3].pos.x = static_cast<float>(tile.position.x);           verts[3].pos.y = static_cast<float>(tile.position.y+cell_size);

    IntPoint modif[4];
    auto cell_size_i = static_cast<int>(cell_size);
    if (tile.flipX) {
        modif[0].x = cell_size_i; modif[1].x = -cell_size_i;
        modif[3].x = cell_size_i; modif[2].x = -cell_size_i;
    }
    if (tile.flipY) {
        modif[0].y =  cell_size_i; modif[1].y =  cell_size_i;
        modif[3].y = -cell_size_i; modif[2].y = -cell_size_i;
    }
    UIntPoint tex_coo[4] = { {0, 0},  {16, 0},  {16, 16},  {0, 16}};
    for (int i = 0; i < 4; ++i) {
        verts[i].tex.x = tile.texture_position.x+tex_coo[i].x+modif[i].x;
        verts[i].tex.y = tile.texture_position.y+tex_coo[i].y+modif[i].y;
    }
}
