#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WORLD_SIZE 48
#define WORLD_HEIGHT 128
#define RENDER_RADIUS 18
#define MAX_MOBS 48
#define MAX_CHAT_LINES 8
#define INVENTORY_SLOTS 36
#define HOTBAR_SLOTS 9
#define MAX_ITEM_DROPS 256
#define PLAYER_MOVE_SPEED 5.0f
#define PLAYER_GRAVITY 18.0f
#define PLAYER_JUMP_SPEED 7.2f
#define ITEM_DROP_HALF_HEIGHT 0.11f
#define ITEM_DROP_PICKUP_RADIUS 1.2f

typedef enum {
    BLOCK_AIR = 0,
    BLOCK_GRASS,
    BLOCK_DIRT,
    BLOCK_STONE,
    BLOCK_COBBLESTONE,
    BLOCK_OAK_WOOD,
    BLOCK_OAK_LEAVES,
    BLOCK_BIRCH_WOOD,
    BLOCK_BIRCH_LEAVES,
    BLOCK_OAK_PLANKS,
    BLOCK_BIRCH_PLANKS,
    BLOCK_CRAFTING_TABLE,
    BLOCK_TORCH,
    BLOCK_FURNACE,
    BLOCK_COAL_ORE,
    BLOCK_IRON_ORE,
    BLOCK_GOLD_ORE,
    BLOCK_DIAMOND_ORE,
    BLOCK_TYPE_COUNT
} BlockType;

typedef enum {
    ITEM_AIR = 0,
    ITEM_GRASS,
    ITEM_DIRT,
    ITEM_STONE,
    ITEM_COBBLESTONE,
    ITEM_OAK_LOG,
    ITEM_OAK_PLANKS,
    ITEM_BIRCH_LOG,
    ITEM_BIRCH_PLANKS,
    ITEM_STICK,
    ITEM_LEATHER,
    ITEM_DIAMOND,
    ITEM_IRON_INGOT,
    ITEM_GOLD_INGOT,
    ITEM_COAL,
    ITEM_TORCH,
    ITEM_FURNACE,
    ITEM_CRAFTING_TABLE,
    ITEM_WOOD_AXE,
    ITEM_STONE_AXE,
    ITEM_IRON_AXE,
    ITEM_DIAMOND_AXE,
    ITEM_GOLD_AXE,
    ITEM_WOOD_PICKAXE,
    ITEM_STONE_PICKAXE,
    ITEM_IRON_PICKAXE,
    ITEM_DIAMOND_PICKAXE,
    ITEM_GOLD_PICKAXE,
    ITEM_WOOD_SHOVEL,
    ITEM_STONE_SHOVEL,
    ITEM_IRON_SHOVEL,
    ITEM_DIAMOND_SHOVEL,
    ITEM_GOLD_SHOVEL,
    ITEM_WOOD_HOE,
    ITEM_STONE_HOE,
    ITEM_IRON_HOE,
    ITEM_DIAMOND_HOE,
    ITEM_GOLD_HOE,
    ITEM_LEATHER_HELMET,
    ITEM_LEATHER_CHESTPLATE,
    ITEM_LEATHER_LEGGINGS,
    ITEM_LEATHER_BOOTS,
    ITEM_STONE_HELMET,
    ITEM_STONE_CHESTPLATE,
    ITEM_STONE_LEGGINGS,
    ITEM_STONE_BOOTS,
    ITEM_IRON_HELMET,
    ITEM_IRON_CHESTPLATE,
    ITEM_IRON_LEGGINGS,
    ITEM_IRON_BOOTS,
    ITEM_DIAMOND_HELMET,
    ITEM_DIAMOND_CHESTPLATE,
    ITEM_DIAMOND_LEGGINGS,
    ITEM_DIAMOND_BOOTS,
    ITEM_GOLD_HELMET,
    ITEM_GOLD_CHESTPLATE,
    ITEM_GOLD_LEGGINGS,
    ITEM_GOLD_BOOTS,
    ITEM_COOKED_BEEF,
    ITEM_PORKCHOP,
    ITEM_COOKED_PORKCHOP,
    ITEM_BEEF,
    ITEM_RAW_MUTTON,
    ITEM_COOKED_MUTTON
} ItemType;

typedef enum {
    ENTITY_NONE = 0,
    ENTITY_SHEEP,
    ENTITY_COW,
    ENTITY_PIG
} EntityType;

typedef enum {
    GAME_MENU = 0,
    GAME_PLAYING,
    GAME_SETTINGS
} GameState;

typedef struct {
    EntityType type;
    Vector3 position;
    bool alive;
} Mob;

typedef struct {
    bool active;
    ItemType item;
    int count;
    Vector3 position;
    Vector3 velocity;
} ItemDrop;

static BlockType world[WORLD_SIZE][WORLD_HEIGHT][WORLD_SIZE];
static Mob mobs[MAX_MOBS];
static ItemDrop itemDrops[MAX_ITEM_DROPS];
static Texture2D blockTextures[BLOCK_TYPE_COUNT];
static bool blockTexturesLoaded[BLOCK_TYPE_COUNT];
static Model blockModels[BLOCK_TYPE_COUNT];
static bool blockModelsLoaded[BLOCK_TYPE_COUNT];
static ItemType inventory[INVENTORY_SLOTS];
static int inventoryCount[INVENTORY_SLOTS];
static ItemType craftingGrid[9];
static int craftingGridCount[9];
static int selectedSlot = 0;
static int craftingCursor = 0;
static bool inventoryOpen = false;
static bool craftingTableMode = false;
static bool chatOpen = false;
static char chatBuffer[96] = {0};
static char chatLog[MAX_CHAT_LINES][96];
static int chatLineCount = 0;
static int hunger = 100;
static float hungerTimer = 0.0f;
static GameState gameState = GAME_MENU;
static bool multiplayerEnabled = false;
static int menuSelection = 0;
static int settingsSelection = 0;
static bool screenshotRequested = false;
static char screenshotPath[256] = {0};

static inline bool InBounds(int x, int y, int z) {
    return x >= 0 && x < WORLD_SIZE && y >= 0 && y < WORLD_HEIGHT && z >= 0 && z < WORLD_SIZE;
}
static inline BlockType GetBlock(int x, int y, int z) {
    return InBounds(x, y, z) ? world[x][y][z] : BLOCK_AIR;
}
static inline void SetBlock(int x, int y, int z, BlockType type) {
    if (InBounds(x, y, z)) world[x][y][z] = type;
}
static inline bool IsBlockVisible(int x, int y, int z) {
    if (GetBlock(x + 1, y, z) == BLOCK_AIR) return true;
    if (GetBlock(x - 1, y, z) == BLOCK_AIR) return true;
    if (GetBlock(x, y + 1, z) == BLOCK_AIR) return true;
    if (GetBlock(x, y - 1, z) == BLOCK_AIR) return true;
    if (GetBlock(x, y, z + 1) == BLOCK_AIR) return true;
    if (GetBlock(x, y, z - 1) == BLOCK_AIR) return true;
    return false;
}
static inline Vector3 Vec3Subtract(Vector3 a, Vector3 b) { return (Vector3){a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline Vector3 Vec3Add(Vector3 a, Vector3 b) { return (Vector3){a.x + b.x, a.y + b.y, a.z + b.z}; }
static inline Vector3 Vec3Scale(Vector3 v, float s) { return (Vector3){v.x * s, v.y * s, v.z * s}; }
static inline float Vec3Length(Vector3 v) { return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }
static inline float Vec3Dot(Vector3 a, Vector3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline Vector3 Vec3Normalize(Vector3 v) { float len = Vec3Length(v); if (len > 0.0001f) return Vec3Scale(v, 1.0f / len); return (Vector3){0, 0, 0}; }
static inline float Clampf(float value, float min, float max) { if (value < min) return min; if (value > max) return max; return value; }

static Color ItemColor(ItemType item) {
    switch (item) {
        case ITEM_GRASS: return (Color){90, 180, 70, 255};
        case ITEM_DIRT: return (Color){135, 85, 35, 255};
        case ITEM_STONE:
        case ITEM_COBBLESTONE:
        case ITEM_STONE_AXE:
        case ITEM_STONE_PICKAXE:
        case ITEM_STONE_SHOVEL:
        case ITEM_STONE_HELMET:
        case ITEM_STONE_CHESTPLATE:
        case ITEM_STONE_LEGGINGS:
        case ITEM_STONE_BOOTS:
            return (Color){140, 140, 140, 255};
        case ITEM_OAK_LOG:
        case ITEM_OAK_PLANKS:
        case ITEM_WOOD_AXE:
        case ITEM_WOOD_PICKAXE:
        case ITEM_WOOD_SHOVEL:
            return (Color){150, 110, 60, 255};
        case ITEM_BIRCH_LOG:
        case ITEM_BIRCH_PLANKS:
            return (Color){220, 215, 190, 255};
        case ITEM_STICK: return (Color){180, 140, 90, 255};
        case ITEM_LEATHER:
        case ITEM_LEATHER_HELMET:
        case ITEM_LEATHER_CHESTPLATE:
        case ITEM_LEATHER_LEGGINGS:
        case ITEM_LEATHER_BOOTS:
            return (Color){190, 150, 110, 255};
        case ITEM_DIAMOND:
        case ITEM_DIAMOND_AXE:
        case ITEM_DIAMOND_PICKAXE:
        case ITEM_DIAMOND_SHOVEL:
        case ITEM_DIAMOND_HOE:
        case ITEM_DIAMOND_HELMET:
        case ITEM_DIAMOND_CHESTPLATE:
        case ITEM_DIAMOND_LEGGINGS:
        case ITEM_DIAMOND_BOOTS:
            return (Color){80, 220, 230, 255};
        case ITEM_IRON_INGOT:
        case ITEM_IRON_AXE:
        case ITEM_IRON_PICKAXE:
        case ITEM_IRON_SHOVEL:
        case ITEM_IRON_HOE:
        case ITEM_IRON_HELMET:
        case ITEM_IRON_CHESTPLATE:
        case ITEM_IRON_LEGGINGS:
        case ITEM_IRON_BOOTS:
            return (Color){190, 190, 190, 255};
        case ITEM_GOLD_INGOT:
        case ITEM_GOLD_AXE:
        case ITEM_GOLD_PICKAXE:
        case ITEM_GOLD_SHOVEL:
        case ITEM_GOLD_HELMET:
        case ITEM_GOLD_CHESTPLATE:
        case ITEM_GOLD_LEGGINGS:
        case ITEM_GOLD_BOOTS:
            return (Color){255, 215, 0, 255};
        case ITEM_COAL: return BLACK;
        case ITEM_TORCH:
        case ITEM_COOKED_BEEF:
        case ITEM_BEEF:
            return RED;
        case ITEM_PORKCHOP:
        case ITEM_COOKED_PORKCHOP:
            return PINK;
        case ITEM_RAW_MUTTON:
        case ITEM_COOKED_MUTTON:
            return (Color){220, 180, 180, 255};
        case ITEM_FURNACE:
        case ITEM_CRAFTING_TABLE:
            return (Color){100, 100, 105, 255};
        default: return WHITE;
    }
}

static const char *ItemName(ItemType item) {
    switch (item) {
        case ITEM_GRASS: return "Grass";
        case ITEM_DIRT: return "Dirt";
        case ITEM_STONE: return "Stone";
        case ITEM_COBBLESTONE: return "Cobblestone";
        case ITEM_OAK_LOG: return "Oak Log";
        case ITEM_OAK_PLANKS: return "Oak Planks";
        case ITEM_BIRCH_LOG: return "Birch Log";
        case ITEM_BIRCH_PLANKS: return "Birch Planks";
        case ITEM_STICK: return "Stick";
        case ITEM_LEATHER: return "Leather";
        case ITEM_DIAMOND: return "Diamond";
        case ITEM_IRON_INGOT: return "Iron";
        case ITEM_GOLD_INGOT: return "Gold";
        case ITEM_COAL: return "Coal";
        case ITEM_TORCH: return "Torch";
        case ITEM_FURNACE: return "Furnace";
        case ITEM_CRAFTING_TABLE: return "Crafting Table";
        case ITEM_WOOD_AXE: return "Wood Axe";
        case ITEM_STONE_AXE: return "Stone Axe";
        case ITEM_IRON_AXE: return "Iron Axe";
        case ITEM_DIAMOND_AXE: return "Diamond Axe";
        case ITEM_GOLD_AXE: return "Gold Axe";
        case ITEM_WOOD_PICKAXE: return "Wood Pickaxe";
        case ITEM_STONE_PICKAXE: return "Stone Pickaxe";
        case ITEM_IRON_PICKAXE: return "Iron Pickaxe";
        case ITEM_DIAMOND_PICKAXE: return "Diamond Pickaxe";
        case ITEM_GOLD_PICKAXE: return "Gold Pickaxe";
        case ITEM_WOOD_SHOVEL: return "Wood Shovel";
        case ITEM_STONE_SHOVEL: return "Stone Shovel";
        case ITEM_IRON_SHOVEL: return "Iron Shovel";
        case ITEM_DIAMOND_SHOVEL: return "Diamond Shovel";
        case ITEM_GOLD_SHOVEL: return "Gold Shovel";
        case ITEM_LEATHER_HELMET: return "Leather Helmet";
        case ITEM_LEATHER_CHESTPLATE: return "Leather Chestplate";
        case ITEM_LEATHER_LEGGINGS: return "Leather Leggings";
        case ITEM_LEATHER_BOOTS: return "Leather Boots";
        case ITEM_STONE_HELMET: return "Stone Helmet";
        case ITEM_STONE_CHESTPLATE: return "Stone Chestplate";
        case ITEM_STONE_LEGGINGS: return "Stone Leggings";
        case ITEM_STONE_BOOTS: return "Stone Boots";
        case ITEM_IRON_HELMET: return "Iron Helmet";
        case ITEM_IRON_CHESTPLATE: return "Iron Chestplate";
        case ITEM_IRON_LEGGINGS: return "Iron Leggings";
        case ITEM_IRON_BOOTS: return "Iron Boots";
        case ITEM_DIAMOND_HELMET: return "Diamond Helmet";
        case ITEM_DIAMOND_CHESTPLATE: return "Diamond Chestplate";
        case ITEM_DIAMOND_LEGGINGS: return "Diamond Leggings";
        case ITEM_DIAMOND_BOOTS: return "Diamond Boots";
        case ITEM_GOLD_HELMET: return "Gold Helmet";
        case ITEM_GOLD_CHESTPLATE: return "Gold Chestplate";
        case ITEM_GOLD_LEGGINGS: return "Gold Leggings";
        case ITEM_GOLD_BOOTS: return "Gold Boots";
        case ITEM_COOKED_BEEF: return "Cooked Beef";
        case ITEM_BEEF: return "Beef";
        case ITEM_PORKCHOP: return "Porkchop";
        case ITEM_COOKED_PORKCHOP: return "Cooked Porkchop";
        case ITEM_RAW_MUTTON: return "Raw Mutton";
        case ITEM_COOKED_MUTTON: return "Cooked Mutton";
        default: return "Air";
    }
}

static void AddChatMessage(const char *text) {
    if (chatLineCount < MAX_CHAT_LINES) {
        strcpy(chatLog[chatLineCount], text);
        chatLineCount++;
    } else {
        for (int i = MAX_CHAT_LINES - 1; i > 0; i--) strcpy(chatLog[i], chatLog[i - 1]);
        strcpy(chatLog[0], text);
    }
}

static void ClearCraftingGrid(void) {
    for (int i = 0; i < 9; i++) {
        craftingGrid[i] = ITEM_AIR;
        craftingGridCount[i] = 0;
    }
    craftingCursor = 0;
}

static void LoadBlockTextures(void) {
    const struct { BlockType type; const char *path; } mappings[] = {
        {BLOCK_GRASS, "block/grass_block_top.png"},
        {BLOCK_DIRT, "block/dirt.png"},
        {BLOCK_STONE, "block/stone.png"},
        {BLOCK_COBBLESTONE, "block/cobblestone.png"},
        {BLOCK_OAK_WOOD, "block/oak_log.png"},
        {BLOCK_OAK_LEAVES, "block/oak_leaves.png"},
        {BLOCK_BIRCH_WOOD, "block/birch_log.png"},
        {BLOCK_BIRCH_LEAVES, "block/birch_leaves.png"},
        {BLOCK_OAK_PLANKS, "block/oak_planks.png"},
        {BLOCK_BIRCH_PLANKS, "block/birch_planks.png"},
        {BLOCK_CRAFTING_TABLE, "block/crafting_table_side.png"},
        {BLOCK_TORCH, "block/torch.png"},
        {BLOCK_FURNACE, "block/furnace_side.png"},
        {BLOCK_COAL_ORE, "block/coal_ore.png"},
        {BLOCK_IRON_ORE, "block/iron_ore.png"},
        {BLOCK_GOLD_ORE, "block/gold_ore.png"},
        {BLOCK_DIAMOND_ORE, "block/diamond_ore.png"}
    };

    for (int i = 0; i < (int)(sizeof(mappings) / sizeof(mappings[0])); i++) {
        BlockType type = mappings[i].type;
        if (FileExists(mappings[i].path)) {
            blockTextures[type] = LoadTexture(mappings[i].path);
            blockTexturesLoaded[type] = true;
            if (!blockModelsLoaded[type]) {
                Mesh cubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
                blockModels[type] = LoadModelFromMesh(cubeMesh);
                blockModelsLoaded[type] = true;
                SetMaterialTexture(&blockModels[type].materials[0], MATERIAL_MAP_ALBEDO, blockTextures[type]);
            }
        } else {
            blockTexturesLoaded[type] = false;
        }
    }
}

static void UnloadBlockTextures(void) {
    for (int i = 1; i < BLOCK_TYPE_COUNT; i++) {
        if (blockModelsLoaded[i]) UnloadModel(blockModels[i]);
        if (blockTexturesLoaded[i]) UnloadTexture(blockTextures[i]);
        blockModelsLoaded[i] = false;
        blockTexturesLoaded[i] = false;
    }
}

static void DrawBlockVisual(BlockType type, Vector3 pos) {
    if (type > BLOCK_AIR && type < BLOCK_TYPE_COUNT && blockModelsLoaded[type]) {
        DrawModel(blockModels[type], pos, 1.0f, WHITE);
        return;
    }

    switch (type) {
        case BLOCK_GRASS: {
            DrawCube(pos, 1.0f, 1.0f, 1.0f, (Color){140, 95, 55, 255});
            DrawCube((Vector3){pos.x, pos.y + 0.2f, pos.z}, 1.0f, 0.6f, 1.0f, (Color){80, 160, 70, 255});
            DrawCube((Vector3){pos.x, pos.y + 0.25f, pos.z}, 1.0f, 0.05f, 1.0f, (Color){40, 120, 30, 255});
            break;
        }
        case BLOCK_DIRT:
            DrawCube(pos, 1.0f, 1.0f, 1.0f, (Color){130, 85, 35, 255});
            for (int i = -1; i <= 1; i++) for (int j = -1; j <= 1; j++) if ((i + j) % 2 == 0) DrawCube((Vector3){pos.x + i * 0.2f, pos.y + 0.18f, pos.z + j * 0.2f}, 0.15f, 0.15f, 0.15f, (Color){95, 55, 20, 255});
            break;
        case BLOCK_STONE:
            DrawCube(pos, 1.0f, 1.0f, 1.0f, (Color){150, 150, 150, 255});
            break;
        case BLOCK_COBBLESTONE:
            DrawCube(pos, 1.0f, 1.0f, 1.0f, (Color){125, 125, 125, 255});
            for (int i = 0; i < 4; i++) DrawCube((Vector3){pos.x + ((i % 2) ? 0.25f : -0.25f), pos.y + 0.1f + (i % 2) * 0.05f, pos.z + ((i % 3) ? 0.2f : -0.2f)}, 0.2f, 0.2f, 0.2f, (Color){90, 90, 90, 255});
            break;
        case BLOCK_OAK_WOOD:
            DrawCube(pos, 1.0f, 1.0f, 1.0f, (Color){140, 100, 60, 255});
            for (int i = -1; i <= 1; i++) DrawCube((Vector3){pos.x + i * 0.12f, pos.y, pos.z}, 0.1f, 1.0f, 0.18f, (Color){105, 70, 35, 255});
            break;
        case BLOCK_OAK_LEAVES:
            DrawCube(pos, 1.0f, 1.0f, 1.0f, (Color){90, 150, 70, 255});
            DrawCube((Vector3){pos.x, pos.y + 0.08f, pos.z}, 0.9f, 0.9f, 0.9f, (Color){120, 180, 90, 255});
            break;
        case BLOCK_BIRCH_WOOD:
            DrawCube(pos, 1.0f, 1.0f, 1.0f, (Color){220, 215, 190, 255});
            DrawCube((Vector3){pos.x, pos.y, pos.z}, 1.0f, 0.1f, 1.0f, (Color){190, 170, 135, 255});
            break;
        case BLOCK_BIRCH_LEAVES:
            DrawCube(pos, 1.0f, 1.0f, 1.0f, (Color){120, 180, 100, 255});
            DrawCube((Vector3){pos.x, pos.y + 0.05f, pos.z}, 0.9f, 0.9f, 0.9f, (Color){160, 210, 130, 255});
            break;
        case BLOCK_OAK_PLANKS:
            DrawCube(pos, 1.0f, 1.0f, 1.0f, (Color){150, 110, 60, 255});
            break;
        case BLOCK_BIRCH_PLANKS:
            DrawCube(pos, 1.0f, 1.0f, 1.0f, (Color){215, 205, 175, 255});
            break;
        case BLOCK_CRAFTING_TABLE:
            DrawCube(pos, 1.0f, 1.0f, 1.0f, (Color){140, 100, 55, 255});
            DrawCube((Vector3){pos.x, pos.y + 0.26f, pos.z}, 0.95f, 0.2f, 0.95f, (Color){190, 150, 100, 255});
            DrawCube((Vector3){pos.x - 0.28f, pos.y + 0.08f, pos.z - 0.28f}, 0.22f, 0.2f, 0.22f, (Color){120, 80, 35, 255});
            DrawCube((Vector3){pos.x + 0.28f, pos.y + 0.08f, pos.z - 0.28f}, 0.22f, 0.2f, 0.22f, (Color){120, 80, 35, 255});
            DrawCube((Vector3){pos.x - 0.28f, pos.y + 0.08f, pos.z + 0.28f}, 0.22f, 0.2f, 0.22f, (Color){120, 80, 35, 255});
            DrawCube((Vector3){pos.x + 0.28f, pos.y + 0.08f, pos.z + 0.28f}, 0.22f, 0.2f, 0.22f, (Color){120, 80, 35, 255});
            break;
        case BLOCK_TORCH:
            DrawCube(pos, 0.14f, 0.5f, 0.14f, (Color){150, 100, 60, 255});
            DrawCube((Vector3){pos.x, pos.y + 0.28f, pos.z}, 0.22f, 0.22f, 0.22f, (Color){255, 210, 80, 255});
            break;
        case BLOCK_FURNACE:
            DrawCube(pos, 1.0f, 1.0f, 1.0f, (Color){90, 90, 95, 255});
            DrawCube((Vector3){pos.x, pos.y + 0.12f, pos.z}, 0.82f, 0.42f, 0.82f, (Color){140, 140, 140, 255});
            break;
        case BLOCK_COAL_ORE:
            DrawCube(pos, 1.0f, 1.0f, 1.0f, (Color){130, 130, 130, 255});
            for (int i = 0; i < 5; i++) {
                float ox = (float)((rand() % 3) - 1) * 0.25f;
                float oy = (float)((rand() % 3) - 1) * 0.25f;
                float oz = (float)((rand() % 3) - 1) * 0.25f;
                DrawCube((Vector3){pos.x + ox, pos.y + oy, pos.z + oz}, 0.15f, 0.15f, 0.15f, BLACK);
            }
            break;
        case BLOCK_IRON_ORE:
            DrawCube(pos, 1.0f, 1.0f, 1.0f, (Color){140, 140, 140, 255});
            for (int i = 0; i < 5; i++) {
                float ox = (float)((rand() % 3) - 1) * 0.25f;
                float oy = (float)((rand() % 3) - 1) * 0.25f;
                float oz = (float)((rand() % 3) - 1) * 0.25f;
                DrawCube((Vector3){pos.x + ox, pos.y + oy, pos.z + oz}, 0.15f, 0.15f, 0.15f, (Color){200, 170, 130, 255});
            }
            break;
        case BLOCK_GOLD_ORE:
            DrawCube(pos, 1.0f, 1.0f, 1.0f, (Color){140, 140, 140, 255});
            for (int i = 0; i < 5; i++) {
                float ox = (float)((rand() % 3) - 1) * 0.25f;
                float oy = (float)((rand() % 3) - 1) * 0.25f;
                float oz = (float)((rand() % 3) - 1) * 0.25f;
                DrawCube((Vector3){pos.x + ox, pos.y + oy, pos.z + oz}, 0.15f, 0.15f, 0.15f, (Color){255, 215, 0, 255});
            }
            break;
        case BLOCK_DIAMOND_ORE:
            DrawCube(pos, 1.0f, 1.0f, 1.0f, (Color){130, 130, 140, 255});
            for (int i = 0; i < 5; i++) {
                float ox = (float)((rand() % 3) - 1) * 0.25f;
                float oy = (float)((rand() % 3) - 1) * 0.25f;
                float oz = (float)((rand() % 3) - 1) * 0.25f;
                DrawCube((Vector3){pos.x + ox, pos.y + oy, pos.z + oz}, 0.15f, 0.15f, 0.15f, (Color){80, 220, 230, 255});
            }
            break;
        default: break;
    }
}

static void PlantTree(int x, int z, bool birch) {
    int groundY = 0;
    for (int y = WORLD_HEIGHT - 1; y >= 0; y--) {
        if (GetBlock(x, y, z) == BLOCK_GRASS) {
            groundY = y;
            break;
        }
    }
    BlockType woodType = birch ? BLOCK_BIRCH_WOOD : BLOCK_OAK_WOOD;
    BlockType leafType = birch ? BLOCK_BIRCH_LEAVES : BLOCK_OAK_LEAVES;
    int trunkHeight = 3 + (rand() % 2);
    for (int y = groundY + 1; y <= groundY + trunkHeight && y < WORLD_HEIGHT; y++) SetBlock(x, y, z, woodType);
    int leafStart = groundY + trunkHeight;
    for (int dx = -2; dx <= 2; dx++) {
        for (int dz = -2; dz <= 2; dz++) {
            for (int dy = 0; dy <= 2; dy++) {
                int lx = x + dx;
                int ly = leafStart + dy;
                int lz = z + dz;
                int distance = abs(dx) + abs(dz) + dy;
                if (InBounds(lx, ly, lz) && distance <= 4 && GetBlock(lx, ly, lz) == BLOCK_AIR) SetBlock(lx, ly, lz, leafType);
            }
        }
    }
}

static ItemType BlockToDrop(BlockType block) {
    switch (block) {
        case BLOCK_GRASS: return ITEM_GRASS;
        case BLOCK_DIRT: return ITEM_DIRT;
        case BLOCK_STONE: return ITEM_COBBLESTONE;
        case BLOCK_COBBLESTONE: return ITEM_COBBLESTONE;
        case BLOCK_OAK_WOOD: return ITEM_OAK_LOG;
        case BLOCK_OAK_PLANKS: return ITEM_OAK_PLANKS;
        case BLOCK_BIRCH_WOOD: return ITEM_BIRCH_LOG;
        case BLOCK_BIRCH_PLANKS: return ITEM_BIRCH_PLANKS;
        case BLOCK_CRAFTING_TABLE: return ITEM_CRAFTING_TABLE;
        case BLOCK_TORCH: return ITEM_TORCH;
        case BLOCK_FURNACE: return ITEM_FURNACE;
        case BLOCK_COAL_ORE: return ITEM_COAL;
        case BLOCK_IRON_ORE: return ITEM_IRON_INGOT;
        case BLOCK_GOLD_ORE: return ITEM_GOLD_INGOT;
        case BLOCK_DIAMOND_ORE: return ITEM_DIAMOND;
        default: return ITEM_AIR;
    }
}

static BlockType ItemToBlock(ItemType item) {
    switch (item) {
        case ITEM_GRASS: return BLOCK_GRASS;
        case ITEM_DIRT: return BLOCK_DIRT;
        case ITEM_COBBLESTONE: return BLOCK_COBBLESTONE;
        case ITEM_OAK_PLANKS: return BLOCK_OAK_PLANKS;
        case ITEM_BIRCH_PLANKS: return BLOCK_BIRCH_PLANKS;
        case ITEM_TORCH: return BLOCK_TORCH;
        case ITEM_FURNACE: return BLOCK_FURNACE;
        case ITEM_CRAFTING_TABLE: return BLOCK_CRAFTING_TABLE;
        default: return BLOCK_AIR;
    }
}

static void EatFood(ItemType item) {
    if (item == ITEM_COOKED_BEEF || item == ITEM_COOKED_PORKCHOP || item == ITEM_COOKED_MUTTON) hunger = (int)Clampf((float)hunger + 18.0f, 0.0f, 100.0f);
    else if (item == ITEM_BEEF || item == ITEM_PORKCHOP || item == ITEM_RAW_MUTTON) hunger = (int)Clampf((float)hunger + 12.0f, 0.0f, 100.0f);
}

static int FindSurfaceY(int x, int z) {
    for (int y = WORLD_HEIGHT - 1; y >= 0; y--) {
        if (GetBlock(x, y, z) != BLOCK_AIR) return y;
    }
    return 0;
}

static void SpawnMobs(void) {
    memset(mobs, 0, sizeof(mobs));
    int count = 0;
    for (int i = 0; i < 12 && count < MAX_MOBS; i++) {
        int x = 4 + rand() % (WORLD_SIZE - 8);
        int z = 4 + rand() % (WORLD_SIZE - 8);
        int y = FindSurfaceY(x, z) + 1;
        if (y >= WORLD_HEIGHT) continue;
        EntityType type = (EntityType)(ENTITY_SHEEP + (rand() % 3));
        mobs[count].type = type;
        mobs[count].position = (Vector3){(float)x + 0.5f, (float)y, (float)z + 0.5f};
        mobs[count].alive = true;
        count++;
    }
}

static void GenerateOreVein(BlockType oreType, int minY, int maxY, int veinCount, int veinSize) {
    for (int v = 0; v < veinCount; v++) {
        int cx = rand() % WORLD_SIZE;
        int cy = minY + rand() % (maxY - minY + 1);
        int cz = rand() % WORLD_SIZE;
        
        for (int i = 0; i < veinSize; i++) {
            int ox = cx + (rand() % 3) - 1;
            int oy = cy + (rand() % 3) - 1;
            int oz = cz + (rand() % 3) - 1;
            
            if (InBounds(ox, oy, oz) && GetBlock(ox, oy, oz) == BLOCK_STONE) {
                SetBlock(ox, oy, oz, oreType);
            }
        }
    }
}

static void GenerateWorld(void) {
    memset(world, 0, sizeof(world));
    for (int x = 0; x < WORLD_SIZE; x++) {
        for (int z = 0; z < WORLD_SIZE; z++) {
            float fx = x / 6.0f;
            float fz = z / 6.0f;
            int height = 48 + (int)(4.0f * sinf(fx) + 3.0f * cosf(fz) + (rand() % 3));
            if (height < 40) height = 40;
            if (height > WORLD_HEIGHT - 8) height = WORLD_HEIGHT - 8;
            for (int y = 0; y < WORLD_HEIGHT; y++) {
                if (y < height - 3) world[x][y][z] = BLOCK_STONE;
                else if (y < height - 1) world[x][y][z] = BLOCK_DIRT;
                else if (y == height - 1) world[x][y][z] = BLOCK_GRASS;
                else world[x][y][z] = BLOCK_AIR;
            }
        }
    }

    GenerateOreVein(BLOCK_COAL_ORE, 5, 50, 80, 8);
    GenerateOreVein(BLOCK_IRON_ORE, 10, 45, 60, 6);
    GenerateOreVein(BLOCK_GOLD_ORE, 20, 35, 30, 5);
    GenerateOreVein(BLOCK_DIAMOND_ORE, 30, 50, 20, 4);

    for (int x = 2; x < WORLD_SIZE - 2; x++) {
        for (int z = 2; z < WORLD_SIZE - 2; z++) {
            int surfaceY = FindSurfaceY(x, z);
            if (GetBlock(x, surfaceY, z) == BLOCK_GRASS && rand() % 16 == 0) PlantTree(x, z, rand() % 2 == 0);
        }
    }
    SpawnMobs();
}

static bool CheckPlayerCollision(Vector3 position) {
    const float radius = 0.25f;
    const float height = 1.7f;
    float minX = position.x - radius;
    float maxX = position.x + radius;
    float minY = position.y + 0.05f;
    float maxY = position.y + height - 0.05f;
    float minZ = position.z - radius;
    float maxZ = position.z + radius;
    int x0 = (int)floorf(minX), x1 = (int)floorf(maxX);
    int y0 = (int)floorf(minY), y1 = (int)floorf(maxY);
    int z0 = (int)floorf(minZ), z1 = (int)floorf(maxZ);
    for (int x = x0; x <= x1; x++) for (int y = y0; y <= y1; y++) for (int z = z0; z <= z1; z++) {
        if (!InBounds(x, y, z)) continue;
        if (GetBlock(x, y, z) == BLOCK_AIR) continue;
        float bx0 = (float)x, bx1 = bx0 + 1.0f;
        float by0 = (float)y, by1 = by0 + 1.0f;
        float bz0 = (float)z, bz1 = bz0 + 1.0f;
        if (minX < bx1 && maxX > bx0 && minY < by1 && maxY > by0 && minZ < bz1 && maxZ > bz0) return true;
    }
    return false;
}

static bool IsPlayerGrounded(Vector3 position) {
    int footX = (int)floorf(position.x);
    int footY = (int)floorf(position.y - 0.01f);
    int footZ = (int)floorf(position.z);
    if (!InBounds(footX, footY, footZ)) return false;
    return GetBlock(footX, footY, footZ) != BLOCK_AIR;
}

static Vector3 FindSafeSpawnPosition(int startX, int startZ) {
    for (int dx = -8; dx <= 8; dx++) {
        for (int dz = -8; dz <= 8; dz++) {
            int testX = startX + dx;
            int testZ = startZ + dz;
            int surfaceY = FindSurfaceY(testX, testZ);
            Vector3 candidate = {(float)testX + 0.5f, (float)surfaceY + 1.2f, (float)testZ + 0.5f};
            if (!CheckPlayerCollision(candidate)) return candidate;
        }
    }
    int surfaceY = FindSurfaceY(startX, startZ);
    return (Vector3){(float)startX + 0.5f, (float)surfaceY + 1.2f, (float)startZ + 0.5f};
}

static void SpawnItemDrop(Vector3 position, ItemType item, int count) {
    for (int i = 0; i < MAX_ITEM_DROPS; i++) {
        if (!itemDrops[i].active) {
            itemDrops[i].active = true;
            itemDrops[i].item = item;
            itemDrops[i].count = count;
            itemDrops[i].position = (Vector3){position.x, position.y, position.z};
            itemDrops[i].velocity = (Vector3){(float)((rand() % 5) - 2) * 0.1f, 0.0f, (float)((rand() % 5) - 2) * 0.1f};
            return;
        }
    }
}

static void AddItemToInventory(ItemType item, int count);
static bool TryPlaceSelectedItem(Vector3 placeBlock, Vector3 playerPos);
static void GenerateOreVein(BlockType oreType, int minY, int maxY, int veinCount, int veinSize);

static void UpdateItemDrops(float dt, Vector3 playerPos) {
    for (int i = 0; i < MAX_ITEM_DROPS; i++) {
        if (!itemDrops[i].active) continue;

        itemDrops[i].velocity.y -= PLAYER_GRAVITY * 0.35f * dt;
        itemDrops[i].position.x += itemDrops[i].velocity.x * dt;
        itemDrops[i].position.y += itemDrops[i].velocity.y * dt;
        itemDrops[i].position.z += itemDrops[i].velocity.z * dt;

        int blockX = (int)floorf(itemDrops[i].position.x);
        int blockY = (int)floorf(itemDrops[i].position.y - ITEM_DROP_HALF_HEIGHT);
        int blockZ = (int)floorf(itemDrops[i].position.z);
        if (blockY < 0) blockY = 0;
        if (InBounds(blockX, blockY, blockZ) && GetBlock(blockX, blockY, blockZ) != BLOCK_AIR) {
            itemDrops[i].position.y = (float)(blockY + 1.0f + ITEM_DROP_HALF_HEIGHT);
            itemDrops[i].velocity.y = 0.0f;
            itemDrops[i].velocity.x *= 0.2f;
            itemDrops[i].velocity.z *= 0.2f;
        }

        Vector3 pickupPoint = {playerPos.x, playerPos.y + 0.55f, playerPos.z};
        float distSq = (itemDrops[i].position.x - pickupPoint.x) * (itemDrops[i].position.x - pickupPoint.x) +
                       (itemDrops[i].position.y - pickupPoint.y) * (itemDrops[i].position.y - pickupPoint.y) +
                       (itemDrops[i].position.z - pickupPoint.z) * (itemDrops[i].position.z - pickupPoint.z);
        if (distSq < ITEM_DROP_PICKUP_RADIUS * ITEM_DROP_PICKUP_RADIUS) {
            AddItemToInventory(itemDrops[i].item, itemDrops[i].count);
            itemDrops[i].active = false;
        }
    }
}

static bool RayCastBlock(Vector3 origin, Vector3 direction, Vector3 *hitBlock, Vector3 *placeBlock) {
    Vector3 current = origin;
    Vector3 previous = origin;
    int prevX = (int)round(previous.x);
    int prevY = (int)round(previous.y);
    int prevZ = (int)round(previous.z);
    for (int step = 1; step <= 80; step++) {
        current.x = origin.x + direction.x * step * 0.1f;
        current.y = origin.y + direction.y * step * 0.1f;
        current.z = origin.z + direction.z * step * 0.1f;
        int cx = (int)round(current.x);
        int cy = (int)round(current.y);
        int cz = (int)round(current.z);
        if (cx != prevX || cy != prevY || cz != prevZ) {
            if (InBounds(cx, cy, cz) && GetBlock(cx, cy, cz) != BLOCK_AIR) {
                *hitBlock = (Vector3){(float)cx, (float)cy, (float)cz};
                *placeBlock = (Vector3){(float)prevX, (float)prevY, (float)prevZ};
                return true;
            }
            prevX = cx; prevY = cy; prevZ = cz;
        }
    }
    return false;
}

static bool RayCastMob(Vector3 origin, Vector3 direction, Mob *hitMob) {
    for (int i = 0; i < MAX_MOBS; i++) {
        Mob *mob = &mobs[i];
        if (!mob->alive) continue;
        Vector3 toMob = Vec3Subtract(mob->position, origin);
        float along = Vec3Dot(toMob, direction);
        if (along < 0.1f) continue;
        Vector3 closest = Vec3Add(origin, Vec3Scale(direction, along));
        Vector3 diff = Vec3Subtract(mob->position, closest);
        float distSq = Vec3Dot(diff, diff);
        if (distSq < 0.45f * 0.45f) {
            *hitMob = *mob;
            return true;
        }
    }
    return false;
}

static void AddItemToInventory(ItemType item, int count) {
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (inventory[i] == item && inventoryCount[i] < 64) {
            inventoryCount[i] += count;
            return;
        }
    }
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (inventory[i] == ITEM_AIR) {
            inventory[i] = item;
            inventoryCount[i] = count;
            return;
        }
    }
}

static bool ConsumeGridInputs(ItemType a, ItemType b, ItemType c, ItemType d, ItemType e, ItemType f, ItemType g, ItemType h, ItemType i, int gridSlots) {
    ItemType expected[9] = {a, b, c, d, e, f, g, h, i};
    for (int j = 0; j < gridSlots; j++) {
        if (expected[j] != ITEM_AIR && craftingGrid[j] != expected[j]) return false;
    }
    for (int j = 0; j < gridSlots; j++) {
        if (expected[j] != ITEM_AIR) {
            craftingGrid[j] = ITEM_AIR;
            craftingGridCount[j] = 0;
        }
    }
    return true;
}

static void ClearInventory(void) {
    memset(inventory, 0, sizeof(inventory));
    memset(inventoryCount, 0, sizeof(inventoryCount));
}

static bool TryCraft(void) {
    int size = craftingTableMode ? 3 : 2;
    int count = size * size;
    for (int i = 0; i < count; i++) if (craftingGrid[i] == ITEM_AIR) craftingGrid[i] = ITEM_AIR;

    if (!craftingTableMode) {
        ItemType a = craftingGrid[0], b = craftingGrid[1], c = craftingGrid[2], d = craftingGrid[3];
        
        // Log to planks recipes
        if (a == ITEM_OAK_LOG && b == ITEM_AIR && c == ITEM_AIR && d == ITEM_AIR) {
            if (ConsumeGridInputs(ITEM_OAK_LOG, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, 4)) {
                AddItemToInventory(ITEM_OAK_PLANKS, 4);
                ClearCraftingGrid();
                return true;
            }
        }
        if (a == ITEM_BIRCH_LOG && b == ITEM_AIR && c == ITEM_AIR && d == ITEM_AIR) {
            if (ConsumeGridInputs(ITEM_BIRCH_LOG, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, 4)) {
                AddItemToInventory(ITEM_BIRCH_PLANKS, 4);
                ClearCraftingGrid();
                return true;
            }
        }
        
        // Planks to sticks recipe (vertical pattern)
        if (a == ITEM_OAK_PLANKS && b == ITEM_AIR && c == ITEM_AIR && d == ITEM_OAK_PLANKS) {
            if (ConsumeGridInputs(ITEM_OAK_PLANKS, ITEM_AIR, ITEM_AIR, ITEM_OAK_PLANKS, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, 4)) {
                AddItemToInventory(ITEM_STICK, 4);
                ClearCraftingGrid();
                return true;
            }
        }
        if (a == ITEM_BIRCH_PLANKS && b == ITEM_AIR && c == ITEM_AIR && d == ITEM_BIRCH_PLANKS) {
            if (ConsumeGridInputs(ITEM_BIRCH_PLANKS, ITEM_AIR, ITEM_AIR, ITEM_BIRCH_PLANKS, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, 4)) {
                AddItemToInventory(ITEM_STICK, 4);
                ClearCraftingGrid();
                return true;
            }
        }

        // Torch recipe: coal on top, stick on bottom
        if (a == ITEM_COAL && b == ITEM_AIR && c == ITEM_STICK && d == ITEM_AIR) {
            if (ConsumeGridInputs(ITEM_COAL, ITEM_AIR, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, 4)) {
                AddItemToInventory(ITEM_TORCH, 4);
                ClearCraftingGrid();
                return true;
            }
        }
        if (a == ITEM_AIR && b == ITEM_COAL && c == ITEM_AIR && d == ITEM_STICK) {
            if (ConsumeGridInputs(ITEM_AIR, ITEM_COAL, ITEM_AIR, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, 4)) {
                AddItemToInventory(ITEM_TORCH, 4);
                ClearCraftingGrid();
                return true;
            }
        }
        
        // Crafting table recipe
        if (a == ITEM_OAK_PLANKS && b == ITEM_OAK_PLANKS && c == ITEM_AIR && d == ITEM_AIR) {
            if (ConsumeGridInputs(ITEM_OAK_PLANKS, ITEM_OAK_PLANKS, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, 4)) {
                AddItemToInventory(ITEM_CRAFTING_TABLE, 1);
                ClearCraftingGrid();
                return true;
            }
        }
        if (a == ITEM_BIRCH_PLANKS && b == ITEM_BIRCH_PLANKS && c == ITEM_AIR && d == ITEM_AIR) {
            if (ConsumeGridInputs(ITEM_BIRCH_PLANKS, ITEM_BIRCH_PLANKS, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_AIR, 4)) {
                AddItemToInventory(ITEM_CRAFTING_TABLE, 1);
                ClearCraftingGrid();
                return true;
            }
        }
        return false;
    }

    ItemType a = craftingGrid[0], b = craftingGrid[1], c = craftingGrid[2], d = craftingGrid[3], e = craftingGrid[4], f = craftingGrid[5], g = craftingGrid[6], h = craftingGrid[7], i = craftingGrid[8];
    if (a == ITEM_OAK_PLANKS && b == ITEM_OAK_PLANKS && c == ITEM_AIR && d == ITEM_OAK_PLANKS && e == ITEM_STICK && f == ITEM_AIR && g == ITEM_AIR && h == ITEM_STICK && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_OAK_PLANKS, ITEM_OAK_PLANKS, ITEM_AIR, ITEM_OAK_PLANKS, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_WOOD_AXE, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_OAK_PLANKS && b == ITEM_OAK_PLANKS && c == ITEM_OAK_PLANKS && d == ITEM_AIR && e == ITEM_STICK && f == ITEM_AIR && g == ITEM_AIR && h == ITEM_STICK && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_OAK_PLANKS, ITEM_OAK_PLANKS, ITEM_OAK_PLANKS, ITEM_AIR, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_WOOD_PICKAXE, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_OAK_PLANKS && b == ITEM_AIR && c == ITEM_AIR && d == ITEM_AIR && e == ITEM_STICK && f == ITEM_AIR && g == ITEM_AIR && h == ITEM_STICK && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_OAK_PLANKS, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_WOOD_SHOVEL, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_COBBLESTONE && b == ITEM_COBBLESTONE && c == ITEM_COBBLESTONE && d == ITEM_COBBLESTONE && e == ITEM_AIR && f == ITEM_COBBLESTONE && g == ITEM_COBBLESTONE && h == ITEM_COBBLESTONE && i == ITEM_COBBLESTONE) {
        if (ConsumeGridInputs(ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_AIR, ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_COBBLESTONE, 9)) {
            AddItemToInventory(ITEM_STONE_CHESTPLATE, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    
    // Stone armor recipes
    if (a == ITEM_COBBLESTONE && b == ITEM_COBBLESTONE && c == ITEM_COBBLESTONE && d == ITEM_COBBLESTONE && e == ITEM_COBBLESTONE && f == ITEM_COBBLESTONE && g == ITEM_AIR && h == ITEM_AIR && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_AIR, ITEM_AIR, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_STONE_HELMET, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_COBBLESTONE && b == ITEM_COBBLESTONE && c == ITEM_AIR && d == ITEM_COBBLESTONE && e == ITEM_COBBLESTONE && f == ITEM_AIR && g == ITEM_COBBLESTONE && h == ITEM_COBBLESTONE && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_AIR, ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_AIR, ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_STONE_LEGGINGS, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_AIR && b == ITEM_AIR && c == ITEM_AIR && d == ITEM_COBBLESTONE && e == ITEM_AIR && f == ITEM_COBBLESTONE && g == ITEM_COBBLESTONE && h == ITEM_AIR && i == ITEM_COBBLESTONE) {
        if (ConsumeGridInputs(ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_COBBLESTONE, ITEM_AIR, ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_AIR, ITEM_COBBLESTONE, 9)) {
            AddItemToInventory(ITEM_STONE_BOOTS, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    
    // Iron armor recipes
    if (a == ITEM_IRON_INGOT && b == ITEM_IRON_INGOT && c == ITEM_IRON_INGOT && d == ITEM_IRON_INGOT && e == ITEM_AIR && f == ITEM_IRON_INGOT && g == ITEM_IRON_INGOT && h == ITEM_IRON_INGOT && i == ITEM_IRON_INGOT) {
        if (ConsumeGridInputs(ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_AIR, ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_IRON_INGOT, 9)) {
            AddItemToInventory(ITEM_IRON_CHESTPLATE, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_IRON_INGOT && b == ITEM_IRON_INGOT && c == ITEM_IRON_INGOT && d == ITEM_IRON_INGOT && e == ITEM_IRON_INGOT && f == ITEM_IRON_INGOT && g == ITEM_AIR && h == ITEM_AIR && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_AIR, ITEM_AIR, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_IRON_HELMET, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_IRON_INGOT && b == ITEM_IRON_INGOT && c == ITEM_AIR && d == ITEM_IRON_INGOT && e == ITEM_IRON_INGOT && f == ITEM_AIR && g == ITEM_IRON_INGOT && h == ITEM_IRON_INGOT && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_AIR, ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_AIR, ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_IRON_LEGGINGS, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_AIR && b == ITEM_AIR && c == ITEM_AIR && d == ITEM_IRON_INGOT && e == ITEM_AIR && f == ITEM_IRON_INGOT && g == ITEM_IRON_INGOT && h == ITEM_AIR && i == ITEM_IRON_INGOT) {
        if (ConsumeGridInputs(ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_IRON_INGOT, ITEM_AIR, ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_AIR, ITEM_IRON_INGOT, 9)) {
            AddItemToInventory(ITEM_IRON_BOOTS, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    
    // Diamond armor recipes
    if (a == ITEM_DIAMOND && b == ITEM_DIAMOND && c == ITEM_DIAMOND && d == ITEM_DIAMOND && e == ITEM_AIR && f == ITEM_DIAMOND && g == ITEM_DIAMOND && h == ITEM_DIAMOND && i == ITEM_DIAMOND) {
        if (ConsumeGridInputs(ITEM_DIAMOND, ITEM_DIAMOND, ITEM_DIAMOND, ITEM_DIAMOND, ITEM_AIR, ITEM_DIAMOND, ITEM_DIAMOND, ITEM_DIAMOND, ITEM_DIAMOND, 9)) {
            AddItemToInventory(ITEM_DIAMOND_CHESTPLATE, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_DIAMOND && b == ITEM_DIAMOND && c == ITEM_DIAMOND && d == ITEM_DIAMOND && e == ITEM_DIAMOND && f == ITEM_DIAMOND && g == ITEM_AIR && h == ITEM_AIR && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_DIAMOND, ITEM_DIAMOND, ITEM_DIAMOND, ITEM_DIAMOND, ITEM_DIAMOND, ITEM_DIAMOND, ITEM_AIR, ITEM_AIR, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_DIAMOND_HELMET, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_DIAMOND && b == ITEM_DIAMOND && c == ITEM_AIR && d == ITEM_DIAMOND && e == ITEM_DIAMOND && f == ITEM_AIR && g == ITEM_DIAMOND && h == ITEM_DIAMOND && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_DIAMOND, ITEM_DIAMOND, ITEM_AIR, ITEM_DIAMOND, ITEM_DIAMOND, ITEM_AIR, ITEM_DIAMOND, ITEM_DIAMOND, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_DIAMOND_LEGGINGS, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_AIR && b == ITEM_AIR && c == ITEM_AIR && d == ITEM_DIAMOND && e == ITEM_AIR && f == ITEM_DIAMOND && g == ITEM_DIAMOND && h == ITEM_AIR && i == ITEM_DIAMOND) {
        if (ConsumeGridInputs(ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_DIAMOND, ITEM_AIR, ITEM_DIAMOND, ITEM_DIAMOND, ITEM_AIR, ITEM_DIAMOND, 9)) {
            AddItemToInventory(ITEM_DIAMOND_BOOTS, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    // Gold tool recipes
    if (a == ITEM_GOLD_INGOT && b == ITEM_GOLD_INGOT && c == ITEM_AIR && d == ITEM_GOLD_INGOT && e == ITEM_STICK && f == ITEM_AIR && g == ITEM_AIR && h == ITEM_STICK && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_GOLD_INGOT, ITEM_GOLD_INGOT, ITEM_AIR, ITEM_GOLD_INGOT, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_GOLD_AXE, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_GOLD_INGOT && b == ITEM_GOLD_INGOT && c == ITEM_GOLD_INGOT && d == ITEM_AIR && e == ITEM_STICK && f == ITEM_AIR && g == ITEM_AIR && h == ITEM_STICK && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_GOLD_INGOT, ITEM_GOLD_INGOT, ITEM_GOLD_INGOT, ITEM_AIR, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_GOLD_PICKAXE, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_GOLD_INGOT && b == ITEM_AIR && c == ITEM_AIR && d == ITEM_AIR && e == ITEM_STICK && f == ITEM_AIR && g == ITEM_AIR && h == ITEM_STICK && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_GOLD_INGOT, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_GOLD_SHOVEL, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    // Gold armor recipes
    if (a == ITEM_GOLD_INGOT && b == ITEM_GOLD_INGOT && c == ITEM_GOLD_INGOT && d == ITEM_GOLD_INGOT && e == ITEM_AIR && f == ITEM_GOLD_INGOT && g == ITEM_GOLD_INGOT && h == ITEM_GOLD_INGOT && i == ITEM_GOLD_INGOT) {
        if (ConsumeGridInputs(ITEM_GOLD_INGOT, ITEM_GOLD_INGOT, ITEM_GOLD_INGOT, ITEM_GOLD_INGOT, ITEM_AIR, ITEM_GOLD_INGOT, ITEM_GOLD_INGOT, ITEM_GOLD_INGOT, ITEM_GOLD_INGOT, 9)) {
            AddItemToInventory(ITEM_GOLD_CHESTPLATE, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_GOLD_INGOT && b == ITEM_GOLD_INGOT && c == ITEM_GOLD_INGOT && d == ITEM_GOLD_INGOT && e == ITEM_GOLD_INGOT && f == ITEM_GOLD_INGOT && g == ITEM_AIR && h == ITEM_AIR && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_GOLD_INGOT, ITEM_GOLD_INGOT, ITEM_GOLD_INGOT, ITEM_GOLD_INGOT, ITEM_GOLD_INGOT, ITEM_GOLD_INGOT, ITEM_AIR, ITEM_AIR, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_GOLD_HELMET, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_GOLD_INGOT && b == ITEM_GOLD_INGOT && c == ITEM_AIR && d == ITEM_GOLD_INGOT && e == ITEM_GOLD_INGOT && f == ITEM_AIR && g == ITEM_GOLD_INGOT && h == ITEM_GOLD_INGOT && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_GOLD_INGOT, ITEM_GOLD_INGOT, ITEM_AIR, ITEM_GOLD_INGOT, ITEM_GOLD_INGOT, ITEM_AIR, ITEM_GOLD_INGOT, ITEM_GOLD_INGOT, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_GOLD_LEGGINGS, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_AIR && b == ITEM_AIR && c == ITEM_AIR && d == ITEM_GOLD_INGOT && e == ITEM_AIR && f == ITEM_GOLD_INGOT && g == ITEM_GOLD_INGOT && h == ITEM_AIR && i == ITEM_GOLD_INGOT) {
        if (ConsumeGridInputs(ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_GOLD_INGOT, ITEM_AIR, ITEM_GOLD_INGOT, ITEM_GOLD_INGOT, ITEM_AIR, ITEM_GOLD_INGOT, 9)) {
            AddItemToInventory(ITEM_GOLD_BOOTS, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_LEATHER && b == ITEM_LEATHER && c == ITEM_LEATHER && d == ITEM_LEATHER && e == ITEM_AIR && f == ITEM_LEATHER && g == ITEM_LEATHER && h == ITEM_LEATHER && i == ITEM_LEATHER) {
        if (ConsumeGridInputs(ITEM_LEATHER, ITEM_LEATHER, ITEM_LEATHER, ITEM_LEATHER, ITEM_AIR, ITEM_LEATHER, ITEM_LEATHER, ITEM_LEATHER, ITEM_LEATHER, 9)) {
            AddItemToInventory(ITEM_LEATHER_CHESTPLATE, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    
    // Leather armor recipes
    if (a == ITEM_LEATHER && b == ITEM_LEATHER && c == ITEM_LEATHER && d == ITEM_LEATHER && e == ITEM_LEATHER && f == ITEM_LEATHER && g == ITEM_AIR && h == ITEM_AIR && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_LEATHER, ITEM_LEATHER, ITEM_LEATHER, ITEM_LEATHER, ITEM_LEATHER, ITEM_LEATHER, ITEM_AIR, ITEM_AIR, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_LEATHER_HELMET, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_LEATHER && b == ITEM_LEATHER && c == ITEM_AIR && d == ITEM_LEATHER && e == ITEM_LEATHER && f == ITEM_AIR && g == ITEM_LEATHER && h == ITEM_LEATHER && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_LEATHER, ITEM_LEATHER, ITEM_AIR, ITEM_LEATHER, ITEM_LEATHER, ITEM_AIR, ITEM_LEATHER, ITEM_LEATHER, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_LEATHER_LEGGINGS, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_AIR && b == ITEM_AIR && c == ITEM_AIR && d == ITEM_LEATHER && e == ITEM_AIR && f == ITEM_LEATHER && g == ITEM_LEATHER && h == ITEM_AIR && i == ITEM_LEATHER) {
        if (ConsumeGridInputs(ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_LEATHER, ITEM_AIR, ITEM_LEATHER, ITEM_LEATHER, ITEM_AIR, ITEM_LEATHER, 9)) {
            AddItemToInventory(ITEM_LEATHER_BOOTS, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    
    // Stone tool recipes
    if (a == ITEM_COBBLESTONE && b == ITEM_COBBLESTONE && c == ITEM_AIR && d == ITEM_COBBLESTONE && e == ITEM_STICK && f == ITEM_AIR && g == ITEM_AIR && h == ITEM_STICK && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_AIR, ITEM_COBBLESTONE, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_STONE_AXE, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_COBBLESTONE && b == ITEM_COBBLESTONE && c == ITEM_COBBLESTONE && d == ITEM_AIR && e == ITEM_STICK && f == ITEM_AIR && g == ITEM_AIR && h == ITEM_STICK && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_AIR, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_STONE_PICKAXE, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_COBBLESTONE && b == ITEM_AIR && c == ITEM_AIR && d == ITEM_AIR && e == ITEM_STICK && f == ITEM_AIR && g == ITEM_AIR && h == ITEM_STICK && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_COBBLESTONE, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_STONE_SHOVEL, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    // Stone hoe recipe
    if (a == ITEM_COBBLESTONE && b == ITEM_COBBLESTONE && c == ITEM_AIR && d == ITEM_AIR && e == ITEM_STICK && f == ITEM_AIR && g == ITEM_AIR && h == ITEM_STICK && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_STONE_HOE, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    
    // Iron tool recipes
    if (a == ITEM_IRON_INGOT && b == ITEM_IRON_INGOT && c == ITEM_AIR && d == ITEM_IRON_INGOT && e == ITEM_STICK && f == ITEM_AIR && g == ITEM_AIR && h == ITEM_STICK && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_AIR, ITEM_IRON_INGOT, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_IRON_AXE, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_IRON_INGOT && b == ITEM_IRON_INGOT && c == ITEM_IRON_INGOT && d == ITEM_AIR && e == ITEM_STICK && f == ITEM_AIR && g == ITEM_AIR && h == ITEM_STICK && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_AIR, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_IRON_PICKAXE, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_IRON_INGOT && b == ITEM_AIR && c == ITEM_AIR && d == ITEM_AIR && e == ITEM_STICK && f == ITEM_AIR && g == ITEM_AIR && h == ITEM_STICK && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_IRON_INGOT, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_IRON_SHOVEL, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    // Iron hoe recipe
    if (a == ITEM_IRON_INGOT && b == ITEM_IRON_INGOT && c == ITEM_AIR && d == ITEM_AIR && e == ITEM_STICK && f == ITEM_AIR && g == ITEM_AIR && h == ITEM_STICK && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_IRON_HOE, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    
    // Diamond tool recipes
    if (a == ITEM_DIAMOND && b == ITEM_DIAMOND && c == ITEM_AIR && d == ITEM_DIAMOND && e == ITEM_STICK && f == ITEM_AIR && g == ITEM_AIR && h == ITEM_STICK && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_DIAMOND, ITEM_DIAMOND, ITEM_AIR, ITEM_DIAMOND, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_DIAMOND_AXE, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_DIAMOND && b == ITEM_DIAMOND && c == ITEM_DIAMOND && d == ITEM_AIR && e == ITEM_STICK && f == ITEM_AIR && g == ITEM_AIR && h == ITEM_STICK && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_DIAMOND, ITEM_DIAMOND, ITEM_DIAMOND, ITEM_AIR, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_DIAMOND_PICKAXE, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    if (a == ITEM_DIAMOND && b == ITEM_AIR && c == ITEM_AIR && d == ITEM_AIR && e == ITEM_STICK && f == ITEM_AIR && g == ITEM_AIR && h == ITEM_STICK && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_DIAMOND, ITEM_AIR, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_DIAMOND_SHOVEL, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    // Diamond hoe recipe
    if (a == ITEM_DIAMOND && b == ITEM_DIAMOND && c == ITEM_AIR && d == ITEM_AIR && e == ITEM_STICK && f == ITEM_AIR && g == ITEM_AIR && h == ITEM_STICK && i == ITEM_AIR) {
        if (ConsumeGridInputs(ITEM_DIAMOND, ITEM_DIAMOND, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, ITEM_AIR, ITEM_STICK, ITEM_AIR, 9)) {
            AddItemToInventory(ITEM_DIAMOND_HOE, 1);
            ClearCraftingGrid();
            return true;
        }
    }
    return false;
}

static bool TryPlaceSelectedItem(Vector3 placeBlock, Vector3 playerPos) {
    int slot = selectedSlot;
    ItemType item = inventory[slot];
    BlockType placeType = ItemToBlock(item);
    if (placeType == BLOCK_AIR || inventoryCount[slot] <= 0) return false;
    
    int bx = (int)placeBlock.x;
    int by = (int)placeBlock.y;
    int bz = (int)placeBlock.z;
    
    if (!InBounds(bx, by, bz) || GetBlock(bx, by, bz) != BLOCK_AIR) return false;
    
    // Temporarily place the block to check collision
    SetBlock(bx, by, bz, placeType);
    if (CheckPlayerCollision(playerPos)) {
        SetBlock(bx, by, bz, BLOCK_AIR); // Revert if collision
        return false;
    }
    
    inventoryCount[slot]--;
    if (inventoryCount[slot] <= 0) {
        inventory[slot] = ITEM_AIR;
        inventoryCount[slot] = 0;
    }
    return true;
}

int main(void) {
    const int screenWidth = 1024;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Minecraft-lite: Better textures + crafting");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    srand((unsigned)time(NULL));

    ClearInventory();
    ClearCraftingGrid();
    LoadBlockTextures();
    GenerateWorld();
    EnableCursor(); // Start with cursor enabled for menu

    int startX = WORLD_SIZE / 2;
    int startZ = WORLD_SIZE / 2;
    Vector3 playerPos = FindSafeSpawnPosition(startX, startZ);
    int spawnBlockX = (int)floorf(playerPos.x);
    int spawnBlockZ = (int)floorf(playerPos.z);
    int spawnSurfaceY = FindSurfaceY(spawnBlockX, spawnBlockZ);
    playerPos.y = (float)spawnSurfaceY + 1.0f;
    Vector3 velocity = {0, 0, 0};
    bool grounded = true;
    float yaw = 0.0f;
    float pitch = 0.0f;
    const float eyeHeight = 1.62f;
    Camera3D camera = {0};
    camera.position = (Vector3){playerPos.x, playerPos.y + eyeHeight, playerPos.z};
    camera.up = (Vector3){0, 1, 0};
    camera.fovy = 70.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Vector2 mouseDelta = GetMouseDelta();
        yaw += mouseDelta.x * 0.14f;
        pitch -= mouseDelta.y * 0.14f;
        pitch = Clampf(pitch, -89.0f, 89.0f);

        Vector3 forward = {cosf(DEG2RAD * yaw) * cosf(DEG2RAD * pitch), sinf(DEG2RAD * pitch), sinf(DEG2RAD * yaw) * cosf(DEG2RAD * pitch)};
        forward = Vec3Normalize(forward);
        Vector3 right = {-forward.z, 0.0f, forward.x};

        if (gameState == GAME_MENU) {
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) menuSelection = (menuSelection + 2) % 3;
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) menuSelection = (menuSelection + 1) % 3;
            if (IsKeyPressed(KEY_ENTER)) {
                if (menuSelection == 0) { 
                    gameState = GAME_PLAYING; 
                    DisableCursor();
                    AddChatMessage("Singleplayer ready."); 
                }
                else if (menuSelection == 1) { 
                    multiplayerEnabled = true; 
                    gameState = GAME_PLAYING; 
                    DisableCursor();
                    AddChatMessage("Multiplayer enabled."); 
                }
                else break;
            }
        } else if (gameState == GAME_SETTINGS) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                gameState = GAME_PLAYING;
                DisableCursor();
            }
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) settingsSelection = (settingsSelection + 2) % 3;
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) settingsSelection = (settingsSelection + 1) % 3;
            if (IsKeyPressed(KEY_ENTER)) {
                if (settingsSelection == 0) {
                    gameState = GAME_PLAYING;
                    DisableCursor();
                } else if (settingsSelection == 1) {
                    gameState = GAME_MENU;
                    ClearInventory();
                    ClearCraftingGrid();
                    GenerateWorld();
                    playerPos = FindSafeSpawnPosition(WORLD_SIZE / 2, WORLD_SIZE / 2);
                    int spawnBlockX = (int)floorf(playerPos.x);
                    int spawnBlockZ = (int)floorf(playerPos.z);
                    int spawnSurfaceY = FindSurfaceY(spawnBlockX, spawnBlockZ);
                    playerPos.y = (float)spawnSurfaceY + 1.0f;
                    velocity = (Vector3){0, 0, 0};
                    hunger = 100;
                    menuSelection = 0;
                    settingsSelection = 0;
                } else {
                    break;
                }
            }
        } else {
            Vector3 move = {0, 0, 0};
            if (IsKeyDown(KEY_W)) move = Vec3Add(move, (Vector3){forward.x, 0.0f, forward.z});
            if (IsKeyDown(KEY_S)) move = Vec3Subtract(move, (Vector3){forward.x, 0.0f, forward.z});
            if (IsKeyDown(KEY_A)) move = Vec3Subtract(move, (Vector3){right.x, 0.0f, right.z});
            if (IsKeyDown(KEY_D)) move = Vec3Add(move, (Vector3){right.x, 0.0f, right.z});
            if (Vec3Length(move) > 0.001f) move = Vec3Scale(Vec3Normalize(move), 5.0f * dt);

            grounded = IsPlayerGrounded(playerPos);
            if (grounded && velocity.y <= 0.0f) {
                velocity.y = 0.0f;
            }
            if (IsKeyPressed(KEY_SPACE) && (grounded || fabsf(velocity.y) < 0.02f)) {
                velocity.y = PLAYER_JUMP_SPEED;
                grounded = false;
            }

            Vector3 resolved = playerPos;
            Vector3 xAttempt = resolved;
            xAttempt.x += move.x;
            if (!CheckPlayerCollision(xAttempt)) {
                resolved.x = xAttempt.x;
            } else if (grounded) {
                Vector3 xStep = xAttempt;
                xStep.y += 0.25f;
                if (!CheckPlayerCollision(xStep)) resolved = xStep;
            }

            Vector3 zAttempt = resolved;
            zAttempt.z += move.z;
            if (!CheckPlayerCollision(zAttempt)) {
                resolved.z = zAttempt.z;
            } else if (grounded) {
                Vector3 zStep = zAttempt;
                zStep.y += 0.25f;
                if (!CheckPlayerCollision(zStep)) resolved = zStep;
            }
            playerPos = resolved;

            if (!grounded || velocity.y > 0.0f) {
                velocity.y -= PLAYER_GRAVITY * dt;
            }
            Vector3 attempted = playerPos;
            attempted.y += velocity.y * dt;
            if (attempted.y < 0.0f) { attempted.y = 0.0f; velocity.y = 0.0f; grounded = true; }
            if (CheckPlayerCollision(attempted)) {
                if (velocity.y > 0.0f) {
                    attempted.y = playerPos.y;
                    velocity.y = 0.0f;
                } else {
                    attempted.y = floorf(attempted.y) + 1.0f;
                    velocity.y = 0.0f;
                    grounded = true;
                }
            } else {
                grounded = false;
            }
            playerPos = attempted;

            camera.position = (Vector3){playerPos.x, playerPos.y + eyeHeight, playerPos.z};
            camera.target = Vec3Add(camera.position, forward);
            UpdateItemDrops(dt, playerPos);

            for (int i = 0; i < HOTBAR_SLOTS; i++) if (IsKeyPressed(KEY_ONE + i)) selectedSlot = i;

            if (IsKeyPressed(KEY_E)) {
                inventoryOpen = !inventoryOpen;
                if (inventoryOpen) {
                    EnableCursor();
                } else {
                    DisableCursor();
                }
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                if (inventoryOpen) {
                    inventoryOpen = false;
                    DisableCursor();
                } else {
                    gameState = GAME_SETTINGS;
                    EnableCursor();
                    settingsSelection = 0;
                }
            }
            if (IsKeyPressed(KEY_C) && inventoryOpen) craftingTableMode = !craftingTableMode;
            if (IsKeyPressed(KEY_X) && inventoryOpen) ClearCraftingGrid();
            if (IsKeyPressed(KEY_ENTER) && inventoryOpen) {
                if (!TryCraft()) AddChatMessage("Crafting failed.");
            }
            if (IsKeyPressed(KEY_F)) {
                ItemType foodItem = inventory[selectedSlot];
                if (foodItem == ITEM_COOKED_BEEF || foodItem == ITEM_BEEF || foodItem == ITEM_PORKCHOP || foodItem == ITEM_COOKED_PORKCHOP || foodItem == ITEM_RAW_MUTTON || foodItem == ITEM_COOKED_MUTTON) {
                    EatFood(foodItem);
                    inventoryCount[selectedSlot]--;
                    if (inventoryCount[selectedSlot] <= 0) { inventory[selectedSlot] = ITEM_AIR; inventoryCount[selectedSlot] = 0; }
                }
            }
            if (IsKeyPressed(KEY_F2)) {
                time_t now = time(NULL);
                struct tm *t = localtime(&now);
                snprintf(screenshotPath, sizeof(screenshotPath), "screenshot_%04d%02d%02d_%02d%02d%02d.png",
                         t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
                screenshotRequested = true;
                AddChatMessage("Screenshot queued.");
            }
            Vector3 hitBlock = {0};
            Vector3 placeBlock = {0};
            bool targetFound = RayCastBlock(camera.position, forward, &hitBlock, &placeBlock);
            Mob hitMob = {0};
            bool mobHit = RayCastMob(camera.position, forward, &hitMob);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (inventoryOpen) {
                    Vector2 m = GetMousePosition();
                    int gridSize = craftingTableMode ? 3 : 2;
                    int startX = 40;
                    int startY = 200;
                    
                    // Check button clicks first
                    Rectangle craftButton = {startX + gridSize * 56 + 20, startY + 20, 120, 40};
                    Rectangle clearButton = {startX + gridSize * 56 + 20, startY + 70, 120, 40};
                    Rectangle modeButton = {startX + gridSize * 56 + 20, startY + 120, 120, 40};
                    
                    if (CheckCollisionPointRec(m, craftButton)) {
                        if (!TryCraft()) AddChatMessage("Crafting failed.");
                        goto afterCraftGrid;
                    }
                    if (CheckCollisionPointRec(m, clearButton)) {
                        ClearCraftingGrid();
                        goto afterCraftGrid;
                    }
                    if (CheckCollisionPointRec(m, modeButton)) {
                        craftingTableMode = !craftingTableMode;
                        ClearCraftingGrid();
                        goto afterCraftGrid;
                    }
                    
                    for (int r = 0; r < gridSize; r++) {
                        for (int c = 0; c < gridSize; c++) {
                            Rectangle rect = {(float)(startX + c * 56), (float)(startY + r * 56), 48, 48};
                            if (CheckCollisionPointRec(m, rect)) {
                                if (craftingGrid[r * gridSize + c] != ITEM_AIR) {
                                    AddItemToInventory(craftingGrid[r * gridSize + c], 1);
                                    craftingGrid[r * gridSize + c] = ITEM_AIR;
                                    craftingGridCount[r * gridSize + c] = 0;
                                }
                                goto afterCraftGrid;
                            }
                        }
                    }
                    for (int i = 0; i < INVENTORY_SLOTS; i++) {
                        int x = 40 + (i % 9) * 56;
                        int y = 420 + (i / 9) * 56;
                        Rectangle rect = {(float)x, (float)y, 48, 48};
                        if (CheckCollisionPointRec(m, rect) && inventory[i] != ITEM_AIR) {
                            if (craftingGrid[craftingCursor] == ITEM_AIR) {
                                craftingGrid[craftingCursor] = inventory[i];
                                craftingGridCount[craftingCursor] = 1;
                                inventoryCount[i]--;
                                if (inventoryCount[i] <= 0) { inventory[i] = ITEM_AIR; inventoryCount[i] = 0; }
                                craftingCursor = (craftingCursor + 1) % (gridSize * gridSize);
                            }
                            goto afterCraftGrid;
                        }
                    }
                }
afterCraftGrid:
                if (mobHit && hitMob.alive) {
                    hitMob.alive = false;
                    ItemType drop = ITEM_AIR;
                    if (hitMob.type == ENTITY_SHEEP) drop = ITEM_RAW_MUTTON;
                    else if (hitMob.type == ENTITY_COW) drop = ITEM_LEATHER;
                    else if (hitMob.type == ENTITY_PIG) drop = ITEM_PORKCHOP;
                    if (drop != ITEM_AIR) SpawnItemDrop((Vector3){hitMob.position.x, hitMob.position.y + 0.4f, hitMob.position.z}, drop, 1);
                } else if (targetFound) {
                    int hx = (int)hitBlock.x, hy = (int)hitBlock.y, hz = (int)hitBlock.z;
                    BlockType removed = GetBlock(hx, hy, hz);
                    if (removed != BLOCK_AIR) {
                        ItemType drop = BlockToDrop(removed);
                        if (drop != ITEM_AIR) SpawnItemDrop((Vector3){(float)hx + 0.5f, (float)hy + 0.75f, (float)hz + 0.5f}, drop, 1);
                        SetBlock(hx, hy, hz, BLOCK_AIR);
                    }
                }
            }

            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !inventoryOpen) {
                int px = (int)placeBlock.x, py = (int)placeBlock.y, pz = (int)placeBlock.z;
                if (TryPlaceSelectedItem((Vector3){(float)px, (float)py, (float)pz}, playerPos)) {
                    ;
                }
            }
        }

        if (screenshotRequested) {
            TakeScreenshot(screenshotPath);
            screenshotRequested = false;
            AddChatMessage(TextFormat("Saved: %s", screenshotPath));
        }

        BeginDrawing();
        if (gameState == GAME_MENU) {
            ClearBackground((Color){20, 25, 40, 255});
            DrawText("Minecraft-lite", 300, 180, 46, RAYWHITE);
            DrawText("Textures, trees, crafting, armor", 250, 250, 24, LIGHTGRAY);
            Rectangle singleRect = {300, 320, 220, 48};
            Rectangle multiRect = {300, 390, 220, 48};
            Rectangle quitRect = {300, 460, 220, 48};
            DrawRectangleRec(singleRect, menuSelection == 0 ? GOLD : DARKBLUE);
            DrawRectangleRec(multiRect, menuSelection == 1 ? GOLD : DARKBLUE);
            DrawRectangleRec(quitRect, menuSelection == 2 ? GOLD : DARKBLUE);
            DrawText("Singleplayer", 345, 332, 24, RAYWHITE);
            DrawText("Multiplayer", 345, 402, 24, RAYWHITE);
            DrawText("Quit", 375, 472, 24, RAYWHITE);
        } else if (gameState == GAME_SETTINGS) {
            ClearBackground((Color){20, 25, 40, 255});
            DrawText("Settings", 380, 150, 46, RAYWHITE);
            DrawText("ESC to resume", 400, 210, 20, LIGHTGRAY);
            
            Rectangle resumeRect = {300, 280, 220, 48};
            Rectangle quitToMenuRect = {300, 350, 220, 48};
            Rectangle quitGameRect = {300, 420, 220, 48};
            
            DrawRectangleRec(resumeRect, settingsSelection == 0 ? GOLD : DARKBLUE);
            DrawRectangleRec(quitToMenuRect, settingsSelection == 1 ? GOLD : DARKBLUE);
            DrawRectangleRec(quitGameRect, settingsSelection == 2 ? GOLD : DARKBLUE);
            
            DrawText("Resume", 380, 292, 24, RAYWHITE);
            DrawText("Quit to Menu", 365, 362, 24, RAYWHITE);
            DrawText("Quit Game", 375, 432, 24, RAYWHITE);
        } else {
            ClearBackground(hunger > 35 ? SKYBLUE : (Color){150, 160, 255, 255});
            BeginMode3D(camera);
            for (int x = 0; x < WORLD_SIZE; x++) for (int y = 0; y < WORLD_HEIGHT; y++) for (int z = 0; z < WORLD_SIZE; z++) {
                BlockType type = GetBlock(x, y, z);
                if (type == BLOCK_AIR) continue;
                if (!IsBlockVisible(x, y, z)) continue;
                Vector3 blockPos = {(float)x, (float)y, (float)z};
                float dx = camera.position.x - blockPos.x;
                float dy = camera.position.y - blockPos.y;
                float dz = camera.position.z - blockPos.z;
                if (dx * dx + dy * dy + dz * dz > RENDER_RADIUS * RENDER_RADIUS) continue;
                DrawBlockVisual(type, blockPos);
            }
            for (int i = 0; i < MAX_MOBS; i++) {
                Mob *mob = &mobs[i];
                if (!mob->alive) continue;
                Color color = WHITE;
                if (mob->type == ENTITY_SHEEP) color = WHITE;
                else if (mob->type == ENTITY_COW) color = (Color){40, 25, 20, 255};
                else color = PINK;
                DrawCube(mob->position, 0.7f, 0.8f, 0.8f, color);
            }
            for (int i = 0; i < MAX_ITEM_DROPS; i++) {
                ItemDrop *drop = &itemDrops[i];
                if (!drop->active) continue;
                DrawCube(drop->position, 0.22f, 0.22f, 0.22f, ItemColor(drop->item));
            }
            DrawGrid(WORLD_SIZE, 1.0f);
            EndMode3D();

            DrawText("WASD: Move | Space: Jump | Left click: Mine | Right click: Place | F: Eat | E: Inventory | C: Crafting", 12, 12, 16, DARKGRAY);
            DrawText(TextFormat("Hunger: %d/100", hunger), 12, 34, 20, DARKGRAY);
            DrawRectangle(12, 60, 200, 20, LIGHTGRAY); DrawRectangle(12, 60, 2 * hunger, 20, RED); DrawRectangleLines(12, 60, 200, 20, DARKGRAY);
            DrawLine(screenWidth / 2 - 8, screenHeight / 2, screenWidth / 2 + 8, screenHeight / 2, BLACK);
            DrawLine(screenWidth / 2, screenHeight / 2 - 8, screenWidth / 2, screenHeight / 2 + 8, BLACK);

            for (int i = 0; i < HOTBAR_SLOTS; i++) {
                int x = 12 + i * 112;
                int y = screenHeight - 84;
                DrawRectangle(x, y, 104, 70, (i == selectedSlot) ? MAROON : DARKGRAY);
                ItemType item = inventory[i];
                if (item != ITEM_AIR) {
                    DrawRectangle(x + 12, y + 12, 80, 40, ItemColor(item));
                    DrawText(ItemName(item), x + 8, y + 55, 10, RAYWHITE);
                    DrawText(TextFormat("x%d", inventoryCount[i]), x + 8, y + 66, 10, WHITE);
                }
                DrawText(TextFormat("%d", i + 1), x + 6, y + 6, 14, RAYWHITE);
            }

            if (inventoryOpen) {
                DrawRectangle(20, 100, screenWidth - 40, 320, Fade(BLACK, 0.75f));
                DrawText(craftingTableMode ? "Crafting Table (3x3)" : "Crafting (2x2)", 40, 120, 24, RAYWHITE);
                DrawText("Click inventory to add items, click the grid to remove", 40, 148, 16, LIGHTGRAY);
                int size = craftingTableMode ? 3 : 2;
                int startX = 40;
                int startY = 200;
                for (int r = 0; r < size; r++) {
                    for (int c = 0; c < size; c++) {
                        Rectangle rect = {(float)(startX + c * 56), (float)(startY + r * 56), 48, 48};
                        DrawRectangleRec(rect, DARKBLUE);
                        DrawRectangleLinesEx(rect, 2, RAYWHITE);
                        int idx = r * size + c;
                        if (craftingGrid[idx] != ITEM_AIR) {
                            DrawRectangle((int)rect.x + 8, (int)rect.y + 8, 32, 32, ItemColor(craftingGrid[idx]));
                            DrawText(ItemName(craftingGrid[idx]), (int)rect.x + 4, (int)rect.y + 42, 10, WHITE);
                        }
                    }
                }
                
                // Craft button
                Rectangle craftButton = {startX + size * 56 + 20, startY + 20, 120, 40};
                DrawRectangleRec(craftButton, GOLD);
                DrawRectangleLinesEx(craftButton, 2, RAYWHITE);
                DrawText("CRAFT (Enter)", (int)craftButton.x + 10, (int)craftButton.y + 10, 16, BLACK);
                
                // Clear grid button
                Rectangle clearButton = {startX + size * 56 + 20, startY + 70, 120, 40};
                DrawRectangleRec(clearButton, MAROON);
                DrawRectangleLinesEx(clearButton, 2, RAYWHITE);
                DrawText("CLEAR (X)", (int)clearButton.x + 15, (int)clearButton.y + 10, 16, WHITE);
                
                // Toggle crafting mode button
                Rectangle modeButton = {startX + size * 56 + 20, startY + 120, 120, 40};
                DrawRectangleRec(modeButton, DARKBLUE);
                DrawRectangleLinesEx(modeButton, 2, RAYWHITE);
                DrawText(craftingTableMode ? "2x2 MODE (C)" : "3x3 MODE (C)", (int)modeButton.x + 5, (int)modeButton.y + 10, 16, WHITE);
                
                for (int i = 0; i < INVENTORY_SLOTS; i++) {
                    int bx = 40 + (i % 9) * 56;
                    int by = 420 + (i / 9) * 56;
                    Rectangle rect = {(float)bx, (float)by, 48, 48};
                    DrawRectangleRec(rect, DARKGRAY);
                    DrawRectangleLinesEx(rect, 2, LIGHTGRAY);
                    if (inventory[i] != ITEM_AIR) {
                        DrawRectangle((int)rect.x + 8, (int)rect.y + 8, 32, 32, ItemColor(inventory[i]));
                        DrawText(TextFormat("x%d", inventoryCount[i]), (int)rect.x + 6, (int)rect.y + 42, 10, WHITE);
                    }
                }
            }

            if (chatLineCount > 0) {
                for (int i = 0; i < chatLineCount; i++) DrawText(chatLog[i], 12, 120 + i * 18, 16, DARKBLUE);
            }
        }
        EndDrawing();
    }

    UnloadBlockTextures();
    CloseWindow();
    return 0;
}
