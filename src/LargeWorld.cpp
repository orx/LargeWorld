/**
 * @file LargeWorld.cpp
 * @date 9-Sep-2020
 */

#include "orx.h"

orxOBJECT **Grid;
orxVECTOR   PreviousPosition;
orxCAMERA  *Camera;
orxS32      CellCount;

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
                                 orxConfig_GetVector("CameraSpeed", &CameraSpeed)),
                   _pstClockInfo->fDT);
    orxCamera_SetPosition(Camera, orxVector_Add(&CameraPos,
                                                orxCamera_GetPosition(Camera, &CameraPos),
                                                &CameraMove));

    // Get grid position
    orxVECTOR   GridPos;
    orxFLOAT    CellSize = orxConfig_GetFloat("CellSize");
    orxVector_Round(&GridPos, orxVector_Divf(&GridPos, &CameraPos, CellSize));

    // Create/Enable neighbor cells
    for(orxS32 i = -1; i <= 1; i++)
    {
        for(orxS32 j = -1; j <= 1; j++)
        {
            orxS32 x = orxF2S(GridPos.fX) + i, y = orxF2S(GridPos.fY) + j;
            if((x >= 0) && (x < CellCount) && (y >= 0) && (y < CellCount))
            {
                orxOBJECT *Cell = Grid[x + CellCount * y];
                if(Cell == orxNULL)
                {
                    // Create new node
                    orxVECTOR Pos;
                    Grid[x + CellCount * y] = Cell = orxObject_CreateFromConfig("Cell");
                    orxObject_SetPosition(Cell, orxVector_Set(&Pos, CellSize * orxS2F(x), CellSize * orxS2F(y), orxFLOAT_0));
                }
                else
                {
                    // Wasn't node enabled?
                    if(!orxObject_IsEnabled(Cell))
                    {
                        // Re-activate it
                        orxObject_EnableRecursive(Cell, orxTRUE);
                        orxObject_SetGroupIDRecursive(Cell, orxString_GetID("default"));
                    }
                }
            }
        }
    }

    //! TODO: Go over "old" neighbor cells and deactivate the ones that don't overlap (by looking around current gridpos)

    // Should quit?
    if(orxInput_IsActive("Quit"))
    {
        // Send close event
        orxEvent_SendShort(orxEVENT_TYPE_SYSTEM, orxSYSTEM_EVENT_CLOSE);
    }
}

/** Init function, it is called when all orx's modules have been initialized
 */
orxSTATUS orxFASTCALL Init()
{
    // Create the viewport
    orxViewport_CreateFromConfig("MainViewport");

    // Sets default config section to "World"
    orxConfig_PushSection("World");

    // Inits grid
    CellCount = orxConfig_GetS32("CellCount");
    Grid = (orxOBJECT **)orxMemory_Allocate(CellCount * CellCount * sizeof(orxOBJECT **), orxMEMORY_TYPE_MAIN);
    orxMemory_Zero(Grid, CellCount * CellCount * sizeof(orxOBJECT **));

    // Inits the camera
    Camera = orxCamera_Get("MainCamera");
    orxCamera_GetPosition(Camera, &PreviousPosition);

    // Create the scene
    orxObject_CreateFromConfig("Scene");

    // Register the Update function to the core clock
    orxClock_Register(orxClock_Get(orxCLOCK_KZ_CORE), Update, orxNULL, orxMODULE_ID_MAIN, orxCLOCK_PRIORITY_NORMAL);

    //! TODO: Register even listener to set cell as owner of spawned objects

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
