#!/usr/bin/env bash
# =============================================================================
# rename_assets.sh
# Copies Kenney Pixel Platformer tiles to game-manifest filenames.
# Originals are preserved — only new copies are created.
# All output goes flat into assets/textures/
# =============================================================================

set -euo pipefail

DIR="assets/textures"
CHAR="$DIR/Characters"
BG="$DIR/Backgrounds"

echo "=== Copying character sprites ==="

# ---- Player (green robot) ----
cp "$CHAR/tile_0000.png" "$DIR/player_idle.png"
cp "$CHAR/tile_0001.png" "$DIR/player_run.png"
cp "$CHAR/tile_0000.png" "$DIR/player_jump.png"       # standing pose reused (no dedicated jump frame)
cp "$CHAR/tile_0001.png" "$DIR/player_fall.png"        # walk frame reused
cp "$CHAR/tile_0001.png" "$DIR/player_dash.png"        # walk frame reused
cp "$CHAR/tile_0000.png" "$DIR/player_wallslide.png"   # idle pose reused
cp "$CHAR/tile_0004.png" "$DIR/player_hurt.png"        # pink robot = hurt tint
cp "$CHAR/tile_0026.png" "$DIR/player_dead.png"        # dark round creature = defeated

# ---- Enemies ----
cp "$CHAR/tile_0011.png" "$DIR/enemy_walker.png"       # yellow angry face — simple ground patrol
cp "$CHAR/tile_0013.png" "$DIR/enemy_jumper.png"       # small brown creature — bouncy
cp "$CHAR/tile_0015.png" "$DIR/enemy_shooter.png"      # red rocket — ranged attacker
cp "$CHAR/tile_0024.png" "$DIR/enemy_guardian.png"     # winged bat — territorial guardian

echo "=== Copying world sprites ==="

# ---- World ----
cp "$DIR/tilemap.png"     "$DIR/tileset_ruins.png"     # full tileset spritesheet
cp "$DIR/tile_0006.png"   "$DIR/destructible.png"      # cracked brick block
cp "$DIR/tile_0090.png"   "$DIR/flagpole.png"          # flagpole with flag
cp "$CHAR/tile_0017.png"  "$DIR/projectile.png"        # small orange triangle projectile
cp "$DIR/tile_0151.png"   "$DIR/particle.png"          # small yellow dot

# ---- Items ----
cp "$DIR/tile_0008.png"   "$DIR/coin.png"              # small round coin
cp "$DIR/tile_0007.png"   "$DIR/gem_shard.png"         # small diamond/gem fragment
cp "$DIR/tile_0010.png"   "$DIR/power_crystal.png"     # gold block with ! mark

echo "=== Copying background layers ==="

# ---- Backgrounds (warm/orange tiles — "ruins" parallax) ----
cp "$BG/tile_0004.png"    "$DIR/bg_ruins_far.png"      # warm solid sky (farthest layer)
cp "$BG/tile_0012.png"    "$DIR/bg_ruins_mid.png"      # orange/sandy with distant ground
cp "$BG/tile_0020.png"    "$DIR/bg_ruins_near.png"     # orange/brown foreground elements

echo ""
echo "Done! 23 manifest files created in $DIR/"
echo "Original tile files are untouched."
