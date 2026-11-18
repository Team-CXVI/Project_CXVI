#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING
#include <HAL/IConsoleManager.h>
#include <AssetRegistry/AssetRegistryModule.h>
#include <EnhancedInputSubsystems.h>

#include "Foundation/Scene/CXSceneSubsystem.h"
#include "Foundation/Scene/CXSceneDefinition.h"
#include "Foundation/Scene/CXGameScene.h"
#include "Foundation/Scene/Scenes/CXCombatScene.h"

#include "Level/CXLevelTransitionSubsystem.h"
#include "Level/CXLevelDefinition.h"

#include "Character/Combat/CXCharacterPartyData.h"
#include "Foundation/Combat/CXCombatTypes.h"
#include "Foundation/Combat/CXCombatManagerSubsystem.h"
#include "Foundation/Combat/CombatMessage/CXCombatMessageData.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"
#include "Subsystems/CXInputControlSubsystem.h"
#include "Tags/CXGameplayTags.h"

#include "Foundation/UserData/CXUserDataSubsystem.h"

#include "UI/CXUIManagerSubsystem.h"

#include <Engine/Engine.h>
#include <Engine/GameViewportClient.h>
#include <Widgets/SWindow.h>
#include <Containers/Ticker.h>

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

namespace CXSceneDebug
{
	/** 에셋 이름 또는 전체 경로(/Game/...)로 T 타입 에셋 검색 */
	template <typename T>
	static T* FindAssetByName(const FString& InNameOrPath)
	{
		if (InNameOrPath.StartsWith(TEXT("/")))
		{
			return LoadObject<T>(nullptr, *InNameOrPath);
		}

		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByClass(T::StaticClass()->GetClassPathName(), Assets, /*bSearchSubClasses*/ true);

		for (const FAssetData& Asset : Assets)
		{
			if (Asset.AssetName.ToString().Equals(InNameOrPath, ESearchCase::IgnoreCase))
			{
				return Cast<T>(Asset.GetAsset());
			}
		}
		return nullptr;
	}

	/** 에셋 이름(DA_Scene_...) 또는 전체 경로(/Game/...)로 씬 정의 검색 */
	static UCXSceneDefinition* FindSceneDefinition(const FString& InNameOrPath)
	{
		return FindAssetByName<UCXSceneDefinition>(InNameOrPath);
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

	// CXVI.Scene.Push <SceneDefinition>
	static FAutoConsoleCommandWithWorldAndArgs PushCommand(
		TEXT("CXVI.Scene.Push"),
		TEXT("씬을 Push합니다. 전투 씬은 CXVI.Combat.Start를 사용하세요. 사용법: CXVI.Scene.Push <씬 정의 이름 또는 경로>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				if (InArgs.IsEmpty())
				{
					UE_LOG(LogCXScene, Warning, TEXT("사용법: CXVI.Scene.Push <씬 정의 이름 또는 경로>"));
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

				// 전투 Scene은 전투 매니저의 진입 상태를 거쳐야 시작되므로 직접 Push하지 않음
				if (Definition->SceneClass && Definition->SceneClass->IsChildOf(UCXCombatScene::StaticClass()))
				{
					UE_LOG(LogCXScene, Warning, TEXT("전투 씬은 CXVI.Combat.Start로 진입하세요."));
					return;
				}

				Subsystem->PushScene(Definition, FCXSceneContext());
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

namespace CXCombatDebug
{
	// CXVI.Combat.Start <EncounterId> [Player]
	static FAutoConsoleCommandWithWorldAndArgs CombatStartCommand(
		TEXT("CXVI.Combat.Start"),
		TEXT("전투 매니저에 전투 진입을 요청합니다. (전투 씬 = 현재 레벨 정의의 CombatScene) 사용법: CXVI.Combat.Start <EncounterId> [Player] (Player = 플레이어 선공)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				if (InArgs.Num() < 1 || InArgs.Num() > 2)
				{
					UE_LOG(LogCXCombat, Warning, TEXT("사용법: CXVI.Combat.Start <EncounterId> [Player]"));
					return;
				}

				UCXCombatManagerSubsystem* CombatManager = UCXCombatManagerSubsystem::TryGet(InWorld);
				if (!CombatManager)
				{
					UE_LOG(LogCXCombat, Warning, TEXT("전투 매니저가 없습니다. 게임 맵에서 실행하세요."));
					return;
				}

				FCXCombatEnterParams Params;
				Params.EncounterId = FName(*InArgs[0]);
				Params.bIsPlayerAttack = InArgs.IsValidIndex(1) && InArgs[1].Equals(TEXT("Player"), ESearchCase::IgnoreCase);
				UE_LOG(LogCXCombat, Log, TEXT("플레이어 선공 조건: %s"), Params.bIsPlayerAttack ? TEXT("true") : TEXT("false"));
				// 거절 사유는 매니저가 로그로 남김
				CombatManager->RequestStartBattle(Params);
			}));

	// CXVI.Combat.End [win|lose]
	static FAutoConsoleCommandWithWorldAndArgs CombatEndCommand(
		TEXT("CXVI.Combat.End"),
		TEXT("전투를 바로 끝냅니다(HP / 경험치 반영 후 결과 화면). 사용법: CXVI.Combat.End [win|lose] (기본 win)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				UCXSceneSubsystem* Subsystem = CXSceneDebug::GetSubsystem(InWorld);
				if (!Subsystem)
				{
					return;
				}

				// 받을 Scene이 없으면 메시지가 버려지므로 미리 확인
				if (nullptr == Cast<UCXCombatScene>(Subsystem->GetCurrentScene()))
				{
					UE_LOG(LogCXScene, Warning, TEXT("CXVI.Combat.End 무시: 현재 Scene이 전투 Scene이 아닙니다."));
					return;
				}

				UCXCombatManagerSubsystem* CombatManager = UCXCombatManagerSubsystem::TryGet(InWorld);
				if (!CombatManager)
				{
					UE_LOG(LogCXCombat, Warning, TEXT("CXVI.Combat.End 무시: 전투 매니저가 없습니다."));
					return;
				}

				// 실제 전투 종료와 같은 경로(매니저 HP / 경험치 반영 -> 메시지 -> CombatScene -> Pop -> 정리)로 진행
				const bool bVictory = !(InArgs.IsValidIndex(0) && InArgs[0].Equals(TEXT("lose"), ESearchCase::IgnoreCase));
				CombatManager->DebugEndBattle(bVictory);
			}));
}

namespace CXInputDebug
{
	// 콘솔에서 받은 핸들 (태그별로 쌓고 나중 것부터 반납)
	// 월드가 바뀌면 서브시스템이 요청을 폐기하므로 남은 핸들은 무효 (반납해도 무시됨)
	static TMap<FGameplayTag, TArray<FCXInputModeHandle>> DebugHandles;

	static UCXInputControlSubsystem* GetSubsystem(UWorld* InWorld)
	{
		UCXInputControlSubsystem* Subsystem = UCXInputControlSubsystem::TryGet(InWorld);
		if (nullptr == Subsystem)
		{
			UE_LOG(LogCXInput, Warning, TEXT("Input Control Subsystem이 없습니다. 게임 맵에서 실행하세요."));
		}
		return Subsystem;
	}

	static FGameplayTag ParseTag(const FString& InTagName)
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*InTagName), /*ErrorIfNotFound*/ false);
		if (!Tag.IsValid())
		{
			UE_LOG(LogCXInput, Warning, TEXT("등록되지 않은 태그입니다: %s"), *InTagName);
		}
		return Tag;
	}

	// CXVI.Input.AddTag <Tag> [Reason]
	static FAutoConsoleCommandWithWorldAndArgs AddTagCommand(
		TEXT("CXVI.Input.AddTag"),
		TEXT("입력 모드에 태그를 추가합니다. 사용법: CXVI.Input.AddTag <태그> [사유] (예: CXVI.Input.AddTag InputMode.Block.All)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				if (InArgs.IsEmpty())
				{
					UE_LOG(LogCXInput, Warning, TEXT("사용법: CXVI.Input.AddTag <태그> [사유]"));
					return;
				}

				UCXInputControlSubsystem* Subsystem = GetSubsystem(InWorld);
				const FGameplayTag Tag = ParseTag(InArgs[0]);
				if (nullptr == Subsystem || !Tag.IsValid())
				{
					return;
				}

				const FName Reason = InArgs.IsValidIndex(1) ? FName(*InArgs[1]) : FName(TEXT("Debug"));

				// 거부 사유(예약 태그, PlayerInput 없음)는 서브시스템이 로그로 남김
				const FCXInputModeHandle Handle = Subsystem->AddInputModeTag(Tag, Reason);
				if (Handle.IsValid())
				{
					DebugHandles.FindOrAdd(Tag).Add(Handle);
				}
			}));

	// CXVI.Input.RemoveTag <Tag>
	static FAutoConsoleCommandWithWorldAndArgs RemoveTagCommand(
		TEXT("CXVI.Input.RemoveTag"),
		TEXT("CXVI.Input.AddTag로 추가한 태그 요청을 하나 반납합니다. 사용법: CXVI.Input.RemoveTag <태그>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				if (InArgs.IsEmpty())
				{
					UE_LOG(LogCXInput, Warning, TEXT("사용법: CXVI.Input.RemoveTag <태그>"));
					return;
				}

				UCXInputControlSubsystem* Subsystem = GetSubsystem(InWorld);
				const FGameplayTag Tag = ParseTag(InArgs[0]);
				if (nullptr == Subsystem || !Tag.IsValid())
				{
					return;
				}

				TArray<FCXInputModeHandle>* Handles = DebugHandles.Find(Tag);
				if (nullptr == Handles || Handles->IsEmpty())
				{
					UE_LOG(LogCXInput, Warning, TEXT("디버그로 추가한 요청이 없습니다: %s"), *Tag.ToString());
					return;
				}

				FCXInputModeHandle Handle = Handles->Pop();
				Subsystem->RemoveInputModeTag(Handle);

				if (Handles->IsEmpty())
				{
					DebugHandles.Remove(Tag);
				}
			}));

	// CXVI.Input.Status
	static FAutoConsoleCommandWithWorldAndArgs StatusCommand(
		TEXT("CXVI.Input.Status"),
		TEXT("현재 Enhanced Input 입력 모드와 디버그로 추가한 태그 요청 수를 출력합니다."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				const ULocalPlayer* LocalPlayer = InWorld ? InWorld->GetFirstLocalPlayerFromController() : nullptr;
				const UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
				if (nullptr == InputSubsystem)
				{
					UE_LOG(LogCXInput, Warning, TEXT("Enhanced Input Subsystem이 없습니다. 게임 맵에서 실행하세요."));
					return;
				}

				// CommonUI(InputMode.Game / Menu)와 시작 입력 모드 태그까지 포함한 실제 입력 모드
				UE_LOG(LogCXInput, Display, TEXT("Current input mode: %s"), *InputSubsystem->GetInputMode().ToStringSimple());

				const UCXInputControlSubsystem* Subsystem = UCXInputControlSubsystem::TryGet(InWorld);
				for (const TPair<FGameplayTag, TArray<FCXInputModeHandle>>& Pair : DebugHandles)
				{
					const bool bActive = Subsystem && Subsystem->HasInputModeTag(Pair.Key);
					UE_LOG(LogCXInput, Display, TEXT("  Debug request: %s x%d (active: %s)"), *Pair.Key.ToString(), Pair.Value.Num(), bActive ? TEXT("true") : TEXT("false"));
				}
			}));

	// 엔진이 요청한 마우스 캡처 / 잠금 설정과 Windows가 실제로 커서를 가두는 영역(ClipCursor)을 비교 출력
	// - 가두는 영역이 모니터 전체 : 잠금이 적용되지 않음
	// - 가두는 영역이 창 영역과 다름 : 잠금 영역 계산이 어긋남
	static void PrintCursorClip(UWorld* InWorld)
	{
		TArray<FString> Lines;

		// 엔진 쪽 설정
		UGameViewportClient* ViewportClient = GEngine ? GEngine->GameViewport : nullptr;
		if (nullptr == ViewportClient)
		{
			UE_LOG(LogCXInput, Warning, TEXT("GameViewportClient가 없습니다. 게임 실행 중에 사용하세요."));
			return;
		}

		const APlayerController* PC = InWorld ? InWorld->GetFirstPlayerController() : nullptr;
		const FViewport* Viewport = ViewportClient->Viewport;

		Lines.Add(FString::Printf(TEXT("MouseCaptureMode=%s, MouseLockMode=%s, HasMouseCapture=%s, ShowMouseCursor=%s"),
			*StaticEnum<EMouseCaptureMode>()->GetNameStringByValue(static_cast<int64>(ViewportClient->GetMouseCaptureMode())),
			*StaticEnum<EMouseLockMode>()->GetNameStringByValue(static_cast<int64>(ViewportClient->GetMouseLockMode())),
			(Viewport && Viewport->HasMouseCapture()) ? TEXT("true") : TEXT("false"),
			(PC && PC->ShouldShowMouseCursor()) ? TEXT("true") : TEXT("false")));

#if PLATFORM_WINDOWS
		// Windows 쪽 실제 상태
		const TSharedPtr<SWindow> Window = ViewportClient->GetWindow();
		const TSharedPtr<FGenericWindow> NativeWindow = Window.IsValid() ? Window->GetNativeWindow() : nullptr;
		const HWND GameWindowHandle = NativeWindow.IsValid() ? static_cast<HWND>(NativeWindow->GetOSWindowHandle()) : nullptr;

		RECT ClipRect = {};
		RECT WindowRect = {};
		POINT CursorPos = {};
		::GetClipCursor(&ClipRect);
		const bool bHasWindowRect = GameWindowHandle && ::GetWindowRect(GameWindowHandle, &WindowRect);
		::GetCursorPos(&CursorPos);

		// 모니터 전체(가상 화면)를 가두면 잠금이 없는 상태
		const RECT VirtualScreen = {
			::GetSystemMetrics(SM_XVIRTUALSCREEN),
			::GetSystemMetrics(SM_YVIRTUALSCREEN),
			::GetSystemMetrics(SM_XVIRTUALSCREEN) + ::GetSystemMetrics(SM_CXVIRTUALSCREEN),
			::GetSystemMetrics(SM_YVIRTUALSCREEN) + ::GetSystemMetrics(SM_CYVIRTUALSCREEN) };
		const bool bClipIsFullScreen = ClipRect.left <= VirtualScreen.left && ClipRect.top <= VirtualScreen.top
			&& ClipRect.right >= VirtualScreen.right && ClipRect.bottom >= VirtualScreen.bottom;

		Lines.Add(FString::Printf(TEXT("ClipCursor=(%d, %d) ~ (%d, %d)%s"),
			ClipRect.left, ClipRect.top, ClipRect.right, ClipRect.bottom, bClipIsFullScreen ? TEXT("  <- 잠금 없음 (모니터 전체)") : TEXT("")));
		Lines.Add(bHasWindowRect
			? FString::Printf(TEXT("GameWindow=(%d, %d) ~ (%d, %d)"), WindowRect.left, WindowRect.top, WindowRect.right, WindowRect.bottom)
			: FString(TEXT("GameWindow=알 수 없음")));
		Lines.Add(FString::Printf(TEXT("CursorPos=(%d, %d), Foreground=%s"),
			CursorPos.x, CursorPos.y, (GameWindowHandle && ::GetForegroundWindow() == GameWindowHandle) ? TEXT("true") : TEXT("false")));
#else
		Lines.Add(TEXT("ClipCursor 확인은 Windows에서만 지원합니다."));
#endif

		for (const FString& Line : Lines)
		{
			UE_LOG(LogCXInput, Display, TEXT("[CursorClip] %s"), *Line);
		}

		// 콘솔 출력이 보이지 않는 상황(전체 화면 등)에서도 확인할 수 있도록 화면에도 표시
		// (화면 메시지는 나중에 추가한 것이 위에 표시되므로 역순으로 추가)
		for (int32 Index = Lines.Num() - 1; Index >= 0; --Index)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Cyan, Lines[Index]);
		}
	}

	// CXVI.Input.CursorClip [지연 초]
	// 콘솔을 여는 동안은 마우스 캡처 상태가 달라질 수 있으므로, 지연을 주면 콘솔을 닫은 뒤의 상태를 확인
	// (일시 정지와 관계없이 흐르도록 Core Ticker 사용)
	static FAutoConsoleCommandWithWorldAndArgs CursorClipCommand(
		TEXT("CXVI.Input.CursorClip"),
		TEXT("마우스 캡처 / 잠금 설정과 Windows의 실제 커서 잠금 영역(ClipCursor), 게임 창 영역을 출력합니다. 사용법: CXVI.Input.CursorClip [지연 초] (콘솔을 닫은 뒤 상태를 보려면 3 등 지정)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				const float DelaySeconds = InArgs.IsValidIndex(0) ? FCString::Atof(*InArgs[0]) : 0.f;
				if (DelaySeconds <= 0.f)
				{
					PrintCursorClip(InWorld);
					return;
				}

				UE_LOG(LogCXInput, Display, TEXT("[CursorClip] %.1f초 뒤에 출력합니다. 콘솔을 닫고 기다리세요."), DelaySeconds);

				TWeakObjectPtr<UWorld> WeakWorld = InWorld;
				FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakWorld](float)
					{
						PrintCursorClip(WeakWorld.Get());
						return false; // 한 번만 실행
					}), DelaySeconds);
			}));
}

namespace CXUserDataDebug
{
	static UCXUserDataSubsystem* GetSubsystem(UWorld* InWorld)
	{
		UCXUserDataSubsystem* Subsystem = UCXUserDataSubsystem::TryGet(InWorld);
		if (nullptr == Subsystem)
		{
			UE_LOG(LogCXUserData, Warning, TEXT("UserData Subsystem이 없습니다. 게임 맵에서 실행하세요."));
		}
		return Subsystem;
	}

	// <ItemId> [Count] 파싱 (Count 기본 1). 인자가 잘못되면 false
	static bool ParseItemArgs(const TArray<FString>& InArgs, FName& OutItemId, int32& OutCount)
	{
		if (InArgs.IsEmpty())
		{
			return false;
		}

		OutItemId = FName(*InArgs[0]);
		OutCount = InArgs.IsValidIndex(1) ? FCString::Atoi(*InArgs[1]) : 1;
		return OutCount > 0;
	}

	// CXVI.UserData.AddItem <ItemId> [Count]
	static FAutoConsoleCommandWithWorldAndArgs AddItemCommand(
		TEXT("CXVI.UserData.AddItem"),
		TEXT("UserData 인벤토리에 아이템을 추가합니다. 사용법: CXVI.UserData.AddItem <ItemId> [개수] (기본 1)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				FName ItemId;
				int32 Count = 0;
				if (false == ParseItemArgs(InArgs, ItemId, Count))
				{
					UE_LOG(LogCXUserData, Warning, TEXT("사용법: CXVI.UserData.AddItem <ItemId> [개수] (개수는 1 이상)"));
					return;
				}

				UCXUserDataSubsystem* Subsystem = GetSubsystem(InWorld);
				if (nullptr == Subsystem)
				{
					return;
				}

				Subsystem->AddItem(ItemId, Count);
				UE_LOG(LogCXUserData, Display, TEXT("%s +%d -> %d"), *ItemId.ToString(), Count, Subsystem->GetItemCount(ItemId));
			}));

	// CXVI.UserData.RemoveItem <ItemId> [Count]
	static FAutoConsoleCommandWithWorldAndArgs RemoveItemCommand(
		TEXT("CXVI.UserData.RemoveItem"),
		TEXT("UserData 인벤토리에서 아이템을 제거합니다. 수량이 부족하면 제거하지 않습니다. 사용법: CXVI.UserData.RemoveItem <ItemId> [개수] (기본 1)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				FName ItemId;
				int32 Count = 0;
				if (false == ParseItemArgs(InArgs, ItemId, Count))
				{
					UE_LOG(LogCXUserData, Warning, TEXT("사용법: CXVI.UserData.RemoveItem <ItemId> [개수] (개수는 1 이상)"));
					return;
				}

				UCXUserDataSubsystem* Subsystem = GetSubsystem(InWorld);
				if (nullptr == Subsystem)
				{
					return;
				}

				if (false == Subsystem->RemoveItem(ItemId, Count))
				{
					UE_LOG(LogCXUserData, Warning, TEXT("수량이 부족해 제거하지 않았습니다: %s (보유 %d, 요청 %d)"), *ItemId.ToString(), Subsystem->GetItemCount(ItemId), Count);
					return;
				}

				UE_LOG(LogCXUserData, Display, TEXT("%s -%d -> %d"), *ItemId.ToString(), Count, Subsystem->GetItemCount(ItemId));
			}));

	// CXVI.UserData.SetHealth <MemberIndex> <Health>
	static FAutoConsoleCommandWithWorldAndArgs SetHealthCommand(
		TEXT("CXVI.UserData.SetHealth"),
		TEXT("파티원의 현재 HP를 지정합니다. 사용법: CXVI.UserData.SetHealth <파티원 인덱스> <HP>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				if (InArgs.Num() < 2 || false == InArgs[0].IsNumeric() || false == InArgs[1].IsNumeric())
				{
					UE_LOG(LogCXUserData, Warning, TEXT("사용법: CXVI.UserData.SetHealth <파티원 인덱스> <HP>"));
					return;
				}

				UCXUserDataSubsystem* Subsystem = GetSubsystem(InWorld);
				if (nullptr == Subsystem)
				{
					return;
				}

				const int32 MemberIndex = FCString::Atoi(*InArgs[0]);
				if (false == Subsystem->GetPartyMembers().IsValidIndex(MemberIndex))
				{
					UE_LOG(LogCXUserData, Warning, TEXT("파티원 인덱스가 범위를 벗어났습니다: %d (파티 인원 %d)"), MemberIndex, Subsystem->GetPartyMembers().Num());
					return;
				}

				Subsystem->SetMemberHealth(MemberIndex, FCString::Atof(*InArgs[1]));
				UE_LOG(LogCXUserData, Display, TEXT("파티원 %d HP -> %.0f"), MemberIndex, Subsystem->GetPartyMembers()[MemberIndex].CurrentHealth);
			}));

	// <SlotIndex> 파싱. 숫자가 아니면 false
	static bool ParseSlotArg(const TArray<FString>& InArgs, int32& OutSlotIndex)
	{
		if (InArgs.IsEmpty() || false == InArgs[0].IsNumeric())
		{
			return false;
		}

		OutSlotIndex = FCString::Atoi(*InArgs[0]);
		return true;
	}

	// CXVI.Save.Manual <SlotIndex>
	static FAutoConsoleCommandWithWorldAndArgs SaveManualCommand(
		TEXT("CXVI.Save.Manual"),
		TEXT("수동 슬롯에 저장합니다. 사용법: CXVI.Save.Manual <슬롯>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				int32 SlotIndex = INDEX_NONE;
				if (false == ParseSlotArg(InArgs, SlotIndex))
				{
					UE_LOG(LogCXUserData, Warning, TEXT("사용법: CXVI.Save.Manual <슬롯>"));
					return;
				}

				UCXUserDataSubsystem* Subsystem = GetSubsystem(InWorld);
				if (nullptr == Subsystem)
				{
					return;
				}

				// 슬롯 범위 검사는 SaveGame이 처리. 완료 결과는 비동기로 "Saved: <슬롯 이름>" 로그 출력
				Subsystem->SaveGame(SlotIndex);
			}));

	// CXVI.Save.Auto
	static FAutoConsoleCommandWithWorldAndArgs SaveAutoCommand(
		TEXT("CXVI.Save.Auto"),
		TEXT("자동 저장 요청 메시지를 방송합니다."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				if (nullptr == GetSubsystem(InWorld))
				{
					return;
				}

				// 실제 요청과 같은 경로를 확인하기 위해 AutoSave를 직접 호출하지 않고 메시지로 요청
				UCXGameplayMessageSubsystem::Get(InWorld).BroadcastMessage(CXVIGameplayTags::Message_UserData_RequestAutoSave, FCXAutoSaveRequestMessage());
			}));

	// CXVI.Save.Load <SlotIndex>
	static FAutoConsoleCommandWithWorldAndArgs SaveLoadCommand(
		TEXT("CXVI.Save.Load"),
		TEXT("슬롯을 불러와 저장된 레벨로 이동합니다. 사용법: CXVI.Save.Load <슬롯> (0 = 자동 저장)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				int32 SlotIndex = INDEX_NONE;
				if (false == ParseSlotArg(InArgs, SlotIndex))
				{
					UE_LOG(LogCXUserData, Warning, TEXT("사용법: CXVI.Save.Load <슬롯> (0 = 자동 저장)"));
					return;
				}

				if (UCXUserDataSubsystem* Subsystem = GetSubsystem(InWorld))
				{
					Subsystem->LoadGameAndTravel(SlotIndex);
				}
			}));
}

namespace CXPopupDebug
{
	static FAutoConsoleCommandWithWorldAndArgs PopupCommand(
		TEXT("CXVI.UI.Popup"),
		TEXT("공용 팝업 열기. CXVI.UI.Popup <1|2> (1 = 확인, 2 = 확인/취소)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& InArgs, UWorld* InWorld)
			{
				UCXUIManagerSubsystem* UIManager = UCXUIManagerSubsystem::TryGet(InWorld);
				if (nullptr == UIManager)
				{
					return;
				}

				const FSimpleDelegate OnConfirm = FSimpleDelegate::CreateLambda([]() { UE_LOG(LogCXUI, Log, TEXT("팝업 콜백: 확인")); });
				const FSimpleDelegate OnCancel = FSimpleDelegate::CreateLambda([]() { UE_LOG(LogCXUI, Log, TEXT("팝업 콜백: 취소")); });

				if (InArgs.Num() > 0 && InArgs[0] == TEXT("2"))
				{
					UIManager->ShowConfirmCancelPopup(NAME_None, OnConfirm, OnCancel);
				}
				else
				{
					UIManager->ShowConfirmPopup(NAME_None, OnConfirm);
				}
			}));
}

#endif // !UE_BUILD_SHIPPING