// Fill out your copyright notice in the Description page of Project Settings.


#include "CXFieldScene.h"
#include "CXVI/Foundation/Scene/Scenes/CXCombatScene.h"
#include "CXVI/Character/Field/CXFieldEnemy.h"
#include "CXVI/Foundation/Field/FieldEnemySpawning/CXFieldEnemySubsystem.h"
#include "CXVI/Foundation/Field/FieldEnemySpawning/CXFieldEnemySpawner.h"

#include "CXVI/Foundation/GameplayData/CXGameplayDataSubsystem.h"
#include "CXVI/Foundation/UserData/CXEnemyDefinition.h"
#include "CXVI/Foundation/UserData/CXUserDataTypes.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"
#include "CXVI/Actor/Field/CXVictoryStage.h"
#include "Foundation/Scene/CXSceneSubsystem.h"
#include "Tags/CXGameplayTags.h"
#include "UI/CXActivatableWidget.h"
#include "UI/CXUIManagerSubsystem.h"
#include "UI/CXUISettings.h"

#include "EngineUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldScene)

void UCXFieldScene::OnResume(const FCXSceneContext& InContext)
{
	Super::OnResume(InContext);

	const FCXCombatResult* Result = InContext.GetPayload<FCXCombatResult>();

	if (nullptr == Result)
	{
		return;
	}

	// 스포너와 관계없이 표시해야 하므로 필드 적 처리보다 먼저 확인
	if (Result->bVictory && Result->bBossDefeated)
	{
		ShowGameClearScreen();
	}

	DefeatCombatEnemy(*Result);

	// 보스전이 아닌 승리만 자동 저장. HP / 경험치는 전투 종료 시, 처치 기록은 위에서 UserData에 반영됨
	if (Result->bVictory && !Result->bBossDefeated)
	{
		UCXGameplayMessageSubsystem::Get(this).BroadcastMessage(CXVIGameplayTags::Message_UserData_RequestAutoSave, FCXAutoSaveRequestMessage());
	}
}

void UCXFieldScene::DefeatCombatEnemy(const FCXCombatResult& InResult)
{
	UWorld* World = GetWorld();
	UCXFieldEnemySubsystem* EnemySubsystem = IsValid(World) ? World->GetSubsystem<UCXFieldEnemySubsystem>() : nullptr;
	if (!IsValid(EnemySubsystem))
	{
		return;
	}

	const FName SpawnId = EnemySubsystem->GetSpawnedId();
	EnemySubsystem->SetSpawnedId(NAME_None);

	if (!InResult.bVictory || SpawnId.IsNone())
	{
		return;
	}

	ACXFieldEnemySpawner* Spawner = EnemySubsystem->FindSpawner(SpawnId);
	ACXFieldEnemy* Enemy = IsValid(Spawner) ? Spawner->GetSpawnedEnemy() : nullptr;
	if (!IsValid(Enemy))
	{
		return;
	}

	// Defeat()에서 적이 제거될 수 있으므로 먼저 꺼내 둔다
	const FName EncounterId = Enemy->GetEncounterId();
	Enemy->Defeat();

	if (IsBossEncounter(EncounterId))
	{
		UE_LOG(LogCXScene, Display, TEXT("[게임 클리어] 보스 처치. Encounter=%s"), *EncounterId.ToString());

		// 레벨에 배치된 승리 무대 재생 (하나만 배치)
		TActorIterator<ACXVictoryStage> It(World);
		if (It)
		{
			It->Play();
		}
		else
		{
			UE_LOG(LogCXScene, Warning, TEXT("[게임 클리어] 레벨에 ACXVictoryStage가 없습니다."));
		}
	}
}

bool UCXFieldScene::IsBossEncounter(FName InEncounterId) const
{
	if (InEncounterId.IsNone())
	{
		return false;
	}

	const UCXGameplayDataSubsystem& GameplayData = UCXGameplayDataSubsystem::Get(this);
	const FCXEncounterRow* Row = GameplayData.FindEncounterRow(InEncounterId);
	if (nullptr == Row)
	{
		return false;
	}

	for (const FPrimaryAssetId& EnemyId : Row->Enemies)
	{
		const UCXEnemyDefinition* EnemyDefinition = GameplayData.LoadEnemyDefinition(EnemyId);
		if (nullptr != EnemyDefinition && EnemyDefinition->bIsBoss)
		{
			return true;
		}
	}

	return false;
}

void UCXFieldScene::OnSuspend()
{
	Super::OnSuspend();
}

void UCXFieldScene::OnExit()
{
	Super::OnExit();
}

void UCXFieldScene::SetupSceneInput()
{
	Super::SetupSceneInput();
}

void UCXFieldScene::ShowGameClearScreen()
{
	UCXUIManagerSubsystem* UIManager = UCXUIManagerSubsystem::TryGet(this);
	if (nullptr == UIManager)
	{
		return;
	}

	const TSubclassOf<UCXActivatableWidget> WidgetClass = GetDefault<UCXUISettings>()->GameClearClass.LoadSynchronous();
	if (nullptr == WidgetClass)
	{
		UE_LOG(LogCXScene, Warning, TEXT("FieldScene : 클리어 화면 클래스가 지정되지 않았습니다. (Project Settings > Game > CXVI UI)"));
		return;
	}

	UIManager->PushWidgetToLayer<UCXActivatableWidget>(CXVIGameplayTags::UI_Layer_GameMenu, WidgetClass);
}