#include "inject_exec.h"

#include "decomp/decomp.h"
#include "decomp/effects.h"
#include "decomp/stats.h"
#include "game/background.h"
#include "game/box.h"
#include "game/camera.h"
#include "game/collide.h"
#include "game/creature.h"
#include "game/demo.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/gameflow.h"
#include "game/gun/gun.h"
#include "game/gun/gun_misc.h"
#include "game/gun/gun_pistols.h"
#include "game/gun/gun_rifle.h"
#include "game/hwr.h"
#include "game/input.h"
#include "game/inventory/backpack.h"
#include "game/inventory/common.h"
#include "game/inventory/ring.h"
#include "game/items.h"
#include "game/lara/cheat.h"
#include "game/lara/col.h"
#include "game/lara/control.h"
#include "game/lara/draw.h"
#include "game/lara/look.h"
#include "game/lara/misc.h"
#include "game/lara/state.h"
#include "game/level.h"
#include "game/los.h"
#include "game/lot.h"
#include "game/math.h"
#include "game/math_misc.h"
#include "game/matrix.h"
#include "game/music.h"
#include "game/objects/common.h"
#include "game/objects/creatures/bird.h"
#include "game/objects/creatures/diver.h"
#include "game/objects/effects/ember.h"
#include "game/objects/effects/flame.h"
#include "game/objects/general/body_part.h"
#include "game/objects/general/door.h"
#include "game/objects/general/final_level_counter.h"
#include "game/objects/traps/ember_emitter.h"
#include "game/objects/traps/flame_emitter.h"
#include "game/objects/vehicles/boat.h"
#include "game/option/option.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/random.h"
#include "game/requester.h"
#include "game/room.h"
#include "game/room_draw.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/text.h"
#include "inject_util.h"
#include "specific/s_audio_sample.h"
#include "specific/s_flagged_string.h"
#include "specific/s_input.h"

static void M_DecompGeneral(const bool enable);
static void M_DecompStats(const bool enable);
static void M_DecompEffects(const bool enable);
static void M_HWR(bool enable);

static void M_Camera(bool enable);
static void M_Collide(bool enable);
static void M_Room(bool enable);
static void M_Math(bool enable);
static void M_Matrix(bool enable);
static void M_Shell(bool enable);
static void M_Requester(bool enable);
static void M_Option(bool enable);
static void M_Text(bool enable);
static void M_Input(bool enable);
static void M_Output(bool enable);
static void M_Music(bool enable);
static void M_Sound(bool enable);

static void M_Demo(bool enable);
static void M_Gameflow(bool enable);
static void M_Overlay(bool enable);
static void M_Random(bool enable);
static void M_Items(bool enable);
static void M_Effects(bool enable);
static void M_LOS(bool enable);
static void M_People(bool enable);
static void M_Level(bool enable);
static void M_Inventory(bool enable);
static void M_Lara_Look(bool enable);
static void M_Lara_Draw(bool enable);
static void M_Lara_Misc(bool enable);
static void M_Lara_State(bool enable);
static void M_Lara_Col(bool enable);
static void M_Gun(bool enable);

static void M_Creature(bool enable);
static void M_Box(bool enable);
static void M_Lot(bool enable);
static void M_Objects(bool enable);

static void M_S_Audio_Sample(bool enable);
static void M_S_Input(bool enable);
static void M_S_FlaggedString(bool enable);

static void M_DecompGeneral(const bool enable)
{
    INJECT(enable, 0x00411F50, Game_SetCutsceneTrack);
    INJECT(enable, 0x00411F60, Game_Cutscene_Start);
    INJECT(enable, 0x00412080, Misc_InitCinematicRooms);
    INJECT(enable, 0x00412120, Game_Cutscene_Control);
    INJECT(enable, 0x004123D0, Room_FindByPos);
    INJECT(enable, 0x00412450, CutscenePlayer_Control);
    INJECT(enable, 0x00412530, Lara_Control_Cutscene);
    INJECT(enable, 0x004125D0, CutscenePlayer1_Initialise);
    INJECT(enable, 0x00412660, CutscenePlayerGen_Initialise);
    INJECT(enable, 0x0043A280, Level_Initialise);
    INJECT(enable, 0x00444D60, WinVidSetMinWindowSize);
    INJECT(enable, 0x00444DB0, WinVidClearMinWindowSize);
    INJECT(enable, 0x00444DC0, WinVidSetMaxWindowSize);
    INJECT(enable, 0x00444E10, WinVidClearMaxWindowSize);
    INJECT(enable, 0x00444E20, CalculateWindowWidth);
    INJECT(enable, 0x00444E70, CalculateWindowHeight);
    INJECT(enable, 0x00444EA0, WinVidGetMinMaxInfo);
    INJECT(enable, 0x00444FB0, WinVidFindGameWindow);
    INJECT(enable, 0x00444FD0, WinVidSpinMessageLoop);
    INJECT(enable, 0x004450C0, WinVidShowGameWindow);
    INJECT(enable, 0x00445110, WinVidHideGameWindow);
    INJECT(enable, 0x00445150, WinVidSetGameWindowSize);
    INJECT(enable, 0x00445190, ShowDDrawGameWindow);
    INJECT(enable, 0x00445240, HideDDrawGameWindow);
    INJECT(enable, 0x004452D0, DDrawSurfaceCreate);
    INJECT(enable, 0x00445320, DDrawSurfaceRestoreLost);
    INJECT(enable, 0x00445370, WinVidClearBuffer);
    INJECT(enable, 0x004453C0, WinVidBufferLock);
    INJECT(enable, 0x00445400, WinVidBufferUnlock);
    INJECT(enable, 0x00445430, WinVidCopyBitmapToBuffer);
    INJECT(enable, 0x004454C0, GetRenderBitDepth);
    INJECT(enable, 0x00445550, WinVidGetColorBitMasks);
    INJECT(enable, 0x004455D0, BitMaskGetNumberOfBits);
    INJECT(enable, 0x00445620, CalculateCompatibleColor);
    INJECT(enable, 0x00445690, WinVidGetDisplayMode);
    INJECT(enable, 0x00445720, WinVidGoFullScreen);
    INJECT(enable, 0x004457B0, WinVidGoWindowed);
    INJECT(enable, 0x004458C0, WinVidSetDisplayAdapter);
    INJECT(enable, 0x004471F0, DInputCreate);
    INJECT(enable, 0x00447220, DInputRelease);
    INJECT(enable, 0x00447240, WinInReadKeyboard);
    INJECT(enable, 0x00448430, CreateScreenBuffers);
    INJECT(enable, 0x00448570, CreatePrimarySurface);
    INJECT(enable, 0x00448610, CreateBackBuffer);
    INJECT(enable, 0x004486B0, CreateClipper);
    INJECT(enable, 0x00448750, CreateWindowPalette);
    INJECT(enable, 0x00448830, CreateZBuffer);
    INJECT(enable, 0x004488F0, GetZBufferDepth);
    INJECT(enable, 0x00448920, CreateRenderBuffer);
    INJECT(enable, 0x004489D0, CreatePictureBuffer);
    INJECT(enable, 0x00448A40, ClearBuffers);
    INJECT(enable, 0x00448BF0, RestoreLostBuffers);
    INJECT(enable, 0x00448D30, UpdateFrame);
    INJECT(enable, 0x00448E00, WaitPrimaryBufferFlip);
    INJECT(enable, 0x00448E40, RenderInit);
    INJECT(enable, 0x00448E50, RenderStart);
    INJECT(enable, 0x00449200, RenderFinish);
    INJECT(enable, 0x004492F0, ApplySettings);
    INJECT(enable, 0x00449500, FmvBackToGame);
    INJECT(enable, 0x00449610, GameApplySettings);
    INJECT(enable, 0x00449850, UpdateGameResolution);
    INJECT(enable, 0x004498C0, DecodeErrorMessage);
    INJECT(enable, 0x0044E4E0, RenderErrorBox);
    INJECT(enable, 0x0044E520, WinMain);
    INJECT(enable, 0x0044E700, GameInit);
    INJECT(enable, 0x0044E7A0, WinGameStart);
    INJECT(enable, 0x0044E820, Shell_Shutdown);
    INJECT(enable, 0x0044E8E0, ScreenshotPCX);
    INJECT(enable, 0x0044E9F0, CompPCX);
    INJECT(enable, 0x0044EAA0, EncodeLinePCX);
    INJECT(enable, 0x0044EB80, EncodePutPCX);
    INJECT(enable, 0x0044EBC0, Screenshot);
    INJECT(enable, 0x00454C50, TitleSequence);
    INJECT(enable, 0x00444540, Enumerate3DDevices);
    INJECT(enable, 0x00444570, D3DCreate);
    INJECT(enable, 0x004445B0, Enum3DDevicesCallback);
    INJECT(enable, 0x00444670, D3DIsSupported);
    INJECT(enable, 0x004446B0, D3DSetViewport);
    INJECT(enable, 0x00444770, D3DDeviceCreate);
    INJECT(enable, 0x00444930, Direct3DRelease);
    INJECT(enable, 0x00444980, Direct3DInit);
    INJECT(enable, 0x00444BD0, DDrawCreate);
    INJECT(enable, 0x00444C30, DDrawRelease);
    INJECT(enable, 0x00444C70, GameWindowCalculateSizeFromClient);
    INJECT(enable, 0x00444CF0, GameWindowCalculateSizeFromClientByZero);
    INJECT(enable, 0x004459A0, CompareVideoModes);
    INJECT(enable, 0x004459F0, WinVidGetDisplayModes);
    INJECT(enable, 0x00445A50, EnumDisplayModesCallback);
    INJECT(enable, 0x00445E10, WinVidInit);
    INJECT(enable, 0x00445E50, WinVidGetDisplayAdapters);
    INJECT(enable, 0x00445F20, EnumerateDisplayAdapters);
    INJECT(enable, 0x00446140, WinVidRegisterGameWindowClass);
    INJECT(enable, 0x004461B0, WinVidGameWindowProc);
    INJECT(enable, 0x00445F40, EnumDisplayAdaptersCallback);
    INJECT(enable, 0x004467C0, WinVidResizeGameWindow);
    INJECT(enable, 0x004469A0, WinVidCheckGameWindowPalette);
    INJECT(enable, 0x00446A60, WinVidCreateGameWindow);
    INJECT(enable, 0x00446B30, WinVidFreeWindow);
    INJECT(enable, 0x00446B60, WinVidExitMessage);
    INJECT(enable, 0x00446BB0, WinVidGetDisplayAdapter);
    INJECT(enable, 0x00446C00, WinVidStart);
    INJECT(enable, 0x00446F80, WinVidFinish);
    INJECT(enable, 0x00414220, Misc_Move3DPosTo3DPos);
    INJECT(enable, 0x00455140, S_LoadSettings);
    INJECT(enable, 0x004550C0, S_SaveSettings);
}

static void M_DecompStats(const bool enable)
{
    INJECT(enable, 0x004262B0, AddAssaultTime);
    INJECT(enable, 0x00426340, ShowGymStatsText);
    INJECT(enable, 0x00426520, ShowStatsText);
    INJECT(enable, 0x004268C0, ShowEndStatsText);
    INJECT(enable, 0x0044C680, LevelStats);
    INJECT(enable, 0x0044C850, GameStats);
}

static void M_DecompEffects(const bool enable)
{
    INJECT(enable, 0x00433360, Effect_ExplodingDeath);
}

static void M_HWR(bool enable)
{
    INJECT(enable, 0x0044CFE0, HWR_InitState);
    INJECT(enable, 0x0044D110, HWR_ResetTexSource);
    INJECT(enable, 0x0044D140, HWR_ResetColorKey);
    INJECT(enable, 0x0044D170, HWR_ResetZBuffer);
    INJECT(enable, 0x0044D1D0, HWR_TexSource);
    INJECT(enable, 0x0044D200, HWR_EnableColorKey);
    INJECT(enable, 0x0044D250, HWR_EnableZBuffer);
    INJECT(enable, 0x0044D2E0, HWR_BeginScene);
    INJECT(enable, 0x0044D310, HWR_DrawPolyList);
    INJECT(enable, 0x0044D490, HWR_LoadTexturePages);
    INJECT(enable, 0x0044D520, HWR_FreeTexturePages);
    INJECT(enable, 0x0044D570, HWR_GetPageHandles);
    INJECT(enable, 0x0044D5B0, HWR_VertexBufferFull);
    INJECT(enable, 0x0044D5E0, HWR_Init);
}

static void M_Background(const bool enable)
{
    INJECT(enable, 0x00443990, BGND_Make640x480);
    INJECT(enable, 0x00443B50, BGND_AddTexture);
    INJECT(enable, 0x00443C10, BGND_GetPageHandles);
    INJECT(enable, 0x00443C50, BGND_DrawInGameBlack);
    INJECT(enable, 0x00443CB0, DrawQuad);
    INJECT(enable, 0x00443D90, BGND_DrawInGameBackground);
    INJECT(enable, 0x00443FB0, DrawTextureTile);
    INJECT(enable, 0x00444210, BGND_CenterLighting);
    INJECT(enable, 0x004444C0, BGND_Free);
    INJECT(enable, 0x00444510, BGND_Init);
}

static void M_Camera(const bool enable)
{
    INJECT(enable, 0x004105A0, Camera_Initialise);
    INJECT(enable, 0x00410650, Camera_Move);
    INJECT(enable, 0x004109D0, Camera_Clip);
    INJECT(enable, 0x00410AB0, Camera_Shift);
    INJECT(enable, 0x00410C10, Camera_GoodPosition);
    INJECT(enable, 0x00410C60, Camera_SmartShift);
    INJECT(enable, 0x004113F0, Camera_Chase);
    INJECT(enable, 0x004114E0, Camera_ShiftClamp);
    INJECT(enable, 0x00411680, Camera_Combat);
    INJECT(enable, 0x00411810, Camera_Look);
    INJECT(enable, 0x00411A00, Camera_Fixed);
    INJECT(enable, 0x00411AA0, Camera_Update);
    INJECT(enable, 0x004126A0, Camera_LoadCutsceneFrame);
    INJECT(enable, 0x00412290, Camera_UpdateCutscene);
    INJECT(enable, 0x00415100, Camera_RefreshFromTrigger);
}

static void M_Collide(const bool enable)
{
    INJECT(enable, 0x004128F0, Collide_GetCollisionInfo);
    INJECT(enable, 0x00412FE0, Collide_CollideStaticObjects);
}

static void M_Game(const bool enable)
{
    INJECT(enable, 0x00414390, Game_Control);
    INJECT(enable, 0x00418990, Game_Draw);
    INJECT(enable, 0x00418950, Game_DrawCinematic);
    INJECT(enable, 0x0044C480, Game_Start);
    INJECT(enable, 0x0044C5D0, Game_Loop);
}

static void M_Room(const bool enable)
{
    INJECT(enable, 0x00412FB0, Room_FindGridShift);
    INJECT(enable, 0x004133D0, Room_GetNearbyRooms);
    INJECT(enable, 0x004134A0, Room_GetNewRoom);
    INJECT(enable, 0x004135A0, Room_GetTiltType);
    INJECT(enable, 0x00414B70, Room_GetSector);
    INJECT(enable, 0x00414D10, Room_GetWaterHeight);
    INJECT(enable, 0x00414E80, Room_GetHeight);
    INJECT(enable, 0x00415930, Room_GetCeiling);
    INJECT(enable, 0x00415B90, Room_GetDoor);
    INJECT(enable, 0x004151F0, Room_TestTriggers);
    INJECT(enable, 0x004340B0, Room_AlterFloorHeight);
    INJECT(enable, 0x00416640, Room_FlipMap);
    INJECT(enable, 0x00416700, Room_RemoveFlipItems);
    INJECT(enable, 0x004167A0, Room_AddFlipItems);
    INJECT(enable, 0x00418C80, Room_GetBounds);
    INJECT(enable, 0x00418E50, Room_SetBounds);
    INJECT(enable, 0x004191D0, Room_Clip);
    INJECT(enable, 0x004189D0, Room_DrawAllRooms);
    INJECT(enable, 0x004195B0, Room_DrawSingleRoomGeometry);
    INJECT(enable, 0x00419670, Room_DrawSingleRoomObjects);
    INJECT(enable, 0x00416800, Room_TriggerMusicTrack);
}

static void M_Matrix(const bool enable)
{
    INJECT(enable, 0x00401000, Matrix_GenerateW2V);
    INJECT(enable, 0x004011D0, Matrix_LookAt);
    INJECT(enable, 0x004012D0, Matrix_RotX);
    INJECT(enable, 0x00401380, Matrix_RotY);
    INJECT(enable, 0x00401430, Matrix_RotZ);
    INJECT(enable, 0x004014E0, Matrix_RotYXZ);
    INJECT(enable, 0x004016C0, Matrix_RotYXZpack);
    INJECT(enable, 0x004018B0, Matrix_TranslateRel);
    INJECT(enable, 0x00401960, Matrix_TranslateAbs);
    INJECT(enable, 0x0041B710, Matrix_InitInterpolate);
    INJECT(enable, 0x0041B750, Matrix_Pop_I);
    INJECT(enable, 0x0041B780, Matrix_Push_I);
    INJECT(enable, 0x0041B7B0, Matrix_RotY_I);
    INJECT(enable, 0x0041B7F0, Matrix_RotX_I);
    INJECT(enable, 0x0041B830, Matrix_RotZ_I);
    INJECT(enable, 0x0041B870, Matrix_TranslateRel_I);
    INJECT(enable, 0x0041B8C0, Matrix_TranslateRel_ID);
    INJECT(enable, 0x0041B910, Matrix_RotYXZ_I);
    INJECT(enable, 0x0041B960, Matrix_RotYXZsuperpack_I);
    INJECT(enable, 0x0041B9A0, Matrix_RotYXZsuperpack);
    INJECT(enable, 0x0041BA80, Matrix_Interpolate);
    INJECT(enable, 0x0041BC30, Matrix_InterpolateArm);
    INJECT(enable, 0x00457280, Matrix_Push);
    INJECT(enable, 0x0045729E, Matrix_PushUnit);
}

static void M_Math(const bool enable)
{
    INJECT(enable, 0x00457C10, Math_Atan);
    INJECT(enable, 0x00457C58, Math_Cos);
    INJECT(enable, 0x00457C5E, Math_Sin);
    INJECT(enable, 0x00457C93, Math_Sqrt);
    INJECT(enable, 0x00401250, Math_GetVectorAngles);
}

static void M_Shell(const bool enable)
{
    INJECT(enable, 0x0044E770, Shell_Cleanup);
    INJECT(enable, 0x0044E890, Shell_ExitSystem);
    INJECT(enable, 0x00454980, Shell_Main);
}

static void M_Requester(const bool enable)
{
    INJECT(enable, 0x004255A0, Requester_Init);
    INJECT(enable, 0x00425630, Requester_Shutdown);
    INJECT(enable, 0x004257C0, Requester_Display);
    INJECT(enable, 0x004256E0, Requester_Item_CenterAlign);
    INJECT(enable, 0x00425700, Requester_Item_LeftAlign);
    INJECT(enable, 0x00425760, Requester_Item_RightAlign);
    INJECT(enable, 0x00426030, Requester_SetHeading);
    INJECT(enable, 0x004260E0, Requester_RemoveAllItems);
    INJECT(enable, 0x00426100, Requester_ChangeItem);
    INJECT(enable, 0x004261C0, Requester_AddItem);
    INJECT(enable, 0x00426270, Requester_SetSize);
}

static void M_Option(const bool enable)
{
    INJECT(enable, 0x0044EDC0, Option_DoInventory);
    INJECT(enable, 0x0044EED0, Option_Passport);
    INJECT(enable, 0x0044F520, Option_Detail);
    INJECT(enable, 0x0044F800, Option_Sound);
    INJECT(enable, 0x0044FCA0, Option_Compass);
    INJECT(enable, 0x0044FE20, Option_Controls);
}

static void M_Text(const bool enable)
{
    INJECT(enable, 0x00440450, Text_Init);
    INJECT(enable, 0x00440480, Text_Create);
    INJECT(enable, 0x00440590, Text_ChangeText);
    INJECT(enable, 0x004405D0, Text_SetScale);
    INJECT(enable, 0x004405F0, Text_Flash);
    INJECT(enable, 0x00440620, Text_AddBackground);
    INJECT(enable, 0x004406B0, Text_RemoveBackground);
    INJECT(enable, 0x004406C0, Text_AddOutline);
    INJECT(enable, 0x004406F0, Text_RemoveOutline);
    INJECT(enable, 0x00440700, Text_CentreH);
    INJECT(enable, 0x00440720, Text_CentreV);
    INJECT(enable, 0x00440740, Text_AlignRight);
    INJECT(enable, 0x00440760, Text_AlignBottom);
    INJECT(enable, 0x00440780, Text_GetWidth);
    INJECT(enable, 0x00440890, Text_Remove);
    INJECT(enable, 0x004408F0, Text_Draw);
    INJECT(enable, 0x00440920, Text_DrawBorder);
    INJECT(enable, 0x00440AB0, Text_DrawText);
    INJECT(enable, 0x00440E90, Text_GetScaleH);
    INJECT(enable, 0x00440ED0, Text_GetScaleV);
}

static void M_Input(const bool enable)
{
    INJECT(enable, 0x0044DA10, Input_Update);
    INJECT(enable, 0x004239C0, Input_GetDebounced);
}

static void M_Output(const bool enable)
{
    INJECT(enable, 0x004019E0, Output_InsertPolygons);
    INJECT(enable, 0x00401AE0, Output_InsertRoom);
    INJECT(enable, 0x00401BD0, Output_CalcSkyboxLight);
    INJECT(enable, 0x00401C10, Output_InsertSkybox);
    INJECT(enable, 0x00401D60, Output_CalcObjectVertices);
    INJECT(enable, 0x00401F40, Output_CalcVerticeLight);
    INJECT(enable, 0x004020B0, Output_CalcRoomVertices);
    INJECT(enable, 0x00402330, Output_RotateLight);
    INJECT(enable, 0x00402400, Output_InitPolyList);
    INJECT(enable, 0x00402430, Output_SortPolyList);
    INJECT(enable, 0x00402470, Output_QuickSort);
    INJECT(enable, 0x00402540, Output_PrintPolyList);
    INJECT(enable, 0x00402580, Output_AlterFOV);
    INJECT(enable, 0x00402690, Output_SetNearZ);
    INJECT(enable, 0x004026E0, Output_SetFarZ);
    INJECT(enable, 0x00402700, Output_Init);
    INJECT(enable, 0x00402970, Output_DrawPolyLine);
    INJECT(enable, 0x00402B10, Output_DrawPolyFlat);
    INJECT(enable, 0x00402B50, Output_DrawPolyTrans);
    INJECT(enable, 0x00402B90, Output_DrawPolyGouraud);
    INJECT(enable, 0x00402BD0, Output_DrawPolyGTMap);
    INJECT(enable, 0x00402C10, Output_DrawPolyWGTMap);
    INJECT(enable, 0x00402C50, Output_XGenX);
    INJECT(enable, 0x00402D30, Output_XGenXG);
    INJECT(enable, 0x00402E80, Output_XGenXGUV);
    INJECT(enable, 0x004030A0, Output_XGenXGUVPerspFP);
    INJECT(enable, 0x00403330, Output_GTMapPersp32FP);
    INJECT(enable, 0x00404300, Output_WGTMapPersp32FP);
    INJECT(enable, 0x004057D0, Output_DrawPolyGTMapPersp);
    INJECT(enable, 0x00405810, Output_DrawPolyWGTMapPersp);
    INJECT(enable, 0x00405850, Output_VisibleZClip);
    INJECT(enable, 0x004058C0, Output_ZedClipper);
    INJECT(enable, 0x00405A00, Output_XYGUVClipper);
    INJECT(enable, 0x00405F20, Output_InsertObjectGT4);
    INJECT(enable, 0x00406980, Output_InsertObjectGT3);
    INJECT(enable, 0x00407200, Output_XYGClipper);
    INJECT(enable, 0x00407630, Output_InsertObjectG4);
    INJECT(enable, 0x00407A10, Output_InsertObjectG3);
    INJECT(enable, 0x00407D30, Output_XYClipper);
    INJECT(enable, 0x00408000, Output_InsertTrans8);
    INJECT(enable, 0x004084B0, Output_InsertTransQuad);
    INJECT(enable, 0x00408590, Output_InsertFlatRect);
    INJECT(enable, 0x00408660, Output_InsertLine);
    INJECT(enable, 0x00408720, Output_InsertGT3_ZBuffered);
    INJECT(enable, 0x00408D70, Output_DrawClippedPoly_Textured);
    INJECT(enable, 0x00408EB0, Output_InsertGT4_ZBuffered);
    INJECT(enable, 0x00409300, Output_InsertObjectGT4_ZBuffered);
    INJECT(enable, 0x004093A0, Output_InsertObjectGT3_ZBuffered);
    INJECT(enable, 0x00409450, Output_InsertObjectG4_ZBuffered);
    INJECT(enable, 0x004097F0, Output_DrawPoly_Gouraud);
    INJECT(enable, 0x004098F0, Output_InsertObjectG3_ZBuffered);
    INJECT(enable, 0x00409BD0, Output_InsertFlatRect_ZBuffered);
    INJECT(enable, 0x00409DA0, Output_InsertLine_ZBuffered);
    INJECT(enable, 0x00409EE0, Output_InsertGT3_Sorted);
    INJECT(enable, 0x0040A5F0, Output_InsertClippedPoly_Textured);
    INJECT(enable, 0x0040A7A0, Output_InsertGT4_Sorted);
    INJECT(enable, 0x0040AC80, Output_InsertObjectGT4_Sorted);
    INJECT(enable, 0x0040AD10, Output_InsertObjectGT3_Sorted);
    INJECT(enable, 0x0040ADB0, Output_InsertObjectG4_Sorted);
    INJECT(enable, 0x0040B1F0, Output_InsertPoly_Gouraud);
    INJECT(enable, 0x0040B370, Output_InsertObjectG3_Sorted);
    INJECT(enable, 0x0040B6C0, Output_InsertSprite_Sorted);
    INJECT(enable, 0x0040BA10, Output_InsertFlatRect_Sorted);
    INJECT(enable, 0x0040BB90, Output_InsertLine_Sorted);
    INJECT(enable, 0x0040BCC0, Output_InsertTrans8_Sorted);
    INJECT(enable, 0x0040BE60, Output_InsertTransQuad_Sorted);
    INJECT(enable, 0x0040BFA0, Output_InsertSprite);
    INJECT(enable, 0x0040C050, Output_DrawSprite);
    INJECT(enable, 0x0040C320, Output_DrawPickup);
    INJECT(enable, 0x0040C3B0, Output_InsertRoomSprite);
    INJECT(enable, 0x0040C510, Output_DrawScreenSprite2D);
    INJECT(enable, 0x0040C5B0, Output_DrawScreenSprite);
    INJECT(enable, 0x0040C650, Output_DrawScaledSpriteC);
    INJECT(enable, 0x0041BA50, Output_InsertPolygons_I);
}

static void M_Music(const bool enable)
{
    INJECT(enable, 0x004553E0, Music_Init);
    INJECT(enable, 0x00455460, Music_Shutdown);
    INJECT(enable, 0x00455500, Music_Play);
    INJECT(enable, 0x00455570, Music_Stop);
    INJECT(enable, 0x004555B0, Music_PlaySynced);
    INJECT(enable, 0x004556B0, Music_SetVolume);
}

static void M_Sound(const bool enable)
{
    INJECT(enable, 0x00455380, Sound_SetMasterVolume);
    INJECT(enable, 0x0041C560, Sound_UpdateEffects);
    INJECT(enable, 0x0043F3C0, Sound_Effect);
    INJECT(enable, 0x0043F860, Sound_StopEffect);
    INJECT(enable, 0x0043F8C0, Sound_EndScene);
    INJECT(enable, 0x0043F950, Sound_Shutdown);
    INJECT(enable, 0x0043F980, Sound_Init);
}

static void M_Demo(const bool enable)
{
    INJECT(enable, 0x00416910, Demo_Control);
    INJECT(enable, 0x00416970, Demo_Start);
    INJECT(enable, 0x00416B20, Demo_LoadLaraPos);
    INJECT(enable, 0x00416BF0, Demo_GetInput);
}

static void M_Gameflow(bool enable)
{
    INJECT(enable, 0x0041FA60, GF_LoadScriptFile);
    INJECT(enable, 0x0041FC50, GF_DoFrontendSequence);
    INJECT(enable, 0x0041FC70, GF_DoLevelSequence);
    INJECT(enable, 0x0041FCE0, GF_InterpretSequence);
    INJECT(enable, 0x004201C0, GF_ModifyInventory);
    INJECT(enable, 0x0044B6C0, GF_LoadFromFile);
}

static void M_Overlay(const bool enable)
{
    INJECT(enable, 0x004219A0, Overlay_FlashCounter);
    INJECT(enable, 0x004219D0, Overlay_DrawAssaultTimer);
    INJECT(enable, 0x00421B20, Overlay_DrawGameInfo);
    INJECT(enable, 0x00421B70, Overlay_DrawHealthBar);
    INJECT(enable, 0x00421C20, Overlay_DrawAirBar);
    INJECT(enable, 0x00421CC0, Overlay_MakeAmmoString);
    INJECT(enable, 0x00421CF0, Overlay_DrawAmmoInfo);
    INJECT(enable, 0x00421E40, Overlay_InitialisePickUpDisplay);
    INJECT(enable, 0x00421E60, Overlay_DrawPickups);
    INJECT(enable, 0x00421F60, Overlay_AddDisplayPickup);
    INJECT(enable, 0x00421FD0, Overlay_DisplayModeInfo);
    INJECT(enable, 0x00422050, Overlay_DrawModeInfo);
}

static void M_Random(const bool enable)
{
    INJECT(enable, 0x0044C970, Random_GetControl);
    INJECT(enable, 0x0044C990, Random_SeedControl);
    INJECT(enable, 0x0044C9A0, Random_GetDraw);
    INJECT(enable, 0x0044C9C0, Random_SeedDraw);
    INJECT(enable, 0x0044D870, Random_Seed);
}

static void M_Items(const bool enable)
{
    INJECT(enable, 0x00426CF0, Item_InitialiseArray);
    INJECT(enable, 0x00426D50, Item_Kill);
    INJECT(enable, 0x00426E70, Item_Create);
    INJECT(enable, 0x00426EB0, Item_Initialise);
    INJECT(enable, 0x00427070, Item_RemoveActive);
    INJECT(enable, 0x00427100, Item_RemoveDrawn);
    INJECT(enable, 0x00427170, Item_AddActive);
    INJECT(enable, 0x004271D0, Item_NewRoom);
    INJECT(enable, 0x00427270, Item_GlobalReplace);
    INJECT(enable, 0x00427520, Item_ClearKilled);
    INJECT(enable, 0x00413500, Item_ShiftCol);
    INJECT(enable, 0x00413540, Item_UpdateRoom);
    INJECT(enable, 0x00413D40, Item_TestBoundsCollide);
    INJECT(enable, 0x00413E10, Item_TestPosition);
    INJECT(enable, 0x00413F50, Item_AlignPosition);
    INJECT(enable, 0x004146F0, Item_Animate);
    INJECT(enable, 0x00414A60, Item_GetAnimChange);
    INJECT(enable, 0x00414B10, Item_Translate);
    INJECT(enable, 0x004158D0, Item_IsTriggerActive);
    INJECT(enable, 0x0041BF90, Item_GetFrames);
    INJECT(enable, 0x0041C030, Item_GetBoundsAccurate);
    INJECT(enable, 0x0041C0B0, Item_GetBestFrame);
}

static void M_Effects(const bool enable)
{
    INJECT(enable, 0x004272F0, Effect_InitialiseArray);
    INJECT(enable, 0x00427320, Effect_Create);
    INJECT(enable, 0x00427390, Effect_Kill);
    INJECT(enable, 0x00427480, Effect_NewRoom);
    INJECT(enable, 0x00419890, Effect_Draw);
}

static void M_LOS(const bool enable)
{
    INJECT(enable, 0x00415BE0, LOS_Check);
    INJECT(enable, 0x00415C80, LOS_CheckZ);
    INJECT(enable, 0x00415F70, LOS_CheckX);
    INJECT(enable, 0x00416260, LOS_ClipTarget);
    INJECT(enable, 0x00416340, LOS_CheckSmashable);
}

static void M_People(const bool enable)
{
    INJECT(enable, 0x00435E00, Creature_CanTargetEnemy);
}

static void M_Level(const bool enable)
{
    INJECT(enable, 0x0044B260, Level_Load);
}

static void M_Inventory(const bool enable)
{
    INJECT(enable, 0x00422080, Inv_Display);
    INJECT(enable, 0x00423310, Inv_Construct);
    INJECT(enable, 0x00423470, Inv_SelectMeshes);
    INJECT(enable, 0x00423500, Inv_AnimateInventoryItem);
    INJECT(enable, 0x00423590, Inv_DrawInventoryItem);
    INJECT(enable, 0x004239E0, Inv_DoInventoryPicture);
    INJECT(enable, 0x004239F0, Inv_DoInventoryBackground);
    INJECT(enable, 0x00423B30, Inv_InitColors);
    INJECT(enable, 0x00423C40, Inv_RingIsOpen);
    INJECT(enable, 0x00423DB0, Inv_RingIsNotOpen);
    INJECT(enable, 0x00423E40, Inv_RingNotActive);
    INJECT(enable, 0x004242B0, Inv_RingActive);
    INJECT(enable, 0x004242F0, Inv_AddItem);
    INJECT(enable, 0x00424B00, Inv_InsertItem);
    INJECT(enable, 0x00424C30, Inv_RequestItem);
    INJECT(enable, 0x00424CB0, Inv_RemoveAllItems);
    INJECT(enable, 0x00424CD0, Inv_RemoveItem);
    INJECT(enable, 0x00424DE0, Inv_GetItemOption);
    INJECT(enable, 0x00425000, Inv_Ring_Init);
    INJECT(enable, 0x00425110, Inv_Ring_GetView);
    INJECT(enable, 0x00425170, Inv_Ring_Light);
    INJECT(enable, 0x004251B0, Inv_Ring_CalcAdders);
    INJECT(enable, 0x004251E0, Inv_Ring_DoMotions);
    INJECT(enable, 0x00425320, Inv_Ring_RotateLeft);
    INJECT(enable, 0x00425350, Inv_Ring_RotateRight);
    INJECT(enable, 0x00425380, Inv_Ring_MotionInit);
    INJECT(enable, 0x004253F0, Inv_Ring_MotionSetup);
    INJECT(enable, 0x00425420, Inv_Ring_MotionRadius);
    INJECT(enable, 0x00425450, Inv_Ring_MotionRotation);
    INJECT(enable, 0x00425480, Inv_Ring_MotionCameraPos);
    INJECT(enable, 0x004254B0, Inv_Ring_MotionCameraPitch);
    INJECT(enable, 0x004254D0, Inv_Ring_MotionItemSelect);
    INJECT(enable, 0x00425530, Inv_Ring_MotionItemDeselect);
}

static void M_Lara_Control(const bool enable)
{
    INJECT(enable, 0x00427580, Lara_HandleAboveWater);
    INJECT(enable, 0x00431670, Lara_HandleSurface);
    INJECT(enable, 0x00431F50, Lara_HandleUnderwater);
    INJECT(enable, 0x004302E0, Lara_Control);
    INJECT(enable, 0x00430EF0, Lara_ControlExtra);
    INJECT(enable, 0x00430970, Lara_Animate);
    INJECT(enable, 0x00430C70, Lara_UseItem);
    INJECT(enable, 0x00430E30, Lara_Cheat_GetStuff);
    INJECT(enable, 0x00430F10, Lara_InitialiseLoad);
    INJECT(enable, 0x00430F40, Lara_Initialise);
    INJECT(enable, 0x00431200, Lara_InitialiseInventory);
    INJECT(enable, 0x00431570, Lara_InitialiseMeshes);
}

static void M_Lara_Draw(const bool enable)
{
    INJECT(enable, 0x00419DF0, Lara_Draw);
    INJECT(enable, 0x0041AB20, Lara_Draw_I);
}

static void M_Lara_Look(const bool enable)
{
    INJECT(enable, 0x00427720, Lara_LookUpDown);
    INJECT(enable, 0x00427790, Lara_LookLeftRight);
    INJECT(enable, 0x00427810, Lara_ResetLook);
}

static void M_Lara_Misc(const bool enable)
{
    INJECT(enable, 0x0042A0A0, Lara_GetCollisionInfo);
    INJECT(enable, 0x0042A0E0, Lara_SlideSlope);
    INJECT(enable, 0x0042A1D0, Lara_HitCeiling);
    INJECT(enable, 0x0042A240, Lara_DeflectEdge);
    INJECT(enable, 0x0042A2C0, Lara_DeflectEdgeJump);
    INJECT(enable, 0x0042A440, Lara_SlideEdgeJump);
    INJECT(enable, 0x0042A530, Lara_TestWall);
    INJECT(enable, 0x0042A640, Lara_TestHangOnClimbWall);
    INJECT(enable, 0x0042A750, Lara_TestClimbStance);
    INJECT(enable, 0x0042A810, Lara_HangTest);
    INJECT(enable, 0x0042AB70, Lara_TestEdgeCatch);
    INJECT(enable, 0x0042AC20, Lara_TestHangJumpUp);
    INJECT(enable, 0x0042AD90, Lara_TestHangJump);
    INJECT(enable, 0x0042AF30, Lara_TestHangSwingIn);
    INJECT(enable, 0x0042AFF0, Lara_TestVault);
    INJECT(enable, 0x0042B2E0, Lara_TestSlide);
    INJECT(enable, 0x0042B410, Lara_FloorFront);
    INJECT(enable, 0x0042B490, Lara_LandedBad);
    INJECT(enable, 0x0042DF70, Lara_CheckForLetGo);
    INJECT(enable, 0x0042B550, Lara_GetJointAbsPosition);
    INJECT(enable, 0x0042B8E0, Lara_GetJointAbsPosition_I);
    INJECT(enable, 0x00413640, Lara_BaddieCollision);
    INJECT(enable, 0x004137E0, Lara_TakeHit);
    INJECT(enable, 0x00413A30, Lara_Push);
    INJECT(enable, 0x00414090, Lara_MovePosition);
    INJECT(enable, 0x0041C4D0, Lara_IsNearItem);
    INJECT(enable, 0x0042E020, Lara_TestClimb);
    INJECT(enable, 0x0042E290, Lara_TestClimbPos);
    INJECT(enable, 0x0042E360, Lara_DoClimbLeftRight);
    INJECT(enable, 0x0042E450, Lara_TestClimbUpPos);
    INJECT(enable, 0x004324A0, Lara_GetWaterDepth);
    INJECT(enable, 0x00432640, Lara_TestWaterDepth);
    INJECT(enable, 0x00432710, Lara_SwimCollision);
    INJECT(enable, 0x00432870, Lara_WaterCurrent);
    INJECT(enable, 0x00442D30, Lara_CatchFire);
    INJECT(enable, 0x00442D80, Lara_TouchLava);
}

static void M_Lara_State(const bool enable)
{
    INJECT(enable, 0x00432180, Lara_SwimTurn);
    INJECT(enable, 0x004278A0, Lara_State_Walk);
    INJECT(enable, 0x00427930, Lara_State_Run);
    INJECT(enable, 0x00427A80, Lara_State_Stop);
    INJECT(enable, 0x00427BD0, Lara_State_ForwardJump);
    INJECT(enable, 0x00427CB0, Lara_State_FastBack);
    INJECT(enable, 0x00427D10, Lara_State_TurnRight);
    INJECT(enable, 0x00427DA0, Lara_State_TurnLeft);
    INJECT(enable, 0x00427E30, Lara_State_Death);
    INJECT(enable, 0x00427E50, Lara_State_FastFall);
    INJECT(enable, 0x00427E90, Lara_State_Hang);
    INJECT(enable, 0x00427EF0, Lara_State_Reach);
    INJECT(enable, 0x00427F10, Lara_State_Splat);
    INJECT(enable, 0x00427F20, Lara_State_Compress);
    INJECT(enable, 0x00428030, Lara_State_Back);
    INJECT(enable, 0x004280C0, Lara_State_Null);
    INJECT(enable, 0x004280D0, Lara_State_FastTurn);
    INJECT(enable, 0x00428120, Lara_State_StepRight);
    INJECT(enable, 0x004281A0, Lara_State_StepLeft);
    INJECT(enable, 0x00428220, Lara_State_Slide);
    INJECT(enable, 0x00428250, Lara_State_BackJump);
    INJECT(enable, 0x004282A0, Lara_State_RightJump);
    INJECT(enable, 0x004282E0, Lara_State_LeftJump);
    INJECT(enable, 0x00428320, Lara_State_UpJump);
    INJECT(enable, 0x00428340, Lara_State_Fallback);
    INJECT(enable, 0x00428370, Lara_State_HangLeft);
    INJECT(enable, 0x004283B0, Lara_State_HangRight);
    INJECT(enable, 0x004283F0, Lara_State_SlideBack);
    INJECT(enable, 0x00428410, Lara_State_PushBlock);
    INJECT(enable, 0x00428440, Lara_State_PPReady);
    INJECT(enable, 0x00428470, Lara_State_Pickup);
    INJECT(enable, 0x004284A0, Lara_State_PickupFlare);
    INJECT(enable, 0x00428500, Lara_State_SwitchOn);
    INJECT(enable, 0x00428540, Lara_State_UseKey);
    INJECT(enable, 0x00428570, Lara_State_Special);
    INJECT(enable, 0x00428590, Lara_State_SwanDive);
    INJECT(enable, 0x004285C0, Lara_State_FastDive);
    INJECT(enable, 0x00428620, Lara_State_WaterOut);
    INJECT(enable, 0x00428640, Lara_State_Wade);
    INJECT(enable, 0x00428710, Lara_State_DeathSlide);
    INJECT(enable, 0x004287B0, Lara_State_Extra_Breath);
    INJECT(enable, 0x00428800, Lara_State_Extra_YetiKill);
    INJECT(enable, 0x00428850, Lara_State_Extra_SharkKill);
    INJECT(enable, 0x004288F0, Lara_State_Extra_Airlock);
    INJECT(enable, 0x00428910, Lara_State_Extra_GongBong);
    INJECT(enable, 0x00428930, Lara_State_Extra_DinoKill);
    INJECT(enable, 0x00428990, Lara_State_Extra_PullDagger);
    INJECT(enable, 0x00428A50, Lara_State_Extra_StartAnim);
    INJECT(enable, 0x00428AA0, Lara_State_Extra_StartHouse);
    INJECT(enable, 0x00428B50, Lara_State_Extra_FinalAnim);
    INJECT(enable, 0x0042D850, Lara_State_ClimbLeft);
    INJECT(enable, 0x0042D890, Lara_State_ClimbRight);
    INJECT(enable, 0x0042D8D0, Lara_State_ClimbStance);
    INJECT(enable, 0x0042D950, Lara_State_Climbing);
    INJECT(enable, 0x0042D970, Lara_State_ClimbEnd);
    INJECT(enable, 0x0042D990, Lara_State_ClimbDown);
    INJECT(enable, 0x004317D0, Lara_State_SurfSwim);
    INJECT(enable, 0x00431840, Lara_State_SurfBack);
    INJECT(enable, 0x004318A0, Lara_State_SurfLeft);
    INJECT(enable, 0x00431900, Lara_State_SurfRight);
    INJECT(enable, 0x00431960, Lara_State_SurfTread);
    INJECT(enable, 0x00432210, Lara_State_Swim);
    INJECT(enable, 0x00432280, Lara_State_Glide);
    INJECT(enable, 0x00432300, Lara_State_Tread);
    INJECT(enable, 0x00432390, Lara_State_Dive);
    INJECT(enable, 0x004323B0, Lara_State_UWDeath);
    INJECT(enable, 0x00432410, Lara_State_UWTwist);
}

static void M_Lara_Col(const bool enable)
{
    INJECT(enable, 0x00428C00, Lara_Fallen);
    INJECT(enable, 0x00428C60, Lara_CollideStop);
    INJECT(enable, 0x00431B40, Lara_SurfaceCollision);
    INJECT(enable, 0x00431C40, Lara_TestWaterStepOut);
    INJECT(enable, 0x00431D30, Lara_TestWaterClimbOut);

    INJECT(enable, 0x00428D20, Lara_Col_Walk);
    INJECT(enable, 0x00428EC0, Lara_Col_Run);
    INJECT(enable, 0x00429040, Lara_Col_Stop);
    INJECT(enable, 0x004290D0, Lara_Col_ForwardJump);
    INJECT(enable, 0x004291B0, Lara_Col_FastBack);
    INJECT(enable, 0x00429270, Lara_Col_TurnRight);
    INJECT(enable, 0x00429310, Lara_Col_TurnLeft);
    INJECT(enable, 0x00429330, Lara_Col_Death);
    INJECT(enable, 0x004293A0, Lara_Col_FastFall);
    INJECT(enable, 0x00429440, Lara_Col_Hang);
    INJECT(enable, 0x00429570, Lara_Col_Reach);
    INJECT(enable, 0x00429600, Lara_Col_Splat);
    INJECT(enable, 0x00429660, Lara_Col_Land);
    INJECT(enable, 0x00429680, Lara_Col_Compress);
    INJECT(enable, 0x00429720, Lara_Col_Back);
    INJECT(enable, 0x00429820, Lara_Col_StepRight);
    INJECT(enable, 0x004298E0, Lara_Col_StepLeft);
    INJECT(enable, 0x00429900, Lara_Col_Slide);
    INJECT(enable, 0x00429920, Lara_Col_BackJump);
    INJECT(enable, 0x00429950, Lara_Col_RightJump);
    INJECT(enable, 0x00429980, Lara_Col_LeftJump);
    INJECT(enable, 0x004299B0, Lara_Col_UpJump);
    INJECT(enable, 0x00429AD0, Lara_Col_Fallback);
    INJECT(enable, 0x00429B60, Lara_Col_HangLeft);
    INJECT(enable, 0x00429BA0, Lara_Col_HangRight);
    INJECT(enable, 0x00429BE0, Lara_Col_SlideBack);
    INJECT(enable, 0x00429C10, Lara_Col_Null);
    INJECT(enable, 0x00429C30, Lara_Col_Roll);
    INJECT(enable, 0x00429CC0, Lara_Col_Roll2);
    INJECT(enable, 0x00429D80, Lara_Col_SwanDive);
    INJECT(enable, 0x00429DF0, Lara_Col_FastDive);
    INJECT(enable, 0x00429E70, Lara_Col_Wade);
    INJECT(enable, 0x00429FE0, Lara_Col_Default);
    INJECT(enable, 0x0042A020, Lara_Col_Jumper);
    INJECT(enable, 0x0042D9B0, Lara_Col_ClimbLeft);
    INJECT(enable, 0x0042DA10, Lara_Col_ClimbRight);
    INJECT(enable, 0x0042DA70, Lara_Col_ClimbStance);
    INJECT(enable, 0x0042DC80, Lara_Col_Climbing);
    INJECT(enable, 0x0042DDD0, Lara_Col_ClimbDown);
    INJECT(enable, 0x00431A50, Lara_Col_SurfSwim);
    INJECT(enable, 0x00431A90, Lara_Col_SurfBack);
    INJECT(enable, 0x00431AC0, Lara_Col_SurfLeft);
    INJECT(enable, 0x00431AF0, Lara_Col_SurfRight);
    INJECT(enable, 0x00431B20, Lara_Col_SurfTread);
    INJECT(enable, 0x00432420, Lara_Col_Swim);
    INJECT(enable, 0x00432440, Lara_Col_UWDeath);
}

static void M_Gun(bool enable)
{
    INJECT(enable, 0x0042BC00, Gun_Rifle_DrawMeshes);
    INJECT(enable, 0x0042BC40, Gun_Rifle_UndrawMeshes);
    INJECT(enable, 0x0042BC70, Gun_Rifle_Ready);
    INJECT(enable, 0x0042BCE0, Gun_Rifle_Control);
    INJECT(enable, 0x0042BDE0, Gun_Rifle_FireShotgun);
    INJECT(enable, 0x0042BEE0, Gun_Rifle_FireM16);
    INJECT(enable, 0x0042BF60, Gun_Rifle_FireHarpoon);
    INJECT(enable, 0x0042C440, Gun_Rifle_FireGrenade);
    INJECT(enable, 0x0042C930, Gun_Rifle_Draw);
    INJECT(enable, 0x0042CAA0, Gun_Rifle_Undraw);
    INJECT(enable, 0x0042CBB0, Gun_Rifle_Animate);
    INJECT(enable, 0x0042CF60, Gun_Pistols_SetArmInfo);
    INJECT(enable, 0x0042CFB0, Gun_Pistols_Draw);
    INJECT(enable, 0x0042D030, Gun_Pistols_Undraw);
    INJECT(enable, 0x0042D260, Gun_Pistols_Ready);
    INJECT(enable, 0x0042D2C0, Gun_Pistols_DrawMeshes);
    INJECT(enable, 0x0042D310, Gun_Pistols_UndrawMeshLeft);
    INJECT(enable, 0x0042D350, Gun_Pistols_UndrawMeshRight);
    INJECT(enable, 0x0042D390, Gun_Pistols_Control);
    INJECT(enable, 0x0042D520, Gun_Pistols_Animate);
    INJECT(enable, 0x0042E6A0, Gun_Control);
    INJECT(enable, 0x0042EC10, Gun_CheckForHoldingState);
    INJECT(enable, 0x0042EC50, Gun_InitialiseNewWeapon);
    INJECT(enable, 0x0042ED90, Gun_TargetInfo);
    INJECT(enable, 0x0042EF30, Gun_GetNewTarget);
    INJECT(enable, 0x0042F150, Gun_FindTargetPoint);
    INJECT(enable, 0x0042F200, Gun_AimWeapon);
    INJECT(enable, 0x0042F2D0, Gun_FireWeapon);
    INJECT(enable, 0x0042F640, Gun_HitTarget);
    INJECT(enable, 0x0042F6E0, Gun_SmashItem);
    INJECT(enable, 0x0042F740, Gun_GetWeaponAnim);
}

static void M_Creature(const bool enable)
{
    INJECT(enable, 0x0040E1B0, Creature_Initialise);
    INJECT(enable, 0x0040E1E0, Creature_Activate);
    INJECT(enable, 0x0040E230, Creature_AIInfo);
    INJECT(enable, 0x0040EA00, Creature_Mood);
    INJECT(enable, 0x0040F2D0, Creature_CheckBaddieOverlap);
    INJECT(enable, 0x0040F460, Creature_Die);
    INJECT(enable, 0x0040F520, Creature_Animate);
    INJECT(enable, 0x0040FDF0, Creature_Turn);
    INJECT(enable, 0x0040FED0, Creature_Tilt);
    INJECT(enable, 0x0040FF10, Creature_Head);
    INJECT(enable, 0x0040FF60, Creature_Neck);
    INJECT(enable, 0x0040FFB0, Creature_Float);
    INJECT(enable, 0x00410060, Creature_Underwater);
    INJECT(enable, 0x004100B0, Creature_Effect);
    INJECT(enable, 0x00410110, Creature_Vault);
    INJECT(enable, 0x00410250, Creature_Kill);
    INJECT(enable, 0x004103C0, Creature_GetBaddieTarget);
    INJECT(enable, 0x00413860, Creature_Collision);
}

static void M_Box(const bool enable)
{
    INJECT(enable, 0x0040E490, Box_SearchLOT);
    INJECT(enable, 0x0040E690, Box_UpdateLOT);
    INJECT(enable, 0x0040E700, Box_TargetBox);
    INJECT(enable, 0x0040E7A0, Box_StalkBox);
    INJECT(enable, 0x0040E8A0, Box_EscapeBox);
    INJECT(enable, 0x0040E950, Box_ValidBox);
    INJECT(enable, 0x0040EE70, Box_CalculateTarget);
    INJECT(enable, 0x0040F3D0, Box_BadFloor);
}

static void M_Lot(const bool enable)
{
    INJECT(enable, 0x00432A60, LOT_InitialiseArray);
    INJECT(enable, 0x00432AC0, LOT_DisableBaddieAI);
    INJECT(enable, 0x00432B10, LOT_EnableBaddieAI);
    INJECT(enable, 0x00432CC0, LOT_InitialiseSlot);
    INJECT(enable, 0x00432ED0, LOT_CreateZone);
    INJECT(enable, 0x00432F90, LOT_ClearLOT);
}

static void M_Objects(const bool enable)
{
    INJECT(enable, 0x0040C880, Bird_Initialise);
    INJECT(enable, 0x0040C910, Bird_Control);
    INJECT(enable, 0x0040CB30, Boat_Initialise);
    INJECT(enable, 0x0040CB70, Boat_CheckGeton);
    INJECT(enable, 0x0040CCE0, Boat_Collision);
    INJECT(enable, 0x0040CE40, Boat_TestWaterHeight);
    INJECT(enable, 0x0040CF40, Boat_DoShift);
    INJECT(enable, 0x0040D110, Boat_DoWakeEffect);
    INJECT(enable, 0x0040D290, Boat_DoDynamics);
    INJECT(enable, 0x0040D2E0, Boat_Dynamics);
    INJECT(enable, 0x0040D7C0, Boat_UserControl);
    INJECT(enable, 0x0040D950, Boat_Animation);
    INJECT(enable, 0x0040DAC0, Boat_Control);
    INJECT(enable, 0x0040E0F0, Gondola_Control);
    INJECT(enable, 0x004138E0, Object_Collision);
    INJECT(enable, 0x00413940, Door_Collision);
    INJECT(enable, 0x004139C0, Object_Collision_Trap);
    INJECT(enable, 0x00416DB0, Diver_Control);
    INJECT(enable, 0x004336F0, BodyPart_Control);
    INJECT(enable, 0x00434400, FinalLevelCounter_Control);
    INJECT(enable, 0x00442B30, FlameEmitter_Control);
    INJECT(enable, 0x00442BC0, Flame_Control);
    INJECT(enable, 0x00442E70, EmberEmitter_Control);
    INJECT(enable, 0x00442F40, Ember_Control);
}

static void M_S_Audio_Sample(const bool enable)
{
    INJECT(enable, 0x00447BC0, S_Audio_Sample_GetAdapter);
    INJECT(enable, 0x00447C10, S_Audio_Sample_CloseAllTracks);
    INJECT(enable, 0x00447C40, S_Audio_Sample_Load);
    INJECT(enable, 0x00447D50, S_Audio_Sample_IsTrackPlaying);
    INJECT(enable, 0x00447DA0, S_Audio_Sample_Play);
    INJECT(enable, 0x00447E90, S_Audio_Sample_GetFreeTrackIndex);
    INJECT(enable, 0x00447ED0, S_Audio_Sample_AdjustTrackVolumeAndPan);
    INJECT(enable, 0x00447F00, S_Audio_Sample_AdjustTrackPitch);
    INJECT(enable, 0x00447F40, S_Audio_Sample_CloseTrack);
    INJECT(enable, 0x00447FB0, S_Audio_Sample_Init);
    INJECT(enable, 0x00448050, S_Audio_Sample_DSoundEnumerate);
    INJECT(enable, 0x00448070, S_Audio_Sample_DSoundEnumCallback);
    INJECT(enable, 0x00448160, S_Audio_Sample_Init2);
    INJECT(enable, 0x004482E0, S_Audio_Sample_DSoundCreate);
    INJECT(enable, 0x00448300, S_Audio_Sample_DSoundBufferTest);
    INJECT(enable, 0x004483D0, S_Audio_Sample_Shutdown);
    INJECT(enable, 0x00448400, S_Audio_Sample_IsEnabled);
    INJECT(enable, 0x00455220, S_Audio_Sample_OutPlay);
    INJECT(enable, 0x00455270, S_Audio_Sample_CalculateSampleVolume);
    INJECT(enable, 0x004552A0, S_Audio_Sample_CalculateSamplePan);
    INJECT(enable, 0x004552D0, S_Audio_Sample_OutPlayLooped);
    INJECT(enable, 0x00455320, S_Audio_Sample_OutSetPanAndVolume);
    INJECT(enable, 0x00455360, S_Audio_Sample_OutSetPitch);
    INJECT(enable, 0x00455390, S_Audio_Sample_OutCloseTrack);
    INJECT(enable, 0x004553B0, S_Audio_Sample_OutCloseAllTracks);
    INJECT(enable, 0x004553C0, S_Audio_Sample_OutIsTrackPlaying);
}

static void M_S_Input(const bool enable)
{
    INJECT(enable, 0x0044D8F0, S_Input_Key);
}

static void M_S_FlaggedString(const bool enable)
{
    INJECT(enable, 0x00445F00, S_FlaggedString_Delete);
    INJECT(enable, 0x00446100, S_FlaggedString_InitAdapter);
    INJECT(enable, 0x00447550, S_FlaggedString_Create);
}

void Inject_Exec(void)
{
    M_DecompGeneral(true);
    M_DecompStats(true);
    M_DecompEffects(true);
    M_HWR(true);
    M_Background(true);

    M_Camera(true);
    M_Collide(true);
    M_Game(true);
    M_Room(true);
    M_Math(true);
    M_Matrix(true);
    M_Shell(true);
    M_Requester(true);
    M_Option(true);
    M_Text(true);
    M_Input(true);
    M_Output(true);
    M_Music(true);
    M_Sound(true);

    M_Demo(true);
    M_Gameflow(true);
    M_Overlay(true);
    M_Random(true);
    M_Items(true);
    M_Effects(true);
    M_LOS(true);
    M_People(true);
    M_Level(true);

    M_Inventory(true);

    M_Lara_Control(true);
    M_Lara_Draw(true);
    M_Lara_Look(true);
    M_Lara_Misc(true);
    M_Lara_State(true);
    M_Lara_Col(true);
    M_Gun(true);

    M_Creature(true);
    M_Box(true);
    M_Lot(true);
    M_Objects(true);

    M_S_Audio_Sample(true);
    M_S_Input(true);
    M_S_FlaggedString(true);
}
