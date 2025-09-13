This folder contains a lightweight scaffold for splitting the legacy sketch into modular "sectors".

Files included:
- Brain.{h,cpp} - thin wrapper around `ai_companion` for AI messaging and callbacks
- MotionSector.{h,cpp} - wraps `motion` IMU-driven updates and provides hooks
- Animations.{h,cpp} - wrapper around `animation_engine` for eye and sfx commands
- UI.{h,cpp} - thin wrapper around sketch-level toasts and future display code
- Sensors.{h,cpp} - low-level sensor initialization (MPU) and touch hooks
- Vitals.{h,cpp} - battery and vitals helpers using `clock` helpers
- Sectors.{h,cpp} - single entrypoint to begin/update all sectors

Design notes
- This is intentionally non-invasive: the wrappers forward to existing legacy functions so you can incrementally migrate behavior into each sector.
- Next steps: wire `Sectors::beginAll()` and `Sectors::updateAll()` into `DogePet.ino`'s setup() and loop(), and progressively move logic from legacy files into these sectors.
