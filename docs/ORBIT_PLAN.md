# Orbit: rediseño nativo de DolphinQt

Actualizado el 24/10/2026. Base inspeccionada: `e1d35f666cc35f6ead55d32ea2679df44d04da72`
(upstream del 22/10/2026). Rama del fork: `main`. Orbit es un nombre provisional.
El remoto `upstream` apunta a Dolphin oficial. El fork público conserva su historial:
[Manuucho27/dolphin-orbit](https://github.com/Manuucho27/dolphin-orbit).

Orbit utiliza la base de código completa de **Dolphin Emulator Project y sus
autores y contribuidores**, procedente de
[dolphin-emu/dolphin](https://github.com/dolphin-emu/dolphin). La idea, el concepto
y la dirección de Orbit son de [Manuucho27](https://github.com/Manuucho27); todo
el trabajo técnico de Orbit en esta colaboración lo realiza **Codex, un modelo
de IA de OpenAI, y sus subagentes**. Esto no atribuye a la IA el emulador ni las
dependencias originales. Se conservan sus créditos y licencias.

## Arquitectura comprobada

DolphinQt usa C++ y Qt Widgets. Su objetivo CMake es `dolphin-emu`; `AUTOMOC` y
`AUTORCC` ya están habilitados. Windows incluye Qt 6.8.3 mediante `Externals/Qt`.
La ruta vigente de compilación utiliza `CMakePresets.json`.

La integración queda así:

```text
GameTracker → GameListModel → GridProxyModel → HomeWidget / cuadrícula
                          → ListProxyModel → tabla clásica

HomeWidget.GameSelected → GameList.GameSelected → MainWindow.Play()
HomeWidget.ajustes     → GameList.señales      → diálogos de MainWindow
MainWindow            → OrbitTitleBar        → acciones originales de Dolphin
Qt.ini del perfil     → OrbitBackground      → imagen/GIF del juego seleccionado
```

`GameList` sigue siendo dueño del único modelo y del seguimiento de archivos.
Home es una tercera página de su `QStackedWidget`. Comparte búsqueda, filtros,
ordenación y artwork con la cuadrícula. Las acciones de contexto consultan la
selección de la vista activa. `MainWindow` conserva su pila y sus funciones
`ShowRenderWidget()` y `HideRenderWidget()` para el ciclo de emulación.

## Fase 0 — base reproducible, primera entrega completada

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

En la primera entrega se validaron la sintaxis MSVC de los cinco `.cpp` modificados y la prueba ejecutable de
Home con Qt real, modelos sintéticos, selección, filtros, borrado, reset,
actualizaciones, foco y botones; compilación completa y apertura de la interfaz
real con perfil aislado. En esa apertura se detectó un fondo blanco; se añadió
`WA_StyledBackground` y la recompilación pasó. La regresión automatizada con
paleta blanca también pasó. Estos resultados corresponden a la primera base;
la segunda entrega añade la validación descrita en la fase 2. Las bibliotecas
grandes y las sesiones completas con juegos siguen pendientes.
Véase `ORBIT_VALIDATION.md`.

## Fase 2 — interfaz moderna, completada parcialmente

La segunda entrega implementa una Home con fondo en toda la superficie,
información del juego en primer plano y carrusel inferior. Incluye transiciones
de aproximadamente 350 ms y una cabecera propia sin las barras blancas clásicas.
El menú **Más** conserva el acceso a las funciones originales; las acciones
siguen perteneciendo a Dolphin. No hay un frontend separado.

| Archivo | Cambio de la segunda entrega |
| --- | --- |
| `Source/Core/DolphinQt/Home/HomeWidget.h/.cpp` | Composición moderna, carrusel, botones de navegación, menú Personalizar fondo y coordinación del fondo seleccionado. |
| `Source/Core/DolphinQt/Home/OrbitBackground.h/.cpp` | Fondos por juego, GIF mediante QMovie sin CacheAll, importación segura al perfil, reducción de movimiento, pausa temporal, recorte proporcional y degradados para legibilidad. |
| `Source/Core/DolphinQt/Home/OrbitTitleBar.h/.cpp` | Cabecera de Orbit, controles de ventana, accesos de emulación y menú Más con acciones existentes. |
| `Source/Core/DolphinQt/MainWindow.h/.cpp` | Integración del marco, política de visibilidad y pausa del fondo durante emulación. |
| `Source/Core/DolphinQt/GameList/GameList.h/.cpp` | Conexión del QSettings del perfil a Home y control temporal de reproducción del fondo. |
| `Source/Core/DolphinQt/CMakeLists.txt` | Registro de los nuevos widgets y dependencia del lector GIF. |
| `Externals/QtGif/` | Código oficial del lector GIF de Qt 6.8.3, integración CMake y licencia. |
| `Tools/Orbit/SmokeTests/` | Suites aprobadas de Home, cabecera, acciones/menús/atajos e importación, persistencia y reproducción GIF con datos sintéticos. |

**Personalizar fondo** permite elegir GIF o PNG por juego en esta distribución
Windows; JPEG, WebP y BMP se ofrecen solo si está instalado su decodificador Qt.
El menú permite quitar la elección y desactivar **Movimiento y transiciones**. El fondo se copia
a `Orbit/Backgrounds`, junto al `Qt.ini` del perfil activo; el mapeo usa una clave
derivada del juego y los archivos se nombran por su contenido. Los límites son
32 MiB, 4096 píxeles por lado y 16 megapíxeles. Los archivos inválidos conservan
la elección anterior y muestran un error. Quitar una elección elimina el mapeo,
sin borrar archivos ajenos ni originales.

Solo el juego seleccionado mantiene su QMovie. Se pausa al ocultar Home,
minimizar la ventana y durante emulación; esa pausa no altera la preferencia de
movimiento. Una actualización de metadatos del mismo juego no reinicia el GIF.
El tamaño escalado se conserva entre repintados y las imágenes no se descargan.

El Qt incluido en Dolphin para Windows tiene GIF desactivado. Se integra el
plugin oficial **Qt 6.8.3**, obra de **The Qt Company Ltd. y los contribuidores
de Qt**, manteniendo su código y avisos en `Externals/QtGif`. Se usa su opción
GPL-3.0-only, con licencia incluida; Codex escribió la integración CMake.
Pasaron la compilación Release limpia en `work/orbit-modern` y la recompilación
incremental, el despliegue de `qgif.dll` y las suites completas de Home, GIF y
cabecera. En la aplicación nativa con perfil aislado se comprobaron Más,
maximizar/restaurar y la ausencia de barras blancas. Un DOL sintético propio
permitió verificar escaneo, recuperación de error, emulación en ventana separada,
pausa y parada con regreso a Home. No se usaron juegos comerciales ni partidas
del usuario. Los detalles y límites se recogen en `ORBIT_VALIDATION.md`.

Pendiente para completar esta fase:

- Ampliar las comprobaciones de diálogos y del redimensionado por bordes; este último no dio un resultado concluyente.
- Comprobar 720p/1080p/4K, escalado 100–200 %, textos largos y accesibilidad.
- Completar el modo salón y su salida al escritorio sin interferir con el fullscreen de emulación.
- Separar GameCube/Wii, añadir favoritos y recientes mediante el modelo original.
- Conservar la selección por ruta entre sesiones y comprobar bibliotecas grandes.
- Evaluar carga de imágenes fuera del hilo de interfaz si los archivos cercanos al límite afectan la respuesta.

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

La segunda entrega cubre el ciclo básico con un DOL sintético propio y render
en ventana separada. Quedan pendientes el render integrado, fullscreen y la
regresión amplia con las funciones y tamaños descritos; esa prueba inicial no
equivale a compatibilidad verificada con juegos comerciales.

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
