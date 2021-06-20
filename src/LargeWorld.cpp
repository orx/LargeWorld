/**
 * @file LargeWorld.cpp
 * @date 9-Sep-2020
 */

#include "orx.h"

orxVECTOR     PreviousCameraPos;
orxOBJECT    *Camera;
orxHASHTABLE *WorldTable;
orxS32        Settings;

// Ask for dedicated GPU, if present
#ifdef __orxWINDOWS__

extern "C"
{
  _declspec(dllexport) orxU32 NvOptimusEnablement                   = 1;
  _declspec(dllexport) orxU32 AmdPowerXpressRequestHighPerformance  = 1;
}

#endif // __orxWINDOWS__

static orxU64 Universe = 0;

/** Apply settings
 */
void orxFASTCALL ApplySettings()
{
    orxConfig_SetParent("World", orxConfig_GetListString("Settings", Settings));
    orxConfig_GetString("UpdateSettings");
    orxHashTable_Clear(WorldTable);
    orxObject_Delete(orxOBJECT(orxStructure_Get(Universe)));
    Universe = orxStructure_GetGUID(orxObject_CreateFromConfig("Universe"));
}

/** Update function, it has been registered to be called every tick of the core clock
 */
void orxFASTCALL Update(const orxCLOCK_INFO *_pstClockInfo, void *_pContext)
{
    // Update camera
    orxVECTOR CameraMove, CameraSpeed, CameraHighSpeed, CameraPos;
    orxVector_Mulf(&CameraMove,
                   orxVector_Mul(&CameraMove,
                                 orxVector_Set(&CameraMove,
                                               orxInput_GetValue("Right") - orxInput_GetValue("Left"),
                                               orxInput_GetValue("Down") - orxInput_GetValue("Up"),
                                               orxFLOAT_0),
                                 orxVector_Lerp(&CameraSpeed,
                                                orxConfig_GetListVector("CameraSpeed", 0, &CameraSpeed),
                                                orxConfig_GetListVector("CameraSpeed", 1, &CameraHighSpeed),
                                                orxInput_GetValue("Fast"))),
                   _pstClockInfo->fDT);
    orxObject_SetPosition(Camera, orxVector_Add(&CameraPos,
                                                orxObject_GetPosition(Camera, &CameraPos),
                                                orxVector_Round(&CameraMove, &CameraMove)));

    // Change settings
    if (orxInput_HasBeenActivated("CycleUp") || orxInput_HasBeenActivated("CycleDown"))
    {
        orxS32 NewSettings = orxInput_IsActive("CycleUp") ? orxMIN(Settings + 1, orxConfig_GetListCount("Settings") - 1) : orxMAX(Settings - 1, 0);
        if (NewSettings != Settings)
        {
            Settings = NewSettings;
            ApplySettings();
            orxObject_CreateFromConfig("ClearSnap");
            orxVector_Copy(&PreviousCameraPos, &CameraPos);
        }
    }

    // Get cell positions
    orxVECTOR CellPos, PreviousCellPos;
    orxFLOAT  CellSize = orxConfig_GetFloat("CellSize");
    orxVector_Round(&CellPos, orxVector_Divf(&CellPos, &CameraPos, CellSize));
    orxVector_Round(&PreviousCellPos, orxVector_Divf(&PreviousCellPos, &PreviousCameraPos, CellSize));

    // For all neighboring cells
    for(orxS32 i = -1; i <= 1; i++)
    {
        for(orxS32 j = -1; j <= 1; j++)
        {
            // Create/Enable new neighbor cells
            orxS32 x        = orxF2S(CellPos.fX) + i, y = orxF2S(CellPos.fY) + j;
            orxU64 CellID   = ((orxU64)x << 32) | (orxU32)y;
            orxOBJECT *Cell = (orxOBJECT *)orxHashTable_Get(WorldTable, CellID);
            if(!Cell)
            {
                // Create new node
                orxVECTOR Pos;
                Cell = orxObject_CreateFromConfig("Cell");
                orxObject_SetParent(Cell, orxOBJECT(orxStructure_Get(Universe)));
                orxObject_SetOwner(Cell, orxOBJECT(orxStructure_Get(Universe)));
                orxObject_SetPosition(Cell, orxVector_Set(&Pos, CellSize * orxS2F(x), CellSize * orxS2F(y), orxFLOAT_0));
                orxHashTable_Add(WorldTable, CellID, (void *)Cell);
            }
            else
            {
                // Wasn't node enabled?
                if(!orxObject_IsEnabled(Cell))
                {
                    // Re-activate it
                    orxObject_EnableRecursive(Cell, orxTRUE);
                }
            }

            // Disable out-of-range cells
            x       = orxF2S(PreviousCellPos.fX) + i, y = orxF2S(PreviousCellPos.fY) + j;
            CellID  = ((orxU64)x << 32) | (orxU32)y;
            if((x < orxF2S(CellPos.fX) - 1) || (x > orxF2S(CellPos.fX) + 1) || (y < orxF2S(CellPos.fY) - 1) || (y > orxF2S(CellPos.fY) + 1))
            {
                Cell = (orxOBJECT *)orxHashTable_Get(WorldTable, CellID);
                orxASSERT(Cell);
                orxASSERT(orxObject_IsEnabled(Cell));

                // De-activate it
                orxObject_EnableRecursive(Cell, orxFALSE);
            }
        }
    }

    // Update previous camera position
    orxVector_Copy(&PreviousCameraPos, &CameraPos);

    // Zoom Out?
    if(orxInput_HasBeenActivated("Zoom"))
    {
        orxObject_AddTimeLineTrack(Camera, "ZoomOut");
    }
    // Zoom In?
    else if(orxInput_HasBeenDeactivated("Zoom"))
    {
        orxObject_AddTimeLineTrack(Camera, "ZoomIn");
    }

    // Snapshot?
    if(orxInput_HasBeenActivated("Snap"))
    {
        // Create its viewport
        orxViewport_CreateFromConfig("SnapViewport");

        // Store position
        orxConfig_PushSection("Runtime");
        orxConfig_SetVector("SnapPos", &PreviousCameraPos);
        orxConfig_PopSection();

        // Create snapshot
        orxObject_CreateFromConfig("Snapshot");
    }
    else
    {
        // Delete its viewport
        if(orxViewport_Get("SnapViewport"))
        {
            orxViewport_Delete(orxViewport_Get("SnapViewport"));
        }

        // Recall?
        if(orxInput_HasBeenActivated("Recall"))
        {
            orxVECTOR Pos;

            // Restore position
            orxConfig_PushSection("Runtime");
            if(orxConfig_GetVector("SnapPos", &Pos))
            {
                orxObject_AddUniqueFX(Camera, "Recall");
            }
            orxConfig_PopSection();
        }
    }

    // Should quit?
    if(orxInput_IsActive("Quit"))
    {
        // Send close event
        orxEvent_SendShort(orxEVENT_TYPE_SYSTEM, orxSYSTEM_EVENT_CLOSE);
    }
}

static orxSTATUS orxFASTCALL EventHandler(const orxEVENT *_pstEvent)
{
    // Sets cell as spawned object's owner
    orxObject_SetOwner(orxOBJECT(_pstEvent->hRecipient), orxStructure_GetOwner(_pstEvent->hSender));

    return orxSTATUS_SUCCESS;
}

/** Init function, it is called when all orx's modules have been initialized
 */
orxSTATUS orxFASTCALL Init()
{
    // Create the viewport
    orxViewport_CreateFromConfig("MainViewport");

    // Sets default config section to "World"
    orxConfig_PushSection("World");

    // Init our world
    WorldTable = orxHashTable_Create(8192, orxHASHTABLE_KU32_FLAG_NONE, orxMEMORY_TYPE_MAIN);

    // Init the camera
    Camera = orxObject_CreateFromConfig("Camera");
    orxCamera_SetParent(orxCamera_Get("MainCamera"), Camera);
    orxObject_GetPosition(Camera, &PreviousCameraPos);

    // Create universe
    Universe = orxStructure_GetGUID(orxObject_CreateFromConfig("Universe"));

    // Create the scene
    orxObject_CreateFromConfig("Scene");

    // Init settings
    Settings = orxConfig_GetListCount("Settings") / 2;
    ApplySettings();

    // Register the Update function to the core clock
    orxClock_Register(orxClock_Get(orxCLOCK_KZ_CORE), Update, orxNULL, orxMODULE_ID_MAIN, orxCLOCK_PRIORITY_NORMAL);

    // Register event handler to set the cell as owner of spawned objects
    orxEvent_AddHandler(orxEVENT_TYPE_SPAWNER, &EventHandler);
    orxEvent_SetHandlerIDFlags(&EventHandler, orxEVENT_TYPE_SPAWNER, orxNULL, orxEVENT_GET_FLAG(orxSPAWNER_EVENT_SPAWN), orxEVENT_KU32_MASK_ID_ALL);

    // Done!
    return orxSTATUS_SUCCESS;
}

/** Run function, it should not contain any game logic
 */
orxSTATUS orxFASTCALL Run()
{
    // Return orxSTATUS_FAILURE to instruct orx to quit
    return orxSTATUS_SUCCESS;
}

/** Exit function, it is called before exiting from orx
 */
void orxFASTCALL Exit()
{
    // Let Orx clean all our mess automatically. :)
}

/** Bootstrap function, it is called before config is initialized, allowing for early resource storage definitions
 */
orxSTATUS orxFASTCALL Bootstrap()
{
    // Add a config storage to find the initial config file
    orxResource_AddStorage(orxCONFIG_KZ_RESOURCE_GROUP, "../data/config", orxFALSE);

    // Return orxSTATUS_FAILURE to prevent orx from loading the default config file
    return orxSTATUS_SUCCESS;
}

/** Main function
 */
int main(int argc, char **argv)
{
    // Set the bootstrap function to provide at least one resource storage before loading any config files
    orxConfig_SetBootstrap(Bootstrap);

    // Execute our game
    orx_Execute(argc, argv, Init, Run, Exit);

    // Done!
    return EXIT_SUCCESS;
}
