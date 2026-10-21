<p align="center">
  <img src="assets/banner.png" alt="Imperivm Resurrected" width="640">
</p>

<p align="center"><a href="README.md">English</a> · <b>Español</b> · <a href="README.it.md">Italiano</a></p>

# Imperivm Resurrected

Mod no oficial para **Imperivm: Great Battles of Rome HD** (Steam). Le da al juego lo que le faltaba para seguir vivo:
online sin abrir puertos, mods con un clic, controles a tu gusto y muchos arreglos.

> **Estado:** versión 1.0. Probada en partidas online de dos jugadores; todavía sin probar con más jugadores. Si algo falla, mira [Si algo falla](#si-algo-falla).

## Qué trae

- **Online por Steam**, sin abrir puertos ni usar VPN: salas, invitaciones a amigos, chat y reconexión si alguien se cae.
- **Mods con un clic** desde el propio juego, con perfiles, versiones y dependencias. Los mods se publican en
  [GameBanana](https://gamebanana.com/games/26069).
- **Controles configurables**: cualquier tecla del juego y de las órdenes, con preajustes.
- **Imagen moderna**: Direct3D 11, resoluciones actuales, ventana o pantalla completa sin parpadeos y overlay de Steam.
- **Sonido moderno**, autoguardado, panel de rendimiento y arreglos de cierres del juego original.
- **Informe de problemas** desde el propio juego, con los datos privados tapados.
- **Se actualiza solo**: el juego avisa cuando hay versión nueva.

## Instalar

1. Descarga el zip de la [última versión](https://github.com/THIONG/imperivm-resurrected/releases/latest).
2. Descomprímelo en la carpeta del juego, donde está `gbr.exe`
   (en Steam: clic derecho en el juego → Administrar → Ver archivos locales).
3. Abre el juego desde Steam.

Para quitarlo, borra `winmm.dll`, `steam_api.dll` y la carpeta `imperivm-resurrected` de la carpeta del juego. No modifica ningún archivo del juego.

Hace falta el juego original de Steam. Este mod no incluye ningún archivo del juego.

## Si algo falla

En el menú principal del juego, pulsa el corazón de abajo a la izquierda y **Crear informe**. Se crea un zip con los registros
y los datos de tu equipo, sin tu usuario de Windows, tus IDs de Steam ni las IP. El mod no envía nada: lo adjuntas tú.

Después, [abre una incidencia](https://github.com/THIONG/imperivm-resurrected/issues/new/choose) con el zip, o pregunta en el
[Discord](https://discord.gg/6kCatX3Zbc). Los registros están en `Logs\imperivm\`, dentro de la carpeta del juego.

## Hacer mods

El **SDK** está en [`sdk/`](sdk): guías, herramientas y ejemplos para hacer contenido sin compilar nada, en
inglés ([empieza aquí](sdk/README.md)). Cada release lo adjunta como zip.
Los mods terminados se publican en [GameBanana](https://gamebanana.com/games/26069).

## Desarrollo

El código del mod está en `src/` (C++ para Windows, 32 bits), sus pruebas en `tests/` y las comprobaciones en `tools/`.
Cómo compilar, qué comprueba `tools\run-tests.bat` y cómo enviar un cambio: [CONTRIBUTING.md](CONTRIBUTING.md) (en inglés).
Compilar la DLL necesita el SDK de Steamworks de Valve, que no se incluye en este repositorio.

Cada release la comprueba `.github/workflows/release.yml` (etiqueta, hashes, firma y contenido del zip). Puedes hacer la misma
comprobación con una release descargada: `node tools/verify-release.js <carpeta>`.

## Code signing policy

Free code signing provided by [SignPath.io](https://about.signpath.io), certificate by [SignPath Foundation](https://signpath.org).

- Autores y revisores: [THIONG](https://github.com/THIONG), [Villoh](https://github.com/Villoh).
- Aprobadores: [THIONG](https://github.com/THIONG), [Villoh](https://github.com/Villoh).
- Política de privacidad: [imperivmresurrected.com/privacy.html](https://www.imperivmresurrected.com/privacy.html)

## Licencia

- **Código** (`src/`, `tests/`, `tools/`): GPL-3.0-or-later con una excepción para Steamworks. Ver [LICENSE](LICENSE) y
  [ADDITIONAL-PERMISSION.md](ADDITIONAL-PERMISSION.md). Software de terceros: [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
  Agradecimientos: [CREDITS.md](CREDITS.md).
- **SDK** (`sdk/`): MIT para el código y CC BY-SA 4.0 para las guías. Ver [sdk/LICENSE.md](sdk/LICENSE.md).
- **Imágenes** (`assets/`): todos los derechos reservados. Ver [assets/LICENSE.md](assets/LICENSE.md).

---

Imperivm Resurrected es un proyecto de aficionados. No está afiliado a FX Interactive ni a Haemimont Games, ni cuenta con su apoyo.
Imperivm y Great Battles of Rome son marcas de sus respectivos propietarios.
