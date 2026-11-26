// Sandbox demo script.
//
// Everything the sandbox shows or plays originates here: the rendered rich text,
// the module file, its volume, the button and what the button does when it is
// clicked. The engine half (GameSystem, GuiScene, the Lotte/Haruka libraries) only
// provides the services the `System`, `GUI`, `Audio` and `IO` modules expose.
//
// The script runs once, on the frame thread, and returns. Click handlers stay
// alive in the context and are invoked from the SDL event handlers.

const CONFIG = require('CONFIG');
const System = require('System');
const GUI = require('GUI');
const Audio = require('Audio');
const IO = require('IO');

print('Hello World from C++/JS!');
/*for (const [key, value] of Object.entries(CONFIG)) {
    console.log(`[${key}]`);
    for (const [jey, jalue] of Object.entries(value)) {
        console.log(`${jey}: ${jalue}`);
    }
}*/
//print(JSON.stringify(CONFIG));
print(CONFIG);

// Configuration with defaults that used to live in GameSystem.
const title = CONFIG.Game.sTitle ?? 'Sandbox';
const songPath = CONFIG.Game.sSongPath ?? '/music/Magnus.it';
const songVolume = CONFIG.Game.fModVolume ?? 0.10;
const titleSize = CONFIG.Graphics.uTitleSize ?? 52;
const margin = CONFIG.Graphics.uTextMargin ?? 32;

// 1. Music. The module object owns the streamer: losing the last reference stops
//    playback and closes the file.
let music = null;
try {
    music = new Audio.Module(songPath, {
        volume: songVolume,
        repeating: true,
        playing: true
    });
    print(`Playing '${songPath}' at volume ${songVolume}`);
} catch (error) {
    print(`Could not start '${songPath}': ${error}`);
}

// 2. Rich text. This used to be an MML string compiled into `GameSystem`.
function showStatus() {
    const status = music === null
        ? 'no module loaded'
        : (music.playing ? 'music on' : 'music off');
    try {
        System.setRichText(
            `[font=Vinque][color=#ffffff][size=${titleSize}]${title}[/size]\n` +
            `[size=30]Click the button to switch the music [b]on[/b] and [i]off[/i].[/size]\n` +
            `[size=26][color=#ffd166]${status}[/color][/size]`,
            { x: margin, y: titleSize + 24, spacing: 4, wordSpacing: 2 }
        );
    } catch (error) {
        print(`Text unavailable: ${error}`);
    }
}

// 3. A clickable widget. `GUI.Button` auto-registers itself in the scene; the
//    texture is optional, the colours are the fill (untextured) or the tint
//    (textured) of each of the four visual states.
let button = null;
try {
    let texture = null;
    try {
        texture = System.loadTexture('/gfx/hourglass2.png');
    } catch (error) {
        print(`No button texture: ${error}`);
    }

    const viewport = System.viewport;
    const buttonWidth = 192;
    const buttonHeight = 72;
    button = new GUI.Button({
        x: viewport.width - buttonWidth - margin,
        y: viewport.height - buttonHeight - margin,
        width: buttonWidth,
        height: buttonHeight,
        texture: texture,
        colours: {
            //idle: '#32455aff',
            idle: '#ff00ffff',
            hover: '#00ffffff',
            active: '#ffff00ff',
            disabled: '#33333388'
        }
    });

    button.enabled = true;

    button.onClick = function () {
        if (music === null) {
            print('There is no module to toggle.');
            return;
        }
        music.playing = !music.playing;
        print(`Music ${music.playing ? 'on' : 'off'}`);
        showStatus();
        // A small side effect through the IO module, to show that the click
        // handler is ordinary script code.
        try {
            const log = new IO.File('/sandbox-session.log', 'a');
            log.writeString(`music=${music.playing ? 'on' : 'off'}\n`);
            log.flush();
        } catch (error) {
            print(`Could not write the session log: ${error}`);
        }
    };
} catch (error) {
    print(`No button: ${error}`);
}

showStatus();
