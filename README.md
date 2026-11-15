# Click Empire — addicting idle/clicker for Roblox

## Studio setup (2 minutes)
Create these in Roblox Studio and paste the code:

| File | Create as | Put it in |
|---|---|---|
| Config.luau | ModuleScript named `Config` | ReplicatedStorage |
| GameServer.server.luau | Script named `GameServer` | ServerScriptService |
| ClientUI.client.luau | LocalScript named `ClientUI` | StarterPlayer > StarterPlayerScripts |

Enable saving: Game Settings > Security > "Enable Studio Access to API Services".
Press Play. No map needed.

## The addiction loop
1. Click for coins (satisfying bounce + floating numbers, red CRIT hits)
2. Buy Click Power / Auto Clicker / Lucky Crit (costs grow ~1.35-1.6x)
3. Golden orb appears every 25-60s for 8s: big bonus, keeps people watching the screen
4. Rebirth: wipe progress for a permanent +50% multiplier. Costs grow 4x per rebirth
5. Leaderboard (Coins, Rebirths) + autosave, so progress is always waiting

## Tune it
Everything is in `Config.luau` (costs, growth, golden timing, rebirth bonus).

## Security
- Server owns coins, levels, rebirths; client only requests
- Every remote is rate-limited and type/range-checked; costs come from server config
- Golden orbs use single-use server tokens with expiry
- Data load failure kicks the player (live) so blank data never overwrites a save

## Intentionally left simple
- No DataStore session locking (fine for a prototype; use ProfileStore before launch)
- No Robux products yet (2x coins gamepass would be the natural first one; needs receipt handling)
- Exploiters can still fire Click at the 20/sec cap; bounded, not eliminated
