// Fill out your copyright notice in the Description page of Project Settings.


#include "CXAsyncAction_ListenForMessage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXAsyncAction_ListenForMessage)

UCXAsyncAction_ListenForGameplayMessage* UCXAsyncAction_ListenForGameplayMessage::ListenForGameplayMessages(UObject* WorldContextObject, const FGameplayTag& InChannel, UScriptStruct* InPayloadType)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (nullptr == World)
	{
		return nullptr;
	}

	UCXAsyncAction_ListenForGameplayMessage* Action = NewObject<UCXAsyncAction_ListenForGameplayMessage>();
	Action->WorldPtr = World;
	Action->ChannelToRegister = InChannel;
	Action->MessageStructType = InPayloadType;
	Action->RegisterWithGameInstance(World);

	return Action;
}

bool UCXAsyncAction_ListenForGameplayMessage::GetPayload(int32& OutPayload)
{
	// C++에서 접근 금지
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UCXAsyncAction_ListenForGameplayMessage::execGetPayload)
{
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	void* MessagePtr = Stack.MostRecentPropertyAddress;
	FStructProperty* StructProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	P_FINISH;

	bool bSuccess = false;

	if ((nullptr != StructProp) &&
		(nullptr != StructProp->Struct) &&
		(nullptr != MessagePtr) &&
		(P_THIS->MessageStructType.Get() == StructProp->Struct) &&
		(nullptr != P_THIS->ReceivedMessagePayloadPtr))
	{
		StructProp->Struct->CopyScriptStruct(MessagePtr, P_THIS->ReceivedMessagePayloadPtr);
		bSuccess = true;
	}

	*(bool*)RESULT_PARAM = bSuccess;
}


void UCXAsyncAction_ListenForGameplayMessage::Activate()
{
	// GameplayMessageSubsystem에 OnMessageReceived 델리게이트를 호출하도록 등록
	if (UWorld* World = WorldPtr.Get())
	{
		if (UCXGameplayMessageSubsystem::HasInstance(World))
		{
			UCXGameplayMessageSubsystem& Router = UCXGameplayMessageSubsystem::Get(World);

			TWeakObjectPtr<UCXAsyncAction_ListenForGameplayMessage> WeakThis(this);
			ListenerHandle = Router.RegisterListenerInternal(
				ChannelToRegister,
				[WeakThis](const FGameplayTag& InChannel, const UScriptStruct* InStructType, const void* InPayload)
				{
					if (UCXAsyncAction_ListenForGameplayMessage* StrongThis = WeakThis.Get())
					{
						StrongThis->HandleMessageReceived(InChannel, InStructType, InPayload);
					}
				}, MessageStructType.Get());
		}
	}

	// 등록할 GameplayMessageSubsystem를 찾지 못했으므로 객체 파괴
	SetReadyToDestroy();
}

void UCXAsyncAction_ListenForGameplayMessage::SetReadyToDestroy()
{
	ListenerHandle.Unregister();

	Super::SetReadyToDestroy();
}

void UCXAsyncAction_ListenForGameplayMessage::HandleMessageReceived(const FGameplayTag InChannel, const UScriptStruct* InStructType, const void* InPayload)
{
	if (nullptr == MessageStructType.Get() || MessageStructType.Get() == InStructType)
	{
		ReceivedMessagePayloadPtr = InPayload;
		OnMessageReceived.Broadcast(this, InChannel);
		ReceivedMessagePayloadPtr = nullptr;
	}

	if (!OnMessageReceived.IsBound())
	{
		SetReadyToDestroy();
	}
}

