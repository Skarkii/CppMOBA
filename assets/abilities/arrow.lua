-- assets/abilities/arrow.lua
return {
    name      = "Arrow",
    type      = "targeted_projectile",
    targeting = "unit",
 
    cooldown  = 1.0,
    manaCost  = 0,
    speed     = 15.0,

	visual = {
        model  = "projectiles/arrow.glb", 
        scale  = 0.5,
        color  = { 200, 160, 90 },
        radius = 0.15,
    },
 
    onHit = function(caster, target)
        target:damage(caster:attackDamage())
    end,
}