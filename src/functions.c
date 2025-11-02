#include "functions.h"
#include "resources.h"

static u16 g_fontTileBase = 0;
static bool g_fontInitialized = FALSE;

void initCustomFont() {
    if(g_fontInitialized) return;
    
    g_fontTileBase = TILE_USER_INDEX + 300;
    VDP_loadTileSet(customFontTiles.tileset, g_fontTileBase, DMA);
    
    PAL_setPalette(PAL0, customFontTiles.palette->data, DMA);

    g_fontInitialized = TRUE;
}

void C_DrawText(const char* str, u16 x, u16 y, u16 palette) {
    if(!g_fontInitialized) return;
    
    u16 len = strlen(str);
    
    for(u16 i = 0; i < len; i++) {
        char c = str[i];
        
        // Only handle printable ASCII
        if(c < 32 || c > 126) {
            c = 32;
        }
        
        // Calculate position in 16x6 grid
        u16 charIndex = c - 32; 
        u16 gridX = charIndex % 16;
        u16 gridY = charIndex / 16;
        
        u16 rowOffset = gridY * 32;
        u16 colOffset = gridX; //Since I'm using 16 height, 2 tiles per character
        
        u16 topTile = g_fontTileBase + rowOffset + colOffset;
        u16 bottomTile = topTile + 16; 
        
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(palette, 0, 0, 0, topTile), x + i, y); //top
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(palette, 0, 0, 0, bottomTile), x + i, y + 1); //bottom
    }
}

void C_SClearText(u16 x, u16 y, u16 length) {
    for(u16 i = 0; i < length; i++) {
        VDP_setTileMapXY(BG_A, 0, x + i, y); 
        VDP_setTileMapXY(BG_A, 0, x + i, y + 1);
    }
}