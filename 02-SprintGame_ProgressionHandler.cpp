// Fill out your copyright notice in the Description page of Project Settings.
#include "ProgressionHandler.h"

// Sets default values
AProgressionHandler::AProgressionHandler()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AProgressionHandler::BeginPlay()
{
	Super::BeginPlay();
	AProgressionHandler::difficulty = 0; // Set starting difficulty

	// Did the data table successfully load?
	if (dataTableAsset)
	{
		// Data table loaded; set levelPoolDataTable based on loaded asset
		levelPoolDataTable.DataTable = dataTableAsset;
	}
	else
	{
		// Data table did NOT load; display error message
		GEngine->AddOnScreenDebugMessage(-1, 999.f, FColor::Red, TEXT("ERROR: Failed to load data table from filepath"));
	}
}

// Called every frame
void AProgressionHandler::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}



//////////////////////////
// Function Definitions //
//////////////////////////

// Progression function; called in editor on overlap
void AProgressionHandler::chamberProgression()
{
	if (!(allowProgression))
	{
		// Progression blocked; return
		return;
	}


	// Block progression until next level is loaded
	allowProgression = false;

	// Is chamberIndex an intermission level?
	if (chamberIndex == 25 || chamberIndex == 50 || chamberIndex == 75)
	{
		// Is the player already in an intermission?
		if (intermission)
		{
			// Already in intermission; return to chambers
			chamberIndex++;
			loadChamber();
		}
		else
		{
			// Not in intermission; load intermission level
			loadIntermission();
		}
	}
	else
	{
		// chamberIndex is NOT an intermission index
		if (chamberIndex != 99)
		{
			// Increment chamberIndex and load next level
			chamberIndex++;
			loadChamber();
		}
		else
		{
			// Load ending
			ending = true;

			FSoftObjectPath endingPath(TEXT("/Game/Levels/Chamber_Pool/lvl_FinalChamber.lvl_FinalChamber"));
			TSoftObjectPtr<UWorld> levelToLoad(endingPath);

			loadLevel(levelToLoad);
		}
	}

	return;
}

// Select a random chamber from the current level pool, then load it
void AProgressionHandler::loadChamber()
{
	TSoftObjectPtr<UWorld> levelToLoad;
	int selectedLevelIndex;

	ending = false;
	intermission = false;


	// Refresh level pool if it's empty
	if (levelPool.IsEmpty())
	{
		refreshLevelPool();
	}


	// Select random level to load
	selectedLevelIndex = UKismetMathLibrary::RandomIntegerInRange(0, (levelPool.Num()) - 1);


	// Safety check to avoid crashes: is selected index valid?
	if (levelPool.IsValidIndex(selectedLevelIndex))
	{
		// Valid index; set selected level
		levelToLoad = levelPool[selectedLevelIndex];
	}
	else
	{
		allowProgression = true;
		return;
	}


	// Load selected level
	loadLevel(levelToLoad);
	return;
}

// Select an intermission level based on chamberIndex, then call loadLevel() with selected level
void AProgressionHandler::loadIntermission()
{
	TSoftObjectPtr<UWorld> levelToLoad;
	intermission = true;


	switch (chamberIndex)
	{
	case 25:
		difficulty++; // Increment difficulty
		levelToLoad = intermissionLevel1;
		break;

	case 50:
		difficulty++; // Increment difficulty
		levelToLoad = intermissionLevel2;
		break;

	case 75:
		difficulty++; // Increment difficulty
		levelToLoad = intermissionLevel3;
		break;

	default:
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to decide intermission to load"));
		levelToLoad = intermissionLevel1;
		difficulty = 0;
		break;
	}


	// Refresh level pool and load intermission level
	refreshLevelPool();
	loadLevel(levelToLoad);
	return;
}

// Refresh level pool based on data table
void AProgressionHandler::refreshLevelPool()
{
	// Is levelPoolDataTable pointing to an asset?
	if (!(levelPoolDataTable.IsNull()))
	{
		// Valid data table; refresh pool
		switch (difficulty)
		{
		case 0:
			levelPoolDataTable.RowName = "01_Easy";
			levelPool = levelPoolDataTable.GetRow<FLevelPoolData>("01_Easy")->level;
			break;

		case 1:
			levelPoolDataTable.RowName = "02_Medium";
			levelPool = levelPoolDataTable.GetRow<FLevelPoolData>("02_Medium")->level;
			break;

		case 2:
			levelPoolDataTable.RowName = "03_Hard";
			levelPool = levelPoolDataTable.GetRow<FLevelPoolData>("03_Hard")->level;
			break;

		case 3:
			levelPoolDataTable.RowName = "04_Expert";
			levelPool = levelPoolDataTable.GetRow<FLevelPoolData>("04_Expert")->level;
			break;

		default:
			levelPoolDataTable.RowName = "00_Test";
			levelPool = levelPoolDataTable.GetRow<FLevelPoolData>("00_Test")->level;
			break;
		}
	}
	else
	{
		// No valid data table; display error and return
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ERROR: Unable to load level pool; no data table asset found"));
		allowProgression = true;
		return;
	}


	// Remove current level from pool
	if (levelPool.Contains(currentLevel))
	{
		levelPool.RemoveSingle(currentLevel);
	}


	return;
}

// Loads a given chamber via TSoftObjectPtr<UWorld>
void AProgressionHandler::loadLevel(TSoftObjectPtr<UWorld> levelToLoad)
{

	// Remove level from pool
	{
		currentLevel = levelToLoad;


		// Check if levelPool contains selected level, and remove it if it does
		if (levelPool.Contains(levelToLoad))
		{
			levelPool.RemoveSingle(levelToLoad);
		}
	}


	// Load level at actor transform
	bool levelStreamSuccess;
	ULevelStreamingDynamic* streamedLevel = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr
	(
		this,
		currentLevel,
		GetActorTransform(),
		levelStreamSuccess,
		FString(currentLevel.GetAssetName()) += FString::FromInt(chamberIndex),
		NULL,
		false
	);


	if (levelStreamSuccess)
	{
		streamedLevel->SetShouldBeVisible(true); // Make level visible
		loadedLevels.Add(streamedLevel); // Add level to loadedLevels array
		allowProgression = true; // Unblock progression
	}
	else
	{
		allowProgression = true; // Unblock progression
	}


	// Unload oldest level if there are more than 2 currently loaded
	if (loadedLevels.Num() > 2)
	{
		// Safety check to help prevent crashes
		if (loadedLevels.IsValidIndex(0))
		{
			loadedLevels[0]->SetIsRequestingUnloadAndRemoval(true); // Unload level
			loadedLevels.RemoveAt(0); // Remove level index from loadedLevels array
			allowProgression = true;
		}
	}

	return;
}