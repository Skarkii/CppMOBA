-- assets/abilities/rangervolley.lua
return {
    name      = "Volley",
    type      = "skillshot",
    targeting = "direction",

    cooldown  = 6.0,
    manaCost  = 40,
    range     = 9.0,
    speed     = 18.0,
    width     = 0.3,
    count     = 7,
    spread    = 50,
    pierce    = false,

    visual = { 
        icon = "icons/ranger_volley.png",
        color = { 200, 160, 90 },
        radius = 0.12 
    },

    onHit = function(caster, target)
        target:damage(20 + caster:attackDamage())
    end,
}
