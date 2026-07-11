# TikStudioPlugin

## Requisitos / Instalación

- **Unreal Engine**: 5.7
- **Dependencia**: `TikFinityPlugin` debe estar habilitado en el proyecto (declarado en `TikStudioPlugin.uplugin`)
- **Instalación**: copiar esta carpeta a `YourProject/Plugins/TikStudioPlugin`, regenerar archivos de proyecto y compilar

## 📋 **Descripción General**

TikStudioPlugin es un plugin integral para Unreal Engine 5.7 que proporciona procesamiento avanzado de eventos de TikTok y funcionalidad de Director de IA. Este plugin está diseñado para trabajar de manera fluida con TikFinityPlugin para crear experiencias de juego dinámicas y receptivas basadas en la actividad del stream de TikTok en tiempo real.

## 🧩 Arquitectura General

El plugin se organiza en **módulos**:

### 🎬 Director (existente)
- **Rol**: Preprocesa los eventos provenientes de TikTok/TikFinity, aplica lógica de "dirección" (prioritización contextual y decisiones de alto nivel) y los entrega a sistemas del juego (p. ej., `DirectorManager`).
- **Entrada/Salida**: Recibe eventos nativos del handler; emite señales/acciones dirigidas al juego.
#### **Status Monitoring Pattern**
```blueprint
Event Tick → Get Current Activity → Update UI Progress Bar
```

## 🧵 Nodos del EventQueue (Sistema Nuevo)

> **Nota**: Este es el **sistema de cola priorizada** para narración serial con TTS. Gestiona TTL, slots, normalización de Likes/RoomUser y emisión de eventos procesados. Diferente del Director (que analiza actividad global), este sistema emite **un evento a la vez** para consumo secuencial.

### **🔧 Core EventQueue Nodes**

#### **1. Get Dispatcher**
- **Función**: Obtiene la referencia al dispatcher para bindear eventos procesados
- **Pines de entrada**:
  - `Target` (Tik Studio Event Queue): Referencia a la cola de eventos
- **Pines de salida**:
  - `Return Value` (Tik Studio Event Dispatcher): Referencia al dispatcher
- **Uso típico**: Obtener dispatcher para bindear OnChatEventProcessed, etc.

#### **2. Pump Once If Free**
- **Función**: Si la cola no tiene evento locked, hace Dequeue + Dispatch en un paso
- **Pines de entrada**:
  - `Target` (Tik Studio Event Queue): Referencia a la cola de eventos
- **Pines de salida**:
  - `Return Value` (Boolean): True si logró despachar un evento
- **Uso típico**: Llamar cuando TTS esté idle para procesar el siguiente evento

#### **3. Confirm Event Processed**
- **Función**: Confirma procesamiento del evento locked y lo remueve de la cola
- **Pines de entrada**:
  - `Target` (Tik Studio Event Queue): Referencia a la cola de eventos
  - `Event Id` (Guid): ID del evento a confirmar
- **Pines de salida**:
  - `Return Value` (Tik Studio Error Status): Estado del resultado
- **Uso típico**: Llamar al finalizar TTS para liberar el lock y permitir el siguiente evento

#### **4. Get Queued Count**
- **Función**: Obtiene el número actual de eventos en la cola
- **Pines de entrada**:
  - `Target` (Tik Studio Event Queue): Referencia a la cola de eventos
- **Pines de salida**:
  - `Return Value` (Integer): Cantidad de eventos en cola
- **Uso típico**: UI de debug, indicadores de backlog

#### **5. Is Free**
- **Función**: Verifica si la cola está libre (sin evento locked)
- **Pines de entrada**:
  - `Target` (Tik Studio Event Queue): Referencia a la cola de eventos
- **Pines de salida**:
  - `Return Value` (Boolean): True si no hay evento locked
- **Uso típico**: Condiciones para llamar PumpOnceIfFree

#### **6. Has Locked Event**
- **Función**: Verifica si hay un evento actualmente locked para procesamiento
- **Pines de entrada**:
  - `Target` (Tik Studio Event Queue): Referencia a la cola de eventos
- **Pines de salida**:
  - `Return Value` (Boolean): True si hay evento locked
- **Uso típico**: Debugging, verificación de estado

#### **7. Get Locked Event Id**
- **Función**: Obtiene el ID del evento actualmente locked
- **Pines de entrada**:
  - `Target` (Tik Studio Event Queue): Referencia a la cola de eventos
- **Pines de salida**:
  - `Return Value` (Guid): ID del evento locked (o Guid vacío si ninguno)
- **Uso típico**: Debugging, tracking de eventos en proceso

### **� Convenience Input Nodes**

Estos nodos proporcionan una interfaz más clara para entrada de eventos por tipo específico:

#### **8. Enqueue Chat Event Json**
- **Función**: Encola un evento de chat desde JSON
- **Pines de entrada**:
  - `Target` (Tik Studio Event Queue): Referencia a la cola de eventos
  - `Payload Json` (String): JSON del evento de chat
- **Pines de salida**: Ninguno
- **Uso típico**: Entrada directa de eventos de chat

#### **9. Enqueue Gift Event Json**
- **Función**: Encola un evento de regalo desde JSON
- **Pines de entrada**:
  - `Target` (Tik Studio Event Queue): Referencia a la cola de eventos
  - `Payload Json` (String): JSON del evento de regalo
- **Pines de salida**: Ninguno
- **Uso típico**: Entrada directa de eventos de regalo

#### **10. Enqueue Follow Event Json**
- **Función**: Encola un evento de follow desde JSON
- **Pines de entrada**:
  - `Target` (Tik Studio Event Queue): Referencia a la cola de eventos
  - `Payload Json` (String): JSON del evento de follow
- **Pines de salida**: Ninguno
- **Uso típico**: Entrada directa de eventos de follow

#### **11. Enqueue Like Event Json**
- **Función**: Encola un evento de like desde JSON
- **Pines de entrada**:
  - `Target` (Tik Studio Event Queue): Referencia a la cola de eventos
  - `Payload Json` (String): JSON del evento de like
- **Pines de salida**: Ninguno
- **Uso típico**: Entrada directa de eventos de like

#### **12. Enqueue Member Event Json**
- **Función**: Encola un evento de member desde JSON
- **Pines de entrada**:
  - `Target` (Tik Studio Event Queue): Referencia a la cola de eventos
  - `Payload Json` (String): JSON del evento de member
- **Pines de salida**: Ninguno
- **Uso típico**: Entrada directa de eventos de member

#### **13. Enqueue Room User Event Json**
- **Función**: Encola un evento de room user desde JSON
- **Pines de entrada**:
  - `Target` (Tik Studio Event Queue): Referencia a la cola de eventos
  - `Payload Json` (String): JSON del evento de room user
- **Pines de salida**: Ninguno
- **Uso típico**: Entrada directa de eventos de room user

#### **14. Enqueue Share Event Json**
- **Función**: Encola un evento de share desde JSON
- **Pines de entrada**:
  - `Target` (Tik Studio Event Queue): Referencia a la cola de eventos
  - `Payload Json` (String): JSON del evento de share
- **Pines de salida**: Ninguno
- **Uso típico**: Entrada directa de eventos de share

### **�📡 Event Processing Delegates**

Estos delegates se emiten desde el Dispatcher después de DispatchLockedEvent. Bindéalos para recibir eventos procesados.

#### **On Chat Event Processed**
- **Función**: Se emite cuando se procesa un evento de chat
- **Parámetros del Delegate**:
  - `Data` (TikTok Chat Event): Datos del evento de chat
  - `Event Id` (Guid): ID único del evento
- **Uso típico**: Generar prompts de chat para TTS

#### **On Follow Event Processed**
- **Función**: Se emite cuando se procesa un evento de follow
- **Parámetros del Delegate**:
  - `Data` (TikTok Follow Event): Datos del evento de follow
  - `Event Id` (Guid): ID único del evento
- **Uso típico**: Anuncios de nuevos seguidores

#### **On Gift Event Processed**
- **Función**: Se emite cuando se procesa un evento de regalo
- **Parámetros del Delegate**:
  - `Data` (TikTok Gift Event): Datos del evento de regalo
  - `Event Id` (Guid): ID único del evento
- **Uso típico**: Celebraciones de regalos, combos

#### **On Like Event Processed**
- **Función**: Se emite cuando se procesa un evento de like (puede ser batch normalizado)
- **Parámetros del Delegate**:
  - `Data` (TikTok Like Event): Datos del evento de like (LikeCount, TotalLikeCount)
  - `Event Id` (Guid): ID único del evento
- **Uso típico**: Anuncios de likes acumulados

#### **On Member Event Processed**
- **Función**: Se emite cuando se procesa un evento de member (join/upgrade/renewal)
- **Parámetros del Delegate**:
  - `Data` (TikTok Member Event): Datos del evento de member
  - `Event Id` (Guid): ID único del evento
- **Uso típico**: Bienvenidas de nuevos miembros

#### **On Room User Event Processed**
- **Función**: Se emite cuando se procesa un milestone de viewer count
- **Parámetros del Delegate**:
  - `Data` (TikTok Room User Event): Datos del milestone (ViewerCount)
  - `Event Id` (Guid): ID único del evento
- **Uso típico**: Anuncios de hitos de espectadores

#### **On Share Event Processed**
- **Función**: Se emite cuando se procesa un evento de share
- **Parámetros del Delegate**:
  - `Data` (TikTok Share Event): Datos del evento de share
  - `Event Id` (Guid): ID único del evento
- **Uso típico**: Celebraciones de shares

### **💡 Usage Patterns for EventQueue**

#### **TTS Integration Pattern**
```blueprint
// Setup
Event Begin Play → Create EventQueue → Get Dispatcher → Bind OnChatEventProcessed

// Main Loop
While (TTS Is Idle):
  If (EventQueue->IsFree()):
    EventQueue->PumpOnceIfFree()
    
// Event Handler
On Chat Event Processed:
  Format Text (Prompt with Data) → Send to LLM/TTS → Play Audio
  On TTS Finished → EventQueue->ConfirmEventProcessed(EventId)
```

#### **Direct Event Input Pattern**
```blueprint
// From TikFinityPlugin or other sources
On TikTok Chat Event → EventQueue->EnqueueChatEventJson(JsonPayload)
On TikTok Gift Event → EventQueue->EnqueueGiftEventJson(JsonPayload)
On TikTok Follow Event → EventQueue->EnqueueFollowEventJson(JsonPayload)
// ... similar for other event types
```

#### **Queue Monitoring Pattern**
```blueprint
Event Tick → Get Queued Count → Update UI Text
If (Has Locked Event) → Show Processing Indicator
```

## 🔄 Integración con Blueprint/StateTreetado actual**: Funcional y documentado en secciones de este README (nodos y ejemplos).

### 🧵 EventQueue (nuevo)
- **Rol**: Gestiona una **cola priorizada** con **TTL**, **slots por tipo**, y **normalización** (Likes por ventana, RoomUser por *milestones*), asegurando **salida serial** (un evento a la vez) para Narrator/TTS.
- **Flujo alto nivel**: Input → **Queue** → `SweepExpired()` (TTL/normalize) → `DequeueNextEvent()` → `DispatchLockedEvent()` → TTS → `ConfirmEventProcessed()`.
- **Integración BP**: exposición vía `UTikStudioEventQueue` + `UTikStudioEventDispatcher` (7 delegates `*_Processed` con `(Data, EventId)`).

### 🔧 API clave del EventQueue (resumen)
- `EnqueueRawEvent(EventType, PayloadJson)`  
- `SweepExpired()`  
- `DequeueNextEvent(OutEvent)`  
- `DispatchLockedEvent()`  
- `ConfirmEventProcessed(EventId)`  
- `PumpOnceIfFree()`  
- Getters de estado: `HasLockedEvent()`, `GetLockedEventId()`, `GetQueuedCount()`, `IsFree()`

### 📤 Delegates del Dispatcher
- `OnChatEventProcessed(Data, EventId)`
- `OnFollowEventProcessed(Data, EventId)`
- `OnGiftEventProcessed(Data, EventId)`
- `OnLikeEventProcessed(Data, EventId)` *(batch)*
- `OnMemberEventProcessed(Data, EventId)`
- `OnRoomUserEventProcessed(Data, EventId)` *(milestone)*
- `OnShareEventProcessed(Data, EventId)`

## 🎯 **Características Principales**� Tabla de Contenidos
- [Descripción General](#-descripción-general)
- [Arquitectura General](#-arquitectura-general)
  - [Director (existente)](#-director-existente)
  - [EventQueue (nuevo)](#-eventqueue-nuevo)
- [Conectividad y Ejemplos Rápidos](#-conectividad-y-ejemplos-rápidos)
- [Referencia de Blueprints](#-referencia-de-blueprints)
  - [Nodos de Conexión](#-nodos-de-conexión)
  - [Nodos de Eventos (Trigger)](#-nodos-de-eventos-trigger)
  - [Delegates Bindeables](#-delegates-bindeables)
- [Integración con Blueprint/StateTree](#-integración-con-blueprintstatetree)
- [Consideraciones de Rendimiento](#-consideraciones-de-rendimiento)
- [Dependencias](#-dependencias)
- [Notas de Versión](#-notas-de-versión)
- [Soporte y Contribuciones](#-soporte-y-contribuciones)
- [Licencia](#-licencia)

## �📋 **Descripción General**

TikStudioPlugin es un plugin integral para Unreal Engine 5.7 que proporciona procesamiento avanzado de eventos de TikTok y funcionalidad de Director de IA. Este plugin está diseñado para trabajar de manera fluida con TikFinityPlugin para crear experiencias de juego dinámicas y receptivas basadas en la actividad del stream de TikTok en tiempo real.

## 🎯 **Características Principales**

### **🧠 Sistema Director de IA**
- **Gestión de Estados Dinámicos**: Sistema de 5 estados (DORMANT, ACTIVE, INTENSIVE, COOLDOWN, LURKING)
- **Umbrales Adaptativos**: Umbrales de clímax dinámicos que escalan con la actividad
- **Procesamiento de Eventos**: Procesamiento en tiempo real de eventos de TikTok (comentarios, regalos, follows, etc.)
- **Sistema Hype**: Sistema de multiplicador de engagement
- **Análisis de Flavor**: Análisis contextual de tipos de eventos dominantes

### **⚙️ Integración con Blueprints**
- **Accesibilidad Completa**: Toda la funcionalidad expuesta a Blueprints
- **Delegates de Eventos**: Notificaciones en tiempo real para cambios de estado, eventos de clímax, etc.
- **Manejo de Errores**: Sistema de errores integral compatible con Blueprints
- **Funciones Helper**: Funciones de utilidad extensas para operaciones comunes

### **🔧 Optimizado para Rendimiento**
- **Núcleo en C++**: Todo el procesamiento pesado realizado en C++ optimizado
- **Arquitectura de Subsistemas**: Patrón singleton eficiente usando subsistemas de UE5
- **Procesamiento Basado en Timers**: Sistemas de decay y análisis no bloqueantes
- **Eficiencia de Memoria**: Limpieza inteligente y gestión del historial de eventos

## 🏗️ **Arquitectura**

```
TikFinityPlugin (Eventos) → Blueprint Glue → TikStudioPlugin (Procesamiento) → Lógica del Juego
```

### **Componentes Principales**

1. **UTikDirectorSubsystem**: Subsistema principal que maneja la lógica del Director de IA
2. **UTikHelpers**: Funciones matemáticas y de utilidad
3. **UTikStudioErrorLibrary**: Manejo de errores accesible desde Blueprints
4. **UTikStudioBlueprintLibrary**: Funciones de conveniencia para operaciones comunes

## � Conectividad y Ejemplos Rápidos

## �🚀 **Inicio Rápido**

### **1. Instalación**
1. Copia el plugin a la carpeta `Plugins` de tu proyecto
2. Habilita el plugin en Project Settings
3. Recompila tu proyecto

### **2. Configuración Básica en Blueprint**

```blueprint
// Obtener el Subsistema Director
TikDirector = Get Tik Director Subsystem

// Iniciar el Director
Start Director → Error Status

// Procesar eventos de TikFinityPlugin
On TikTok Gift Event:
  → Process Gift Event (Username, Gift Value, Gift Name)
  → Branch (Is Error?) 
    → False: Continuar lógica del juego
    → True: Manejar error

// Escuchar cambios de estado
On Director State Changed:
  → Switch on Enum (New State)
    → DORMANT: Configurar iluminación ambiental
    → ACTIVE: Activar efectos de partículas  
    → INTENSIVE: Generar efectos visuales
    → COOLDOWN: Reproducir animación de cooldown
```

### **3. Integración con TikFinityPlugin**

```blueprint
// Eventos TikFinityPlugin → Procesamiento TikStudioPlugin
Event: OnTikTokComment
  → Process Comment Event (Username, Message)

Event: OnTikTokGift  
  → Process Gift Event (Username, Gift Value, Gift Name)

Event: OnTikTokFollow
  → Process Follow Event (Username)
```

## 📊 **Estados del Director de IA**

El sistema Director de IA opera a través de 5 estados distintos, cada uno diseñado para responder apropiadamente al nivel de actividad del stream:

### **🌙 DORMANT (Latente)**
- **Rango de Actividad**: 0 - 10
- **Descripción**: Estado de línea base con actividad mínima o nula
- **Comportamiento Técnico**: 
  - Decay Rate: 10% por segundo (0.90)
  - Respuesta mínima del sistema
  - Ideal para períodos de baja interacción
- **Uso Típico**: Efectos ambientales sutiles, música de fondo, iluminación base

### **⚡ ACTIVE (Activo)**
- **Rango de Actividad**: 10 - 30
- **Descripción**: Actividad moderada con engagement regular
- **Comportamiento Técnico**:
  - Decay Rate: 5% por segundo (0.95)
  - Procesamiento estándar de eventos
  - Balance entre respuesta y estabilidad
- **Uso Típico**: Efectos visuales moderados, feedback de audio, animaciones dinámicas

### **🔥 INTENSIVE (Intensivo)**
- **Rango de Actividad**: 30 - Umbral de Clímax
- **Descripción**: Alta actividad con engagement intenso
- **Comportamiento Técnico**:
  - Decay Rate: 2% por segundo (0.98)
  - Procesamiento acelerado de eventos
  - Preparación para posible clímax
- **Uso Típico**: Efectos visuales intensos, música dinámica, spawning aumentado

### **❄️ COOLDOWN (Enfriamiento)**
- **Activación**: Actividad ≥ Umbral de Clímax Dinámico
- **Descripción**: Período de recuperación post-clímax obligatorio
- **Comportamiento Técnico**:
  - Duración fija: 15 segundos (configurable)
  - Decay inmediato al salir del cooldown
  - Previene spam de clímax consecutivos
  - Sistema de countdown visible
- **Uso Típico**: Animaciones de celebración, pausa dramática, reset visual

### **👁️ LURKING (Observación/Suspense)** 
- **Activación**: Probabilística después del COOLDOWN (no por umbral)
- **Descripción**: Estado de "calma tensa" - actividad baja pero ambiente inquietante
- **Comportamiento Técnico**:
  - Probabilidad de activación configurable (`LurkingChance`)
  - Duración fija configurable (`LurkingDuration`)
  - Sin decay de actividad (mantiene nivel actual)
  - Interrumpible por actividad de audiencia
- **Uso Típico**: Crear suspense, efectos inquietantes, preparar tensión antes de picos

## 🎨 **Sistema de Flavors (Tipos de Actividad)**

El sistema de Flavors analiza el contexto de los eventos para determinar el tipo dominante de actividad, permitiendo respuestas específicas según el tipo de engagement:

### **💤 IDLE (Inactivo)**
- **Condición**: Sin eventos durante tiempo configurado (`IdleTimeout`)
- **Eventos que lo Activan**: Ninguno (ausencia de eventos)
- **Análisis Técnico**: 
  - Se activa tras período de inactividad total (15s por defecto)
  - No requiere análisis de historial de eventos
  - Independiente del estado del Director (puede ocurrir en cualquier estado)
- **Respuesta Sugerida**: Efectos pasivos, música ambiental

### **👥 COMMUNITY (Comunidad)**
- **Eventos que lo Activan**: 
  - `Process Comment Event` (peso: 2.0)
  - Comentarios de TikTok con alta frecuencia
- **Análisis Técnico**:
  - Dominancia de eventos de comentarios en la ventana de análisis
  - Peso moderado por evento individual pero alto volumen
  - Indica engagement conversacional y participación activa del chat
- **Respuesta Sugerida**: Efectos sociales, highlight de chat, animaciones de comunidad

### **💎 GENEROSITY (Generosidad)**
- **Eventos que lo Activan**: 
  - `Process Gift Event` (peso: 1.0 base + valor del regalo)
  - Regalos de TikTok con valores en diamantes
  - Combos de regalos (detectados con `Repeat End`)
- **Análisis Técnico**:
  - Peso variable según valor del regalo en diamantes
  - Detección especial de combos para mayor impacto
  - Indica generosidad y apoyo económico de la audiencia
- **Respuesta Sugerida**: Efectos de celebración, animaciones de regalos, feedback premium

### **🚀 HYPE (Emoción)**
- **Eventos que lo Activan**: 
  - Combinación de múltiples tipos de eventos simultáneamente
  - Alta actividad en estado INTENSIVE
  - Multiplicador de hype elevado (>1.5x)
  - Diversidad de tipos de eventos únicos
- **Análisis Técnico**:
  - Condición especial que requiere múltiples factores
  - No dominancia de un solo tipo, sino mezcla equilibrada
  - Actividad sostenida y multiplicador de hype alto
- **Respuesta Sugerida**: Efectos explosivos, música épica, máxima intensidad visual

### **❤️ ENGAGEMENT (Compromiso)**
- **Eventos que lo Activan**: 
  - `Process Follow Event` (peso: 3.0)
  - `Process Share Event` (peso: 4.0)
  - `Process Like Event` (peso: 1.0 por like)
- **Análisis Técnico**:
  - Alto peso de eventos de follow y share
  - Indica crecimiento de audiencia y engagement a largo plazo
  - Eventos que expanden el alcance del stream
- **Respuesta Sugerida**: Celebraciones de milestone, efectos de crecimiento, feedback de bienvenida

### **🔬 Análisis Técnico de Flavors**

El sistema determina el flavor dominante mediante:

1. **Ventana de Análisis**: Eventos recientes en ventana temporal configurable (`AnalysisWindow`)
2. **Peso Acumulativo**: Suma ponderada por tipo de evento con scores reales
3. **Umbral de Dominancia**: Porcentaje mínimo configurable (`DominanceThreshold`) 
4. **Frecuencia de Análisis**: Análisis periódico configurable (`AnalysisFrequency`)
5. **Timeout de IDLE**: Tiempo sin eventos para forzar IDLE (`IdleTimeout`)
6. **Limpieza Automática**: Eventos antiguos se eliminan automáticamente del análisis
7. **Condición Especial HYPE**: Alta actividad + alto hype + múltiples tipos de eventos

```cpp
// Lógica mejorada de determinación
if (TimeSinceLastEvent > IdleTimeout) → IDLE
else if (NoRecentEvents) → Mantener flavor actual  
else if (ShouldForceHype()) → HYPE
else → CalculateDominantFlavor(Events, DominanceThreshold)
```

| Estado | Descripción | Trigger | Respuesta Típica |
|-------|-------------|---------|-----------------|
| **DORMANT** | Baja actividad, estado base | Actividad < 10 | Efectos ambientales, feedback mínimo |
| **ACTIVE** | Actividad media, respuesta moderada | Actividad 10-30 | Visuales mejorados, efectos moderados |
| **INTENSIVE** | Alta actividad, respuesta intensa | Actividad 30-50+ | Efectos dinámicos, feedback fuerte |
| **COOLDOWN** | Período de recuperación post-clímax | Actividad > Umbral de Clímax | Animaciones de cooldown, pausa temporal |
| **LURKING** | Estado especial de observación | Trigger manual | Monitoreo pasivo |

## ⚙️ **Configuración**

Todos los aspectos del Director de IA son configurables a través de structs accesibles desde Blueprints:

### **Pesos de Eventos**
```cpp
Comment: 2.0
Follow: 3.0  
Gift: 1.0
Like: 1.0
Share: 4.0
```

### **Umbrales de Estado**
```cpp
Dormant → Active: 10.0
Active → Intensive: 30.0
Climax Threshold: 50.0
```

### **Tasas de Decay** (por segundo)
```cpp
Dormant: 0.90 (10% decay)
Active: 0.95 (5% decay)
Intensive: 0.98 (2% decay)
```

### **🔧 Configuración**

Todos los aspectos del Director de IA son configurables a través de structs accesibles desde Blueprints:

### **Pesos de Eventos**
```cpp
Comment: 2.0
Follow: 3.0  
Gift: 1.0
Like: 1.0
Share: 4.0
```

### **Umbrales de Estado**
```cpp
Dormant → Active: 10.0
Active → Intensive: 30.0
Climax Threshold: 50.0
```

### **Tasas de Decay** (por segundo)
```cpp
Dormant: 0.90 (10% decay)
Active: 0.95 (5% decay)
Intensive: 0.98 (2% decay)
```

### **Configuración de Timing**
```cpp
Cooldown Duration: 15 segundos
Natural Plateau Time: 5 segundos
Decay Delay Threshold: 20 segundos
Climax Threshold Percent: 10%
```

### **Configuración de Flavors**
```cpp
Analysis Window: 10 segundos
Dominance Threshold: 60% (0.6)
Analysis Frequency: 2 segundos  
Idle Timeout: 15 segundos
```

### **Configuración de LURKING**
```cpp
Lurking Chance: 50% (0.5)
Lurking Duration: 15 segundos
```

## 🎮 **Ejemplos de Integración con Gameplay**

### **Efectos Visuales**
```blueprint
On Director State Changed:
  INTENSIVE → Spawn Particle System
  COOLDOWN → Stop All Effects + Play Cooldown VFX
```

### **Sistema de Audio**
```blueprint
On Activity Changed:
  Activity > 80% → Set Music Intensity to High
  Activity < 20% → Set Music Intensity to Low
```

### **Dificultad Dinámica**
```blueprint
On Hype Changed:
  Hype > 2.0x → Increase Enemy Spawn Rate
  Hype < 1.2x → Decrease Enemy Spawn Rate
```

### **Feedback de UI**
```blueprint
On Climax Reached → Show "HYPE CLIMAX!" UI
On Cooldown Started → Show Cooldown Timer
On Flavor Changed → Update Activity Type Display
```

## 🔧 **Uso Avanzado**

### **Procesamiento de Eventos Personalizados**
```blueprint
// Crear estructura de evento personalizada
Event Data = Make Map:
  "customValue" → "123"
  "eventMetadata" → "special"

Process Tik Event (Custom Event) → Error Status
```

### **Configuración en Runtime**
```blueprint
// Obtener configuración actual
Current Config = Get Director Config

// Modificar valores
Current Config.Weights.Gift = 5.0
Current Config.Thresholds.Climax = 75.0

// Aplicar cambios
Set Director Config (Modified Config)
```

### **Monitoreo de Estado**
```blueprint
// Obtener estado detallado
Director Status = Get Director Status
Current Activity = Director Status.Current Activity
Hype Multiplier = Director Status.Hype Multiplier
Events Processed = Director Status.Total Events
```

## 🐛 **Manejo de Errores**

El plugin utiliza un sistema de errores integral:

```blueprint
Result = Process Comment Event (Username, Message)

Branch (Is Error? Result):
  True:
    Error Message = Get Error Message (Result)
    Print Error: Error Message
  False:
    // Éxito - continuar procesamiento
```

## 📈 **Consideraciones de Rendimiento**

- **Frecuencia de Eventos**: El sistema puede manejar eventos de alta frecuencia (100+ por segundo)
- **Uso de Memoria**: Historial de eventos recientes limitado a ventana de análisis (configurable)
- **Impacto de CPU**: Mínimo - procesamiento de decay a 1Hz, análisis de flavor a 0.5Hz
- **Overhead de Blueprint**: Lógica central en C++ minimiza el impacto de rendimiento en Blueprint

## 🔗 **Dependencies**

- **Unreal Engine 5.7+**
- **TikFinityPlugin** (for event source)
- **Core UE5 Modules**: Core, CoreUObject, Engine, UMG

## � Referencia de Blueprints

### 🎯 Nodos de Conexión

## �📚 **Blueprint Nodes Reference**

### **🎯 Core System Nodes**

#### **1. Get Tik Director Subsystem**
- **Función**: Obtiene la referencia al subsistema principal del Director
- **Pines de entrada**: Ninguno
- **Pines de salida**: 
  - `Return Value` (Tik Director Subsystem): Referencia al subsistema

#### **2. Set Director Config**
- **Función**: Aplica una nueva configuración al Director
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
  - `New Config` (Tik Director Config): Nueva configuración a aplicar
- **Pines de salida**: Ninguno

#### **3. Start Director**
- **Función**: Inicia el sistema Director y comienza el procesamiento
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Tik Studio Error Status): Estado del resultado

---

### **📡 Event Binding Nodes**

#### **4. Bind Event to On Director State Changed**
- **Función**: Vincula un evento personalizado para cambios de estado del Director
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
  - `Event` (Custom Event): Evento a ejecutar cuando cambie el estado
- **Pines de salida**: Ninguno
- **Evento relacionado**:
  - `New State` (ETik Director State): Nuevo estado del Director
  - `Current Activity` (Float): Actividad actual

#### **5. Bind Event to On Climax Reached**
- **Función**: Vincula un evento para cuando se alcanza un clímax
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
  - `Event` (Custom Event): Evento a ejecutar al alcanzar clímax
- **Pines de salida**: Ninguno
- **Evento relacionado**:
  - `Climax Threshold` (Float): Threshold de clímax alcanzado

#### **6. Bind Event to On Flavor Changed**
- **Función**: Vincula un evento para cambios en el tipo de actividad dominante
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
  - `Event` (Custom Event): Evento a ejecutar al cambiar flavor
- **Pines de salida**: Ninguno
- **Evento relacionado**:
  - `New Flavor` (ETik Flavor Type): Nuevo tipo de actividad dominante
  - `Confidence` (Float): Confianza en la determinación del flavor

#### **7. Bind Event to On Activity Changed**
- **Función**: Vincula un evento para cambios en el nivel de actividad
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
  - `Event` (Custom Event): Evento a ejecutar al cambiar actividad
- **Pines de salida**: Ninguno
- **Evento relacionado**:
  - `New Activity` (Float): Nuevo nivel de actividad
  - `Previous Activity` (Float): Nivel de actividad anterior

#### **8. Bind Event to On Hype Changed**
- **Función**: Vincula un evento para cambios en el multiplicador de hype
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
  - `Event` (Custom Event): Evento a ejecutar al cambiar hype
- **Pines de salida**: Ninguno
- **Evento relacionado**:
  - `New Hype Multiplier` (Float): Nuevo multiplicador de hype
  - `Previous Hype Multiplier` (Float): Multiplicador anterior

#### **9. Bind Event to On Cooldown Started**
- **Función**: Vincula un evento para el inicio del período de cooldown
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
  - `Event` (Custom Event): Evento a ejecutar al iniciar cooldown
- **Pines de salida**: Ninguno
- **Evento relacionado**:
  - `Cooldown Duration` (Float): Duración total del cooldown

#### **10. Bind Event to On Cooldown Ended**
- **Función**: Vincula un evento para el final del período de cooldown
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
  - `Event` (Custom Event): Evento a ejecutar al terminar cooldown
- **Pines de salida**: Ninguno
- **Evento relacionado**:
  - `New State` (ETik Director State): Estado al que transiciona después del cooldown

---

### **🎮 Event Processing Nodes**

#### **11. Process Comment Event**
- **Función**: Procesa un evento de comentario de TikTok
- **Pines de entrada**: Ninguno (usa datos internos del evento)
- **Pines de salida**:
  - `Return Value` (Tik Studio Error Status): Estado del procesamiento

#### **12. Process Gift Event**
- **Función**: Procesa un evento de regalo de TikTok con detección de combos
- **Pines de entrada**:
  - `Diamond Count` (Integer): Valor en diamantes del regalo
  - `Repeat End` (Boolean): True si es el final de un combo de regalos
- **Pines de salida**:
  - `Return Value` (Tik Studio Error Status): Estado del procesamiento

#### **13. Process Like Event**
- **Función**: Procesa un evento de like de TikTok
- **Pines de entrada**:
  - `Like Count` (Integer): Número de likes en el evento
- **Pines de salida**:
  - `Return Value` (Tik Studio Error Status): Estado del procesamiento

#### **14. Process Follow Event**
- **Función**: Procesa un evento de follow de TikTok
- **Pines de entrada**: Ninguno (usa datos internos del evento)
- **Pines de salida**:
  - `Return Value` (Tik Studio Error Status): Estado del procesamiento

#### **15. Process Share Event**
- **Función**: Procesa un evento de share de TikTok
- **Pines de entrada**: Ninguno (usa datos internos del evento)
- **Pines de salida**:
  - `Return Value` (Tik Studio Error Status): Estado del procesamiento

---

### **📊 Status Getter Nodes**

> **💡 Clasificación de Nodos Getter:**
> - **🔄 DINÁMICOS**: Valores que cambian en tiempo real durante el gameplay - **ÚTILES PARA DESARROLLO**
> - **⚙️ ESTÁTICOS**: Valores de configuración que no cambian - **ÚTILES PARA DEPURACIÓN/DEBUGGING**

---

## **🔄 Nodos Getter Dinámicos (Para Desarrollo de Gameplay)**

#### **16. Get Current State** 🔄
- **Función**: Obtiene el estado actual del Director
- **Tipo**: **DINÁMICO** - Cambia según la actividad del stream
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (ETik Director State): Estado actual (DORMANT, ACTIVE, INTENSIVE, COOLDOWN, LURKING)
- **Uso típico**: Cambiar efectos visuales, música, mecánicas según el estado

#### **17. Get Current Activity** 🔄
- **Función**: Obtiene el nivel de actividad actual
- **Tipo**: **DINÁMICO** - Se actualiza con cada evento procesado
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Float): Nivel de actividad actual
- **Uso típico**: Barras de progreso, intensidad de efectos, escalado de dificultad

#### **18. Get Hype Multiplier** 🔄
- **Función**: Obtiene el multiplicador de hype actual
- **Tipo**: **DINÁMICO** - Varía según la secuencia de eventos
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Float): Multiplicador de hype (≥1.0)
- **Uso típico**: Multiplicar puntuaciones, intensificar recompensas

#### **19. Get Current Flavor** 🔄
- **Función**: Obtiene el tipo de actividad dominante actual
- **Tipo**: **DINÁMICO** - Cambia según el análisis de eventos recientes
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (ETik Flavor Type): Tipo dominante (IDLE, COMMUNITY, GENEROSITY, HYPE, ENGAGEMENT)
- **Uso típico**: Adaptar contenido del juego al tipo de interacción dominante

#### **20. Get Current Climax Threshold** 🔄
- **Función**: Obtiene el threshold de clímax actual (dinámico)
- **Tipo**: **DINÁMICO** - Se adapta según la actividad histórica
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Float): Threshold actual para alcanzar clímax
- **Uso típico**: Mostrar progreso hacia eventos especiales

#### **21. Get Cooldown Remaining Time** 🔄
- **Función**: Obtiene el tiempo restante del cooldown actual
- **Tipo**: **DINÁMICO** - Cuenta regresiva en tiempo real
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Float): Segundos restantes de cooldown (0 si no está en cooldown)
- **Uso típico**: Temporizadores visuales, animaciones de cooldown

#### **22. Get Decay Threshold Remaining Time** 🔄
- **Función**: Obtiene el tiempo restante para que se active el decay del threshold
- **Tipo**: **DINÁMICO** - Cuenta regresiva cuando está en zona de decay
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Float): Segundos restantes para decay
- **Uso típico**: Indicadores de urgencia, efectos de presión temporal

#### **23. Get Total Events** 🔄
- **Función**: Obtiene el número total de eventos procesados
- **Tipo**: **DINÁMICO** - Incrementa con cada evento
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Integer): Total de eventos procesados desde el inicio
- **Uso típico**: Estadísticas de sesión, logros, métricas de engagement

#### **25. Get Director Status Text** 🔄
- **Función**: Obtiene un texto descriptivo del estado actual del Director
- **Tipo**: **DINÁMICO** - Refleja el estado actual del sistema
- **Pines de entrada**: Ninguno
- **Pines de salida**:
  - `Return Value` (String): Texto descriptivo del estado para debugging/UI
- **Uso típico**: UI de debug, logs de estado, información para desarrolladores

#### **26. Get Lurking Remaining Time** 🔄
- **Función**: Obtiene el tiempo restante del estado LURKING actual
- **Tipo**: **DINÁMICO** - Cuenta regresiva durante LURKING
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Float): Segundos restantes de LURKING (0 si no está en LURKING)
- **Uso típico**: Temporizadores de estado especial, efectos de transición

#### **27. Is In Lurking** 🔄
- **Función**: Verifica si el Director está actualmente en estado LURKING
- **Tipo**: **DINÁMICO** - Cambia según el estado actual
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Boolean): True si está en estado LURKING
- **Uso típico**: Lógica condicional para mecánicas especiales de LURKING

#### **32. Get Time Since Last Event** 🔄
- **Función**: Obtiene el tiempo transcurrido desde el último evento procesado
- **Tipo**: **DINÁMICO** - Se actualiza continuamente
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Float): Segundos desde el último evento
- **Uso típico**: Detectar períodos de inactividad, activar modo idle

#### **33. Get Recent Events Count** 🔄
- **Función**: Obtiene el número de eventos en la ventana de análisis actual
- **Tipo**: **DINÁMICO** - Varía según la actividad reciente
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Integer): Cantidad de eventos recientes considerados
- **Uso típico**: Métricas de actividad, indicadores de intensidad

#### **34. Should Force Hype Condition** 🔄
- **Función**: Verifica si se cumplyen las condiciones para forzar flavor HYPE
- **Tipo**: **DINÁMICO** - Evalúa condiciones en tiempo real
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Boolean): True si se debe forzar HYPE (alta actividad + alto hype + eventos mixtos)
- **Uso típico**: Lógica especial para momentos de máxima intensidad

---

## **⚙️ Nodos Getter Estáticos (Para Depuración/Configuración)**

> **⚠️ Nota**: Estos nodos devuelven valores de configuración que no cambian durante el gameplay. Son útiles principalmente para depuración, validación de configuración, UI de configuración y sistemas de telemetría.

#### **24. Get Director Config** ⚙️
- **Función**: Obtiene la configuración actual del Director
- **Tipo**: **ESTÁTICO** - Valores de configuración fijos
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Tik Director Config): Configuración completa actual
- **Uso típico**: Validación de configuración, UI de settings, debugging

#### **28. Get Flavor Analysis Window** ⚙️
- **Función**: Obtiene la ventana temporal de análisis de flavors actual
- **Tipo**: **ESTÁTICO** - Valor de configuración (por defecto: 30.0 segundos)
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Float): Segundos de ventana de análisis (AnalysisWindow)
- **Uso típico**: Debugging del sistema de flavor, validación de configuración

#### **29. Get Flavor Dominance Threshold** ⚙️
- **Función**: Obtiene el umbral de dominancia para determinación de flavors
- **Tipo**: **ESTÁTICO** - Valor de configuración (por defecto: 0.6)
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Float): Umbral de dominancia (DominanceThreshold) entre 0.0-1.0
- **Uso típico**: Debugging del sistema de flavor, ajuste de sensibilidad

#### **30. Get Flavor Analysis Frequency** ⚙️
- **Función**: Obtiene la frecuencia de análisis de flavors
- **Tipo**: **ESTÁTICO** - Valor de configuración (por defecto: 5.0 segundos)
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Float): Segundos entre análisis (AnalysisFrequency)
- **Uso típico**: Debugging del sistema de flavor, optimización de rendimiento

#### **31. Get Flavor Idle Timeout** ⚙️
- **Función**: Obtiene el tiempo sin eventos necesario para activar IDLE
- **Tipo**: **ESTÁTICO** - Valor de configuración (por defecto: 60.0 segundos)
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Float): Segundos de timeout para IDLE (IdleTimeout)
- **Uso típico**: Debugging del sistema de flavor, ajuste de comportamiento idle

---

---

### **🔧 Additional Helper Nodes**

Estos nodos adicionales están disponibles para funcionalidad extendida:

#### **Stop Director**
- **Función**: Detiene el procesamiento del Director
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Tik Studio Error Status): Estado del resultado

#### **Reset Director**
- **Función**: Reinicia completamente el Director a valores por defecto
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Tik Studio Error Status): Estado del resultado

#### **Is Director Running**
- **Función**: Verifica si el Director está actualmente procesando eventos
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
- **Pines de salida**:
  - `Return Value` (Boolean): True si está ejecutándose

#### **Set Manual State**
- **Función**: Permite establecer manualmente el estado del Director (para testing)
- **Pines de entrada**:
  - `Target` (Tik Director Subsystem): Referencia al subsistema
  - `New State` (ETik Director State): Estado a establecer
- **Pines de salida**:
  - `Return Value` (Tik Studio Error Status): Estado del resultado

---

### **💡 Usage Patterns**

#### **Initialization Pattern**
```blueprint
Begin Play → Get Tik Director Subsystem → Set Director Config → Start Director
```

#### **Event Binding Pattern**  
```blueprint
Start Director → Bind Event to On Director State Changed → Custom Event Logic
```

#### **TikTok Integration Pattern**
```blueprint
OnTikTokGift → Process Gift Event → Branch (Success/Error) → Game Response
```

#### **Status Monitoring Pattern**
```blueprint
Event Tick → Get Current Activity → Update UI Progress Bar
```

## � Integración con Blueprint/StateTree

Ciclo sugerido para narración con TTS:
1. Cuando TTS esté libre, llama **`PumpOnceIfFree()`** (EventQueue).
2. El `Dispatcher` emite **uno** de los 7 eventos `*_Processed(Data, EventId)`.
3. En BP, arma el **prompt** con `Format Text` a partir de **Data**.
4. Envía a LLM/TTS y reproduce el audio.
5. Al finalizar TTS, llama **`ConfirmEventProcessed(EventId)`**.
6. Repite desde 1.

> Nota: Likes y RoomUser se **normalizan**: Like → batch por ventana; RoomUser → solo *milestones*.  
> Prioridad y TTL son configurables vía Settings del EventQueue.

---

## 🔎 Módulo EventQueue (detallado y separado)

Ruta del módulo: `Source/TikStudioPlugin/Private/EventQueue/`

Componentes principales:
- `UTikStudioEventQueue` (clase principal de cola priorizada)
- `UTikStudioEventDispatcher` (dispatcher de eventos procesados)
- `FTikStudioEventQueueSettings` (configuración categorizada en `Public/EventQueue/TikStudioEventQueueSettings.h`)

Objetivo del módulo:
- Gestiona una cola priorizada por tipo con TTL, slots, fairness y reglas de normalización (Likes por ventana, RoomUser por milestones). Emite un único evento a la vez, con confirmación explícita del procesamiento para mantener narración serial (TTS).

### API de UTikStudioEventQueue (resumen completo)
- Entrada tipada (C++/Blueprint):
  - `EnqueueChatEvent(const FTSE_ChatIn&)`
  - `EnqueueGiftEvent(const FTSE_GiftIn&)`
  - `EnqueueFollowEvent(const FTSE_FollowIn&)`
  - `EnqueueLikeEvent(const FTSE_LikeIn&)`
  - `EnqueueMemberEvent(const FTSE_MemberIn&)`
  - `EnqueueRoomUserEvent(const FTSE_RoomUserIn&)`
  - `EnqueueShareEvent(const FTSE_ShareIn&)`
- Encolado interno y bombeo:
  - `EnqueueTypedEvent(FName EventType, const FQueuedTikTokEvent& E)` → Inserta en orden por `PriorityScore` con política de slots y (opcional) evicción competitiva.
  - `SweepExpired()` → TTL y consolidaciones (Like batch window, RoomUser milestones).
  - `RecomputePriorities()` → Recalcula `PriorityScore` y reordena globalmente (descendente; tie-break por `Timestamp`).
  - `PumpOnceIfFree()` → Si no hay lock, hace `DequeueNextEvent` + `DispatchLockedEvent`.
  - `ConfirmEventProcessed(FGuid EventId)` → Libera lock, procesa snapshots de combos pendientes bajo lock y programa bombeo si quedan items.
- Utilidades/estado:
  - `GetDispatcher()` → Obtiene dispatcher.
  - `GetQueuedCount()`, `GetQueueSize()`
  - `IsFree()`, `HasLockedEvent()`, `GetLockedEventId()`
  - `PeekNextEvent(FQueuedTikTokEvent& OutEvent)`
  - `ResetQueue(bool bClearCombos, bool bClearLikes)`

### Nodos de Entrada (Conveniencia)
- JSON input (en Blueprint/Glue, ver `TikStudioBlueprintLibrary`):
  - `Enqueue Chat Event Json`
  - `Enqueue Gift Event Json`
  - `Enqueue Follow Event Json`
  - `Enqueue Like Event Json`
  - `Enqueue Member Event Json`
  - `Enqueue Room User Event Json`
  - `Enqueue Share Event Json`

### Delegates (11 bind events)
Los siguientes delegates se emiten desde `UTikStudioEventDispatcher` durante `DispatchLockedEvent`:
- `OnChatEventProcessed(FTSE_ChatOut, FGuid)`
- `OnFollowEventProcessed(FTSE_FollowOut, FGuid)`
- `OnGiftEventProcessed(FTSE_GiftOut, FGuid)`
- `OnLikeEventProcessed(FTSE_LikeBatchOut, FGuid)`
- `OnLikeUserEventProcessed(FTSE_LikeUserOut, FGuid)`
- `OnMemberNormalizedProcessed(FTSE_MemberNormalizedOut, FGuid)`
- `OnMemberIdentityProcessed(FTSE_MemberOut, FGuid)`
- `OnRoomUserEventProcessed(FTSE_RoomUserMilestoneOut, FGuid)`
- `OnRoomUserSnapshotProcessed(FTSE_RoomUserSnapshotOut, FGuid)`
- `OnRoomUserTop1ChangeProcessed(FTSE_RoomUserTop1ChangeOut, FGuid)`
- `OnShareEventProcessed(FTSE_ShareOut, FGuid)`

Cada delegate entrega el `Data` tipado ya normalizado y el `EventId` original, útil para correlación y confirmación.

### Políticas internas (comportamientos clave)
- TTL y Expiration: por tipo (`Settings.TTLSeconds` y `Settings.ExpirePolicies`). Like se consolida por ventana; RoomUser reduce raw y emite milestones/snapshots/Top1.
- Slots por tipo (`Settings.MaxSlots`) y evicción competitiva: si el tipo está lleno y `Settings.Eviction.bEnableCompetitiveEviction` está activo, el nuevo evento puede reemplazar el más débil (tie-break por timestamp). Tipos en `Settings.Eviction.ExemptFromEviction` nunca son evictados.
- Fairness (Aging): `Settings.AgingPointsPerSecond` + cap `Settings.AgingMaxBonus` agregan puntos de antigüedad para evitar starvation.
- Combos Gift: manejo de estado de combos, snapshots periódicos/finales, protección de snapshots en evicción/reemplazo y pruning de combos cerrados.
- Bombeo: modo automático (`Settings.bEnableAutoPump` + `Settings.AutoPumpInterval`), y bombeos inmediatos oportunos (`RequestImmediatePump`).

### Configuración categorizada (FTikStudioEventQueueSettings)
Las configuraciones del EventQueue están agrupadas en bloques colapsables en el Editor. A continuación, cada categoría y sus efectos:

1) Like (`Settings.Like`)
- `BatchLikesWindowSeconds`
  - Ventana (segundos) para agrupar likes en un único evento normalizado. Al expirar la ventana, se emite `Like` con `LikeCount = suma` y `TotalLikeCount = máximo` observados durante la ventana.
  - Subir el valor: ventanas más largas, menos eventos `Like` pero más grandes (útil para reducir ruido en streams con actividad constante).
  - Bajar el valor: ventanas más cortas, más eventos `Like` pero más pequeños (útil para reaccionar más rápido a ráfagas cortas).
  - Ejemplo: si pasas de `5.0s` a `10.0s`, en una actividad uniforme de 2 likes/seg, emitirás 1 `Like` cada 10s con `LikeCount≈20` en lugar de 1 cada 5s con `LikeCount≈10`.

- `bUseLikeUserViewerGate`
  - Si está activo, el evento derivado `LikeUser` sólo se encola cuando `ViewerCount ≤ LikeUserViewerGateThreshold`. Controla la granularidad por-usuario en salas concurridas.
  - **IMPORTANTE**: Ahora usa una caché independiente poblada desde RoomUser base, aislada de otros flujos RoomUser/milestones.
  - Activado: el gate aplica el umbral usando la caché independiente; Desactivado: siempre se permite `LikeUser` (si el toggle está habilitado).
  - **Comportamiento interno**: Si ViewerCount no está disponible, bloquea LikeUser hasta recibir el primer RoomUser base (fail-closed).

- `LikeUserViewerGateThreshold`
  - Umbral de `ViewerCount` para permitir `LikeUser`.
  - Subir el valor: se permite `LikeUser` en salas más grandes (más granularidad por-usuario). Bajar el valor: se restringe `LikeUser` a salas pequeñas/medianas (menos granularidad, menos carga).
  - Ejemplo: con `ViewerCount` actual = 30:
    - `Threshold = 25` → no se emite `LikeUser` (30 > 25).
    - `Threshold = 35` → se emite `LikeUser` (30 ≤ 35).

2) Member (`Settings.MemberConfig`)
- `MemberIdentityViewerGateThreshold`
  - Umbral de viewers para abrir el gate de identidad (permite `MemberIdentity`).
- `MemberIdentityGateHysteresisDelta`
  - Histéresis (Exit = Enter + Delta) para cerrar el gate cuando sube el VC, evitando flapping.
- `MemberNormalizedJoinMilestone`
  - Hito de joins acumulados para emitir `MemberNormalized` (se resetea el conteo tras emitir).

3) RoomUser (`Settings.RoomUserConfig`)
- `RoomUserMilestoneStep`
  - Paso del hito de `ViewerCount` para emitir `RoomUser` (milestones discretos).
- `RoomUserSnapshotPeriodSeconds`
  - Periodicidad mínima entre `RoomUserSnapshot` (si `ViewerCount` supera el umbral).
- `RoomUserSmartSwitchMaxVC`
  - Umbral máximo de viewers para PERMITIR snapshots cuando el Smart Switch está habilitado. Si VC > MaxVC → BLOQUEA snapshot.
- `bEnableRoomUserSmartSwitch`
  - Habilita el interruptor inteligente. Si true → bloquea snapshots cuando VC > MaxVC. Si false → ignora umbral.
- `RoomUserTop1CooldownSeconds`
  - Cooldown entre emisiones de `RoomUserTop1Change` (evita spam de cambios frecuentes).
- `RoomUserTop1MinTopViewers`
  - Mínimo de elementos en `TopViewers` para validar cambios de Top1.
- `bRoomUserTop1StrictEquality`
  - Igualdad estricta (case-sensitive) para comparar `UniqueId` del Top1; si está desactivado, comparación `IgnoreCase`.

5) Gift (`Settings.GiftConfig`)
- `bEnableGiftCombos`
  - Activa el seguimiento de combos de regalo (estado vivo por `GroupId` y totales acumulados).
- `GiftComboRepeatSnapshotStep`
  - Cada cuántas repeticiones se emite un snapshot de combo. Tras insert/reemplazo, el threshold se incrementa.
- `GiftComboInactivitySeconds`
  - Inactividad tras la cual se cierra el combo por inactividad y se emite snapshot final por cierre.
- `GiftComboActiveBonus`
  - Bono de prioridad mientras el combo está activo (aplicado a snapshots no finales).
- `GiftComboClosedPruneSeconds`
  - Segundos tras el cierre del combo para purgar su estado de memoria.
- `bGiftType1CombosOnly`
  - Para `GiftType=1` con `GroupId` válido, suprime los `Gift` individuales y emite sólo snapshots (narrativa limpia).
- `GiftComboSnapshotPriorityBonus`
  - Bono de prioridad adicional para snapshots (activos y finales) para evitar inanición por competencia.
- `bProtectGiftComboSnapshots`
  - Protege snapshots activos de evicción/reemplazo. Si todos los candidatos son snapshots, se evita evicción y se registra métrica de protección.

6) Eviction (`Settings.Eviction`)
- `bEnableCompetitiveEviction`
  - Activa reemplazo del evento más débil del tipo cuando el slot está lleno y llega uno más fuerte (tie-break por `Timestamp`).
- `ExemptFromEviction`
  - Conjunto de tipos inmunes a la evicción competitiva (p. ej., desactivar para `Gift`).
- `bTrackEvictionMetrics`
  - Registra métricas por tipo (`EvictionsByType`) y “evicciones evitadas por protección” (`EvictionsSkippedByProtection`).

7) Generales (sin categoría) 
- `bEnableAutoPump`, `AutoPumpInterval`
  - Habilita/deshabilita el bombeo automático y su intervalo.
- `bPumpOnFirstEnqueue`, `bPumpAfterConfirm`, `bRecomputeOnPump`
  - Comportamientos de bombeo inmediato, bombeo tras confirmar y recomputo de prioridades antes de bombear.
- Fairness: `AgingPointsPerSecond`, `AgingMaxBonus`
  - Añade puntos de antigüedad por segundo (cap) a `PriorityScore` para evitar starvation.

### Reglas de Prioridad (resumen técnico)
- Base por tipo (`Settings.BaseWeights`): Chat 40, Gift 80, Follow 60, Like 10, Member 70, Share 55, RoomUser 30, RoomUserSnapshot 25, RoomUserTop1Change 35, MemberNormalized 20, MemberIdentity 30 (defaults).
- Modificadores comunes (`Settings.CommonModifiers`): FollowRole, flags de usuario (Moderator/Subscriber/NewGifter), TopGifterRank, GifterLevel, TeamMemberLevel.
- Modificadores específicos (`Settings.SpecificModifiers`):
  - Gift: DiamondCount, bRepeatEnd, RepeatCount (aporte lineal: puntos proporcionales a diamantes y repeticiones, más bono fijo si `bRepeatEnd`).
  - Chat: HasCommand.
  - Like: LikeCount/TotalLikeCount.
  - Member: ActionId (Join/Upgrade/Renewal).
  - RoomUser/RoomUserSnapshot: escala por `ViewerCount` y `RoomUserMilestoneStep`.

### Flujo de Combos Gift
- Apertura/actualización: por `UniqueId|GiftId|GroupId` (estado vivo, totales y umbrales).
- Snapshots periódicos y finales: inserción ordenada por score, deduplicación de snapshot previo del combo y reemplazo del Gift más débil (excluyendo snapshots si `bProtectGiftComboSnapshots` está activo).
- Cierre por inactividad o `RepeatEnd`: emite snapshot final; pruning diferido (`GiftComboClosedPruneSeconds`).

### Métricas y Logs
- `EvictionsByType` y `EvictionsSkippedByProtection` (si `bTrackEvictionMetrics`).
- Logs detallados en inserciones, reemplazos, consolidaciones, milestones, snapshots y pump cycle.

### Patrones de Uso (Blueprint)
- Narración serial con TTS (recomendado):
  1) `PumpOnceIfFree()` cuando el sistema TTS esté libre.
  2) Recibir `*_Processed(Data, EventId)` y reproducir narración.
  3) `ConfirmEventProcessed(EventId)` al terminar.
  4) Repetir.

---

### Ciclo de Activación de Delegates (11)
- Disparo general: Todos los delegates se emiten dentro de `DispatchLockedEvent()` tras un `DequeueNextEvent()` exitoso en `PumpOnceIfFree()`. Esto garantiza salida serial (uno por vez).
- Frecuencia: Depende del ritmo de entrada, de las reglas de normalización (Like/RoomUser) y del tiempo de bombeo. Con `bEnableAutoPump`, el ciclo se mantiene activo al intervalo configurado.
- Condiciones específicas de activación:
  - `OnChatEventProcessed`: cada vez que un `Chat` encolado es el siguiente en prioridad y se despacha.
  - `OnFollowEventProcessed`: igual para `Follow` cuando alcanza la cabeza de la cola.
  - `OnGiftEventProcessed`: para regalos individuales (no snapshot), si no están suprimidos por `bGiftType1CombosOnly` y ganan prioridad.
  - `OnLikeEventProcessed` (batch normalizado): se emite cuando expira la ventana `Settings.Like.BatchLikesWindowSeconds`, consolidando todos los Likes acumulados en ese período; el batch se encola y se despacha cuando llega a ser el más prioritario.
  - `OnLikeUserEventProcessed`: se encola y procesa sólo si el gate `Settings.Like.bEnableLikeUserViewerGate` está activo y `ViewerCount ≤ LikeUserViewerGateThreshold` usando una **caché independiente** poblada desde RoomUser base. Si ViewerCount no está disponible, bloquea hasta primer RoomUser (comportamiento interno fail-closed).
  - `OnMemberNormalizedProcessed`: se encola cuando el contador acumulado de joins alcanza `Settings.MemberConfig.MemberNormalizedJoinMilestone`; se despacha cuando gana prioridad.
  - `OnMemberIdentityProcessed`: se encola cuando el gate de identidad está abierto (`ViewerCount ≤ MemberIdentityViewerGateThreshold`, con histéresis) y pasa el anti-spam (LRU + cooldown 20s).
  - `OnRoomUserEventProcessed` (milestone): se emite desde `SweepExpired()` cuando el `ViewerCount` observado cruza un múltiplo de `RoomUserMilestoneStep`. Se encola el milestone y se despacha en orden.
  - `OnRoomUserSnapshotProcessed`: se emite desde `SweepExpired()` si el Smart Switch permite el snapshot (VC ≤ MaxVC cuando está habilitado) y han pasado al menos `RoomUserSnapshotPeriodSeconds` desde el último snapshot.
  - `OnRoomUserTop1ChangeProcessed`: se emite desde `SweepExpired()` cuando el Top1 cambia (según `bRoomUserTop1StrictEquality`) y han pasado al menos `RoomUserTop1CooldownSeconds` desde el último cambio emitido.
  - `OnShareEventProcessed`: cada vez que un `Share` encolado alcanza la cabeza.

### Encolado por Tipo (qué entra y cómo)
- `Chat`: se mapean campos de usuario; `PriorityScore` incluye `HasCommand` (+20). Se inserta en orden por prioridad.
- `Gift` (individual): se mapean campos y `RepeatEnd`; se suprime si `GiftType=1` con `GroupId` válido y `GiftConfig.bGiftType1CombosOnly=true` (se prefiere snapshot). Si no, se inserta por prioridad y respeta slots.
- `Follow`: se inserta por prioridad con pesos base y comunes.
- `Like`: entra normalizado por ventana. Cada Like input se acumula; al expirar la ventana se encola el batch `Like` con `LikeCount=sum`, `TotalLikeCount=max`.
- `LikeUser`: derivado del input de `Like` si el gate `LikeUser` está activo y `ViewerCount` cumple el umbral; se encola con identidad completa.
- `Member`: dos flujos derivados del input `Member`:
  - `MemberIdentity`: sólo cuando el gate abre y pasa anti-spam LRU + cooldown.
  - `MemberNormalized`: se encola al alcanzar el milestone `MemberNormalizedJoinMilestone` y se reinicia el contador.
- `RoomUser`: el input `RoomUser` alimenta estado (VC y TopViewers). En `SweepExpired()` no se re-encola raw; se generan y encolan los derivados: `RoomUser` (milestones), `RoomUserSnapshot` (periódicos) y `RoomUserTop1Change` (cambios de Top1).
- `Share`: se inserta por prioridad usando campos comunes.

### Sistema de Slots y Evicción
- Límite por tipo (`Settings.MaxSlots[Type]`): antes de insertar, se cuenta `CurrentCount` del tipo.
- Si `CurrentCount < MaxSlots`, se inserta por orden de prioridad (descendente; tie → más antiguo primero).
- Si `CurrentCount ≥ MaxSlots`:
  - Si `Settings.Eviction.bEnableCompetitiveEviction` y el tipo no está en `ExemptFromEviction`, se intenta `TryCompetitiveEvict_Locked(Type, Candidate)`:
    - Busca el más débil del tipo (menor `PriorityScore`, tie por `Timestamp` más antiguo).
    - Excluye snapshots `Gift` si `GiftConfig.bProtectGiftComboSnapshots=true`.
    - Si el candidato gana (mayor score o tie con timestamp más nuevo), reemplaza; se registra `EvictionsByType`.
    - Si no hay candidato válido (p.ej. todos protegidos), no evicta; se registra `EvictionsSkippedByProtection`.
  - Si evicción está desactivada o falla, el nuevo evento se descarta (log: slot limit alcanzado).

### Orden Global y Bombeo
- Inserción: se calcula `PriorityScore` y se ubica por búsqueda lineal en `Queue` para mantener orden descendente.
- `RecomputePriorities()`: recalcula `PriorityScore` de todos y ordena (`Sort`) con comparador: primero mayor `PriorityScore`; en empates, más antiguo (`Timestamp` menor) adelante.
- Bombeo:
  - `PumpOnceIfFree()` limpia expirados, recomputa (si `bRecomputeOnPump`) y si no hay lock hace Dequeue + Dispatch.
  - `ConfirmEventProcessed()` libera lock y procesa snapshots de combos pendientes; si quedan elementos, programa bombeo inmediato.
  - Auto Pump: si `bEnableAutoPump=true`, se programa `AutoPumpTick()` con intervalo `AutoPumpInterval` para disparar `PumpOnceIfFree()` periódicamente.

### Combos Gift (estado vivo y snapshots)
- Estado por combo (`UniqueId|GiftId|GroupId`): totales (`RepeatCountTotal`, `DiamondTotal`), timestamps y `NextSnapshotThreshold`.
- Trigger de snapshot:
  - Periódico: cuando `RepeatCountTotal ≥ NextSnapshotThreshold` (se incrementa en pasos `GiftComboRepeatSnapshotStep` tras insertar/reemplazar).
  - Final: al cerrar por inactividad (`GiftComboInactivitySeconds`) o por `RepeatEnd`.
- Emisión de snapshot:
  - Deduplica snapshot previo del mismo combo (no locked) antes de insertar.
  - Si hay cupo `Gift`, inserta por score; si no hay cupo, intenta reemplazar el `Gift` más débil excluyendo otros snapshots si `bProtectGiftComboSnapshots=true`.
  - Si no hay reemplazo válido, descarta el snapshot (log y métrica `EvictionsSkippedByProtection`).
- Pruning de combos cerrados: se purgan tras `GiftComboClosedPruneSeconds`.

### Fairness y Prioridad (detalles)
- `AgingPointsPerSecond` añade puntos por segundo en cola, limitado por `AgingMaxBonus`, para evitar inanición.
- Modificadores comunes (roles/flags) y específicos por tipo se suman al peso base; Gift aplica un aporte lineal a Diamonds/Repeats (sin amortiguación).

### Defaults útiles (resumen)
- `TTLSeconds[Gift]=30.0f`, `MaxSlots[Gift]=1000`.
- RoomUser: `MilestoneStep=25`, `SnapshotPeriodSeconds=6.0`, `SnapshotMinViewers=25`, `Top1CooldownSeconds=5.0`.
- Gift Combos: `RepeatSnapshotStep=5`, `InactivitySeconds=3.0`, `ClosedPruneSeconds=60.0`, `SnapshotPriorityBonus=12`, protección snapshots activa por defecto.

---

## �📚 **API Reference**

### **Main Classes**
- `UTikDirectorSubsystem`: Primary interface for AI Director functionality
- `UTikStudioBlueprintLibrary`: Convenience functions for common operations
- `UTikHelpers`: Mathematical and utility functions
- `UTikStudioErrorLibrary`: Error handling utilities

### **Key Structs**
- `FTikDirectorConfig`: Complete configuration structure
- `FTikDirectorStatus`: Current system status and metrics
- `FTikEvent`: Structured event data
- `FTikStudioErrorStatus`: Error reporting structure

### **Enums**
- `ETikDirectorState`: Director states (DORMANT, ACTIVE, INTENSIVE, COOLDOWN, LURKING)
- `ETikFlavorType`: Event context types (IDLE, COMMUNITY, GENEROSITY, HYPE, ENGAGEMENT)
- `ETikEventType`: Supported event types (COMMENT, GIFT, FOLLOW, LIKE, SHARE)

## 🤝 **Support**

For issues, feature requests, or integration help, please refer to the project documentation or contact the development team.

---

**TikStudioPlugin** - Bringing TikTok stream interactivity to Unreal Engine 5.7