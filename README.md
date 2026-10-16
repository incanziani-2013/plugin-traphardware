# HARDWARETRAP — Mutation Engine (VST3)

Plugin de efecto VST3 de 64 bits para Windows (JUCE 8 + C++17 + CMake), pensado para FL Studio.
Transforma sonidos normales en texturas rotas, distorsionadas y glitcheadas (trap, jerk, crank, rage, ambient, underground).

## Controles (LIVE MODE) — todos procesan audio y son automatizables

| Control | Qué hace |
|---|---|
| **DESTROY** | Saturación asimétrica + hard clip + wavefolder. 0 % = señal limpia. |
| **MOVEMENT** | LFO (0.4–14 Hz) que modula el drive, la textura y el nivel del efecto → movimiento rítmico. |
| **TEXTURE** | Bitcrusher (16→3 bits) + reducción de sample rate (hasta 31×). |
| **UNSTABLE** | Deriva aleatoria de parámetros + glitches: stutter, grano corto, reversa y gates. |
| **MIX** | Dry / Wet. |
| **OUTPUT** | Nivel de salida (−24 a +12 dB). Hay un limitador suave de seguridad. |

Mono y estéreo. Los parámetros se suavizan (25 ms) para evitar clicks. Estado, región y ruta del sample se guardan con el proyecto de FL Studio.

## SAMPLE LAB
1. Pestaña **SAMPLE LAB** → **LOAD WAV** (o arrastrá un archivo WAV/AIFF/FLAC sobre la onda).
2. Se dibuja la forma de onda real. Arrastrá para elegir una región; arrastrá los bordes blancos para ajustarla; doble click = todo el sample.
3. **PLAY / STOP** y **LOOP**: previsualiza la región pasando por el mismo motor y con los mismos controles de abajo. (La vista previa reemplaza la entrada del plugin mientras suena.)
4. **EXPORT WAV**: renderiza la región con los ajustes actuales a WAV de 24 bits, a la frecuencia de muestreo original del archivo.

Límite: 10 minutos por sample.

## Presets
Combo + flechas arriba. 8 presets de fábrica. **SAVE** guarda un preset de usuario en
`%APPDATA%\HARDWARETRAP\Presets\*.htpreset`.

## Compilar

### Opción A — GitHub Actions (no necesitás instalar nada)
1. Subí esta carpeta a un repositorio de GitHub (rama `main`).
2. Pestaña **Actions** → workflow **Build HARDWARETRAP VST3 (Windows x64)** (corre solo en cada push; también con *Run workflow*).
3. Al terminar, en **Artifacts** descargá `HARDWARETRAP-VST3-win64.zip` (contiene `HARDWARETRAP.vst3`).

### Opción B — Local en Windows
Requisitos: Visual Studio 2022 (carga de trabajo "Desarrollo para el escritorio con C++"), CMake ≥ 3.22 y Git.
```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
```
Resultado: `build\HARDWARETRAP_artefacts\Release\VST3\HARDWARETRAP.vst3`
(la primera configuración descarga JUCE 8.0.6 desde GitHub).

## Instalar y abrir en FL Studio
1. Copiá `HARDWARETRAP.vst3` a `C:\Program Files\Common Files\VST3\` (hace falta permiso de administrador).
2. FL Studio → **Options → Manage plugins** → **Find installed plugins** (o *Start scan*).
3. Abrí un slot de efecto del Mixer → **Select** → buscá **HARDWARETRAP** (en *Installed → Effects*).
4. Para SAMPLE LAB: en el ícono de engranaje del plugin desactivá **Smart disable** (si no, FL puede dejar de procesar cuando no hay audio entrando y no oirías la vista previa).
5. Automatización: click derecho sobre una perilla → *Create automation clip*.

## Estructura
```
CMakeLists.txt
Source/Engine.h             motor DSP (compartido por live, preview y export)
Source/PluginProcessor.*    parámetros, estado, presets, sample lab
Source/PluginEditor.*       interfaz gráfica 920×620 dibujada por código
.github/workflows/build.yml compilación automática + ZIP
```

## Estado de verificación
- Compilado sin errores (JUCE 8.0.6, GCC, Linux x86_64) como VST3.
- Motor DSP probado offline: cada control cambia la señal, sin NaN, picos acotados, mono y estéreo.
- **No verificado todavía:** la compilación en Windows/MSVC (la hace el workflow) y la carga dentro de FL Studio.
