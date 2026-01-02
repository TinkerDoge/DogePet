# DogePet Touch Controls - Quick Reference

## 🎮 Simple Touch Gestures

```
┌─────────────────────────────────────────────┐
│         TOUCH_DOWN (GPIO35)                 │
│                                             │
│  👆👆  Double-Tap                           │
│  →  Switch Faces                            │
│      (Eyes → Clock → Notif → Eyes)          │
└─────────────────────────────────────────────┘

┌─────────────────────────────────────────────┐
│         TOUCH_UP (GPIO39)                   │
│                                             │
│  👆👆  Double-Tap                           │
│  →  Silent Mode Toggle                      │
│      (🤫 Silent ↔ 🎵 Chatty)                │
└─────────────────────────────────────────────┘

┌─────────────────────────────────────────────┐
│    SQUEEZE (DOWN + UP together)             │
│                                             │
│  🤏  Hold Both                              │
│  →  WiFi Config Portal                      │
│      (Setup AP: "DogePet-Setup")            │
└─────────────────────────────────────────────┘
```

## 🔘 FUNC Button Combinations

```
┌─────────────────────────────────────────────┐
│    FUNC (GPIO41) + TOUCH_DOWN               │
│                                             │
│  Hold 2 seconds                             │
│  →  Toggle BLE On/Off                       │
│      (Blue LED shows status)                │
└─────────────────────────────────────────────┘

┌─────────────────────────────────────────────┐
│    FUNC (GPIO41) + TOUCH_UP                 │
│                                             │
│  Hold 2 seconds                             │
│  →  Toggle WiFi On/Off                      │
│      (Connect to configured network)        │
└─────────────────────────────────────────────┘

┌─────────────────────────────────────────────┐
│    FUNC (GPIO41) Hardware Functions         │
│                                             │
│  Single press  →  Power On                  │
│  Double press  →  Power Off                 │
└─────────────────────────────────────────────┘
```

## 📱 Typical Usage

### Daily Use
1. **Double-tap DOWN** to check clock
2. **Double-tap DOWN** again to see notifications  
3. **Double-tap DOWN** once more to return to eyes

### Going Quiet
- **Double-tap UP** to enable silent mode
- **Double-tap UP** again to restore sound

### First Setup
1. **Squeeze** (hold both sensors) to start WiFi config
2. Connect phone to "DogePet-Setup" AP
3. Open browser to configure WiFi
4. **Squeeze** again to connect

### Toggle Connectivity
- **FUNC + DOWN** (hold 2s) for BLE
- **FUNC + UP** (hold 2s) for WiFi

## 🎯 Tips

- **Double-tap timing**: 600ms window (about half a second between taps)
- **Squeeze**: Hold both sensors simultaneously for ~500ms
- **FUNC holds**: Must hold for full 2 seconds
- **Watch serial monitor**: All gestures print debug messages

## 🐛 Troubleshooting

**Double-tap not working?**
- Tap faster (within 600ms)
- Check serial for "[TAP]" messages

**Squeeze not working?**
- Hold BOTH sensors at the same time
- Don't press FUNC button simultaneously
- Check serial for "[SQUEEZE]" message

**FUNC combinations not working?**
- Hold for full 2 seconds
- Check serial for "[HOLD]" messages
- Make sure only ONE touch sensor is held (not both)

---

**Simple. Intuitive. Responsive.** ✨
