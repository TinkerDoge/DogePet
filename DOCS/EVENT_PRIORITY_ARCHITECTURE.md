# DogePet Event Priority Architecture

## Visual Priority Flow Chart

```
┌─────────────────────────────────────────────────────────────────┐
│                 EVENT PRIORITY HIERARCHY                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  PRIORITY 3 ▲                                                   │
│  (HIGHEST) │  ┌──────────────────────────────────┐              │
│            │  │  FURIOUS_SHAKE                   │              │
│            │  │  • Aggressive movement detected  │              │
│            │  │  • Eyes: angry + sweat + jiggle  │              │
│            │  │  • Sound: Angry/startled         │              │
│            │  │  • Duration: 500ms               │              │
│            │  │  • Wakes device if sleeping      │              │
│            │  │  • Clears: Shake, Tilt, Confused│              │
│            │  └──────────────────────────────────┘              │
│            │         ▲                                           │
│            │         │ PREEMPTS (clears all below)              │
│            │         │                                          │
│            │  ┌──────────────────────────────────┐              │
│            │  │  COMBO (HEAD + CHIN)             │              │
│            │  │  • Touch head + chin together    │              │
│            │  │  • Eyes: confused + h-flicker   │              │
│            │  │  • Sound: Curious whistle        │              │
│            │  │  • Duration: 2000ms             │              │
│            │  │  • Clears: Shake, Tilt          │              │
│            │  └──────────────────────────────────┘              │
│            │         ▲                                           │
│            │         │ PREEMPTS (clears all below)              │
│            │         │                                          │
│  PRIORITY 1 │  ┌──────────────────────────────────┐              │
│ (MEDIUM)   │  │  SHAKE / TAP                     │              │
│            │  │  • Head shaking detected         │              │
│            │  │  • Eyes: happy + v-flicker       │              │
│            │  │  • Sound: Happy chatter          │              │
│            │  │  • Duration: 800ms               │              │
│            │  │  • Clears: Tilt                  │              │
│            │  └──────────────────────────────────┘              │
│            │         ▲                                           │
│            │         │ PREEMPTS (clears below)                  │
│            │         │                                          │
│            │  ┌──────────────────────────────────┐              │
│            │  │  TILT                            │              │
│            │  │  • Gentle head tilt detected     │              │
│            │  │  • Eyes: curious                 │              │
│            │  │  • Sound: Curious whistle        │              │
│            │  │  • Duration: 2000ms (debounced) │              │
│            │  │  • No clearing (lowest priority) │              │
│            │  └──────────────────────────────────┘              │
│            │                                                    │
│  PRIORITY 0 │                                                    │
│  (LOWEST)   │                                                    │
│            │                                                    │
└─────────────┴────────────────────────────────────────────────────┘

OVERRIDE BY: ▼

┌─────────────────────────────────────────────────────────────────┐
│            SPECIAL: STILL EVENT (Complete Reset)               │
│                                                                  │
│  • Fires when no motion detected                               │
│  • Clears ALL animation flags and flicker settings             │
│  • Resets activeEventPriority to -1                            │
│  • Allows lower-priority events to resume                      │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## State Transition Matrix

```
       From\To    TILT   SHAKE   COMBO   FURIOUS   STILL
       ────────────────────────────────────────────────────
       TILT        —      ✓       ✓       ✓         ✓
       SHAKE      [×]     —       ✓       ✓         ✓
       COMBO      [×]    [×]      —       ✓         ✓
       FURIOUS    [×]    [×]     [×]      —         ✓
       STILL      ✓      ✓       ✓       ✓         —

Legend:
  ✓   = Clean transition (higher priority clears lower)
  [×] = Blocked (lower priority cannot override higher)
  —   = Same state (no-op)
```

## Animation State Cleanup Map

```
When SHAKE fires during TILT:
  Before: TILT (curious=true, ...)
  Action: clearLowPriorityAnimation() → curious=false, setVFlicker(0,0), setHFlicker(0,0)
  After:  SHAKE (happy=true, setVFlicker(1,2))

When FURIOUS fires during SHAKE:
  Before: SHAKE (happy=true, setVFlicker(1,2))
  Action: clearLowPriorityAnimation() → happy=false, setVFlicker(0,0), setHFlicker(0,0)
  After:  FURIOUS (angry=true, sweat=true, confused=true, setHFlicker(3,1))

When STILL fires:
  Before: ANY state (various mood flags + flicker settings)
  Action: Complete reset of all 5 mood flags + both flicker settings
  After:  Clean slate for new events
```

## Timeline Example: Multiple Events at Same Time

```
t=0ms     t=100ms      t=500ms      t=800ms       t=1600ms
│         │            │            │             │
└─────┬───────────────────────────────────────────┘
      │
      TILT event fires
      tiltTriggered = true
      activeEventPriority = 0
      eyes->curious = true

t=50ms
│
└─────┬───────────────────────────────────────────┐
      │                                           │
      SHAKE event fires (same instant!)           │
      clearLowPriorityAnimation(eyes, 0) called   │
        → eyes->curious = false                   │
        → eyes->setVFlicker(0,0)                  │
      shakeTriggered = true                       │
      activeEventPriority = 1                     │
      eyes->happy = true                          │
      eyes->setVFlicker(1, 2)                     │
                                                  │
                                    Shake timeout │
                                    ▼             │
                                    eyes->happy=false
                                    eyes->setVFlicker(0,0)
                                    shakeTriggered=false
                                    activeEventPriority=-1
                                                  │
                                                  │ TILT resumes? NO
                                                  │ STILL fires from sensor
                                                  │ activeEventPriority=-1
                                                  │
                                                  ✓ Clean state
```

## Critical Fix Points

### 1. **Flicker Settings Cleanup** ⭐
**Problem**: Flicker settings persist across event transitions  
**Fix**: Explicitly call `setVFlicker(0,0)` and `setHFlicker(0,0)` in:
- Shake timeout handler
- Furious shake timeout handler
- Still reset handler
- clearLowPriorityAnimation() helper

### 2. **Preemption on Event Trigger** ⭐
**Problem**: Higher-priority event sets flags but doesn't clear lower-priority animations  
**Fix**: Call `clearLowPriorityAnimation(eyes, minPriorityToClear)` BEFORE setting new animation state

### 3. **Priority Release** ⭐
**Problem**: activeEventPriority not cleared when animation naturally completes  
**Fix**: Check `if (activeEventPriority == CURRENT_PRIORITY)` before releasing to -1

### 4. **Complete Reset on Still** ⭐
**Problem**: Still event only cleared some flags, leaving orphaned state  
**Fix**: Reset ALL 5 mood flags + both flicker settings + all triggered flags

## Testing Scenarios

### Scenario 1: Rapid Shake Taps
```
1. Shake event fires → happy=true, setVFlicker(1,2)
2. Still fires immediately (sensor detects no motion)
3. All state cleared → happy=false, setVFlicker(0,0)
4. Repeat with no stuck animations ✓
```

### Scenario 2: Shake During Combo
```
1. Combo fires → confused=true, setHFlicker(3,1)
2. Furious shake fires (aggressive movement)
3. clearLowPriorityAnimation() called
4. confused=false, setHFlicker(0,0) ← CRITICAL
5. Then angry=true, sweat=true, confused=true ✓
```

### Scenario 3: Furious Then Still
```
1. Furious fires → angry=true, sweat=true, confused=true, setHFlicker(3,1)
2. After 500ms, timeout handler clears setHFlicker(0,0)
3. Still fires → all flags reset + setVFlicker(0,0)
4. Complete clean state ✓
```

---

**Implementation Location**: [Events.cpp](../libraries/DogePetLib/src/Events.cpp)  
**Helper Function**: `clearLowPriorityAnimation()` (lines 43-63)  
**Test File**: (Add integration tests in test suite)
