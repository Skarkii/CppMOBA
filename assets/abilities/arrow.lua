-- assets/abilities/arrow.lua
return {
    name            = "Arrow",
    cooldown        = 1.0,
    manaCost        = 0,
    targeting       = "unit",
    projectileSpeed = 15.0,

    onHit = function(caster, target)
        target:damage(10)
    end,
}