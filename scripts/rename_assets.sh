#!/usr/bin/env bash
# rename_assets.sh — Copy/rename Kenney assets to match manifest.json
# Uses cp (not mv) so originals are preserved.
set -euo pipefail

TEX="assets/textures"
AUD="assets/audio"

echo "=== TEXTURES ==="

# ── Mario (green robot) ──────────────────────────────────────────────
# "Small" Mario — use the existing player_* sprites (which are the green robot)
cp -v "$TEX/player_idle.png"      "$TEX/mario_small_idle.png"
cp -v "$TEX/player_run.png"       "$TEX/mario_small_run.png"
cp -v "$TEX/player_jump.png"      "$TEX/mario_small_jump.png"
cp -v "$TEX/player_dash.png"      "$TEX/mario_small_skid.png"      # dash pose → skid
cp -v "$TEX/player_hurt.png"      "$TEX/mario_small_hurt.png"
cp -v "$TEX/player_dead.png"      "$TEX/mario_small_dead.png"
cp -v "$TEX/player_idle.png"      "$TEX/mario_small_grow.png"      # reuse idle for grow
cp -v "$TEX/player_wallslide.png" "$TEX/mario_small_climb.png"     # wallslide → climb

# "Big" Mario — same sprites, representing powered-up state
cp -v "$TEX/player_idle.png"      "$TEX/mario_big_idle.png"
cp -v "$TEX/player_run.png"       "$TEX/mario_big_run.png"
cp -v "$TEX/player_jump.png"      "$TEX/mario_big_jump.png"
cp -v "$TEX/player_dash.png"      "$TEX/mario_big_skid.png"
cp -v "$TEX/player_wallslide.png" "$TEX/mario_big_climb.png"

# "Fire" Mario — same sprites
cp -v "$TEX/player_idle.png"      "$TEX/mario_fire_idle.png"
cp -v "$TEX/player_run.png"       "$TEX/mario_fire_run.png"
cp -v "$TEX/player_jump.png"      "$TEX/mario_fire_jump.png"
cp -v "$TEX/player_dash.png"      "$TEX/mario_fire_skid.png"
cp -v "$TEX/player_wallslide.png" "$TEX/mario_fire_climb.png"

# ── Luigi (pink robot) ───────────────────────────────────────────────
# Use Characters/ subfolder individual tiles for pink robot
CHAR="$TEX/Characters"
cp -v "$CHAR/tile_0004.png"  "$TEX/luigi_small_idle.png"
cp -v "$CHAR/tile_0005.png"  "$TEX/luigi_small_run.png"       # walk frame
cp -v "$CHAR/tile_0007.png"  "$TEX/luigi_small_jump.png"      # jump frame
cp -v "$CHAR/tile_0006.png"  "$TEX/luigi_small_skid.png"      # walk2 → skid
cp -v "$CHAR/tile_0004.png"  "$TEX/luigi_small_hurt.png"      # reuse idle
cp -v "$CHAR/tile_0004.png"  "$TEX/luigi_small_dead.png"      # reuse idle
cp -v "$CHAR/tile_0004.png"  "$TEX/luigi_small_grow.png"      # reuse idle
cp -v "$CHAR/tile_0005.png"  "$TEX/luigi_small_climb.png"     # walk → climb

cp -v "$CHAR/tile_0004.png"  "$TEX/luigi_big_idle.png"
cp -v "$CHAR/tile_0005.png"  "$TEX/luigi_big_run.png"
cp -v "$CHAR/tile_0007.png"  "$TEX/luigi_big_jump.png"
cp -v "$CHAR/tile_0006.png"  "$TEX/luigi_big_skid.png"
cp -v "$CHAR/tile_0005.png"  "$TEX/luigi_big_climb.png"

cp -v "$CHAR/tile_0004.png"  "$TEX/luigi_fire_idle.png"
cp -v "$CHAR/tile_0005.png"  "$TEX/luigi_fire_run.png"
cp -v "$CHAR/tile_0007.png"  "$TEX/luigi_fire_jump.png"
cp -v "$CHAR/tile_0006.png"  "$TEX/luigi_fire_skid.png"
cp -v "$CHAR/tile_0005.png"  "$TEX/luigi_fire_climb.png"

# ── Enemies ──────────────────────────────────────────────────────────
cp -v "$TEX/enemy_walker.png"   "$TEX/enemy_goomba.png"     # walker → goomba
cp -v "$TEX/enemy_jumper.png"   "$TEX/enemy_koopa.png"      # jumper → koopa
cp -v "$TEX/enemy_shooter.png"  "$TEX/enemy_piranha.png"    # shooter → piranha
cp -v "$TEX/enemy_guardian.png" "$TEX/enemy_bowser.png"      # guardian → bowser
cp -v "$TEX/projectile.png"    "$TEX/bowser_fire.png"        # projectile → bowser fire
cp -v "$TEX/projectile.png"    "$TEX/fireball.png"           # projectile → fireball

# ── Blocks & Tiles ───────────────────────────────────────────────────
cp -v "$TEX/power_crystal.png"  "$TEX/question_block.png"   # ? block look-alike
cp -v "$TEX/destructible.png"   "$TEX/brick.png"            # destructible → brick
cp -v "$TEX/tileset_ruins.png"  "$TEX/tileset_mario.png"    # main tileset
cp -v "$TEX/tile_0100.png"      "$TEX/pipe.png"             # door/pipe tile

# ── Items ────────────────────────────────────────────────────────────
# coin.png already exists with correct name
cp -v "$TEX/gem_shard.png"      "$TEX/mushroom.png"         # gem → mushroom
cp -v "$TEX/tile_0067.png"      "$TEX/fire_flower.png"      # blue gem tile → fire flower
cp -v "$TEX/tile_0151.png"      "$TEX/star.png"             # yellow orb → star
cp -v "$TEX/tile_0027.png"      "$TEX/one_up.png"           # special block → 1-up

# flagpole.png and particle.png already exist with correct names

# ── Backgrounds ──────────────────────────────────────────────────────
cp -v "$TEX/bg_ruins_far.png"   "$TEX/bg_sky.png"           # far parallax → sky
cp -v "$TEX/bg_ruins_mid.png"   "$TEX/bg_hills.png"         # mid parallax → hills
cp -v "$TEX/bg_ruins_near.png"  "$TEX/bg_bushes.png"        # near parallax → bushes

echo ""
echo "=== AUDIO ==="

# ── Sounds that already exist with correct names ──
# jump.ogg, land.ogg, coin_pickup.ogg, powerup_pickup.ogg, powerup_expire.ogg,
# enemy_stomp.ogg, enemy_shoot.ogg, player_hurt.ogg, player_death.ogg,
# block_break.ogg, level_complete.ogg, game_over.ogg,
# menu_select.ogg, menu_confirm.ogg, pause.ogg — all present!

# Sounds that need to be created from existing Kenney audio
cp -v "$AUD/powerUp2.ogg"       "$AUD/one_up.ogg"           # powerup sound → 1-up
cp -v "$AUD/pepSound1.ogg"      "$AUD/kick.ogg"             # pep sound → kick
cp -v "$AUD/laser1.ogg"         "$AUD/fireball.ogg"         # laser → fireball
cp -v "$AUD/tone1.ogg"          "$AUD/block_hit.ogg"        # tone → block hit
cp -v "$AUD/phaseJump1.ogg"     "$AUD/pipe_enter.ogg"       # phase sound → pipe
cp -v "$AUD/threeTone1.ogg"     "$AUD/flagpole.ogg"         # three-tone → flagpole

# ── Music ────────────────────────────────────────────────────────────
cp -v "$AUD/ruins_theme.ogg"    "$AUD/overworld_theme.ogg"   # main theme
cp -v "$AUD/boss_theme.ogg"     "$AUD/castle_theme.ogg"      # boss → castle
cp -v "$AUD/ruins_theme.ogg"    "$AUD/underground_theme.ogg" # reuse for underground
cp -v "$AUD/boss_theme.ogg"     "$AUD/star_theme.ogg"        # reuse boss for star
# menu_theme.ogg already exists

echo ""
echo "=== VERIFICATION ==="

# Check every file referenced in manifest.json
MISSING=0
FOUND=0
while IFS= read -r file; do
    if [ -f "assets/$file" ]; then
        ((FOUND++))
    else
        echo "MISSING: assets/$file"
        ((MISSING++))
    fi
done <<'MANIFEST'
textures/mario_small_idle.png
textures/mario_small_run.png
textures/mario_small_jump.png
textures/mario_small_skid.png
textures/mario_small_hurt.png
textures/mario_small_dead.png
textures/mario_small_grow.png
textures/mario_small_climb.png
textures/mario_big_idle.png
textures/mario_big_run.png
textures/mario_big_jump.png
textures/mario_big_skid.png
textures/mario_big_climb.png
textures/mario_fire_idle.png
textures/mario_fire_run.png
textures/mario_fire_jump.png
textures/mario_fire_skid.png
textures/mario_fire_climb.png
textures/luigi_small_idle.png
textures/luigi_small_run.png
textures/luigi_small_jump.png
textures/luigi_small_skid.png
textures/luigi_small_hurt.png
textures/luigi_small_dead.png
textures/luigi_small_grow.png
textures/luigi_small_climb.png
textures/luigi_big_idle.png
textures/luigi_big_run.png
textures/luigi_big_jump.png
textures/luigi_big_skid.png
textures/luigi_big_climb.png
textures/luigi_fire_idle.png
textures/luigi_fire_run.png
textures/luigi_fire_jump.png
textures/luigi_fire_skid.png
textures/luigi_fire_climb.png
textures/enemy_goomba.png
textures/enemy_koopa.png
textures/enemy_piranha.png
textures/enemy_bowser.png
textures/bowser_fire.png
textures/fireball.png
textures/tileset_mario.png
textures/question_block.png
textures/brick.png
textures/pipe.png
textures/coin.png
textures/mushroom.png
textures/fire_flower.png
textures/star.png
textures/one_up.png
textures/flagpole.png
textures/particle.png
textures/bg_sky.png
textures/bg_hills.png
textures/bg_bushes.png
audio/jump.ogg
audio/land.ogg
audio/coin_pickup.ogg
audio/powerup_pickup.ogg
audio/powerup_expire.ogg
audio/one_up.ogg
audio/enemy_stomp.ogg
audio/enemy_shoot.ogg
audio/kick.ogg
audio/fireball.ogg
audio/player_hurt.ogg
audio/player_death.ogg
audio/block_hit.ogg
audio/block_break.ogg
audio/pipe_enter.ogg
audio/flagpole.ogg
audio/level_complete.ogg
audio/game_over.ogg
audio/menu_select.ogg
audio/menu_confirm.ogg
audio/pause.ogg
audio/overworld_theme.ogg
audio/underground_theme.ogg
audio/castle_theme.ogg
audio/star_theme.ogg
audio/menu_theme.ogg
fonts/main.ttf
MANIFEST

echo ""
echo "✓ Found: $FOUND files"
if [ "$MISSING" -gt 0 ]; then
    echo "✗ Missing: $MISSING files"
    exit 1
else
    echo "✓ All manifest files present!"
fi
