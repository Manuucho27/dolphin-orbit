# Prueba aislada de Orbit Home

Compila `HomeWidget`, `OrbitBackground` y `OrbitTitleBar` de producción con
Qt Widgets y un modelo sintético. También compila el lector GIF oficial de Qt;
la prueba añade automáticamente la ruta del plugin generado.
No necesita juegos, Qt Test ni ejecutar el emulador completo. No sustituye las
pruebas de integración con `GameList` y el ciclo de emulación.

Desde la raíz del repositorio, en una terminal de desarrollo x64 de Visual Studio:

```powershell
cmake -S Tools/Orbit/SmokeTests -B build/orbit-smoke -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/orbit-smoke --parallel 2
$orbitQt = Join-Path (Get-Location) 'Externals\Qt\Qt6.8.3\x64'
$env:PATH = "$orbitQt\bin;$env:PATH"
$env:QT_PLUGIN_PATH = "$orbitQt\plugins"
$env:QT_QPA_PLATFORM = 'windows'
& .\build\orbit-smoke\orbit-home-smoke.exe
if ($LASTEXITCODE -ne 0) { throw 'Orbit Home smoke test failed.' }
```

Requiere los submódulos preparados. La prueba abre brevemente una ventana Qt en
el escritorio para comprobar el foco; se cierra por sí sola. Devuelve 0 y muestra
`PASS` cuando todos los casos pasan. Genera `orbit-preview.png` y
`orbit-empty-preview.png` en su directorio de compilación.

El paquete Qt Windows de upstream no incluye el plugin `offscreen`; utilizar
`windows` permite ejecutar esta prueba con el paquete incluido. Todos los datos
y dibujos de la captura son ejemplos sintéticos.

La suite comprueba menús añadidos y retirados dinámicamente, asociación de
atajos, conservación de las acciones de Dolphin y maximizar/restaurar. F10
queda reservado a la acción original de Pausa/Jugar.

El GIF de `data/background.gif` es un dibujo sintético de dos colores creado
para esta prueba, sin recursos de videojuegos. Se verifican cambios reales de
fotograma, importación al perfil, persistencia, independencia entre juegos,
rechazo de archivos corruptos y pausa al ocultar la vista, reducir movimiento
o suspender el fondo durante emulación.
