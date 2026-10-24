# Dolphin Orbit

Fork no oficial de Dolphin Emulator para Windows con una nueva vista
**Orbit Home**, implementada directamente dentro de DolphinQt en C++/Qt Widgets.
Combina referencias de navegación de PS5 y Nintendo Switch con una identidad
propia. El nombre y el diseño son provisionales.

- Repositorio: [Manuucho27/dolphin-orbit](https://github.com/Manuucho27/dolphin-orbit).
- Descargas y versiones: [Releases de Orbit](https://github.com/Manuucho27/dolphin-orbit/releases).

## Origen y autoría

Orbit utiliza la **base de código completa de Dolphin Emulator**. El emulador
original es obra de **Dolphin Emulator Project, sus autores y contribuidores**.

- Repositorio oficial: [dolphin-emu/dolphin](https://github.com/dolphin-emu/dolphin).
- Commit de partida: [e1d35f666cc35f6ead55d32ea2679df44d04da72](https://github.com/dolphin-emu/dolphin/commit/e1d35f666cc35f6ead55d32ea2679df44d04da72), del 22/10/2026.
- [Atribución detallada](docs/ATTRIBUTION.md).

**La idea, el concepto y la dirección de Dolphin Orbit corresponden a
[Manuucho27](https://github.com/Manuucho27). Todo el trabajo técnico del rediseño Orbit realizado en
esta colaboración —investigación, implementación, integración, scripts, pruebas
y documentación— ha sido realizado por Codex, un modelo de inteligencia
artificial de OpenAI, con ayuda de sus subagentes.** El motor de emulación y el
código original de Dolphin pertenecen a sus autores y contribuidores; esta
declaración de trabajo con IA solo describe las aportaciones de Orbit.

## Documentación y código

- [Plan por fases y mapa de archivos](docs/ORBIT_PLAN.md)
- [Registro de validación](docs/ORBIT_VALIDATION.md)
- [Pantalla implementada](Source/Core/DolphinQt/Home/HomeWidget.cpp)
- [Helper para compilar en Windows](Tools/Orbit/Build-Windows.ps1)
- [Prueba reproducible de la pantalla Qt](Tools/Orbit/SmokeTests/README.md)

## Interfaz de la segunda entrega

Home usa ahora el fondo en toda la superficie, información del juego en primer
plano, carrusel inferior y transiciones discretas. La cabecera propia de Orbit
reemplaza las barras blancas del marco clásico; **Más** da acceso a las funciones
originales de Dolphin. La biblioteca, los filtros, las vistas clásicas y la ruta
de arranque siguen compartidos con DolphinQt. La navegación de Home por mando
todavía está pendiente.

Los juegos se ejecutan dentro de la ventana de Orbit por defecto y al detenerlos
se vuelve a Home. Los perfiles anteriores que hayan guardado la opción contraria
pueden activar **Render to Main Window** en **Gráficos → General**.

Para dar un fondo a un juego, selecciónalo y abre **Personalizar fondo → Elegir
imagen o GIF…**. La distribución Windows incorpora GIF y PNG; JPEG, WebP y BMP
aparecen en el selector solo si está instalado su decodificador Qt. Cada juego
guarda su elección; **Quitar fondo personalizado** recupera el fondo predeterminado.
Desactiva **Movimiento y transiciones** en ese mismo menú para reducir movimiento.

Orbit copia las imágenes importadas a `Orbit/Backgrounds`, junto al `Qt.ini` del
perfil activo. La elección se conserva aunque muevas el archivo original. Cada
archivo debe ocupar como máximo **32 MiB**, con **4096 píxeles por lado y
16 megapíxeles** como límites de imagen. Los GIF usan únicamente el fotograma
necesario; se pausan con Home oculto, la ventana minimizada o la emulación activa.
Quitar una elección no borra el archivo original ni otras imágenes del usuario.

El soporte GIF se integra con el plugin oficial de Qt 6.8.3 incluido en
[Externals/QtGif](Externals/QtGif/README.md), porque la distribución Qt utilizada
por Dolphin en Windows desactiva ese formato. Pasaron la compilación Release,
las suites de Home, cabecera y GIF, y un ciclo básico de emulación con un programa
sintético propio. El [registro de validación](docs/ORBIT_VALIDATION.md) detalla
las comprobaciones y los casos que todavía quedan pendientes.

## Compilar en Windows

Con los requisitos de compilación de la base de Dolphin instalados, ejecutar
desde la raíz del repositorio:

```powershell
git clone --recursive https://github.com/Manuucho27/dolphin-orbit.git
cd dolphin-orbit
.\Tools\Orbit\Build-Windows.ps1 -Configuration Release -Jobs 6 -Portable
```

Si ya tienes el repositorio, ejecuta solamente el helper desde su raíz.
`-BuildDirectory` permite elegir una ruta corta y limpia. Tras actualizar el
compilador, utiliza un directorio nuevo o `-CleanFirst` para regenerar los objetos;
una caché con codificación antigua requiere un directorio nuevo.

El destino de salida es `build/release/x64/Binaries/Dolphin.exe`. El modo portable
usa datos propios del fork. El registro de validación documenta las comprobaciones
y limitaciones observadas en cada entrega; esta guía no certifica una compilación
o publicación concreta.

Las capturas de la primera entrega corresponden al widget Qt real probado con
entradas sintéticas, sin juegos incluidos; no son una sesión de emulación.

## Licencias

Se conservan los avisos de autoría y las licencias originales. La mayor parte del
código original de Dolphin y las nuevas modificaciones de Orbit están bajo
**GPL versión 2 o posterior (GPLv2+)**. Las dependencias conservan sus licencias
respectivas; [COPYING](COPYING) explica el conjunto y [LICENSES](LICENSES) contiene
los textos de las licencias. Los identificadores SPDX de cada archivo indican
la licencia que le corresponde. Orbit es un fork independiente del proyecto
oficial de Dolphin.

El lector GIF de [Externals/QtGif](Externals/QtGif/README.md) procede de **The Qt
Company Ltd. y los contribuidores de Qt**, versión oficial 6.8.3. Sus archivos
originales conservan los avisos de autoría. Se distribuye con su código fuente
completo bajo la opción **GPL-3.0-only**, cuyo texto está en
[Externals/QtGif/LICENSES/GPL-3.0-only.txt](Externals/QtGif/LICENSES/GPL-3.0-only.txt).
La integración de este plugin con Orbit forma parte del trabajo de Codex.
