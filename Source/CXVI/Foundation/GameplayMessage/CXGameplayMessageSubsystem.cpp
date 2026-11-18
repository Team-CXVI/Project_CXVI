// Fill out your copyright notice in the Description page of Project Settings.

#include "CXGameplayMessageSubsystem.h"
#include "HAL/IConsoleManager.h"

// CXGameplayMessageSubsystem.cpp와 CXGameplayMessageSubsystem.gen.cpp를 같은 번역 단위로 묶는 처리 -> 따로 컴파일하지 않고 한번에 컴파일함
#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGameplayMessageSubsystem)

DEFINE_LOG_CATEGORY(LogGameplayMessageSubsystem);

namespace CXVI
{
	namespace GameplayMessageSubsystem
	{
		static int32 ShouldLogMessages = 0;
		static FAutoConsoleVariableRef CVarShouldLogMessages(
			TEXT("CXVI.GameplayMessageSubsystem.LogMessages"), ShouldLogMessages, TEXT("Should messages broadcast through the gameplay message subsystem be logged?"));
	}
}

void FCXGameplayMessageListenerHandle::Unregister()
{
	if (UCXGameplayMessageSubsystem* Subsystem = SubsystemRef.Get())
	{
		Subsystem->UnregisterListener(*this);
		SubsystemRef.Reset();
		Channel = FGameplayTag();
		Id = 0;
	}
}


UCXGameplayMessageSubsystem& UCXGameplayMessageSubsystem::Get(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	check(World);

	UCXGameplayMessageSubsystem* Router = UGameInstance::GetSubsystem<UCXGameplayMessageSubsystem>(World->GetGameInstance());
	check(Router);

	return *Router;
}

bool UCXGameplayMessageSubsystem::HasInstance(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	UCXGameplayMessageSubsystem* Router = (nullptr != World) ? UGameInstance::GetSubsystem<UCXGameplayMessageSubsystem>(World->GetGameInstance()) : nullptr;

	return nullptr != Router;
}

void UCXGameplayMessageSubsystem::Deinitialize()
{
	ListenerMap.Reset();

	Super::Deinitialize();
}

void UCXGameplayMessageSubsystem::UnregisterListener(const FCXGameplayMessageListenerHandle& InHandle)
{
	if (InHandle.IsValid())
	{
		check(InHandle.SubsystemRef == this);

		UnregisterListenerInternal(InHandle.Channel, InHandle.Id);
	}
	else
	{
		UE_LOG(LogGameplayMessageSubsystem, Warning, TEXT("Trying to unregister an invalid Handle."));
	}
}

void UCXGameplayMessageSubsystem::K2_BroadcastMessage(const FGameplayTag& InChannel, const int32& InMessage)
{
	// 이 함수를 C++에서 직접 호출하지 않음
	checkNoEntry();
}

DEFINE_FUNCTION(UCXGameplayMessageSubsystem::execK2_BroadcastMessage)
{
	// 첫 번째 인자는 FGameplayTag 고정이라 그대로 가져옴
	P_GET_STRUCT(FGameplayTag, InChannel);

	Stack.MostRecentPropertyAddress = nullptr;				// 이전의 프로퍼티를 가리키는 메모리 주소를 날림

	// 다음 인자의 실제 변수가 담겨있는 메모리 주소 획득 
	Stack.StepCompiledIn<FStructProperty>(nullptr);			// nullptr로 받는 경우는 와일드카드 핀이라 실제 값 타입을 알지 못하기 때문에 타입을 지정해서 값을 받아올 수 없기 때문에	
	void* MessagePtr = Stack.MostRecentPropertyAddress;		// -> StepCompiledIn로 받아온 다음 인자 구조체 데이터의 주소
	FStructProperty* StructProp = CastField<FStructProperty>(Stack.MostRecentProperty);

	// 함수 인자 읽기 종료
	P_FINISH;

	if (ensure((nullptr != StructProp) && (nullptr != StructProp->Struct) && (nullptr != MessagePtr)))
	{
		// P_THIS : Context를 실제 클래스 타입으로 캐스팅
		P_THIS->BroadcastMessageInternal(InChannel, StructProp->Struct, MessagePtr);
	}
}

FCXGameplayMessageListenerHandle UCXGameplayMessageSubsystem::RegisterListenerInternal(const FGameplayTag& InChannel, TFunction<void(const FGameplayTag&, const UScriptStruct*, const void*)>&& InCallback, const UScriptStruct* InStructType)
{
	FCXChannelListenerList& List = ListenerMap.FindOrAdd(InChannel);

	FCXGameplayMessageListenerData& Entry = List.Listeners.AddDefaulted_GetRef();
	Entry.ReceivedCallback = MoveTemp(InCallback);
	Entry.ListenerStructType = InStructType;
	Entry.bHadValidType = InStructType != nullptr;
	Entry.HandleId = ++List.HandleId;

	return FCXGameplayMessageListenerHandle(this, InChannel, Entry.HandleId);
}

void UCXGameplayMessageSubsystem::BroadcastMessageInternal(const FGameplayTag& InChannel, const UScriptStruct* InStructType, const void* InMessageBytes)
{
	if (CXVI::GameplayMessageSubsystem::ShouldLogMessages != 0)
	{
		FString HumanReadableMessage;
		InStructType->ExportText(/*out*/ HumanReadableMessage, InMessageBytes, /*Defaults=*/ nullptr, /*OwnerObject=*/ nullptr, PPF_None, /*ExportRootScope=*/ nullptr);
		UE_LOG(LogGameplayMessageSubsystem, Log, TEXT("BroadcastMessage(%s, %s)"), *InChannel.ToString(), *HumanReadableMessage);
	}

	if (const FCXChannelListenerList* ListenerList = ListenerMap.Find(InChannel))
	{
		TArray<FCXGameplayMessageListenerData> ListenerArray(ListenerList->Listeners);

		for (const FCXGameplayMessageListenerData& Listener : ListenerArray)
		{
			if (Listener.bHadValidType && !Listener.ListenerStructType.IsValid())
			{
				UE_LOG(LogGameplayMessageSubsystem, Warning, TEXT("Listener struct type has gone invalid on Channel %s. Removing listener from list"), *InChannel.ToString());
				UnregisterListenerInternal(InChannel, Listener.HandleId);
				continue;
			}

			if (!Listener.bHadValidType || InStructType->IsChildOf(Listener.ListenerStructType.Get()))
			{
				Listener.ReceivedCallback(InChannel, InStructType, InMessageBytes);
			}
			else
			{
				UE_LOG(LogGameplayMessageSubsystem, Error, TEXT("Struct type mismatch on channel %s (broadcast type %s, listener at %s was expecting type %s)"),
					*InChannel.ToString(), *InStructType->GetPathName(), *InChannel.ToString(), *Listener.ListenerStructType->GetPathName());
			}
		}
	}

}

void UCXGameplayMessageSubsystem::UnregisterListenerInternal(const FGameplayTag& InChannel, int32 InHandleId)
{
	if (FCXChannelListenerList* ListenerList = ListenerMap.Find(InChannel))
	{
		int32 MatchIndex = ListenerList->Listeners.IndexOfByPredicate(
			[Id = InHandleId](const FCXGameplayMessageListenerData& Other) 
			{
				return Other.HandleId == Id;
			});

		if (INDEX_NONE != MatchIndex)
		{
			ListenerList->Listeners.RemoveAt(MatchIndex);
		}

		if (0 == ListenerList->Listeners.Num())
		{
			ListenerMap.Remove(InChannel);
		}
	}
}

