// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TikStudioEventQueueSettings.generated.h"

/** Enum for expiration policy when an event expires. */
UENUM(BlueprintType)
enum class EEventExpirePolicy : uint8
{
	Discard UMETA(DisplayName = "Discard"),
	Consolidate UMETA(DisplayName = "Consolidate")
};

/** Structure for event type specific modifiers. */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FEventTypeModifiers
{
	GENERATED_BODY()

	/** Map de modificadores por nombre (p. ej. "DiamondCount" -> 5) */
	UPROPERTY(EditAnywhere, Category = "Prioritization")
	TMap<FName, int32> Modifiers;
};

/** Sub-contenedores para agrupar toggles de habilitación por tipo de evento en la UI del Editor. */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FChatToggles
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category = "Chat", meta=(ToolTip="Habilita la emisión del evento 'Chat'."))
	bool bEnableChat = true;
};

USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FGiftToggles
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category = "Gift", meta=(ToolTip="[Flujo Gift Normal] Habilita la emisión del evento 'Gift' (regalos individuales normales)."))
	bool bEnableGift = true;
	UPROPERTY(EditAnywhere, Category = "Gift", meta=(ToolTip="[Flujo GiftCombo] Habilita la emisión del evento 'GiftCombo' (snapshots de combos acumulados)."))
	bool bEnableGiftCombo = true;
};

USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FFollowToggles
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category = "Follow", meta=(ToolTip="Habilita la emisión del evento 'Follow'."))
	bool bEnableFollow = true;
};

USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FLikeToggles
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category = "Like", meta=(ToolTip="Habilita la emisión del evento 'Like' (normalizado por batching)."))
	bool bEnableLike = true;
	UPROPERTY(EditAnywhere, Category = "Like", meta=(ToolTip="Habilita la emisión del evento 'LikeUser' (por usuario)."))
	bool bEnableLikeUser = true;
};

USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FMemberToggles
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category = "Member", meta=(ToolTip="Habilita emisión del flujo normalizado de Member (por hito)."))
	bool bEnableMemberNormalized = true;
	UPROPERTY(EditAnywhere, Category = "Member", meta=(ToolTip="Habilita emisión del flujo por identidad de Member (con viewer gate)."))
	bool bEnableMemberIdentity = true;
};

USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FShareToggles
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category = "Share", meta=(ToolTip="Habilita la emisión del evento 'Share'."))
	bool bEnableShare = true;
	
	UPROPERTY(EditAnywhere, Category = "Share", meta=(ToolTip="Habilita la emisión del evento 'ShareMilestone' (agregación de eventos Share)."))
	bool bEnableShareMilestone = true;
};

USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FRoomUserToggles
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category = "RoomUser", meta=(ToolTip="Habilita la emisión de RoomUserMilestone (hitos de ViewerCount cada N espectadores, ej: 25, 50, 75...)."))
	bool bEnableRoomUserMilestone = true;
	UPROPERTY(EditAnywhere, Category = "RoomUser", meta=(ToolTip="Habilita la emisión de RoomUser (snapshot completo periódico con ViewerCount + TopViewers[])."))
	bool bEnableRoomUser = true;
	UPROPERTY(EditAnywhere, Category = "RoomUser", meta=(ToolTip="Habilita la emisión de RoomUserTop1Change (notificación cuando cambia el Top1 gifter)."))
	bool bEnableRoomUserTop1Change = true;
};

/** Contenedor único para agrupar todos los toggles de habilitación de eventos. */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FEventToggles
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category = "Events", meta=(DisplayName="Chat"))
	FChatToggles Chat;
	UPROPERTY(EditAnywhere, Category = "Events", meta=(DisplayName="Gift"))
	FGiftToggles Gift;
	UPROPERTY(EditAnywhere, Category = "Events", meta=(DisplayName="Follow"))
	FFollowToggles Follow;
	UPROPERTY(EditAnywhere, Category = "Events", meta=(DisplayName="Like"))
	FLikeToggles Like;
	UPROPERTY(EditAnywhere, Category = "Events", meta=(DisplayName="Member"))
	FMemberToggles Member;
	UPROPERTY(EditAnywhere, Category = "Events", meta=(DisplayName="Share"))
	FShareToggles Share;
	UPROPERTY(EditAnywhere, Category = "Events", meta=(DisplayName="RoomUser"))
	FRoomUserToggles RoomUser;
};

// ===== Contenedores de configuración por tipo =====
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FLikeConfig
{
    GENERATED_BODY()
    
    // === NUEVO FLUJO: Like por Hitos de TotalLikeCount ===
    UPROPERTY(EditAnywhere, Category = "Like", meta=(ClampMin="1", ClampMax="10000", ToolTip="[Flujo Like Milestone] Paso del hito para TotalLikeCount. Default=500 → emite Like en TotalLikeCount=500, 1000, 1500... ↓ Menor (ej: 100): hitos más frecuentes, mayor feedback pero más eventos. ↑ Mayor (ej: 2000): hitos menos frecuentes, reduce tráfico pero feedback menos granular."))
    int32 LikeMilestoneStep = 500;
    
    UPROPERTY(EditAnywhere, Category = "Like", meta=(ToolTip="[Flujo Like Milestone] Comportamiento en saltos grandes de TotalLikeCount. Default=false → emite solo el hito mayor alcanzado con StepsCrossed. true → emite un evento por cada hito cruzado. ✓ Activado: máximo detalle, útil para análisis granular pero puede generar ráfagas de eventos. ✗ Desactivado: eficiente, un solo evento con resumen del salto."))
    bool bEmitAllLikeMilestonesOnLargeJump = false;
    
    // === Configuración LikeUser (independiente, se mantiene) ===
    /** Interruptor booleano para controlar emisión de LikeUser según audiencia */
    UPROPERTY(EditAnywhere, Category = "Like", meta=(ToolTip="[Flujo LikeUser] Activa el gate de ViewerCount para controlar emisión de LikeUser (eventos de like por identidad de usuario). Default=false → gate desactivado, no se filtra por audiencia. true → gate activado, se aplica el umbral. ✓ Activado: LikeUser controlado por threshold, previene sobrecarga en salas grandes/populares. ✗ Desactivado: LikeUser se emite según su toggle, sin filtrar por audiencia."))
    bool bEnableLikeUserViewerGate = true;
    UPROPERTY(EditAnywhere, Category = "Like", meta=(ClampMin="2", ClampMax="500", ToolTip="[Flujo LikeUser] Umbral de ViewerCount para permitir encolado de LikeUser (solo si bEnableLikeUserViewerGate=true). Default=25 → LikeUser se emite solo cuando VC ≤ 25 (salas pequeñas). ↓ Menor (ej: 10): LikeUser solo en streams muy pequeños/íntimos, optimiza carga máxima pero excluye salas medianas. ↑ Mayor (ej: 100): LikeUser habilitado en salas más grandes, mayor granularidad de likes por usuario (útil para engagement tracking) pero aumenta tráfico en streams populares. Nota: usar valores > 1 para evitar edge cases."))
    int32 LikeUserViewerGateThreshold = 25;
};

// Eliminado: FLikeUserConfig se integró en FLikeConfig

USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FMemberConfig
{
    GENERATED_BODY()
    
    // === Smart Switch para MemberIdentity ===
    /** Interruptor booleano para controlar emisión de MemberIdentity según audiencia */
    UPROPERTY(EditAnywhere, Category = "Member", meta=(ToolTip="[Flujo Member Identity] Activa el smart switch de ViewerCount para controlar emisión de MemberIdentity. Default=false → smart switch desactivado, usa comportamiento legacy con histéresis. true → smart switch activado, se aplica umbral simple sin histéresis. ✓ Activado: MemberIdentity controlado por threshold, previene sobrecarga en salas grandes. ✗ Desactivado: MemberIdentity usa gate legacy con histéresis (comportamiento actual)."))
    bool bEnableMemberIdentityViewerGate = true;
    
    UPROPERTY(EditAnywhere, Category = "Member", meta=(ClampMin="0", ClampMax="200", ToolTip="[Flujo Member Identity] Umbral de ViewerCount para permitir MemberIdentity (usado por smart switch si está activado, o como base para histéresis si está desactivado). Default=25 → MemberIdentity se emite solo cuando VC ≤ 25. ↓ Menor (ej: 10): MemberIdentity solo en streams pequeños/íntimos, optimiza carga máxima. ↑ Mayor (ej: 100): MemberIdentity habilitado en salas más grandes, mayor granularidad pero aumenta tráfico. 0 = siempre permite (máximo tráfico)."))
    int32 MemberIdentityViewerGateThreshold = 25;
    UPROPERTY(EditAnywhere, Category = "Member", meta=(ClampMin="0", ClampMax="50", ToolTip="[Flujo Member Identity] Delta de histéresis para cerrar el gate de identidad cuando el VC sube (Exit = Enter + Delta). Default=3 → si Enter=25, gate cierra en VC ≥ 28. ↓ Menor (ej: 0-1): poca histéresis, gate oscila rápidamente al cruzar umbral (puede causar abrir/cerrar frecuente cerca del threshold), menos estable pero respuesta inmediata. ↑ Mayor (ej: 10-20): alta histéresis, gate permanece abierto hasta VC mucho mayor (ej: 25+20=45), más estable ante fluctuaciones de audiencia pero mantiene gate abierto más tiempo en crecimiento. 0 = sin histéresis (Exit = Enter, máxima oscilación)."))
    int32 MemberIdentityGateHysteresisDelta = 3;
    UPROPERTY(EditAnywhere, Category = "Member", meta=(ClampMin="1", ClampMax="100", ToolTip="[Flujo Member Normalized] Hito de entradas acumuladas para emitir MemberNormalized (cuenta TODAS las entradas independiente del gate). Default=10 → emite cada 10 miembros nuevos (10, 20, 30...). ↓ Menor (ej: 5): hitos frecuentes, feedback visual más granular de crecimiento de membresía pero mayor tráfico Blueprint. ↑ Mayor (ej: 50): hitos espaciados, reduce eventos significativamente pero feedback menos frecuente, útil en streams con alto tráfico de miembros. Nota: siempre activo (no afectado por viewer gate)."))
    int32 MemberNormalizedJoinMilestone = 20;
};

USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FRoomUserConfig
{
    GENERATED_BODY()
    
    // [RoomUserMilestone] Hitos de ViewerCount (cada N espectadores)
    UPROPERTY(EditAnywhere, Category = "RoomUser", meta=(ClampMin="1", ClampMax="500", ToolTip="[RoomUserMilestone] Paso de hito de ViewerCount. Default=25 → emite en VC=25, 50, 75..."))
    int32 RoomUserMilestoneStep = 25;
    UPROPERTY(EditAnywhere, Category = "RoomUser", meta=(ClampMin="1.0", ClampMax="60.0", ToolTip="[RoomUserMilestone] Duración del cooldown temporal (τ) en segundos. Default=10.0s. Una banda de milestone bloqueada expira después de τ segundos desde su activación."))
    float MilestoneCooldownDuration = 10.0f;
    UPROPERTY(EditAnywhere, Category = "RoomUser", meta=(ClampMin="0", ClampMax="10", ToolTip="[RoomUserMilestone] Radio de adyacencia (±N milestones). Default=0 → limpia todos los cooldowns anteriores. 1+ → conserva cooldowns de milestones adyacentes (≤N pasos de distancia)."))
    int32 MilestoneAdjacencyRadius = 0;
    
    // [RoomUser] Snapshot periódico completo (ViewerCount + TopViewers[])
    UPROPERTY(EditAnywhere, Category = "RoomUser", meta=(ClampMin="1.0", ClampMax="60.0", ToolTip="[RoomUser] Periodo entre snapshots periódicos. Default=6.0s → snapshot cada 6 segundos."))
    float RoomUserSnapshotPeriodSeconds = 20.0f;
    UPROPERTY(EditAnywhere, Category = "RoomUser", meta=(ClampMin="0", ClampMax="500", ToolTip="[RoomUser Smart Switch] Umbral máximo de ViewerCount para PERMITIR snapshots. Si VC > MaxVC → BLOQUEA snapshot. Default=25."))
	int32 RoomUserSmartSwitchMaxVC = 25;
	UPROPERTY(EditAnywhere, Category = "RoomUser", meta=(ToolTip="[RoomUser Smart Switch] Habilita el interruptor inteligente. Si true → bloquea snapshots cuando VC > MaxVC. Si false → ignora umbral. Default=false."))
    bool bEnableRoomUserSmartSwitch = true;
    
    // [RoomUserTop1Change] Cambio de Top1 gifter
    UPROPERTY(EditAnywhere, Category = "RoomUser", meta=(ClampMin="0.0", ClampMax="60.0", ToolTip="[RoomUserTop1Change] Cooldown mínimo entre cambios de Top1. Default=5.0s. 0s=sin cooldown."))
    float RoomUserTop1CooldownSeconds = 0.0f;
    UPROPERTY(EditAnywhere, Category = "RoomUser", meta=(ClampMin="1", ClampMax="20", ToolTip="[RoomUserTop1Change] TopViewers mínimos requeridos para considerar cambios de Top1. Default=3."))
    int32 RoomUserTop1MinTopViewers = 3;
    UPROPERTY(EditAnywhere, Category = "RoomUser", meta=(ToolTip="[RoomUserTop1Change] Usa igualdad case-sensitive al comparar nombres de Top1. Default=true."))
    bool bRoomUserTop1StrictEquality = true;
};

USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FGiftConfig
{
    GENERATED_BODY()
    
    // [Flujo Gift Normal]
    // Sistema de puntuación lineal: 1 punto cada 10 diamantes, 3 puntos por repetición
    // Sin límite superior - refleja valor intrínseco real del regalo
    
    // [Flujo GiftCombo]
    UPROPERTY(EditAnywhere, Category = "Gift", meta=(ClampMin="1", ClampMax="50", ToolTip="[Flujo GiftCombo] Umbral de repeticiones para emitir snapshots intermedios. Default=5 → emite cada 5 regalos (rc=5, 10, 15...). ↓ Menor (ej: 2): más snapshots frecuentes, mayor tráfico Blueprint pero feedback visual más inmediato. ↑ Mayor (ej: 20): menos snapshots, reduce carga pero feedback menos frecuente en combos largos."))
    int32 GiftComboRepeatSnapshotStep = 5;
    UPROPERTY(EditAnywhere, Category = "Gift", meta=(ClampMin="1.0", ClampMax="30.0", ToolTip="[Flujo GiftCombo] Tiempo de inactividad (sin nuevos regalos) para cerrar automáticamente un combo. Default=5.0s. ↓ Menor (ej: 2s): cierra combos rápidamente, útil para gifts rápidos/burst pero puede cortar combos legítimos prematuramente. ↑ Mayor (ej: 15s): mantiene combos abiertos más tiempo, tolera pausas largas pero puede retrasar la emisión del snapshot final."))
    float GiftComboInactivitySeconds = 5.0f;
    UPROPERTY(EditAnywhere, Category = "Gift", meta=(ClampMin="10.0", ClampMax="600.0", ToolTip="[Flujo GiftCombo] Tiempo de retención del tracking de combos cerrados antes de eliminar de memoria. Default=60.0s. ↓ Menor (ej: 15s): limpieza agresiva, reduce memoria pero puede eliminar tracking antes de que deudas pendientes se paguen (causa SB huérfanos). ↑ Mayor (ej: 300s): más margen de seguridad para combos con procesamiento lento, consume más memoria pero previene pérdida de datos."))
    float GiftComboClosedPruneSeconds = 90.0f;
    // [OBSOLETO / NO FUNCIONAL] Este campo no se lee en ningún .cpp: mover este valor NO tiene efecto.
    // Su intención (reforzar la prioridad de los snapshots de combo) ya está cubierta por:
    //   1) BaseWeight[GiftCombo]=80 (el más alto), 2) el +15 del snapshot final (bIsFinalSnapshot),
    //   3) bProtectGiftComboSnapshots (anti-evicción).
    // Se mantiene temporalmente para no romper assets/settings guardados. Eliminar cuando se confirme que nada lo referencia.
    UPROPERTY(EditAnywhere, Category = "Gift", meta=(ClampMin="0", ClampMax="50", ToolTip="[OBSOLETO – NO FUNCIONAL] Este parámetro NO tiene efecto actualmente (ningún código lo lee). La prioridad de los snapshots de combo se controla vía BaseWeight[GiftCombo]=80, el bonus +15 del snapshot final (bIsFinalSnapshot) y bProtectGiftComboSnapshots. Conservado temporalmente por compatibilidad; pendiente de eliminar."))
    int32 GiftComboSnapshotPriorityBonus = 12;
    UPROPERTY(EditAnywhere, Category = "Gift", meta=(ToolTip="[Flujo GiftCombo] Protección absoluta contra evicción competitiva de snapshots GiftCombo. Default=true (recomendado). ✓ Activado: los snapshots NUNCA se desalojan de la cola, garantiza que todos los combos se procesen sin pérdida de datos (ideal para monetización). ✗ Desactivado: snapshots pueden ser evictados por eventos de mayor prioridad, riesgo de perder combos importantes en colas saturadas (NO recomendado)."))
    bool bProtectGiftComboSnapshots = true;
    UPROPERTY(EditAnywhere, Category = "Gift", meta=(ClampMin="30.0", ClampMax="300.0", ToolTip="[Flujo GiftCombo] TTL extendido para SB con deuda Final que expiraron (previene re-expiración en cadena). Default=90.0s. ↓ Menor (ej: 45s): menos tiempo de espera pero riesgo de re-expirar si Blueprint es muy lento (puede causar loop de re-encolado). ↑ Mayor (ej: 180s): margen generoso para procesamiento lento, previene pérdida de datos pero snapshots finales persisten más tiempo en memoria."))
    float GiftComboFinalTTLMinSeconds = 90.0f;
};

USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FShareConfig
{
    GENERATED_BODY()
    
    /** Interruptor booleano para controlar emisión de Share según audiencia */
    UPROPERTY(EditAnywhere, Category = "Share", meta=(ToolTip="[Flujo Share] Activa el gate de ViewerCount para controlar emisión de Share (eventos de compartir stream). Default=false → gate desactivado, Share se emite según su toggle sin filtrar por audiencia. true → gate activado, se aplica el umbral fail-closed. ✓ Activado: Share controlado por threshold, previene spam de shares en salas grandes/populares donde el impacto viral es menor. ✗ Desactivado: Share se emite según su toggle, sin filtrar por audiencia (comportamiento actual)."))
    bool bEnableShareViewerGate = true;
    
    UPROPERTY(EditAnywhere, Category = "Share", meta=(ClampMin="0", ClampMax="500", ToolTip="[Flujo Share] Umbral de ViewerCount para permitir encolado de Share (solo si bEnableShareViewerGate=true). Default=25 → Share se emite solo cuando VC ≤ 25 (salas pequeñas/medianas). ↓ Menor (ej: 10): Share solo en streams muy pequeños/íntimos, maximiza impacto viral relativo pero excluye salas medianas. ↑ Mayor (ej: 100): Share habilitado en salas más grandes, mayor alcance potencial pero puede generar ruido en streams populares. 0 = siempre permite (máximo tráfico). Nota: fail-closed → si VC no disponible (≤0), bloquea Share independiente del threshold."))
    int32 ShareViewerGateThreshold = 25;
    
    // ShareMilestone configuration
    UPROPERTY(EditAnywhere, Category = "Share", meta=(ClampMin="1", ClampMax="100", ToolTip="[ShareMilestone] Número de eventos Share necesarios para emitir un ShareMilestone. Default=10 → cada 10 shares se emite 1 milestone."))
    int32 ShareMilestoneStep = 10;
};

USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FEvictionConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category = "Eviction", meta=(ToolTip="Si está activado, los eventos nuevos de mayor prioridad pueden reemplazar al más débil cuando el tipo alcanza su límite de slots."))
    bool bEnableCompetitiveEviction = false;
    UPROPERTY(EditAnywhere, Category = "Eviction", meta=(ToolTip="Tipos de evento que nunca serán desplazados por competencia."))
    TSet<FName> ExemptFromEviction;
    UPROPERTY(EditAnywhere, Category = "Eviction", meta=(ToolTip="Habilita el conteo de evicciones por tipo para inspección y depuración."))
    bool bTrackEvictionMetrics = false;
};

/** Structure for TikTok Event Queue settings. */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTikStudioEventQueueSettings
{
    GENERATED_BODY()

    FTikStudioEventQueueSettings()
        : bEnableAutoPump(false)
        , AutoPumpInterval(1.0f)
    {
		// Initialize base weights (jerarquía: Monetización > Crecimiento > Engagement > Métricas > Eventos Básicos)
		// Orden: Chat, Gift, GiftCombo, Follow, Like, LikeUser, MemberIdentity, MemberNormalized, RoomUser, RoomUserMilestone, RoomUserTop1Change, Share
		BaseWeights.Add(TEXT("Chat"), 40);               // TIER 3 - Importante: interacción directa en tiempo real (puede tener comandos)
		BaseWeights.Add(TEXT("Gift"), 70);               // TIER 1 - Alta prioridad: regalos individuales (monetización directa)
		BaseWeights.Add(TEXT("GiftCombo"), 80);          // TIER 1 - Máxima prioridad: combos acumulados (monetización crítica)
		BaseWeights.Add(TEXT("Follow"), 60);             // TIER 2 - Muy importante: crecimiento de comunidad permanente
		BaseWeights.Add(TEXT("Like"), 25);               // TIER 4 - Medio-bajo: engagement pasivo por milestones
		BaseWeights.Add(TEXT("LikeUser"), 10);           // TIER 5 - Bajo: like individual con viewer gate
		BaseWeights.Add(TEXT("MemberIdentity"), 5);      // TIER 5 - Mínimo: entrada individual con gate (evento más básico/frecuente)
		BaseWeights.Add(TEXT("MemberNormalized"), 20);   // TIER 4 - Medio-bajo: hito de miembros (cada N entradas)
		BaseWeights.Add(TEXT("RoomUser"), 35);           // TIER 3 - Medio: estado periódico completo de audiencia (snapshot con todos los campos)
		BaseWeights.Add(TEXT("RoomUserMilestone"), 30);  // TIER 3 - Medio: hitos de crecimiento de audiencia (milestone derivado)
		BaseWeights.Add(TEXT("RoomUserTop1Change"), 50); // TIER 2 - Importante: competencia de gifters (relacionado con monetización)
		BaseWeights.Add(TEXT("Share"), 55);              // TIER 2 - Importante: alcance viral/marketing orgánico
		BaseWeights.Add(TEXT("ShareMilestone"), 50);     // TIER 2 - Importante: hito de shares (agregación de eventos Share)

		// Common modifiers
		CommonModifiers.Add(TEXT("FollowRole"), 5);
		CommonModifiers.Add(TEXT("bIsModerator"), 15);
		CommonModifiers.Add(TEXT("bIsSubscriber"), 10);
		CommonModifiers.Add(TEXT("bIsNewGifter"), 15);
		CommonModifiers.Add(TEXT("TopGifterRank"), 4);
		CommonModifiers.Add(TEXT("GifterLevel"), 1);
		CommonModifiers.Add(TEXT("TeamMemberLevel"), 1);

		// Specific modifiers
		{
			FEventTypeModifiers& GiftMods = SpecificModifiers.FindOrAdd(TEXT("Gift"));
			GiftMods.Modifiers.Add(TEXT("DiamondCount"), 5);
			GiftMods.Modifiers.Add(TEXT("bRepeatEnd"), 10);
			GiftMods.Modifiers.Add(TEXT("RepeatCount"), 2);
		}
		{
			FEventTypeModifiers& GiftComboMods = SpecificModifiers.FindOrAdd(TEXT("GiftCombo"));
			GiftComboMods.Modifiers.Add(TEXT("TotalDiamondCount"), 1);
			GiftComboMods.Modifiers.Add(TEXT("TotalRepeatCount"), 3);
			GiftComboMods.Modifiers.Add(TEXT("bIsFinalSnapshot"), 15);
		}
		{
			FEventTypeModifiers& ChatMods = SpecificModifiers.FindOrAdd(TEXT("Chat"));
			ChatMods.Modifiers.Add(TEXT("HasCommand"), 20);
		}
		{
			FEventTypeModifiers& LikeMods = SpecificModifiers.FindOrAdd(TEXT("Like"));
			// LikeCount=1: bonus por delta de likes desde el último commit (delta/100 * 1).
			// Se mantiene en 1 para que un stream con muchos likes no haga que un Like milestone supere a Gift/GiftCombo.
			LikeMods.Modifiers.Add(TEXT("LikeCount"), 1);
			LikeMods.Modifiers.Add(TEXT("TotalLikeCount"), 5);
		}
		{
			FEventTypeModifiers& LikeUserMods = SpecificModifiers.FindOrAdd(TEXT("LikeUser"));
			// LikeCount=1: E.LikeCount es el burst de likes de ESTE evento (1..15). Se baja de 5 a 1
			// para que un LikeUser (TIER 5, base más bajo) no supere a Gift/GiftCombo con un solo burst.
			LikeUserMods.Modifiers.Add(TEXT("LikeCount"), 1);
			// TotalLikeCount=0: E.TotalLikeCount es el acumulado de la SALA y crece sin tope durante el stream,
			// además es igual para todos los LikeUser del momento (no discrimina por usuario). Se desactiva (0)
			// para evitar que con el tiempo cualquier LikeUser aplaste a la monetización.
			LikeUserMods.Modifiers.Add(TEXT("TotalLikeCount"), 0);
		}
		{
			// Removed orphan SpecificModifiers for base Member event (not enqueued)
		}
		{
			FEventTypeModifiers& RoomUserMods = SpecificModifiers.FindOrAdd(TEXT("RoomUser"));
			RoomUserMods.Modifiers.Add(TEXT("ViewerCount"), 15);
		}
		{
			// RoomUserMilestone: puntos por cada nivel de hito de audiencia cruzado.
			// Va bajo la clave "RoomUserMilestone" (no "RoomUser") porque el evento es de ese tipo y el lookup es por EventType.
			FEventTypeModifiers& RoomUserMilestoneMods = SpecificModifiers.FindOrAdd(TEXT("RoomUserMilestone"));
			RoomUserMilestoneMods.Modifiers.Add(TEXT("Milestone"), 15);
		}

		// TTL seconds (jerarquía: Monetización [45-60s] > Engagement [25-30s] > Tiempo Real [6-8s] > Métricas [10-15s] > Snapshots [5-8s])
		// Orden: Chat, Gift, GiftCombo, Follow, Like, LikeUser, MemberIdentity, MemberNormalized, RoomUser, RoomUserMilestone, RoomUserTop1Change, Share
		TTLSeconds.Add(TEXT("Chat"), 8.0f);              // TIER 3 - Balance entre contexto conversacional y frescura (puede ser muy frecuente)
		TTLSeconds.Add(TEXT("Gift"), 45.0f);             // TIER 1 - Monetización directa, asegura que gifts grandes no se pierdan
		TTLSeconds.Add(TEXT("GiftCombo"), 60.0f);        // TIER 1 - Monetización crítica con sistema SB+Copias (TTL extendido adicional para Final debt)
		TTLSeconds.Add(TEXT("Follow"), 30.0f);           // TIER 2 - Compromiso permanente, muy valioso para crecimiento comunidad
		TTLSeconds.Add(TEXT("Like"), 10.0f);             // TIER 4 - Policy=Discard, Like milestones se procesan en tiempo real
		TTLSeconds.Add(TEXT("LikeUser"), 5.0f);          // TIER 5 - Like individual con gate, muy frecuente en salas pequeñas
		TTLSeconds.Add(TEXT("MemberIdentity"), 6.0f);    // TIER 3 - Evento más básico/frecuente con gate, TTL corto suficiente
		TTLSeconds.Add(TEXT("MemberNormalized"), 12.0f); // TIER 4 - Hito de miembros, similar a milestone pero sin Consolidate
		TTLSeconds.Add(TEXT("RoomUser"), 15.0f);         // TIER 5 - Snapshot periódico cada 6s con datos completos (ViewerCount + TopViewers[])
		TTLSeconds.Add(TEXT("RoomUserMilestone"), 8.0f); // TIER 4 - Milestone con reemplazo automático (solo 1 en cola, siempre el más fresco), TTL corto garantiza frescura
		TTLSeconds.Add(TEXT("RoomUserTop1Change"), 10.0f); // TIER 4 - Cambio de Top1, importante para competencia de gifters (cooldown=5s)
		TTLSeconds.Add(TEXT("Share"), 25.0f);            // TIER 2 - Alcance viral/marketing orgánico, menos frecuente que otros eventos
		TTLSeconds.Add(TEXT("ShareMilestone"), 15.0f);   // TIER 2 - Hito de shares (agregación), TTL medio para milestone

		// Max slots
		// Orden: Chat, Gift, GiftCombo, Follow, Like, LikeUser, MemberIdentity, MemberNormalized, RoomUser, RoomUserMilestone, RoomUserTop1Change, Share
		MaxSlots.Add(TEXT("Chat"), 30);
		MaxSlots.Add(TEXT("Gift"), 1000);            // Alta capacidad para múltiples regalos normales concurrentes
		MaxSlots.Add(TEXT("GiftCombo"), 1000);       // Alta capacidad para combos concurrentes
		MaxSlots.Add(TEXT("Follow"), 10);
		MaxSlots.Add(TEXT("Like"), 1);               // Milestone único - 1 evento activo
		MaxSlots.Add(TEXT("LikeUser"), 5);           // Con viewer gate - buffer moderado
		MaxSlots.Add(TEXT("MemberIdentity"), 10);
		MaxSlots.Add(TEXT("MemberNormalized"), 1);   // Hito único - estado actual
		MaxSlots.Add(TEXT("RoomUser"), 1);           // Snapshot actual - estado completo sin historial
		MaxSlots.Add(TEXT("RoomUserMilestone"), 1);  // Milestone único - estado actual del hito
		MaxSlots.Add(TEXT("RoomUserTop1Change"), 2); // Cambios recientes - transición suave
		MaxSlots.Add(TEXT("Share"), 10);
		MaxSlots.Add(TEXT("ShareMilestone"), 1);     // Milestone único - máximo 1 evento ShareMilestone en cola

        // Expire policies
        // Orden: Chat, Gift, GiftCombo, Follow, Like, LikeUser, MemberIdentity, MemberNormalized, RoomUser, RoomUserMilestone, RoomUserTop1Change, Share
        ExpirePolicies.Add(TEXT("Chat"), EEventExpirePolicy::Discard);
		ExpirePolicies.Add(TEXT("Gift"), EEventExpirePolicy::Discard);
		ExpirePolicies.Add(TEXT("GiftCombo"), EEventExpirePolicy::Discard);
		ExpirePolicies.Add(TEXT("Follow"), EEventExpirePolicy::Discard);
		ExpirePolicies.Add(TEXT("Like"), EEventExpirePolicy::Discard);
		ExpirePolicies.Add(TEXT("LikeUser"), EEventExpirePolicy::Discard);
		ExpirePolicies.Add(TEXT("MemberIdentity"), EEventExpirePolicy::Discard);
		ExpirePolicies.Add(TEXT("MemberNormalized"), EEventExpirePolicy::Discard);
		ExpirePolicies.Add(TEXT("RoomUser"), EEventExpirePolicy::Discard);
		ExpirePolicies.Add(TEXT("RoomUserMilestone"), EEventExpirePolicy::Discard); // FASE 1: Discard para invalidar handle correctamente
		ExpirePolicies.Add(TEXT("RoomUserTop1Change"), EEventExpirePolicy::Discard);
		ExpirePolicies.Add(TEXT("Share"), EEventExpirePolicy::Discard);
		ExpirePolicies.Add(TEXT("ShareMilestone"), EEventExpirePolicy::Discard); // Discard para invalidar handle correctamente

		// Gift defaults (normal + combo)
		GiftConfig.GiftComboRepeatSnapshotStep = 5;
		GiftConfig.GiftComboInactivitySeconds = 5.0f;
		GiftConfig.GiftComboClosedPruneSeconds = 60.0f;
		GiftConfig.GiftComboSnapshotPriorityBonus = 12; // [OBSOLETO/NO FUNCIONAL] no se lee en ningún .cpp; pendiente de eliminar
		GiftConfig.bProtectGiftComboSnapshots = true;

        // Fairness defaults
        AgingPointsPerSecond = 0.0f;
        AgingMaxBonus = 20;

        // Pump behavior defaults (generales)
        bPumpOnFirstEnqueue = true;
        bPumpAfterConfirm = true;
        bRecomputeOnPump = true;

    }

	/** [CONFIG] Base priority weights per event type (0-150 recomendado) */
	UPROPERTY(EditAnywhere, Category = "Prioritization", meta=(ToolTip="Peso base de prioridad por tipo de evento. Acepta cualquier int32 positivo PERO valores recomendados: 0-150. Score final = Base + Modificadores Comunes (hasta ~200) + Modificadores Específicos (ilimitado: ej. Gift 10000 diamantes = +1000) + Aging (hasta AgingMaxBonus). Defaults (orden: Chat, Gift, GiftCombo, Follow, Like, LikeUser, MemberIdentity, MemberNormalized, RoomUser, RoomUserMilestone, RoomUserTop1Change, Share): Chat=40, Gift=70, GiftCombo=80, Follow=60, Like=25, LikeUser=10, MemberIdentity=5, MemberNormalized=20, RoomUser=35, RoomUserMilestone=30, RoomUserTop1Change=50, Share=55. ⚠ Valores muy altos (>500): eventos siempre primero independiente de modificadores, puede causar starvation de otros eventos. ✓ Rango 0-150: balance flexible, modificadores tienen impacto significativo en competencia."))
	TMap<FName, int32> BaseWeights;

	/** [CONFIG] Common modifiers (additive scores for all events) */
	UPROPERTY(EditAnywhere, Category = "Prioritization", meta=(ToolTip="Modificadores comunes por identidad/rol del usuario (se suman al peso base)."))
	TMap<FName, int32> CommonModifiers;

	/** [CONFIG] Specific modifiers per event type */
	UPROPERTY(EditAnywhere, Category = "Prioritization", meta=(ToolTip="Modificadores específicos por tipo de evento (se suman al peso base y a los comunes)."))
	TMap<FName, FEventTypeModifiers> SpecificModifiers;

    

	/** [CONFIG] TTL in seconds per event type */
	UPROPERTY(EditAnywhere, Category = "Expiration", meta=(ToolTip="Tiempo de vida (TTL) en segundos por tipo de evento; tras expirar, aplica la política de expiración."))
	TMap<FName, float> TTLSeconds;

	/** [CONFIG] Maximum slots per event type (queue limits) */
	UPROPERTY(EditAnywhere, Category = "Limits", meta=(ToolTip="Número máximo de elementos en cola por tipo de evento."))
	TMap<FName, int32> MaxSlots;

		/** [CONFIG] Expire policies per event type */
	UPROPERTY(EditAnywhere, Category = "Expiration", meta=(ToolTip="Políticas de expiración por tipo de evento (Descartar o Consolidar) cuando el TTL vence."))
	TMap<FName, EEventExpirePolicy> ExpirePolicies;

    /** Bloques de configuración por tipo (colapsables en Editor) */
    UPROPERTY(EditAnywhere, Category = "Config", meta=(DisplayName="Like"))
    FLikeConfig Like;
    UPROPERTY(EditAnywhere, Category = "Config", meta=(DisplayName="Member"))
    FMemberConfig MemberConfig;
    UPROPERTY(EditAnywhere, Category = "Config", meta=(DisplayName="RoomUser"))
    FRoomUserConfig RoomUserConfig;
    UPROPERTY(EditAnywhere, Category = "Config", meta=(DisplayName="Gift"))
    FGiftConfig GiftConfig;
    UPROPERTY(EditAnywhere, Category = "Config", meta=(DisplayName="Share"))
    FShareConfig ShareConfig;

    // (GiftConfig contiene toda la configuración de combos Gift)

    // [Fairness] aging bonus settings
    UPROPERTY(EditAnywhere, Category = "Fairness", meta=(ClampMin="0.0", ClampMax="10.0", ToolTip="[Sistema de Equidad] Tasa de puntos de prioridad por segundo que acumulan los eventos esperando en cola (previene starvation). Default=0.0 (desactivado). Fórmula: AgingBonus = AgingPointsPerSecond × TiempoEsperaSegundos (limitado por AgingMaxBonus). ↓ 0.0: sin bonus de antigüedad, eventos mantienen prioridad original (puede causar starvation de eventos de baja prioridad en colas saturadas). ↑ 0.5-1.0: acumulación lenta, eventos antiguos adelantan gradualmente tras minutos de espera. ↑ 2.0-5.0: acumulación moderada, eventos antiguos compiten mejor tras ~30-60s. ↑ 5.0-10.0: acumulación agresiva, eventos antiguos priorizan rápidamente (puede invertir orden de prioridad original). ⚠️ Valores >10: acumulación extrema, no recomendado (eventos antiguos siempre primero independiente de importancia)."))
    float AgingPointsPerSecond;
    UPROPERTY(EditAnywhere, Category = "Fairness", meta=(ClampMin="0", ClampMax="100", ToolTip="[Sistema de Equidad] Tope máximo de puntos de prioridad que un evento puede ganar por antigüedad (cap del bonus de aging). Default=20. Fórmula: AgingBonus = Clamp(AgingPointsPerSecond × Segundos, 0, AgingMaxBonus). ↓ 0: anula el bonus de antigüedad completamente (incluso si AgingPointsPerSecond > 0), sistema de equidad desactivado. ↑ 10-20: bonus moderado, eventos antiguos ganan ventaja leve (~1-2 niveles de prioridad), balance entre orden original y equidad. ↑ 30-50: bonus significativo, eventos antiguos pueden superar varios niveles de prioridad (~3-5 tiers), equidad fuerte. ↑ 60-100: bonus máximo, eventos antiguos garantizan procesamiento eventual (pueden superar casi cualquier evento nuevo), equidad extrema. ⚠️ Valores >100: no recomendado (eventos antiguos siempre primero, distorsiona jerarquía de BaseWeights)."))
    int32 AgingMaxBonus;

    /** [CONFIG] Auto pump */
    UPROPERTY(EditAnywhere, Category = "Pump", meta=(ToolTip="Activa el bombeo automático periódico. Encendido: reduce latencia dependiendo de 'AutoPumpInterval'. Apagado: requiere bombeo manual/por eventos."))
    bool bEnableAutoPump;
    /** [CONFIG] Intervalo del Auto pump */
    UPROPERTY(EditAnywhere, Category = "Pump", meta=(ClampMin="0.016", ClampMax="10.0", ToolTip="[Bombeo Automático] Intervalo en segundos entre bombeos automáticos de eventos (timer periódico). Default=1.0s. Requiere bEnableAutoPump=true. Fórmula: cada AutoPumpInterval segundos ejecuta PumpOnceIfFree(). ↓ 0.016s (1 frame @ 60fps): bombeo casi continuo, latencia mínima (<20ms) pero máxima carga CPU (ejecuta pump cada tick), recomendado solo para debugging o demos interactivas. ↑ 0.1-0.5s: bombeo frecuente, latencia baja (100-500ms), balance para streams interactivos de alta frecuencia. ↑ 1.0-2.0s: bombeo moderado (default), latencia aceptable (~1s), balance óptimo para mayoría de casos (reduce carga sin perder reactividad). ↑ 3.0-5.0s: bombeo espaciado, mayor latencia pero menor carga CPU, útil para streams de baja prioridad o testing. ↑ 5.0-10.0s: bombeo muy espaciado, alta latencia (~10s retraso), solo para sistemas batch o baja prioridad. ⚠️ Valores <0.016s: no recomendado (sobrecarga extrema), valores >10s: latencia inaceptable para tiempo real."))
    float AutoPumpInterval;

    // [Pump behavior]
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pump", meta=(ToolTip="Bombea inmediatamente al encolar el primer evento en una cola vacía. Activado: mínima latencia del primer evento. Desactivado: espera a auto pump o acción manual."))
    bool bPumpOnFirstEnqueue;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pump", meta=(ToolTip="Tras confirmar un evento procesado, solicita otro bombeo si hay más en cola. Activado: encadena eventos de forma continua. Desactivado: deja pausas hasta el próximo pump."))
    bool bPumpAfterConfirm;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pump", meta=(ToolTip="Recalcula prioridades justo antes de cada bombeo. Activado: refleja cambios dinámicos (antigüedad, estados) al bombear. Desactivado: ahorra CPU pero usa prioridades potencialmente desactualizadas."))
    bool bRecomputeOnPump;

    /** Contenedor agrupado de toggles de eventos */
    UPROPERTY(EditAnywhere, Category = "Events", meta=(DisplayName="Event Toggles", ToolTip="Contenedor de toggles por tipo de evento."))
    FEventToggles EventToggles;

    /** Contenedor de configuración de evicción competitiva */
    UPROPERTY(EditAnywhere, Category = "Config", meta=(DisplayName="Eviction"))
    FEvictionConfig Eviction;
};
