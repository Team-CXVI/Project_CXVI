#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING
#include <HAL/IConsoleManager.h>
#include <AssetRegistry/AssetRegistryModule.h>

#include "Foundation/Scene/CXSceneSubsystem.h"
#include "Foundation/Scene/CXSceneDefinition.h"
#include "Foundation/Scene/CXGameScene.h"
#include "Foundation/Scene/Scenes/CXCombatScene.h"

#include "Level/CXLevelTransitionSubsystem.h"
#include "Level/CXLevelDefinition.h"

namespace CXSceneDebug
{
	/** 에셋 이름(DA_Scene_...) 또는 전체 경로(/Game/...)로 씬 정의 검색 */
	static UCXSceneDefinition* FindSceneDefinition(const FString& InNameOrPath)
	{
		if (InNameOrPath.StartsWith(TEXT("/")))
		{
			return LoadObject<UCXSceneDefinition>(nullptr, *InNameOrPath);
		}

		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByClass(UCXSceneDefinition::StaticClass()->GetClassPathName(), Assets, /*bSearchSubClasses*/ true);

		for (const FAssetData& Asset : Assets)
		{
			if (Asset.AssetName.ToString().Equals(InNameOrPath, ESearchCase::IgnoreCase))
			{
				return Cast<UCXSceneDefinition>(Asset.GetAsset());
			}
		}
		return nullptr;
	}

	static UCXSceneSubsystem* GetSubsystem(UWorld* InWorld)
	{
		UCXSceneSubsystem* Subsystem = InWorld ? InWorld->GetSubsystem<UCXSceneSubsystem>() : nullptr;
		if (!Subsystem)
		{
			UE_LOG(LogCXScene, Warning, TEXT("Scene Subsystem이 없습니다. 게임 맵에서 실행하세요."));
		}
		return Subsystem;
	}

	// CXVI.Scene.Push <SceneDefinition> [EncounterId]
	static FAutoConsoleCommandWithWorldAndArgs PushCommand(
		TEXT("CXVI.Scene.Push"),
		TEXT("씬을 Push합니다. 사용법: CXVI.Scene.Push <씬 정의 이름 또는 경로> [EncounterId]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				if (InArgs.IsEmpty())
				{
					UE_LOG(LogCXScene, Warning, TEXT("사용법: CXVI.Scene.Push <씬 정의 이름 또는 경로> [EncounterId]"));
					return;
				}

				UCXSceneSubsystem* Subsystem = GetSubsystem(InWorld);
				if (!Subsystem)
				{
					return;
				}

				UCXSceneDefinition* Definition = FindSceneDefinition(InArgs[0]);
				if (!Definition)
				{
					UE_LOG(LogCXScene, Warning, TEXT("씬 정의 '%s'를 찾을 수 없습니다."), *InArgs[0]);
					return;
				}

				FCXSceneContext Context;
				if (Definition->SceneClass && Definition->SceneClass->IsChildOf(UCXCombatScene::StaticClass()))
				{
					FCXCombatEnterParams Params;
					Params.EncounterId = InArgs.IsValidIndex(1) ? FName(*InArgs[1]) : FName(TEXT("Debug"));
					Context = FCXSceneContext::Make(Params);
				}

				Subsystem->PushScene(Definition, Context);
			}));

	// CXVI.Scene.Pop [win|lose]
	static FAutoConsoleCommandWithWorldAndArgs PopCommand(
		TEXT("CXVI.Scene.Pop"),
		TEXT("현재 씬을 Pop합니다. 전투 씬이면 결과를 지정할 수 있습니다. 사용법: CXVI.Scene.Pop [win|lose]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				UCXSceneSubsystem* Subsystem = GetSubsystem(InWorld);
				if (!Subsystem)
				{
					return;
				}

				FCXSceneContext Context;
				if (Cast<UCXCombatScene>(Subsystem->GetCurrentScene()))
				{
					FCXCombatResult Result;
					Result.bVictory = !(InArgs.IsValidIndex(0) && InArgs[0].Equals(TEXT("lose"), ESearchCase::IgnoreCase));
					Context = FCXSceneContext::Make(Result);
				}

				Subsystem->PopScene(Context);
			}));

	// CX.Scene.Status
	//static FAutoConsoleCommandWithWorldAndArgs StatusCommand(
	//	TEXT("CX.Scene.Status"),
	//	TEXT("씬 스택과 유지 중인 씬 정의를 출력합니다."),
	//	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
	//		{
	//			if (const UCXSceneSubsystem* Subsystem = GetSubsystem(InWorld))
	//			{
	//				Subsystem->DumpDebugInfo();
	//			}
	//		}));
}

namespace CXLevelDebug
{
	static UCXLevelTransitionSubsystem* GetSubsystem(UWorld* InWorld)
	{
		UCXLevelTransitionSubsystem* Subsystem = UCXLevelTransitionSubsystem::TryGet(InWorld);
		if (nullptr == Subsystem)
		{
			UE_LOG(LogCXLevelTransition, Warning, TEXT("Level Transition Subsystem이 없습니다."));
		}
		return Subsystem;
	}

	/** 에셋 이름(DA_Level_...) 또는 전체 ID(CXLevelDefinition:DA_Level_...)를 PrimaryAssetId로 변환 */
	static FPrimaryAssetId MakeLevelId(const FString& InNameOrId)
	{
		if (InNameOrId.Contains(TEXT(":")))
		{
			return FPrimaryAssetId(InNameOrId);
		}

		// 엔진 기본 규칙 : 타입 = 네이티브 클래스 이름, 이름 = 에셋 이름
		return FPrimaryAssetId(FPrimaryAssetType(UCXLevelDefinition::StaticClass()->GetFName()), FName(*InNameOrId));
	}

	// CXVI.Level.Travel <LevelDefinition> [EntryTag]
	static FAutoConsoleCommandWithWorldAndArgs TravelCommand(
		TEXT("CXVI.Level.Travel"),
		TEXT("인게임 레벨로 이동합니다. 사용법: CXVI.Level.Travel <레벨 정의 이름 또는 ID> [진입 태그]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				if (InArgs.IsEmpty())
				{
					UE_LOG(LogCXLevelTransition, Warning, TEXT("사용법: CXVI.Level.Travel <레벨 정의 이름 또는 ID> [진입 태그]"));
					return;
				}

				UCXLevelTransitionSubsystem* Subsystem = GetSubsystem(InWorld);
				if (nullptr == Subsystem)
				{
					return;
				}

				const FPrimaryAssetId LevelId = MakeLevelId(InArgs[0]);
				const FName EntryTag = InArgs.IsValidIndex(1) ? FName(*InArgs[1]) : NAME_None;

				// 전환 중 / Scene 전환 중 / 타이틀 ID 거부 등의 검사는 TravelToLevel이 그대로 처리
				Subsystem->TravelToLevel(LevelId, EntryTag);
			}));

	// CXVI.Level.Title
	static FAutoConsoleCommandWithWorldAndArgs TitleCommand(
		TEXT("CXVI.Level.Title"),
		TEXT("타이틀 레벨로 이동합니다."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				if (UCXLevelTransitionSubsystem* Subsystem = GetSubsystem(InWorld))
				{
					Subsystem->TravelToTitle();
				}
			}));
}


#endif // !UE_BUILD_SHIPPING