#pragma once

// Lara's guns while a cutscene plays: what she wears before the scene, the
// flashes and smoke of the shots that the scene fires, and the light of the
// flashes.

// Keeps Lara's outfit, the guns in her hands and holsters, and whether her
// holsters show, so that the end of the scene can give them back.
void Lara_Cutscene_CaptureState(void);
// Gives back what Lara_Cutscene_CaptureState kept.
void Lara_Cutscene_RestoreState(void);

// Counts down the flashes of the shots that the scene fires, rises their smoke
// and lights the area around them. Called once per tick.
void Lara_Cutscene_ControlGun(void);
void Lara_Cutscene_ResetGunFlashes(void);
void Lara_Cutscene_DrawGunFlashes(void);
