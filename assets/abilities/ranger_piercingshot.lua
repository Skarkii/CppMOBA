-- assets/abilities/piercingshot.lua
return {
    name      = "Piercing Shot",
    type      = "skillshot",
    targeting = "direction",

    cooldown  = 4.0,
    manaCost  = 30,
    range     = 12.0,
    speed     = 25.0,
    width     = 0.4,
    pierce    = false,

    visual = { 
        icon = "icons/ranger_piercing_shot.png",
        color = { 255, 230, 120 },
        radius = 0.15
    },

    onHit = function(caster, target)
        target:damage(30 + caster:attackDamage() * 1.3)
    end,
}