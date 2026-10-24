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

Home presenta biblioteca compartida, carrusel, información del juego, Jugar,
ajustes y selección de carpeta. La opción **View → Orbit Home** permite volver
desde lista/cuadrícula; la elección se guarda. La navegación por mando y el
modo salón completo pertenecen a las siguientes fases.

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
