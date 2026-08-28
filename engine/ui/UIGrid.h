#pragma once

#include "UIContainer.h"

namespace engine::ui {

// UIGrid - 2D grid layout (like Godot GridContainer)
class UIGrid : public UIContainer {
public:
    UIGrid();
    ~UIGrid() override = default;
    
    // Grid dimensions
    int getColumns() const { return columns_; }
    void setColumns(int columns) { columns_ = columns; setLayoutDirty(); }
    
    int getRows() const { return rows_; }
    void setRows(int rows) { rows_ = rows; setLayoutDirty(); }
    
    // Cell size
    float getCellWidth() const { return cellWidth_; }
    void setCellWidth(float width) { cellWidth_ = width; setLayoutDirty(); }
    
    float getCellHeight() const { return cellHeight_; }
    void setCellHeight(float height) { cellHeight_ = height; setLayoutDirty(); }
    
    // Spacing
    float getSpacing() const { return spacing_; }
    void setSpacing(float spacing) { spacing_ = spacing; setLayoutDirty(); }
    
    // Layout
    void layout() override;

private:
    int columns_ = 1;
    int rows_ = 1;
    float cellWidth_ = 32.0f;
    float cellHeight_ = 32.0f;
    float spacing_ = 0.0f;
};

} // namespace engine::ui
