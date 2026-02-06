# GDTSurvivor
An isometric spaceship shooter. Shoot down enemies, dodge asteroids, collect resources and upgrade yourself!

Developed by Martin Weier, Leon Adam and Lam Trinh

Developed with Unreal Engine 5.5

Version:  5.5.4-40574608+++UE5+Release-5.5

Plattform: Windows 11


Tools:
- Git https://git-scm.com/downloads
- Git Large File Storage https://git-lfs.com/
- For developing with IDE use Rider https://www.jetbrains.com/de-de/rider/ 

## Project Structure
 
 ```
 GDTSurvivor/
 ├── Source/                              # C++ source code
 │   └── GDTSurvivor/
 │       ├── PlayerSpaceShipPawn.h/.cpp   # Player spaceship (controls, weapons, network)
 │       ├── Utils/
 │       │   └── MineBehaviour.h/.cpp     # Mine AI behaviour
 │       └── GDTSurvivor.Build.cs         # Build configuration
 │
 ├── Content/GDTSurvivor/                 # Unreal assets
 │   ├── Maps/                            # Levels & menu maps
 │   │   ├── Main_Menu.umap
 │   │   ├── Level_Singleplayer1/2/3.umap
 │   │   └── FinishGameHighscoreBoard_Menu.umap
 │   │
 │   ├── Core/                            # Gameplay logic (Blueprints)
 │   │   ├── GameLogic/                   # GameMode, GameState, Objectives
 │   │   ├── Enemy/                       # Enemies (Turret, Mine, SpaceShip)
 │   │   ├── AI/                          # Behavior Trees (Patrol, Chase, Fire)
 │   │   ├── Projectile/                  # Projectiles (Standard, Homing, Turret)
 │   │   ├── Asteroids/                   # Asteroids & Spawner
 │   │   ├── Minerals/                    # Minerals & Resources
 │   │   ├── Spaceship/                   # Player spaceship Blueprints
 │   │   ├── Collectables/                # Collectible items (HealthPack)
 │   │   ├── Utility/                     # Components (Health, Shield, Score)
 │   │   └── Menu/                        # UI-System
 │   │       ├── MainMenu/
 │   │       ├── HUD/
 │   │       ├── PauseMenu/
 │   │       ├── Settings/
 │   │       ├── LevelSelection/
 │   │       ├── Victory/ & Lose/
 │   │       ├── HighscoreBoard/
 │   │       └── FinishGame/
 │   │
 │   ├── Meshes/                          # 3D models
 │   ├── Materials/                       # Materials
 │   ├── Effects/                         # Particle effects (Niagara)
 │   ├── Audio/                           # Music & sound effects
 │   ├── Skyboxes/                        # Background skyboxes
 │   ├── UI/                              # UI assets (Fonts, Icons, Cursor)
 │   ├── Controller/                      # Input mappings
 │   ├── GameInstances/                   # Global GameInstance
 │   └── SaveGame/                        # Save system (State, Highscore, Settings)
 │
 ├── Config/                              # Engine configuration
 ├── Binaries/                            # Compiled binaries
 └── GDTSurvivor.uproject                 # Unreal project file
 ```
