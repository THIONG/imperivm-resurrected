<p align="center">
  <img src="assets/banner.png" alt="Imperivm Resurrected" width="640">
</p>

# Imperivm Resurrected

Mod no oficial para **Imperivm: Great Battles of Rome HD** (Steam). Le da al juego lo que le faltaba para seguir vivo:
online sin abrir puertos, mods con un clic, controles a tu gusto y muchos arreglos.

*English below.*

> **Estado:** en pruebas antes de la 1.0 (versiones 0.x). Si algo falla, mira [Si algo falla](#si-algo-falla).

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

Para quitarlo, borra `winmm.dll` de la carpeta del juego. No modifica ningún archivo del juego.

Hace falta el juego original de Steam. Este mod no incluye ningún archivo del juego.

## Si algo falla

En el menú principal del juego, pulsa el corazón de abajo a la izquierda y **Crear informe**. Se crea un zip con los registros
y los datos de tu equipo, sin tu usuario de Windows, tus IDs de Steam ni las IP. El mod no envía nada: lo adjuntas tú.

Después, [abre una incidencia](https://github.com/THIONG/imperivm-resurrected/issues/new/choose) con el zip, o pregunta en el
[Discord](https://discord.gg/76gee3GMB7). Los registros están en `Logs\imperivm\`, dentro de la carpeta del juego.

## Hacer mods

El **SDK** está en [`sdk/`](sdk): guías, herramientas y ejemplos para hacer contenido sin compilar nada, en
[español](sdk/es/README.md), [inglés](sdk/en/README.md) e [italiano](sdk/it/README.md). Cada release lo adjunta como zip.
Los mods terminados se publican en [GameBanana](https://gamebanana.com/games/26069).

## Desarrollo

El código del mod está en `src/` (C++ para Windows, 32 bits), sus pruebas en `tests/` y las comprobaciones en `tools/`.
Cómo compilar, qué comprueba `tools\run-tests.bat` y cómo enviar un cambio: [CONTRIBUTING.md](CONTRIBUTING.md).
Compilar la DLL necesita el SDK de Steamworks de Valve, que no se incluye en este repositorio.

## Licencia

- **Código** (`src/`, `tests/`, `tools/`): GPL-3.0-or-later con una excepción para Steamworks. Ver [LICENSE](LICENSE) y
  [ADDITIONAL-PERMISSION.md](ADDITIONAL-PERMISSION.md). Software de terceros: [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
  Agradecimientos: [CREDITS.md](CREDITS.md).
- **SDK** (`sdk/`): MIT para el código y CC BY-SA 4.0 para las guías. Ver el archivo de licencia de cada idioma
  (por ejemplo [sdk/es/LICENCIA.md](sdk/es/LICENCIA.md)).
- **Imágenes** (`assets/`): todos los derechos reservados. Ver [assets/LICENSE.md](assets/LICENSE.md).

---

# Imperivm Resurrected (English)

Unofficial mod for **Imperivm: Great Battles of Rome HD** (Steam): online play through Steam with no port forwarding,
one-click mods from [GameBanana](https://gamebanana.com/games/26069), rebindable controls, modern graphics and sound,
autosave, crash fixes, a problem report built into the game, and automatic updates.

> **Status:** in testing before 1.0 (0.x versions). If something fails, see [If something fails](#if-something-fails).

**Install:** download the zip from the [latest release](https://github.com/THIONG/imperivm-resurrected/releases/latest),
extract it into the game folder (where `gbr.exe` is) and start the game from Steam. To uninstall, delete `winmm.dll`.
It doesn't change any game file, and you need your own copy of the game.

## If something fails

In the game's main menu, click the heart at the bottom left and then **Create report**. It makes a zip with the logs and
your system, with your Windows user name, Steam IDs and IP addresses replaced. The mod sends nothing: you attach it yourself.

Then [open an issue](https://github.com/THIONG/imperivm-resurrected/issues/new/choose) with the zip, or ask on
[Discord](https://discord.gg/76gee3GMB7). The logs are in `Logs\imperivm\`, inside the game folder.

## Making mods

The **SDK** is in [`sdk/`](sdk): guides, tools and examples to make content without compiling anything, in
[Spanish](sdk/es/README.md), [English](sdk/en/README.md) and [Italian](sdk/it/README.md). Each release attaches it as a zip.
Finished mods are published on [GameBanana](https://gamebanana.com/games/26069).

## Development

The mod's code is in `src/` (C++ for Windows, 32-bit), its tests in `tests/` and the checks in `tools/`.
How to build, what `tools\run-tests.bat` checks and how to send a change: [CONTRIBUTING.md](CONTRIBUTING.md).
Building the DLL needs Valve's Steamworks SDK, which is not in this repository.

## License

- **Code** (`src/`, `tests/`, `tools/`): GPL-3.0-or-later with an additional permission for Steamworks. See [LICENSE](LICENSE)
  and [ADDITIONAL-PERMISSION.md](ADDITIONAL-PERMISSION.md). Third-party software: [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
  Credits: [CREDITS.md](CREDITS.md).
- **SDK** (`sdk/`): MIT for the code and CC BY-SA 4.0 for the guides. See the license file of each language
  (for example [sdk/en/LICENSE.md](sdk/en/LICENSE.md)).
- **Images** (`assets/`): all rights reserved. See [assets/LICENSE.md](assets/LICENSE.md).

---

Imperivm Resurrected is a fan project. It is not affiliated with or endorsed by FX Interactive or Haemimont Games.
Imperivm and Great Battles of Rome are trademarks of their respective owners.
