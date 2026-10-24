# Orbit: rediseño nativo de DolphinQt

Actualizado el 24/10/2026. Base inspeccionada: `e1d35f666cc35f6ead55d32ea2679df44d04da72`
(upstream del 22/10/2026). Rama del fork: `main`. Orbit es un nombre provisional.
El remoto `upstream` apunta a Dolphin oficial. El fork público conserva su historial:
[Manuucho27/dolphin-orbit](https://github.com/Manuucho27/dolphin-orbit).

## Arquitectura comprobada

DolphinQt usa C++ y Qt Widgets. Su objetivo CMake es `dolphin-emu`; `AUTOMOC` y
`AUTORCC` ya están habilitados. Windows incluye Qt 6.8.3 mediante `Externals/Qt`.
La ruta vigente de compilación utiliza `CMakePresets.json`.

La integración inicial queda así:

```text
GameTracker → GameListModel → GridProxyModel → HomeWidget / cuadrícula
                          → ListProxyModel → tabla clásica

HomeWidget.GameSelected → GameList.GameSelected → MainWindow.Play()
HomeWidget.ajustes     → GameList.señales      → diálogos de MainWindow
```

`GameList` sigue siendo dueño del único modelo y del seguimiento de archivos.
Home es una tercera página de su `QStackedWidget`. Comparte búsqueda, filtros,
ordenación y artwork con la cuadrícula. Las acciones de contexto consultan la
selección de la vista activa. `MainWindow` conserva su pila y sus funciones
`ShowRenderWidget()` y `HideRenderWidget()` para el ciclo de emulación.

## Fase 0 — base reproducible

Completada: repositorio oficial, submódulos fijados, helper, compilación completa
de los 2031 objetivos y apertura de Dolphin con Home y un perfil aislado.
Entorno: Build Tools 2026 18.11.1, MSVC 19.51.36260, CMake 4.3.1-msvc1,
Qt 6.8.3 y Windows SDK 10.0.26100. `Dolphin.exe --version` terminó con código 0.

El bloqueo inicial con MSVC 19.51.36243 quedó resuelto al actualizar las
herramientas; se conserva el requisito original de `Source/PCH/pch.h` sin
eliminarlo ni falsear versiones. Se usó un árbol limpio y corto en
`work/orbit-release`, fuera del repositorio, para evitar PCH antiguos C1853.
El ejecutable también se copió al escritorio, con hash idéntico y acceso directo.

## Fase 1 — Home nativa inicial, implementada

| Archivo | Cambio preparado |
| --- | --- |
| `Source/Core/DolphinQt/Home/HomeWidget.h/.cpp` | Identidad azul noche/cian/coral, panel del juego seleccionado, carrusel horizontal con delegate, estados vacío/sin coincidencias y botones Jugar, carpeta, mandos, gráficos y ajustes. |
| `Source/Core/DolphinQt/GameList/GameList.h/.cpp` | Tercera vista, selección activa, transferencia de selección mediante índice fuente, menú contextual y propagación de acciones. |
| `Source/Core/DolphinQt/MenuBar.h/.cpp` | Opción exclusiva `Orbit Home` junto a lista y cuadrícula. |
| `Source/Core/DolphinQt/MainWindow.cpp` | Conexiones Home y diálogo de mandos; arranque por la ruta existente. |
| `Source/Core/DolphinQt/Settings.h/.cpp` | Preferencia `Orbit/HomeView`, activada por defecto; conserva `PreferredView` para lista/cuadrícula. |
| `Source/Core/DolphinQt/CMakeLists.txt` | Registro de los dos archivos nuevos para compilación y moc. |
| `Tools/Orbit/Build-Windows.ps1` | Detección de herramientas, submódulos, presets, distribución Orbit, modo portable y opciones `-CleanFirst`/`-BuildDirectory`; captura UTF-8 del entorno. |

Teclado: flechas para el carrusel, Enter para jugar, Alt+Enter para las
propiedades existentes y Tab para los botones. Los botones absorben Enter;
la cabecera no puede arrancar por accidente el juego seleccionado. El clic
derecho conserva las acciones de Dolphin. Los metadatos se muestran como texto
plano. Las tarjetas usan un delegate y no crean un widget por juego.

Validado: sintaxis MSVC de los cinco `.cpp` modificados y prueba ejecutable de
Home con Qt real, modelos sintéticos, selección, filtros, borrado, reset,
actualizaciones, foco y botones; compilación completa y apertura de la interfaz
real con perfil aislado. En esa apertura se detectó un fondo blanco; se añadió
`WA_StyledBackground` y la recompilación pasó. La regresión automatizada con
paleta blanca también pasó; queda la revisión visual de la aplicación corregida.
También falta verificar biblioteca y juegos
reales. Véase `ORBIT_VALIDATION.md`.

## Fase 2 — experiencia de salón

Archivos previstos: ampliar `Home/HomeWidget.*`; añadir `Home/HomeTheme.h` y,
si el contenido lo exige, `Home/HomeLibraryProxyModel.h/.cpp`; ajustar
`MainWindow.*` y `Settings.*`.

- Crear modo salón a pantalla completa con salida clara al modo escritorio.
- Refinar jerarquía, panel y portadas a 720p/1080p/4K con escalado de Windows.
- Incorporar transiciones discretas y opción de reducir movimiento.
- Separar GameCube/Wii, favoritos mediante etiquetas existentes y recientes
  persistidos por ruta de archivo; compartir siempre el modelo original.
- Guardar selección por ruta, no por número de fila, entre sesiones.
- Si se añade artwork grande, cargarlo fuera del hilo de interfaz y mantener
  caché limitada, con fallback cuando falte. No descargar assets en cada repintado.

Cierre: navegación de biblioteca y controles accesible con teclado, diseño sin
recortes a 720p y 100–200 % de DPI, búsqueda y cambio de vistas coherentes.
El marco clásico de menús y barra de herramientas aún permanece en esta entrega.

## Fase 3 — navegación por mando

Archivos previstos: `Home/HomeInputRouter.h/.cpp`, `Home/HomeWidget.*`,
`MainWindow.*` y registro CMake. Referencias existentes:
`DolphinQt/HotkeyScheduler.*` e `InputCommon/ControllerInterface/`.

Usar las abstracciones de entrada de Dolphin para mapear cruceta/stick,
aceptar/volver, pestañas y menú de opciones. Aplicar zona muerta, repetición
controlada, reconexión y un indicador de foco visible. No abrir un segundo
bucle SDL. Entregar acciones al hilo Qt y suspender navegación de Home durante
emulación y diálogos modales, evitando conflictos con hotkeys y juego.

Cierre: manejo completo de Home con mando, conexión/desconexión y ausencia de
acciones duplicadas durante emulación. El botón Controllers de fase 1 abre la
configuración de Dolphin; la navegación de Home por mando todavía no está implementada.

## Fase 4 — integración y regresiones

Archivos: `MainWindow.*`, `GameList/GameList.*`, `Home/*`; ampliar pruebas de UI
en un objetivo separado si se mantienen dentro del fork.

Probar arranque, error de arranque, pausa, parada y retorno; render integrado y
en ventana separada; cambio de disco; búsqueda durante escaneo; archivos
duplicados por título; eliminación del seleccionado; filtros sin resultados;
NetPlay, propiedades y vistas clásicas. Verificar perfiles portable y normal,
accesibilidad, tamaños de fuente y biblioteca grande.

Cierre: binario Windows reproducible y sesión completa de juego/parada/retorno
sin pérdida de selección ni regresiones en los flujos anteriores.

## Fase 5 — identidad y mantenimiento del fork

Conservar avisos y licencia GPL-2.0-or-later; usar recursos propios para el
branding. Crear rama de seguimiento de upstream y revisar cambios de
`MainWindow`, `GameList` y entradas al actualizar la base. Evitar sustituir
el fork con el actualizador oficial: el helper compila con
`-DDISTRIBUTOR=Orbit -DENABLE_AUTOUPDATE=OFF`.

`Contributing.md` exige revisión y responsabilidad humana al enviar cambios a
Dolphin y declarar las partes generadas con IA. Los cambios de esta entrega
están asistidos por IA y se limitan a UI y herramientas; no modifican el
comportamiento de la consola emulada.

## Compilar en Windows

Requisitos: Visual Studio/Build Tools 2026 actualizado, herramientas C++ x64,
Windows SDK, componentes CMake/Ninja y Git. Desde la raíz del repositorio:

```powershell
.\Tools\Orbit\Build-Windows.ps1 -Configuration Release -Jobs 6 -Portable -CleanFirst -BuildDirectory ..\..\work\orbit-release
```

`-SkipSubmodules` omite la actualización de dependencias ya preparadas.
`-BuildDirectory` permite elegir un árbol corto fuera del repositorio;
`-CleanFirst` limpia los objetos previos de ese árbol antes de compilar, conservando
la configuración. Tras cambiar de versión de MSVC, usar un árbol limpio para evitar
incompatibilidades con archivos PCH anteriores.
`-Configuration Debug` genera `DolphinD.exe`. El helper no instala herramientas
ni abre el emulador. `-Portable` crea `portable.txt` junto al ejecutable solo
después de una compilación correcta, para usar un directorio User propio.
No copiar el perfil principal sobre el perfil de prueba sin una copia previa.

Alternativa desde una terminal de desarrollo x64 de VS, en la raíz:

```powershell
git -c core.longpaths=true submodule update --init --recursive --depth 1 --jobs 6
cmake --preset ninja-release-x64 -DDISTRIBUTOR=Orbit -DENABLE_AUTOUPDATE=OFF -DENABLE_TESTS=OFF
cmake --build --preset ninja-build-release-x64 --target dolphin-emu --parallel 6
```

Salida del helper con el directorio indicado: `../../work/orbit-release/Binaries/Dolphin.exe`.
Sin `-BuildDirectory`, la salida Release es `build/release/x64/Binaries/Dolphin.exe`.
CMake despliega Qt.
El preset `visualstudio-release-x64` usa VS 18 2026/v145 y necesita CMake 4.2+;
el mínimo general del proyecto para Ninja es CMake 3.25.

## Fuentes oficiales consultadas

- [Compilar en Windows](https://github.com/dolphin-emu/dolphin/wiki/Building-for-Windows)
- [Compilar con CMake](https://github.com/dolphin-emu/dolphin/wiki/Building-with-CMake)
- [Clonar y submódulos](https://github.com/dolphin-emu/dolphin/wiki/Cloning)
- [Presets de la base fijada](https://github.com/dolphin-emu/dolphin/blob/e1d35f666cc35f6ead55d32ea2679df44d04da72/CMakePresets.json)
- [Requisito real de MSVC y SDK](https://github.com/dolphin-emu/dolphin/blob/e1d35f666cc35f6ead55d32ea2679df44d04da72/Source/PCH/pch.h)
- [CMake: generador VS 18 2026](https://cmake.org/cmake/help/latest/generator/Visual%20Studio%2018%202026.html)
