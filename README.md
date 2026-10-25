# Cricket07LineupFix

An ASI plugin for **EA Sports Cricket 07** that improves automatic team selection and batting order using the players in your loaded squad.

## Improvements over native selection

- **Batting-focused wicketkeeper choice.** Chooses the strongest batter among eligible keepers within the game's keeper priority groups, instead of ranking them by Catching.
- **More natural keeper placement.** Removes the fixed keeper position and the native 80-Skill opening promotion. Keepers fit into the batting order according to batting ability and opening role.
- **Broader batting opportunities.** Considers batting ability across specialist batters, openers, keepers and allrounders, helping capable players compete for batting places beyond their primary role.
- **Opening batters higher in the order.** All selected opening batters appear ahead of other roles, with the stronger batters first within each group. Additional selected openers can bat at No. 3, No. 4 and beyond.
- **No hardcoded player exclusions.** Removes the native format-specific player-ID exclusion lists from the replacement selector.

## Other features

- Supports limited-overs and Test team selection, with pitch-dependent squad balance.
- Handles squads missing specialist roles through fallback selections.
- Uses your currently loaded roster and its player ratings.
- Retains the game's captain selection and bowling-order completion.
- Writes selected XIs and keeper information to `Cricket07Lineup.log`.
- Works as an ASI plugin without editing player databases or the game executable on disk.

## Installation

You need Cricket 07 for Windows and **Ultimate ASI Loader** installed.

1. Close Cricket 07.
2. Copy [`Cricket07Lineup.asi`](dist/Cricket07Lineup.asi) into the game folder, beside `Cricket07.exe`.
3. Start the game normally.

If updating, replace the previous `Cricket07Lineup.asi`. Keep your ASI loader and other plugins in place.

## Usage

Let the game generate the playing XI as usual. The plugin applies automatically; there is no configuration file to edit.

Selection depends on the roles and ratings in your loaded squad. Check the resulting XI on the lineup screen or in `Cricket07Lineup.log`, created beside the plugin.

## Uninstall

Close the game and remove `Cricket07Lineup.asi`. You can also delete its log file. Keep the ASI loader if other plugins use it.
