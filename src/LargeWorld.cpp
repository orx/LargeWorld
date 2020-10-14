/**
 * @file LargeWorld.cpp
 * @date 9-Sep-2020
 */

#include "orx.h"

orxVECTOR     PreviousCameraPos;
orxOBJECT    *Camera;
orxHASHTABLE *WorldTable;

/** Update function, it has been registered to be called every tick of the core clock
 */
void orxFASTCALL Update(const orxCLOCK_INFO *_pstClockInfo, void *_pContext)
{
    // Update camera
    orxVECTOR CameraMove, CameraSpeed, CameraPos;
    orxVector_Mulf(&CameraMove,
                   orxVector_Mul(&CameraMove,
                                 orxVector_Set(&CameraMove,
                                               orxInput_GetValue("Right") - orxInput_GetValue("Left"),
                                               orxInput_GetValue("Down") - orxInput_GetValue("Up"),
                                               orxFLOAT_0),
                                 orxConfig_GetListVector("CameraSpeed", orxInput_IsActive("Fast") ? 1 : 0, &CameraSpeed)),
                   _pstClockInfo->fDT);
    orxObject_SetPosition(Camera, orxVector_Add(&CameraPos,
                                                orxObject_GetPosition(Camera, &CameraPos),
                                                orxVector_Round(&CameraMove, &CameraMove)));

    // Get grid positions
    orxVECTOR GridPos, PreviousGridPos;
    orxFLOAT  CellSize = orxConfig_GetFloat("CellSize");
    orxVector_Round(&GridPos, orxVector_Divf(&GridPos, &CameraPos, CellSize));
    orxVector_Round(&PreviousGridPos, orxVector_Divf(&PreviousGridPos, &PreviousCameraPos, CellSize));

    // For all neighboring cells
    for(orxS32 i = -1; i <= 1; i++)
    {
        for(orxS32 j = -1; j <= 1; j++)
        {
            // Create/Enable new neighbor cells
            orxS32 x        = orxF2S(GridPos.fX) + i, y = orxF2S(GridPos.fY) + j;
            orxU64 CellID   = ((orxU64)x << 32) | (orxU32)y;
            orxOBJECT *Cell = (orxOBJECT *)orxHashTable_Get(WorldTable, CellID);
            if(!Cell)
            {
                // Create new node
                orxVECTOR Pos;
                Cell = orxObject_CreateFromConfig("Cell");
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
            x       = orxF2S(PreviousGridPos.fX) + i, y = orxF2S(PreviousGridPos.fY) + j;
            CellID  = ((orxU64)x << 32) | (orxU32)y;
            if((x < orxF2S(GridPos.fX) - 1) || (x > orxF2S(GridPos.fX) + 1) || (y < orxF2S(GridPos.fY) - 1) || (y > orxF2S(GridPos.fY) + 1))
            {
                Cell = (orxOBJECT *)orxHashTable_Get(WorldTable, CellID);
                orxASSERT(Cell);
                orxASSERT(orxObject_IsEnabled(Cell));

                // De-activate it
                orxObject_EnableRecursive(Cell, orxFALSE);
            }
        }
    }

    // Update previous  camera position
    orxVector_Copy(&PreviousCameraPos, &CameraPos);

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

    // Create the scene
    orxObject_CreateFromConfig("Scene");

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
