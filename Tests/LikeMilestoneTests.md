# Pruebas de Like Milestones - Casos Sugeridos

## Configuración de Prueba
- **Step**: 500
- **bEmitAllLikeMilestonesOnLargeJump**: false (por defecto)

## Casos de Prueba

### 1. Progresión Normal: 100→480→520→1000
**Descripción**: Progresión gradual que cruza milestones
**Secuencia**:
- TotalLikeCount: 100 → No emite (< 500)
- TotalLikeCount: 480 → No emite (< 500)  
- TotalLikeCount: 520 → **EMITE milestone 500** (PreviousMilestone=0, StepsCrossed=1)
- TotalLikeCount: 1000 → **EMITE milestone 1000** (PreviousMilestone=500, StepsCrossed=1)

**Resultado Esperado**: 2 eventos Like emitidos (500, 1000)

### 2. Salto Grande: 1000→2600
**Descripción**: Salto que cruza múltiples milestones
**Secuencia**:
- TotalLikeCount: 1000 → Milestone 1000 ya emitido
- TotalLikeCount: 2600 → **EMITE milestone 2500** (PreviousMilestone=1000, StepsCrossed=3)

**Resultado Esperado**: 1 evento Like (solo 2500, no 1500 ni 2000)
**Nota**: StepsCrossed=3 indica que se cruzaron 3 hitos (1500, 2000, 2500)

### 3. Reset: 2500→50
**Descripción**: Caída brusca del contador (reset detectado)
**Secuencia**:
- TotalLikeCount: 2500 → Milestone 2500 ya emitido
- TotalLikeCount: 50 → **NO EMITE** (reset detectado, diff=2450 > step=500)
- Estado reseteado: Like_LastTotal=50, Like_LastCommittedTotal=0, Like_LastCommittedMilestone=0
- Próximo hito será 500

**Resultado Esperado**: No emisión, estado reseteado, log de warning

### 4. Reemplazo In-Place
**Descripción**: Múltiples eventos Like antes del dispatch
**Secuencia**:
- TotalLikeCount: 520 → Crea evento Like milestone 500, se encola
- TotalLikeCount: 750 → **REEMPLAZA** evento anterior con milestone 500 (mismo milestone)
- TotalLikeCount: 1200 → **REEMPLAZA** evento anterior con milestone 1000 (nuevo milestone)

**Resultado Esperado**: Solo 1 evento en cola (el último), reemplazo in-place exitoso

### 5. TTL / Evicción / Dispatch
**Descripción**: Validar invalidación de handles
**Casos**:
- **Dispatch exitoso**: Handle invalidado, commit realizado
- **Expiración por TTL**: Handle invalidado, sin commit
- **Evicción competitiva**: Handle invalidado, sin commit

**Resultado Esperado**: Handles siempre invalidados correctamente

## Validaciones Adicionales

### Cambio de Step en Runtime
**Secuencia**:
- Step=500, TotalLikeCount=1200 → Milestone 1000 emitido
- Cambiar Step a 300 en runtime
- TotalLikeCount=1500 → Nuevo milestone calculado con Step=300

**Resultado Esperado**: Recálculo correcto del estado, log de warning

### Valores Extremos
**Casos**:
- TotalLikeCount negativo → Clamped a 0
- Step <= 0 → Early return con warning
- Step muy grande → Funciona correctamente

**Resultado Esperado**: Validaciones robustas, sin crashes

## Logs Esperados

### Inicialización
```
[Like Milestone] Initialized: TotalLikeCount=100, CommittedMilestone=0, Step=500
```

### Milestone Detectado
```
[Like Milestone] Milestone detected: 0 → 500 (StepsCrossed=1, DeltaLikes=520, Elapsed=0.00s, LPS=0.00)
```

### Reset Detectado
```
[Like Milestone] Reset detected: TotalLikeCount dropped from 2500 to 50 (diff=2450 > step=500). Resetting state.
```

### Cambio de Step
```
[Like Milestone] Step changed from 500 to 300. Recalculating state.
```

### Reemplazo In-Place
```
[Like Milestone] Replacing existing Like milestone in queue (pos=5)
```

## Estado Final Esperado

Después de todas las pruebas, el estado debe ser consistente:
- `bMilestoneInitialized = true`
- `Like_LastTotal` = último TotalLikeCount procesado
- `Like_LastCommittedTotal` = TotalLikeCount del último evento despachado
- `Like_LastCommittedMilestone` = último milestone realmente despachado
- `bHasLikeMilestoneInQueue` = false (si no hay eventos pendientes)

## Invariantes a Verificar

1. **Monotonía**: Los milestones emitidos son siempre crecientes
2. **Unicidad**: Máximo 1 evento Like en cola en cualquier momento
3. **Consistencia**: PreviousMilestone siempre refiere al último despachado
4. **Robustez**: Sistema maneja resets, cambios de configuración y valores extremos
5. **Aislamiento**: LikeUser no se ve afectado por estos cambios