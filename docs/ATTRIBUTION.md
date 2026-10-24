# Atribución de Dolphin Orbit

- Repositorio de este fork: [Manuucho27/dolphin-orbit](https://github.com/Manuucho27/dolphin-orbit).
- Descargas y versiones: [Releases de Orbit](https://github.com/Manuucho27/dolphin-orbit/releases).

## Base original: Dolphin Emulator

Dolphin Orbit utiliza la **base de código completa de Dolphin Emulator**,
incluidos su motor de emulación, DolphinQt, herramientas, recursos y dependencias.
La autoría del proyecto original corresponde a **Dolphin Emulator Project y sus
autores y contribuidores**. Dolphin es un proyecto colectivo; este fork no
atribuye todo ese trabajo a una sola persona.

- Repositorio original: [https://github.com/dolphin-emu/dolphin](https://github.com/dolphin-emu/dolphin).
- Sitio oficial: [https://dolphin-emu.org/](https://dolphin-emu.org/).
- Base de partida de Orbit:
  [e1d35f666cc35f6ead55d32ea2679df44d04da72](https://github.com/dolphin-emu/dolphin/commit/e1d35f666cc35f6ead55d32ea2679df44d04da72), del 22/10/2026.

El historial de Git, los avisos de copyright, los créditos originales y los
archivos de las dependencias conservan las atribuciones de sus respectivos
autores. Orbit no es una versión oficial de Dolphin ni implica que el proyecto
original respalde este rediseño.

## Idea y trabajo de Orbit

**La idea, el concepto y la dirección de Dolphin Orbit corresponden a
[Manuucho27](https://github.com/Manuucho27)**: modificar Dolphin directamente para ofrecer
una interfaz nativa híbrida de PS5 y Nintendo Switch con identidad propia.

**Todo el trabajo técnico del rediseño Orbit realizado en esta colaboración ha
sido realizado por Codex, un modelo de inteligencia artificial de OpenAI, con
ayuda de sus subagentes.** Esto abarca la investigación del repositorio y sus
requisitos de compilación, el diseño técnico, la implementación de los cambios
en DolphinQt, la integración, los scripts, las pruebas y la documentación
producidas para Orbit durante esta colaboración.

Esta atribución no incluye la creación del motor de emulación, de DolphinQt
original ni del resto del código y recursos heredados de Dolphin y sus
dependencias. Dicho trabajo sigue correspondiendo a sus autores y contribuidores.
Tampoco implica que OpenAI mantenga, respalde o publique oficialmente este fork.

## Licencias y avisos

Se mantienen las licencias y los avisos de autoría originales. Según
[COPYING](../COPYING), la mayor parte del código original de Dolphin está bajo
**GNU General Public License, versión 2 o posterior (GPLv2+)**. Las nuevas
modificaciones de Orbit se distribuyen también bajo **GPLv2+**.

Parte del repositorio proviene de otros proyectos con condiciones propias;
COPYING indica que el conjunto completo es compatible con GPLv3. Los
identificadores SPDX de cada archivo y los avisos de las dependencias determinan
la licencia aplicable a cada parte. Los textos se conservan en
[LICENSES](../LICENSES) y en los directorios correspondientes de las dependencias.

La mención de Codex y del trabajo de IA describe cómo se han producido las
aportaciones de Orbit y no sustituye ni elimina las atribuciones y licencias
existentes.

## Lector GIF de Qt

La segunda entrega incluye el lector GIF oficial de Qt 6.8.3, obra de
The Qt Company Ltd. y los contribuidores de Qt. Su código original sin cambios,
origen y hashes se conservan en [Externals/QtGif](../Externals/QtGif/README.md).
Se utiliza la opción GPL-3.0-only indicada en sus cabeceras; la licencia acompaña
al plugin en el paquete portable. Codex realizó la integración de compilación
y su uso en Orbit; no escribió el decodificador original de Qt.
