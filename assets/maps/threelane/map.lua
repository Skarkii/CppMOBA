-- assets/maps/threelane/map.lua
-- Positions are world units: x to the right, z towards the camera. y is always 0.
-- The grid file has one character per 1x1 cell: row = z, column = x.
return {
    name     = "Three Lanes",
    size     = 160,
    cellSize = 1.0,
    model    = "maps/threelane/map.glb",
    grid     = "maps/threelane/map.grid",

    -- characters used in the grid file
    tiles = { wall = "#", jungle = ".", lane = "=", river = "~", bush = "*", blueBase = "b", redBase = "r" },

    -- towers are listed from the outermost one to the one nearest the base
    blue = {
        spawn = { x = 11, z = 149 },
        nexus = { x = 24, z = 136 },
        nexusTowers = { { x = 26, z = 129 }, { x = 31, z = 134 } },
        towers = {
            top = { { x = 14, z = 56 }, { x = 14, z = 90 }, { x = 14, z = 114 } },
            mid = { { x = 62, z = 98 }, { x = 48, z = 112 }, { x = 38, z = 122 } },
            bot = { { x = 104, z = 146 }, { x = 70, z = 146 }, { x = 46, z = 146 } },
        },
        inhibitors = {
            top = { x = 14, z = 120 },
            mid = { x = 33, z = 127 },
            bot = { x = 40, z = 146 },
        },
    },
    red = {
        spawn = { x = 149, z = 11 },
        nexus = { x = 136, z = 24 },
        nexusTowers = { { x = 134, z = 31 }, { x = 129, z = 26 } },
        towers = {
            top = { { x = 56, z = 14 }, { x = 90, z = 14 }, { x = 114, z = 14 } },
            mid = { { x = 98, z = 62 }, { x = 112, z = 48 }, { x = 122, z = 38 } },
            bot = { { x = 146, z = 104 }, { x = 146, z = 70 }, { x = 146, z = 46 } },
        },
        inhibitors = {
            top = { x = 120, z = 14 },
            mid = { x = 127, z = 33 },
            bot = { x = 146, z = 40 },
        },
    },

    -- minion paths, from the blue nexus to the red nexus (red minions walk them backwards)
    lanes = {
        top = { { x = 24, z = 136 }, { x = 14, z = 130 }, { x = 14, z = 22 }, { x = 22, z = 14 }, { x = 130, z = 14 }, { x = 136, z = 24 } },
        mid = { { x = 24, z = 136 }, { x = 136, z = 24 } },
        bot = { { x = 24, z = 136 }, { x = 30, z = 146 }, { x = 138, z = 146 }, { x = 146, z = 138 }, { x = 146, z = 30 }, { x = 136, z = 24 } },
    },

    camps = {
        { side = "blue", kind = "large", pos = { x = 34, z = 74 } },
        { side = "blue", kind = "large", pos = { x = 86, z = 126 } },
        { side = "blue", kind = "small", pos = { x = 30, z = 102 } },
        { side = "blue", kind = "small", pos = { x = 44, z = 96 } },
        { side = "blue", kind = "small", pos = { x = 54, z = 76 } },
        { side = "blue", kind = "small", pos = { x = 58, z = 130 } },
        { side = "blue", kind = "small", pos = { x = 64, z = 116 } },
        { side = "blue", kind = "small", pos = { x = 84, z = 106 } },
        { side = "red", kind = "large", pos = { x = 74, z = 34 } },
        { side = "red", kind = "large", pos = { x = 126, z = 86 } },
        { side = "red", kind = "small", pos = { x = 76, z = 54 } },
        { side = "red", kind = "small", pos = { x = 96, z = 44 } },
        { side = "red", kind = "small", pos = { x = 102, z = 30 } },
        { side = "red", kind = "small", pos = { x = 106, z = 84 } },
        { side = "red", kind = "small", pos = { x = 116, z = 64 } },
        { side = "red", kind = "small", pos = { x = 130, z = 58 } },
    },

    -- boss pits beside the river
    pits = { { x = 97, z = 111 }, { x = 63, z = 49 } },
}
