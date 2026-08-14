// TR4 in-game cutscene ("cutseq") playback: drives the camera and actors
// from the delta-compressed tracks in cutseq.pak while the level keeps
// running underneath.
#pragma once

#include <trx/game/objects/ids.h>
#include <trx/game/types.h>

#include <stdint.h>

// The original engine addresses its cutscenes with a 32-bit mask, so no more
// than this are playable. Retail cutseq.pak holds 31.
#define CUTSEQ_MAX_CUTSCENES 32

// A cutscene trigger names a number the pak need not hold a scene for - TR4
// uses 32 to ask for an FMV. The played-once memory covers every number a
// trigger may carry, so one the engine cannot play is still answered once
// rather than asked again on the next frame.
#define CUTSEQ_MAX_TRIGGERS 64

// Reads cutseq.pak, if this game has one, and takes this level's default
// framing. Called once per level load, so the first cutscene of a level does
// not pay for the read.
void CutSeq_Load(void);

bool CutSeq_IsAvailable(void);
// How many cutscenes this game can play; 0 when it has none. Every number a
// caller passes below is a valid one only while it is under this.
int32_t CutSeq_GetCount(void);
bool CutSeq_IsActive(void);
bool CutSeq_IsPlaying(void);
int32_t CutSeq_GetCurrent(void);
// Which frame of the running scene is on screen, or -1 when none is. A scene
// has no other clock: its actors are pose tracks rather than items, so a
// script with something to do part-way through has only the frame to name it
// by, as the original engine does.
int32_t CutSeq_GetFrame(void);

// Fades out, then plays the cutscene. A scene that opens a level asks not to
// fade: the original game leaves the screen black rather than showing the
// level for a moment first, and the scene's own fade in is what follows.
void CutSeq_Request(int32_t num, bool fade_out);

// Whether a cutscene trigger naming this number has already been answered.
// The number need not be one the pak can play; see CUTSEQ_MAX_TRIGGERS.
bool CutSeq_IsPlayed(int32_t num);
void CutSeq_SetPlayed(int32_t num, bool played);
// The whole played-once bitmask, for the paths that carry it as one value:
// the savegame, and the start of a playthrough.
uint64_t CutSeq_GetPlayedMask(void);
void CutSeq_SetPlayedMask(uint64_t mask);

// How many actors the running scene has, or 0 when none is running. Actor 0
// is Lara, who is posed rather than drawn as an actor; the rest are the cast.
int32_t CutSeq_GetActorCount(void);

// Whether an actor is drawn. A scene brings its whole cast on at once, so a
// script that wants one of them held back until later says so here; the OG
// carries the same rule per scene in its own code.
void CutSeq_SetActorVisible(int32_t actor, bool visible);

// Draws another object's mesh in place of the one the actor's node carries,
// which is how a talking head is put on a body. Pass NO_OBJECT to take the
// override back off.
void CutSeq_SetActorNodeMesh(
    int32_t actor, int32_t node, OBJECT_ID obj_id, int32_t src_node);

// Where Lara stands once the running cutscene ends. It starts as where she
// was when the cutscene was requested; a script may place her elsewhere, as
// the original engine does for the scenes that move her.
void CutSeq_SetLaraReturn(XYZ_32 pos, int16_t rot);

// Ends the running cutscene early, fading out as it would at its own last
// frames, so the scene finishes rather than being torn away: Lara is put back
// where she belongs and the end event still fires. Does nothing when no
// cutscene is playing or one is already ending.
void CutSeq_Skip(void);

// Handles a TO_CUTSCENE floor trigger: plays the cutscene once, unless a
// script answers the trigger itself.
void CutSeq_HandleTrigger(int32_t num);

// How a cutscene is framed: the field of view it plays at, and the depth of
// each cinematic bar as a fraction of the screen height.
int32_t CutSeq_GetFOV(void);
void CutSeq_SetFOV(int32_t fov);
float CutSeq_GetLetterbox(void);
void CutSeq_SetLetterbox(float ratio);

// Engine hooks.
void CutSeq_Reset(void); // drops all runtime state (level end)
void CutSeq_Control(void); // state machine and per-tick decode
void CutSeq_PostControl(void); // reasserts the cinematic camera
void CutSeq_UpdateCamera(void); // places the camera (Camera_Update)
void CutSeq_PreDraw(void); // interpolates poses for rendering (Game_Draw)
void CutSeq_DrawActors(void); // draws actors 1..N (Room_DrawAllRooms)
void CutSeq_DrawOverlay(void); // fade + letterbox bars (Game_Draw)
