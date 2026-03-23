# RTSProject — CLAUDE.md

## Project Overview

Unreal Engine **4.27** RTS game built on top of the **RealTimeStrategy** plugin (v1.2.0, by Nick Pruehs / ue4-rts). The game module (`Source/RTSProject/`) is minimal — nearly all gameplay code lives inside the plugin at `Plugins/RealTimeStrategy/`.

Platform: **Win64 only**.

---

## Repository Layout

```
RTSProject/
  RTSProject.uproject          # UE4.27 project file, no marketplace plugins
  Source/RTSProject/           # Thin game module (RTSProject.h / .cpp only)
  Plugins/RealTimeStrategy/    # All gameplay code (see below)
    RealTimeStrategy.uplugin
    Source/RealTimeStrategy/
      Classes/                 # Public headers (organized by subsystem)
      Private/                 # Implementation (.cpp files)
      Public/                  # IRealTimeStrategy module interface
```

---

## Plugin Subsystems

### Core Framework
| Class | Role |
|---|---|
| `ARTSPlayerController` | Player input, unit selection, camera, order dispatch, building placement |
| `ARTSPawnAIController` | Per-unit AI controller; owns the order queue and BT execution |
| `ARTSPlayerAIController` | Top-level AI player controller |
| `ARTSGameMode` / `ARTSGameState` | Game rules and global state |
| `ARTSPlayerState` | Per-player state; tracks owned actors |
| `ARTSTeamInfo` | Team membership and team-vs-team relationships |

### Orders (`Classes/Orders/`)
Base class `URTSOrder`. Concrete orders:
- `RTSMoveOrder`, `RTSAttackOrder`, `RTSStopOrder`
- `RTSGatherOrder`, `RTSReturnResourcesOrder`
- `RTSBeginConstructionOrder`, `RTSContinueConstructionOrder`
- `RTSSetRallyPointToActorOrder`, `RTSSetRallyPointToLocationOrder`

Orders carry `FRTSOrderData` (order class + target actor/location). The order queue lives in `ARTSPawnAIController` and supports shift+click append.

### Combat (`Classes/Combat/`)
- `URTSAttackComponent` — one or more `FRTSAttackData` entries, acquisition/chase radius, shared cooldown
- `URTSHealthComponent` — HP, death type
- `RTSProjectile` / `RTSProjectileTargetComponent`
- `URTSBountyComponent` — resource bounty on kill
- `RTSActorDeathType` — enum for death behaviour

### Economy (`Classes/Economy/`)
- `URTSGathererComponent` — unit-side gathering; tracks `CarriedResourceType/Amount`, cooldown, `CurrentResourceSource`
- `URTSResourceSourceComponent` — resource node; tracks remaining resources
- `URTSResourceDrainComponent` — building that accepts returned resources; enforces `GathererCapacity`
- `URTSPlayerResourcesComponent` — per-player resource totals
- `URTSResourceType` — resource type CDO (gold, wood, etc.)
- `RTSPaymentType` — enum (pay now vs. pay over time)

### Construction (`Classes/Construction/`)
- `URTSConstructionSiteComponent` — building under construction; tracks progress, assigned builders
- `URTSBuilderComponent` — unit that can construct buildings
- `ARTSBuildingCursor` — ghost preview during placement

### Production (`Classes/Production/`)
- `URTSProductionComponent` — building that produces units; manages `RTSProductionQueue`
- `URTSProductionCostComponent` — cost data attached to produced unit class
- `RTSProductionRallyPoint` — rally point for newly produced units

### Tags (`Classes/`)
- `URTSGameplayTagsComponent` — actor gameplay tags; replicated via `CurrentTags`; exposes `CurrentTagsChanged` delegate
- `RTSGameplayTagsProvider` / `RTSGameplayTagsComponent` — interface for components that contribute tags to an actor
- `RTSOrderTagRequirements` — required/blocked tag sets that gate order issuance

### Vision (`Classes/Vision/`)
- `ARTSFogOfWarActor`, `ARTSVisionActor`, `ARTSVisionInfo`, `ARTSVisionManager`
- `URTSVisionComponent` — per-unit sight radius
- `URTSVisibleComponent` — marks actors that can be seen
- `RTSVisionVolume` — defines vision grid bounds

### UI (`Classes/UI/`)
- `ARTSHUD` — selection frame rendering, minimap input
- `URTSMinimapWidget` — minimap rendering
- `URTSActorWidgetComponent` / `URTSHoveredActorWidgetComponent` — world-space UI on actors
- `URTSFloatingCombatTextComponent` — damage/heal numbers
- `URTSRangeIndicator` — circle indicator for attack/gather range

### Misc Components
- `URTSOwnerComponent` — maps actor to owning `ARTSPlayerState`; replicated
- `URTSSelectableComponent` — marks actor as player-selectable
- `URTSContainerComponent` / `URTSContainableComponent` — transport/garrison
- `URTSRequirementsComponent` — prerequisite building/tech check
- `URTSPawnMovementComponent` — movement for RTS pawns
- `ARTSCameraBoundsVolume`, `ARTSPlayerStart`, `ARTSPlayerAdvantageComponent`

### Libraries (static, Blueprint-callable)
- `RTSOrderLibrary` — order issuance helpers, CDO tag-requirements cache
- `RTSGameplayLibrary` — general actor queries
- `RTSGameplayTagLibrary` — tag relationship helpers, per-frame cache
- `RTSConstructionLibrary`, `RTSEconomyLibrary`, `RTSCollisionLibrary`

---

## Custom Modifications to the Plugin

The plugin has been modified from the upstream ue4-rts baseline. Do **not** revert these changes.

### 1. Continuous Order Tag Validation
`ARTSPawnAIController` stores `CurrentOrder` (FRTSOrderData) and binds delegates on `URTSGameplayTagsComponent::CurrentTagsChanged` for both the unit and the target, plus `OnDestroyed` on the target. `ValidateCurrentOrder()` re-checks `CanObeyOrder` / `IsValidOrderTarget`; failure triggers `IssueStopOrder()`. `UnbindOrderValidationDelegates()` is called before every new order.

### 2. Resource Drain Capacity Enforcement
`URTSResourceDrainComponent` tracks active gatherers via `RegisteredGatherers` (TSet of weak pointers). `CanAcceptGatherer()`, `RegisterGatherer()`, `UnregisterGatherer()` enforce `GathererCapacity`. `FindClosestResourceDrain()` in `URTSGathererComponent` skips full drains.

### 3. OwnActors Memory Leak Fix
`ARTSPlayerState` has a `RegisterOwnedActor()` helper that binds `OnDestroyed` to clean up stale entries. Both `DiscoverOwnActors` and `NotifyOnActorOwnerChanged` go through this helper.

### 4. Order Queuing (Shift+Click)
`ARTSPawnAIController` has a second `IssueOrder(Order, bAppendToQueue)` overload (non-UFUNCTION). Added: `FinishCurrentOrder()`, `ClearOrderQueue()`, `GetOrderQueue()`, `OnOrderQueueChanged` delegate. `ARTSPlayerController::IssueOrderToSelectedActors` detects shift and sets `bAppendToQueue`. `ServerIssueOrder` RPC signature includes `bool bAppendToQueue`. BT/Blueprint queue display still needs editor wiring.

### 5. Gameplay Tag Caching
- `URTSGameplayTagsComponent::GetCurrentTags()` uses a dirty-flag cache (`CachedTags` + `bTagsCacheDirty`); rebuilt only on tag changes.
- `RTSOrderLibrary.cpp`: static `GOrderTagRequirementsCache` (TMap<UClass*, FRTSOrderTagRequirements>) caches CDO tag requirements.
- `RTSGameplayTagLibrary.cpp`: per-frame cache (`GRelationshipTagCache`, keyed on actor pair + frame counter) for relationship tag lookups.

---

## Key Patterns & Conventions

- **All plugin classes** are prefixed `RTS` and use the `REALTIMESTRATEGY_API` export macro.
- **Components** derive from `URTSActorComponent` (itself an `UActorComponent` subclass that also contributes gameplay tags).
- **Orders** are `UObject` CDOs (`URTSOrder` subclasses). Actual order parameters travel as `FRTSOrderData` structs.
- **Replication**: player-facing state is on `ARTSPlayerState`; unit state replicates via components. Server RPCs follow `Server<Action>` naming with `Reliable, Server, WithValidation`.
- **Blueprint events** follow the `NotifyOn*` (C++ virtual) + `ReceiveOn*` (BlueprintImplementableEvent) pattern throughout `ARTSPlayerController`.
- **Logging**: use the `RTS` log category (defined in `RTSLog.h`).

---

## Build & Iteration Notes

- Open `.uproject` with **Unreal Engine 4.27**.
- The plugin is **not** Installed; it compiles from source with the project.
- No third-party marketplace plugins are referenced in the `.uproject`.
- Hot-reload works for C++ changes to the plugin; full rebuild required when adding new UHT-reflected types.
- The project targets Win64 only (see `WhitelistPlatforms` in the `.uplugin`).
