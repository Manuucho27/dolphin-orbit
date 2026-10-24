# Validación de Dolphin Orbit — 24/10/2026

Base: `e1d35f666cc35f6ead55d32ea2679df44d04da72`.

| Comprobación | Resultado |
| --- | --- |
| Clonado y submódulos recursivos fijados | Completados; checkout de SPIRV-Cross con soporte de rutas largas. |
| Configuración de CMake para Windows x64 Release | Correcta; Build Tools 2026 18.11.1, MSVC 19.51.36260, CMake 4.3.1-msvc1, Qt 6.8.3 y SDK 10.0.26100. |
| Compilación completa `dolphin-emu` | Correcta: 2031 pasos, árbol limpio y corto `work/orbit-release`, fuera del repositorio. |
| `Dolphin.exe --version` | Correcto; código de salida 0. |
| Apertura de Dolphin real con perfil aislado | Correcta. Se corrigió el fondo blanco con `WA_StyledBackground`, se recompiló y se verificó visualmente el fondo azul noche, el panel, el carrusel y los metadatos dentro del emulador completo. |
| Regresión de fondo con paleta blanca | Correcta el 24/10: píxel de Home `#0a1323`, salida 0; suite de selección, filtros, reset, Jugar, Enter y foco también correcta. |
| Sintaxis MSVC con opciones del objetivo real | Correcta para HomeWidget.cpp, GameList.cpp, MainWindow.cpp, MenuBar.cpp y Settings.cpp. |
| Compilación y enlace aislados del HomeWidget real con Qt Widgets | Correctos. |
| Prueba ejecutable de comportamiento de Home | Correcta; casos descritos debajo. |
| Helper PowerShell | Opciones `-CleanFirst` y `-BuildDirectory`, captura UTF-8 del entorno; comprobado con miniproyecto Ninja que conserva dependencias de cabeceras. |
| Copia al escritorio y acceso directo | Completados; el ejecutable copiado tiene el mismo hash que el compilado. |
| Espacios y parche | `git diff --check` y comprobación inversa del parche realizadas al preparar la entrega. |

## Historial del bloqueo de compilación, resuelto

En la primera entrega estaba instalado Visual Studio Build Tools 2026 18.6.0, MSVC 19.51.36243
(`_MSC_FULL_VER=195136243`). Requerido por el código fijado: MSVC
19.51.36252 o superior (`195136252`). Error observado:

```text
Source/PCH/pch.h(11): fatal error C1189:
#error: Please update your build environment to the latest Visual Studio 2026!
```

El 24/10/2026, MSVC 19.51.36260 supera ese requisito y la compilación completa
pasó. Se conservó el guard original; no se falseó la versión ni se modificó Core.
El árbol anterior contenía PCH de otro compilador y produjo C1853; se resolvió
usando el árbol limpio y corto `work/orbit-release`. El helper admite ahora
seleccionar ese directorio y limpiar sus objetos antes de compilar.

## Prueba aislada del widget

Se compiló el mismo `Home/HomeWidget.cpp/.h` entregado, con AUTOMOC y Qt 6.8.3,
en un ejecutable de prueba temporal. Usa QStandardItemModel y un proxy de
presentación con las columnas reales de GameListModel; todas las entradas y
artwork son sintéticos. Ejecutado con el módulo gráfico Windows de Qt.
La prueba reproducible se entrega en `Tools/Orbit/SmokeTests/`, con instrucciones
de compilación y ejecución en su README.

Se verificaron estos comportamientos:

- Biblioteca vacía: sin selección, Jugar desactivado y acceso a añadir carpeta.
- Inserción: selección inicial utilizable y Jugar habilitado.
- Cambio de selección: el panel sigue el juego seleccionado.
- Botón Jugar y Enter sobre Jugar: una señal de arranque por acción.
- Enter sobre ajustes/carpeta: activa el botón y no propaga un arranque.
- Enter sobre el carrusel: se propaga al contenedor para su ruta de arranque.
- Metadatos con marcado: se conservan como texto plano.
- Actualización de metadatos, inserción, filtros y reset: no roban el foco a ajustes.
- Filtro sin resultados: estado correspondiente y Jugar desactivado.
- Restauración del filtro y borrado de la fila seleccionada: selección recuperable.
- Limpiar selección y reset: no queda una selección reproducible obsoleta.

La prueba descubrió que QListView podía tener índice actual sin selección al
insertar la primera fila. Se corrigió la recuperación de selección y se repitió
la prueba completa, con este resultado:

```text
PASS: empty/insert/select/filter/remove/reset/plain-text/Play/Enter/focus;
saved synthetic previews
```

`orbit-preview.png` y `orbit-empty-preview.png`, adjuntos a esta entrega, son
capturas del widget real a 1100×720. El ejecutable de prueba es una herramienta
de verificación temporal, no un frontend del fork.

## Comprobaciones pendientes

1. Completar la revisión visual de la aplicación real tras corregir el fondo
   con `WA_StyledBackground`; su regresión automatizada ya pasó.
2. Añadir una biblioteca propia y verificar escaneo, búsqueda, filtros, portadas
   y transferencia de selección Home/lista/cuadrícula.
3. Ejecutar un juego, detenerlo y volver a Home; repetir con render integrado
   y en ventana separada, incluyendo error de arranque.
4. Revisar propiedades y acciones contextuales, NetPlay y mandos existentes.
5. Comprobar tamaños 720p–4K, DPI 100–200 %, accesibilidad y biblioteca grande.

La compilación y la apertura verificadas no demuestran arranque de juegos,
vuelta desde emulación ni ausencia de regresiones en todos los flujos de Dolphin.
La navegación de Home por mando todavía no está implementada. No se ha validado
una sesión real de emulación ni se han usado partidas o juegos del usuario.
