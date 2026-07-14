# Layout UI — Boxer Video

Ten dokument opisuje, jak działa układ interfejsu, jak budować aplikację oraz jak uruchamiać różne warianty layoutu (pionowy produkcyjny, poziomy dev na laptopie, 4K).

## Koncepcja

UI nie używa sztywnych pikseli ekranu fizycznego. Wszystkie współrzędne są liczone w **design-space** — wirtualnej rozdzielczości z configu (`window_width` × `window_height`). Na końcu `Renderer` mapuje je na prawdziwe piksele okna/monitora przez `ui::Layout` (jednolita skala + ewentualne pasy letterbox).

```
Config (design-space)  →  Ekrany / widżety (procenty PW/PH/PM)  →  Layout::X/Y/S  →  SDL (piksele)
```

### Orientacja

- **Portrait** — gdy `window_height > window_width` (docelowy monitor w automacie: 1080×1920 lub 2160×3840).
- **Landscape** — gdy szerokość ≥ wysokość (dev na laptopie: 1920×1080).

Ekrany sprawdzają `layout().IsPortrait()` i układają bloki inaczej (np. ranking pod panelem w pionie, obok panelu w poziomie).

### Helpery procentowe (`src/ui/Layout.h`)

| Metoda | Znaczenie |
|--------|-----------|
| `PW(frac)` | `frac × design_w` — szerokości, pozycje X |
| `PH(frac)` | `frac × design_h` — wysokości, pozycje Y |
| `PM(frac)` | `frac × min(design_w, design_h)` — pady, promienie, ikony, fonty pomocnicze |
| `PRect(x, y, w, h)` | prostokąt ze wszystkich frakcji naraz |
| `IsPortrait()` | true gdy design-space jest pionowy |
| `CenterX()` / `CenterY()` | środek design-space |

Przykład w ekranie:

```cpp
const ui::Layout& lay = r.layout();
const int btn_w = lay.PW(lay.IsPortrait() ? 0.70f : 0.30f);
const int btn_h = lay.PH(0.065f);
```

### Fonty

Rozmiary TTF są liczone jako ułamek `min(design_w, design_h)` (domyślnie baza 1080 pt):

| FontSize | Frakcja | ~pt przy 1080 |
|----------|---------|---------------|
| Small    | 2.8%    | 30            |
| Normal   | 4.3%    | 46            |
| Large    | 7.6%    | 82            |
| Huge     | 13.0%   | 140           |

Na 4K portrait (min = 1080) proporcje tekstu są takie same jak na FullHD portrait.

---

## Pliki konfiguracyjne

| Plik | Design-space | Okno / ekran | Zastosowanie |
|------|--------------|--------------|--------------|
| [`config/app.yaml`](config/app.yaml) | 1080×1920 | fullscreen (KMS) | **Produkcja RPI** — monitor pionowy |
| [`config/app-windows.yaml`](config/app-windows.yaml) | 1920×1080 | 1280×720 okno | **Dev laptop** — układ poziomy |
| [`config/app-windows-portrait.yaml`](config/app-windows-portrait.yaml) | 1080×1920 | 480×854 okno | **Dev laptop** — podgląd układu pionowego |

### Kluczowe pola configu

```yaml
window_width: 1080      # szerokość design-space (logika layoutu)
window_height: 1920     # wysokość design-space
display_width: 1280     # fizyczny rozmiar okna (tylko windowed); 0 = jak window_*
display_height: 720
fullscreen: false       # true na RPI (pełny ekran)
layout_scale: auto      # auto = dopasuj do okna; lub liczba np. 1.0 (debug)
```

- **`window_width` / `window_height`** — definiują orientację i proporcje UI. Zmiana tych wartości przełącza portrait ↔ landscape.
- **`display_width` / `display_height`** — tylko rozmiar okna SDL w trybie okienkowym; **nie** zmieniają logiki layoutu.
- **`layout_scale: auto`** — skala = `min(actual_w/design_w, actual_h/design_h)`; przy innym aspekcie pojawiają się czarne pasy.
- **`layout_scale: 1.0`** — wymusza skalę 1:1 (debug, możliwy clipping).

### 4K portrait (produkcja)

W [`config/app.yaml`](config/app.yaml) ustaw:

```yaml
window_width: 2160
window_height: 3840
```

Layout jest procentowy — nie trzeba zmieniać kodu ekranów.

---

## Budowanie

### Windows (MSYS2 / MinGW)

```cmd
scripts\setup_windows.cmd
```

lub, gdy zależności są już zainstalowane:

```cmd
cmake --build build-mingw
```

### Raspberry Pi

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
```

---

## Uruchamianie różnych layoutów

### 1. Dev — landscape (laptop, domyślny)

Okno 1280×720, design-space 1920×1080. Panel „INSERT COIN” po lewej, ranking po prawej.

```cmd
scripts\run_windows.cmd
```

Równoważnik:

```cmd
build-mingw\tvbox_gui.exe config\app-windows.yaml
```

(Uruchamiaj przez `run_windows.cmd` — kopiuje DLL MinGW i ustawia PATH.)

### 2. Dev — portrait (podgląd automatu na laptopie)

Wąskie okno 480×854, design-space 1080×1920 — ten sam układ co na monitorze docelowym.

```cmd
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\copy_mingw_dlls.ps1
set PATH=C:\msys64\mingw64\bin;%PATH%
build-mingw\tvbox_gui.exe config\app-windows-portrait.yaml
```

### 3. Produkcja — RPI, monitor pionowy FullHD

```bash
SDL_VIDEODRIVER=kmsdrm SDL_RENDER_DRIVER=opengles2 \
  ./build/tvbox_gui config/app.yaml
```

Po deploy (`scripts/deploy_rpi5.sh` lub `make rpi5`) config leży w `/home/erwinek/tvbox/config/app.yaml`.

### 4. Pełny ekran na Windows (test bez okienka)

Skopiuj config i ustaw:

```yaml
window_width: 1080
window_height: 1920
display_width: 0
display_height: 0
fullscreen: true
```

```cmd
build-mingw\tvbox_gui.exe config\app-windows-portrait.yaml
```

---

## Układ ekranów (skrót)

| Ekran | Portrait | Landscape |
|-------|----------|-----------|
| **Attract** | Panel „INSERT COIN” u góry (~86% szer.), ranking pod spodem (~90% szer.) | Panel po lewej, ranking w prawej kolumnie (~30% szer.) |
| **ModeSelect** | Przyciski ~70% szer., wyśrodkowane | Przyciski ~30% szer., wyśrodkowane |
| **Measure** | Badge trybu u góry, „UDERZ TERAZ” / wynik na środku | j.w. (procenty wysokości) |
| **EndGame** | Wynik → replay → ranking jeden pod drugim | Lewa kolumna: wynik + replay; prawo: ranking |

Wspólne elementy na wszystkich ekranach:
- **Header** — góra (~8% h portrait / ~11% h landscape)
- **HUD** — lewy dół: REKORD + CREDIT
- **ScrollBar** — dolny pasek z przewijanym tekstem (Attract, EndGame)

---

## Rozwijanie layoutu

### Nowy element UI

1. Używaj `lay.PW()` / `lay.PH()` / `lay.PM()` zamiast stałych pikseli.
2. Dla elementów zależnych od orientacji: `if (lay.IsPortrait()) { ... } else { ... }`.
3. Ranking: przekaż obszar docelowy do `LeaderboardWidget::Render(r, entries, area)` — widżet sam dopasuje wiersze do wysokości `area`.

### Pliki do edycji

| Warstwa | Pliki |
|---------|--------|
| Helpery skali | [`src/ui/Layout.h`](src/ui/Layout.h), [`src/ui/Layout.cpp`](src/ui/Layout.cpp) |
| Fonty | [`src/ui/FontManager.cpp`](src/ui/FontManager.cpp) |
| Widżety | [`src/ui/widgets/`](src/ui/widgets/) |
| Ekrany | [`src/screens/`](src/screens/) |

### Debug skali

W logu przy starcie aplikacji:

```
Layout: design 1080x1920, actual 480x854, scale=0.44, offset=(0,0)
```

- **design** — z configu
- **actual** — rozdzielczość SDL po starcie
- **scale** — współczynnik mapowania design → ekran

---

## Szybka ściągawka

```cmd
:: Build
cmake --build build-mingw

:: Landscape dev (laptop)
scripts\run_windows.cmd

:: Portrait dev (podgląd automatu)
build-mingw\tvbox_gui.exe config\app-windows-portrait.yaml
```

```bash
# RPI produkcja (portrait FullHD)
./build/tvbox_gui config/app.yaml
```

Sterowanie dev: `ESC` wyjście, `SPACE` uderzenie, `1`–`4` skok między stanami, `C` kredyt.
