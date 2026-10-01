-- Melee basic attack: hits instantly, no projectile.
return {
    name      = "Slash",
    type      = "targeted",
    targeting = "unit",
 
    cooldown  = 1.0,
    manaCost  = 0,
 
    onHit = function(caster, target)
        target:damage(caster:attackDamage())
    end,
}
 