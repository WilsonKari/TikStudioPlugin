# Plan de Testeo — 13 Flujos del EventQueue

> Objetivo: validar que **cada uno de los 13 flujos de salida** emite correctamente y que
> la **prioridad / orden de la cola** sale como se espera, empezando por los flujos simples
> y terminando en el más complejo (Gift / GiftCombo).
>
> Metodología: arrancas el sistema con valores **por default**, pero **activando solo los toggles
> del grupo que toca** (el resto OFF). Tras cada fase, pegas el log de consola en `logs.md` y lo
> reviso.

---

## 0. Convenciones y preparación (leer una vez)

### 0.1 Cómo procesa la cola
- Se procesa **1 evento a la vez**: se saca el de mayor prioridad, se **bloquea** (lock), se despacha al Blueprint, y hasta que no llamas `ConfirmEventProcessed` no avanza al siguiente.
- Tu variable **`Duration event processed`** (delay del Blueprint que simula el habla del TTS) es la que mantiene el lock ocupado. Es tu herramienta principal de testeo:
  - **Delay LARGO (ej. 6–10 s)** → la cola se llena mientras "habla" → sirve para **probar el ORDEN/prioridad** (observas en qué orden se despachan).
  - **Delay CORTO (ej. 0.1–0.5 s)** → alto throughput → sirve para **probar la lógica de cada flujo** (milestones, combos, gates).

### 0.2 Fórmula de prioridad
```
Final = BaseWeight + ModificadoresComunes + ModificadoresEspecíficos + Aging
```
- **Aging**: con `AgingPointsPerSecond = 0.0` (default) → **Aging = 0** siempre. **Mantenlo en 0 durante el testeo** para que los scores sean deterministas y fáciles de verificar.
- **Comunes**: se aplican a **todos los tipos EXCEPTO `Like` milestone** (a Like milestone se le fuerza Common=0).

### 0.3 La línea de log más importante
Cada evento que entra a la cola imprime su desglose exacto:
```
ComputePriority for <Tipo>: Base=<b>, Common=<c>, Specific=<s>, Aging=<a>, Final=<f>
```
**No necesitas calcular a mano**: comparas `Base/Common/Specific/Final` del log contra lo esperado. Otras líneas útiles:
- `[EventQueue] Dispatched <Tipo> Id=... ` → el evento que salió (el orden de estas líneas = orden de la cola).
- Gates: `[LikeUser] ViewerGate: BLOCK/ALLOW (VC=.. Th=..)`, equivalentes para MemberIdentity/Share.
- Milestones: `[Like Milestone] Enqueued ... Priority=..`, `[MemberNormalized] COMMIT`, `[ShareMilestone] DISPATCH/COMMIT`, `[EventQueue] RoomUserMilestone EMIT/REPLACED`.

### 0.4 Tabla de pesos base (default)
| Tipo (salida) | Base | Tier |
|---|---|---|
| GiftCombo | 80 | 1 |
| Gift | 70 | 1 |
| Follow | 60 | 2 |
| Share | 55 | 2 |
| RoomUserTop1Change | 50 | 2 |
| ShareMilestone | 50 | 2 |
| Chat | 40 | 3 |
| RoomUser (snapshot) | 35 | 3 |
| RoomUserMilestone | 30 | 3 |
| Like (milestone) | 25 | 4 |
| MemberNormalized | 20 | 4 |
| LikeUser | 10 | 5 |
| MemberIdentity | 5 | 5 |

### 0.5 Modificadores comunes (default)
| Condición | Aporte |
|---|---|
| `bIsModerator` | +15 |
| `bIsSubscriber` | +10 |
| `bIsNewGifter` | +15 |
| `FollowRole` (0/1/2) | `CommonModifiers["FollowRole"] * rol` (default 5) → rol0=+0, rol1=+5, rol2=+10. Aplica a **todos** los tipos con comunes |
| `TopGifterRank` (1–5) | `(6 - rank) * 4` → rank1=+20, rank2=+16, rank3=+12, rank4=+8, rank5=+4 |
| `GifterLevel` | `1 * nivel` (cap 75 en Chat/LikeUser/Gift/GiftCombo, 20 en el resto) |
| `TeamMemberLevel` | `1 * nivel` (cap 100 en Chat/LikeUser/Gift/GiftCombo, 20 en el resto) |

### 0.6 Modificadores específicos (default)
| Tipo | Fórmula específica |
|---|---|
| Gift | `(DiamondCount/10)*5` + `(bRepeatEnd?10:0)` + `RepeatCount*2` |
| GiftCombo | `(DiamondTotal/10)*1` + `RepeatCountTotal*3` + `(bIsFinal?15:0)` |
| Chat | `(bHasCommand?20:0)` (comando = Comment empieza con `!`) |
| Like (milestone) | `(TotalLikeCount/1000)*5` + si milestone: `10*max(1,steps-1)` + `(delta/100)*1` |
| LikeUser | `LikeCount*1` + `(TotalLikeCount/1000)*0` |
| RoomUser (snapshot) | `(ViewerCount/25)*15` |
| RoomUserMilestone | `(Milestone/25)*15` |
| Follow / Share / ShareMilestone / Member* / Top1Change | 0 (solo Base + Comunes) |

### 0.7 Toggles (en el editor: categoría **Events**)
| Grupo (DisplayName) | Campos bool |
|---|---|
| Chat | `bEnableChat` |
| Gift | `bEnableGift`, `bEnableGiftCombo` |
| Follow | `bEnableFollow` |
| Like | `bEnableLike` (milestone), `bEnableLikeUser` |
| Member | `bEnableMemberNormalized`, `bEnableMemberIdentity` |
| Share | `bEnableShare`, `bEnableShareMilestone` |
| RoomUser | `bEnableRoomUser` (snapshot), `bEnableRoomUserMilestone`, `bEnableRoomUserTop1Change` |

### 0.8 Funciones de entrada (Blueprint) — 7 inputs → 13 salidas
`EnqueueChatEvent`, `EnqueueGiftEvent`, `EnqueueFollowEvent`, `EnqueueLikeEvent`, `EnqueueMemberEvent`, `EnqueueRoomUserEvent`, `EnqueueShareEvent`.

### 0.9 Protocolo por fase
1. Pon los toggles indicados (resto OFF). Confirma `AgingPointsPerSecond=0`.
2. Ajusta `Duration event processed` según indique la fase (largo/corto).
3. Envía los inputs del caso.
4. Copia **todo** el log de consola en `logs.md` (incluye las líneas `ComputePriority`, `Dispatched`, y las de gate/milestone/commit).
5. Avísame y lo reviso antes de pasar a la siguiente fase.

---

## GRUPO 1 — Passthrough simples: **Chat + Follow**
Flujos cubiertos: **Chat**, **Follow**. Son los más simples (sin estado interno, sin gates, sin agregación).

**Toggles ON:** `Chat.bEnableChat`, `Follow.bEnableFollow`. **Resto OFF.**
**Delay TTS:** corto (0.2 s) para casos 1.1–1.5; **largo (8 s)** para el caso 1.6 (orden).

| # | Input | Esperado (verificar en log) |
|---|---|---|
| 1.1 | Chat, usuario normal, `Comment="hola"` | `Chat Base=40 Common=0 Specific=0 Final=40` |
| 1.2 | Chat, `bIsModerator=true` | `Common=15 Final=55` |
| 1.3 | Chat, `bIsSubscriber=true` | `Common=10 Final=50` |
| 1.4 | Chat, `Comment="!sorteo"` (usuario normal) | `Specific=20 Final=60` + `bHasCommand=true` |
| 1.5 | Chat, mod + comando (`!x`, `bIsModerator=true`) | `Common=15 Specific=20 Final=75` |
| 1.6 | **Shadow merge:** 2 Chats seguidos del **mismo `UniqueId`** | 1 solo evento, `MergedCount=2` (no dos eventos en cola) |
| 1.7 | Follow, usuario normal | `Follow Base=60 Common=0 Final=60` |
| 1.8 | Follow, `bIsModerator=true` | `Common=15 Final=75` |
| 1.9 | **Orden (delay 8 s):** dispara Chat plano (40) y Follow plano (60) casi a la vez | Se despacha **Follow primero** (60 > 40), luego Chat |

**Criterio de éxito:** scores exactos, shadow merge funciona, y en 1.9 el orden de las líneas `Dispatched` respeta la prioridad.

---

## GRUPO 2 — Individuales con gate de audiencia: **LikeUser + MemberIdentity + Share**
Flujos cubiertos: **LikeUser**, **MemberIdentity**, **Share** (atómico, NO milestone). Los tres son eventos individuales 1:1 que dependen del `ViewerCount` (gate fail-closed). Se testea en 2 sub-fases: primero **sin gate** (prioridad pura), luego **con gate** (comportamiento de filtro).

> ⚠️ **Aislar el Share atómico:** el input `EnqueueShareEvent` alimenta DOS flujos (Share y ShareMilestone). Para testear **solo** el Share atómico aquí, mantén **`Share.bEnableShare=ON` y `Share.bEnableShareMilestone=OFF`** (el milestone se prueba en el Grupo 3).
>
> ⚠️ **Logs de gate en `Verbose`:** las líneas `[LikeUser/MemberIdentity/Share] ViewerGate: ALLOW/BLOCK/fail-closed` se emiten con severidad **Verbose**. Para verlas, ejecuta en la consola del editor: `Log LogTemp Verbose` (también habilita las líneas `ComputePriority`). Para volver al modo limpio: `Log LogTemp Log`.

### Fase 2A — Sin gate (prioridad pura)
**Toggles ON:** `Like.bEnableLikeUser`, `Member.bEnableMemberIdentity`, `Share.bEnableShare`. **Resto OFF** (incluido `Share.bEnableShareMilestone=OFF`).
**Config:** `Like.bEnableLikeUserViewerGate=false`, `Member.bEnableMemberIdentityViewerGate=false`, `Share.bEnableShareViewerGate=false` (ojo: este viene **ON** por default, hay que apagarlo).
**Delay:** corto.

| # | Input | Esperado |
|---|---|---|
| 2A.1 | LikeUser: `LikeCount=10`, usuario normal | `LikeUser Base=10 Common=0 Specific=10 Final=20` (10*1; TotalLikeCount aporta 0) |
| 2A.2 | LikeUser: `LikeCount=15`, `bIsSubscriber=true` | `Common=10 Specific=15 Final=35` |
| 2A.3 | MemberIdentity: `ActionId=1` (join), usuario normal | `MemberIdentity Base=5 Common=0 Final=5` |
| 2A.4 | MemberIdentity, `bIsModerator=true` | `Common=15 Final=20` |
| 2A.5 | Share, usuario normal | `Share Base=55 Common=0 Specific=0 Final=55` |
| 2A.6 | Share, `bIsModerator=true` | `Common=15 Final=70` |
| 2A.7 | Share, `bIsSubscriber=true` + `TopGifterRank=1` | `Common=10+20=30 Final=85` |

### Fase 2B — Con gate (filtro por audiencia)
**Toggles ON:** los mismos **+ `RoomUser.bEnableRoomUser`** (para sembrar el `ViewerCount`). Mantén `Share.bEnableShareMilestone=OFF`.
**Config:** `bEnableLikeUserViewerGate=true` (Th=25), `bEnableMemberIdentityViewerGate=true` (Th=25), `Share.bEnableShareViewerGate=true` (Th=25).
**Delay:** corto.

| # | Secuencia | Esperado |
|---|---|---|
| 2B.1 | LikeUser **sin** haber enviado RoomUser aún | `[LikeUser] ViewerGate: VC unavailable -> fail-closed (BLOCK)` → **no se encola** |
| 2B.2 | Envía RoomUser con `ViewerCount=10`, luego LikeUser | `[LikeUser] ViewerGate: ALLOW (VC=10 ≤ 25)` → se encola |
| 2B.3 | Envía RoomUser con `ViewerCount=40`, luego LikeUser | `[LikeUser] ViewerGate: BLOCK (VC=40 > 25)` → **no se encola** |
| 2B.4 | Igual que 2B.2/2B.3 pero con MemberIdentity | mismo patrón ALLOW/BLOCK (ojo: MemberIdentity usa histéresis, Exit=Enter+3) |
| 2B.5 | Share **sin** RoomUser previo | `[Share] ViewerGate: VC unavailable -> fail-closed` → **no se encola** |
| 2B.6 | RoomUser `ViewerCount=10`, luego Share | `[Share] ViewerGate: ALLOW (VC=10 ≤ 25)` → se encola (`Share Final=55`) |
| 2B.7 | RoomUser `ViewerCount=40`, luego Share | `[Share] ViewerGate: BLOCK (VC=40 > 25)` → **no se encola** |

> Nota: Share lee el VC desde `RoomUserState.RUS_LastRawVC` (lo siembra cualquier `EnqueueRoomUserEvent`), igual que LikeUser. Su gate es **simple** (sin histéresis, a diferencia de MemberIdentity): bloquea estrictamente cuando `VC > Th`.

**Criterio de éxito:** sin gate los scores salen exactos (Share=55 base); con gate se respeta ALLOW/BLOCK según VC, y el fail-closed bloquea los tres flujos cuando no hay VC.

---

## GRUPO 3 — Milestones por conteo: **Like (milestone) + MemberNormalized + ShareMilestone**
Flujos cubiertos: **Like milestone**, **MemberNormalized**, **ShareMilestone**. Los tres **agregan un contador** y emiten al cruzar un paso, con **reemplazo in-place** (máx. 1 en cola por tipo).

**Toggles ON:** `Like.bEnableLike`, `Member.bEnableMemberNormalized`, `Share.bEnableShareMilestone`. **Resto OFF.**
> El milestone es independiente del Share atómico (el código procesa ShareMilestone aunque `bEnableShare=OFF`). Deja **`Share.bEnableShare=OFF`** aquí para que la cola no se mezcle con Shares atómicos (ya cubiertos en el Grupo 2).

**Config relevante:** `Like.LikeMilestoneStep=500`, `Member.MemberNormalizedJoinMilestone=20`, `Share.ShareMilestoneStep=10`.
**Delay:** corto para 3.x de emisión; largo para 3.Z (reemplazo).

### Like milestone (step 500)
| # | Input (TotalLikeCount creciente) | Esperado |
|---|---|---|
| 3.1 | TotalLikeCount=100, luego 300, luego 480 | **No** emite milestone (no cruza 500). Primer evento solo inicializa estado |
| 3.2 | TotalLikeCount=520 | Emite milestone **500**. `Like Base=25 Common=0` + Specific = `(520/1000)*5=0` + `10*max(1,1-1)=10` + `(delta/100)*1`. Verifica `LikeMilestone=500` |
| 3.3 | TotalLikeCount=1600 (salto de 2 pasos) | Emite milestone **1500**, `LikeStepsCrossed`≈3, Specific incluye `(1600/1000)*5=5` + bonus de steps |

> Nota: a Like milestone **no** se le aplican comunes (Common=0 siempre).

### MemberNormalized (hito cada 20 joins)
| # | Input | Esperado |
|---|---|---|
| 3.4 | 19 Member joins (`ActionId=1`) | No emite milestone aún |
| 3.5 | join #20 | Emite MemberNormalized, `CurrentMilestone=20`, `Base=20` |
| 3.6 | hasta join #40 | Emite milestone `40` |

### ShareMilestone (cada 10 shares)
| # | Input | Esperado |
|---|---|---|
| 3.7 | 9 Shares | No emite milestone |
| 3.8 | share #10 | Emite ShareMilestone, `CurrentMilestone=10`, `Base=50` |

### Reemplazo in-place
| # | Secuencia (delay LARGO para mantener cola ocupada) | Esperado |
|---|---|---|
| 3.Z | Provoca 2 milestones del mismo tipo seguidos **mientras la cola está ocupada** | Solo **1** evento de ese tipo en cola (el segundo **reemplaza** al primero, no se acumulan) |

**Criterio de éxito:** los milestones se emiten en el paso correcto, los campos (milestone/steps/delta) son coherentes, y nunca hay 2 del mismo tipo en cola.

---

## GRUPO 4 — Audiencia (familia RoomUser): **RoomUser + RoomUserMilestone + RoomUserTop1Change**
Flujos cubiertos: **RoomUser (snapshot)**, **RoomUserMilestone**, **RoomUserTop1Change**. Los tres derivan del **mismo input** `EnqueueRoomUserEvent` (ViewerCount + TopViewers[] + TopGifterRank).

**Toggles ON:** `RoomUser.bEnableRoomUser`, `RoomUser.bEnableRoomUserMilestone`, `RoomUser.bEnableRoomUserTop1Change`. **Resto OFF.**
**Config relevante:** `RoomUserMilestoneStep=25`, `RoomUserSmartSwitchMaxVC=25` (+`bEnableRoomUserSmartSwitch=true`), `RoomUserSnapshotPeriodSeconds=20`, `RoomUserTop1CooldownSeconds=0`, `RoomUserTop1MinTopViewers=3`.
**Delay:** corto.

| # | Input | Esperado |
|---|---|---|
| 4.1 | RoomUser `ViewerCount=10` (con ≥3 TopViewers) | **RoomUser snapshot** emitido (VC≤25). `Base=35` + Specific `(10/25)*15=0`. Posible RoomUserTop1Change (primer Top1) |
| 4.2 | RoomUser `ViewerCount=26` | **RoomUserMilestone 25** (`Base=30` + `(25/25)*15=15` → Final 45). Snapshot **bloqueado** (VC>25 por smart switch) |
| 4.3 | RoomUser `ViewerCount=51` | **RoomUserMilestone 50** (`Base=30` + `(50/25)*15=30` → Final 60) |
| 4.4 | RoomUser `ViewerCount=120` | **RoomUserMilestone** mayor (`(milestone/25)*15`). Verifica el número de hito |
| 4.5 | RoomUser con TopViewers donde **cambia el Top1** (nuevo nombre en pos 0, ≥3 viewers) | **RoomUserTop1Change** emitido, `Base=50` |
| 4.6 | Reenvía mismo Top1 sin cambios | **No** emite Top1Change (no hubo cambio) |

> Interacción clave: el snapshot (smart switch) se **bloquea con VC>25**, mientras los milestones disparan justo en VC≥25. Es esperado que no salgan ambos al mismo VC alto.

**Criterio de éxito:** snapshot respeta el smart switch y el período; los milestones escalan `(milestone/25)*15`; Top1Change solo al cambiar el Top1 real.

---

## GRUPO 5 — Monetización (lo más complejo): **Gift + GiftCombo**
Flujos cubiertos: **Gift** (normal) y **GiftCombo** (snapshots de combo). Testeo **estricto** simulando patrones de combos para validar que **se respeta la cola** y no se pierden datos.

**Toggles ON:** `Gift.bEnableGift`, `Gift.bEnableGiftCombo`. **Resto OFF.**
**Config relevante:** `GiftComboRepeatSnapshotStep=5`, `GiftComboInactivitySeconds=5`, `bProtectGiftComboSnapshots=true`.
**Convención de input:** Gift normal → `GiftType=0`. Gift de combo → `GiftType=1` + mismo `GroupId` para toda la serie; `bRepeatEnd=true` en el último.

### 5A — Gift normal (sin combo)
| # | Input | Esperado |
|---|---|---|
| 5A.1 | Gift `GiftType=0`, `DiamondCount=10`, `RepeatCount=1` | `Gift Base=70` + Specific `(10/10)*5=5` + `1*2=2` → Final 77 |
| 5A.2 | Gift `GiftType=0`, `DiamondCount=100`, `RepeatCount=1` | Specific `(100/10)*5=50 + 2=52` → Final 122 |
| 5A.3 | Gift normal + `bIsModerator=true` | `Common=15` se suma al anterior |

### 5B — GiftCombo (un combo completo)
**Delay:** corto para ver toda la secuencia.
| # | Secuencia (mismo `GroupId="G1"`, `GiftType=1`) | Esperado |
|---|---|---|
| 5B.1 | gift rc=1 → rc=2 → ... → rc=5 | Al llegar a `rc=5` (= `GiftComboRepeatSnapshotStep`) emite **snapshot intermedio**. `GiftCombo Base=80` + Specific `(DiamondTotal/10)*1 + RepeatCountTotal*3` |
| 5B.2 | sigue rc=6..10 | Otro snapshot intermedio en rc=10 |
| 5B.3 | último gift con `bRepeatEnd=true` | **Snapshot FINAL** (`bIsFinal=true` → Specific +15). `bRepeatEnd` en el OutData |
| 5B.4 | no enviar nada 5 s tras un combo abierto sin `bRepeatEnd` | Cierre **por inactividad** (`bClosedByInactivity=true`) a los `GiftComboInactivitySeconds` |

### 5C — Respeto de la cola bajo presión (estricto)
**Delay:** **LARGO (8–10 s)** para forzar acumulación.
| # | Secuencia | Esperado |
|---|---|---|
| 5C.1 | Mientras un evento "habla" (lock), envía un combo de `G1` rc=1..8 | El combo acumula; al liberarse el lock, se despacha el snapshot del combo. **No se pierden** repeticiones |
| 5C.2 | Dos combos simultáneos `G1` y `G2` intercalados | Cada GroupId mantiene su propio combo (no se mezclan); ambos terminan emitiendo su FINAL |
| 5C.3 | Un combo `G1` + un Gift normal grande (`DiamondCount=500`) compitiendo | Verifica el orden por prioridad (GiftCombo 80 vs Gift normal 70 + diamantes); revisa que el combo no sea evictado (`bProtectGiftComboSnapshots`) |

**Criterio de éxito:** snapshots intermedios cada 5 repeticiones, snapshot final con +15, cierre por inactividad a los 5 s, y **cero pérdida de repeticiones/diamantes** aún con la cola saturada.

---

## FASE FINAL — Jerarquía global (todos los flujos juntos)
**Toggles:** **TODOS ON.** **Delay:** **LARGO (10 s)** para que se acumule la cola.

**Procedimiento:** con un evento "hablando", dispara en ráfaga 1 input de cada tipo (un Gift combo, un Gift normal, un Follow, un Share, un Chat, un RoomUser que genere milestone, un Like milestone, etc.). Luego deja que la cola se vacíe sola.

**Esperado (orden de las líneas `Dispatched`, asumiendo usuarios planos):**
```
GiftCombo (80+) > Gift (70+) > Follow (60) > Share (55) > {RoomUserTop1Change 50, ShareMilestone 50}
> Chat (40) > RoomUser (35) > RoomUserMilestone (30) > Like (25+) > MemberNormalized (20) > LikeUser (10+) > MemberIdentity (5)
```
**Criterio de éxito:** el orden de despacho respeta los scores (recuerda que modificadores y diamantes pueden alterar el orden relativo — usa el `Final=` del log como verdad).

---

## Checklist de avance
- [ ] Grupo 1 — Chat + Follow
- [ ] Grupo 2A — LikeUser + MemberIdentity + Share (sin gate)
- [ ] Grupo 2B — LikeUser + MemberIdentity + Share (con gate)
- [ ] Grupo 3 — Like / MemberNormalized / ShareMilestone (milestones)
- [ ] Grupo 4 — RoomUser / RoomUserMilestone / RoomUserTop1Change
- [ ] Grupo 5A — Gift normal
- [ ] Grupo 5B — GiftCombo (combo completo)
- [ ] Grupo 5C — Respeto de cola bajo presión
- [ ] Fase Final — Jerarquía global

> Tras cada grupo: pega el log en `logs.md` y avísame para revisarlo antes de continuar.
