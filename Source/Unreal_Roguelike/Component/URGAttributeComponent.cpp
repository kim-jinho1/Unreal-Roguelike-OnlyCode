#include "Component/URGAttributeComponent.h"

UURGAttributeComponent::UURGAttributeComponent()
{
	// 체력 처리는 이벤트 기반이므로 매 프레임 Tick은 필요하지 않다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UURGAttributeComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	// 게임 시작 시 컨트롤러의 OnPossess(HUD 바인딩)가 이 BeginPlay보다 먼저 올 수 있다.
	// 그 경우 HUD가 읽은 초기값은 0이므로, 여기서 한 번 브로드캐스트해 올바른 값으로 맞춘다.
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UURGAttributeComponent::HandleTakeAnyDamage);
	}
}

bool UURGAttributeComponent::IsAlive() const
{
	return !bIsDead && CurrentHealth > 0.0f;
}

void UURGAttributeComponent::Heal(float Amount)
{
	if (!IsAlive() || Amount <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UURGAttributeComponent::AddMaxHealth(float Amount)
{
	if (Amount == 0.0f)
	{
		return;
	}

	MaxHealth = FMath::Max(1.0f, MaxHealth + Amount);

	// 최대 체력 증가분만큼 현재 체력도 회복 (감소 시에는 상한으로 클램프)
	CurrentHealth = FMath::Clamp(CurrentHealth + FMath::Max(0.0f, Amount), 0.0f, MaxHealth);

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UURGAttributeComponent::SetMaxHealth(float NewMaxHealth)
{
	MaxHealth = FMath::Max(1.0f, NewMaxHealth);
	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UURGAttributeComponent::RestoreHealth(float InCurrentHealth, float InMaxHealth)
{
	// 최대 체력이 0 이하로 넘어오면 저장된 값이 없는 것으로 보고 BP 기본값을 유지한다.
	if (InMaxHealth > 0.0f)
	{
		MaxHealth = InMaxHealth;
	}

	CurrentHealth = FMath::Clamp(InCurrentHealth, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UURGAttributeComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (!IsAlive() || Damage <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);

	// 브로드캐스트 전에 기록해 두어야 OnDeath 수신자가 마지막 가해자를 조회할 수 있다.
	LastDamageCauser = DamageCauser;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		bIsDead = true;
		OnDeath.Broadcast();
	}
	else
	{
		OnDamaged.Broadcast(Damage, DamageCauser);
	}
}

