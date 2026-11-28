// Sandbox menu.
//
// Everything the player sees is declared here: the screens, their captions, the
// volume sliders and what each entry does. The engine half (GameSystem, GuiScene and
// the Lotte/Haruka/Euphemy libraries) only provides the services these modules expose:
// `System`, `GUI`, `Audio`, `IO` and `Localization`.
//
// Two rules keep this file free of things that belong in C++:
//
// * No visible string is written literally. Every caption is a localization key read
//   from `require('Localization')`; a key the table does not have falls back to the
//   key itself, which is visible but never empty.
// * No layout constant pretends to be a theme. Captions take their font, size and
//   colour from the theme through `element: 'button'` / `role: 'heading'`, so a new
//   theme restyles the menu without touching this script.

const CONFIG = require('CONFIG');
const System = require('System');
const GUI = require('GUI');
const Audio = require('Audio');
const L = require('Localization');

print(`Localization: ${L.count} key(s) loaded`);

/** Caption for a key: the translation when it exists, the key itself otherwise. */
function t(key) {
    return L[key] !== undefined ? L[key] : L.get(key);
}

// 1. The click sound. A missing file is not an error - `valid` is simply false and
//    `play()` does nothing, which is exactly "play it if it exists".
let clickSound = null;
try {
    const sound = new Audio.Sound('/Cursor1.wav');
    if (sound.valid) {
        clickSound = sound;
        print(`Click sound: ${sound.duration.toFixed(3)} s`);
    } else {
        print('No click sound; enabled widgets stay silent');
    }
} catch (error) {
    print(`Click sound unavailable: ${error}`);
}

// 2. Volume state. The values start from the configuration and are written back the
//    moment they change, so the ini file the application saves at exit always matches
//    what the player hears.
const volumes = {
    master: CONFIG.Audio?.fVolumeMaster ?? 1,
    music: CONFIG.Audio?.fVolumeMusic ?? CONFIG.Game?.fModVolume ?? 0.1,
    effects: CONFIG.Audio?.fVolumeEffects ?? 1
};

const songPath = CONFIG.Game?.sSongPath ?? '/music/Magnus.it';
let music = null;
try {
    music = new Audio.Module(songPath, {
        volume: volumes.music * volumes.master,
        repeating: true,
        playing: true
    });
} catch (error) {
    print(`Could not start '${songPath}': ${error}`);
}

/** Pushes the volume state into the transport and the one-shot player. */
function applyVolumes() {
    if (music !== null) music.volume = volumes.music * volumes.master;
    if (clickSound !== null) clickSound.volume = volumes.effects * volumes.master;
}

/** Stores the volume state in the configuration the application saves on exit. */
function persistVolumes() {
    try {
        System.setConfig('Audio', 'fVolumeMaster', volumes.master);
        System.setConfig('Audio', 'fVolumeMusic', volumes.music);
        System.setConfig('Audio', 'fVolumeEffects', volumes.effects);
    } catch (error) {
        print(`Could not store the volumes: ${error}`);
    }
}

applyVolumes();

// 3. A disabled entry never reaches its handler, so playing the click here is the
//    same as "only enabled widgets click".
function playClick() {
    if (clickSound !== null) clickSound.play();
}

// 4. Screens. A screen is the set of widgets and captions that are shown or hidden
//    together. Nothing is destroyed when switching, so going back is instant and the
//    sliders keep their state.
const screens = {
    menu: { widgets: [], labels: [] },
    settings: { widgets: [], labels: [] }
};

function addButton(screen, captionKey, rect, options = {}) {
    const enabled = options.enabled ?? true;
    const button = new GUI.Button({
        x: rect.x, y: rect.y, width: rect.width, height: rect.height, enabled
    });
    // A disabled entry cannot be clicked, so its caption has to look the part. The
    // theme already carries a muted style for that job, which is why the *role*
    // changes instead of a colour being hard-coded here.
    const caption = new GUI.Label({
        key: captionKey,
        role: enabled ? 'label' : 'placeholder',
        x: rect.x,
        y: rect.y,
        width: rect.width,
        height: rect.height
    });
    button.onClick = function () {
        playClick();
        if (options.onClick) options.onClick();
    };
    screen.widgets.push(button);
    screen.labels.push(caption);
    return button;
}

function addLabel(screen, options) {
    const label = new GUI.Label(options);
    screen.labels.push(label);
    return label;
}

/** Shows one screen and hides the other. */
function showScreen(name) {
    for (const [key, screen] of Object.entries(screens)) {
        const visible = key === name;
        for (const widget of screen.widgets) widget.hidden = !visible;
        for (const label of screen.labels) label.hidden = !visible;
    }
}

// 5. The layout. `UI` is a *virtual canvas*: every number below is in canvas units, so
//    the same menu appears on a 640x480 window and on a 1920x1080 one. The engine fits
//    the canvas into the window (0.8x at 640x480, letterboxed when the window is wider
//    than 4:3), and the script never reads the real resolution to do it.
//
//    Two ways to write a coordinate, both resolution-free:
//      * canvas units      - `y: 160`, `width: 360`
//      * normalized 0..1   - `UI.x(0.5)`, `UI.w(0.45)`
const UI = new GUI.Layout({ width: 800, height: 600 });
print(`Layout: ${UI.width}x${UI.height} canvas at ${UI.scale.toFixed(2)}x ` +
      `(${UI.resolution.width}x${UI.resolution.height} window)`);

const BUTTON_WIDTH = 360;
const BUTTON_HEIGHT = 56;
const BUTTON_GAP = 14;
const COLUMN_X = (UI.width - BUTTON_WIDTH) / 2;
const ROW_PITCH = BUTTON_HEIGHT + BUTTON_GAP;

addLabel(screens.menu, {
    key: 'menu_title',
    role: 'heading',
    align: 'centre',
    x: COLUMN_X,
    y: 56,
    width: BUTTON_WIDTH,
    height: 64
});

const entries = [
    { key: 'menu_new_game', enabled: false },
    { key: 'menu_load_game', enabled: false },
    { key: 'menu_settings', enabled: true, onClick: () => showScreen('settings') },
    { key: 'menu_credits', enabled: false },
    { key: 'menu_exit', enabled: true, onClick: () => System.quit() }
];

let nextY = 160;
for (const entry of entries) {
    addButton(screens.menu, entry.key,
        { x: COLUMN_X, y: nextY, width: BUTTON_WIDTH, height: BUTTON_HEIGHT },
        { enabled: entry.enabled, onClick: entry.onClick });
    nextY += ROW_PITCH;
}

// 6. The settings screen. Only volumes for now; the resolution picker waits for a
//    combo box, which is a widget this tree does not have yet.
addLabel(screens.settings, {
    key: 'settings_title',
    role: 'heading',
    x: COLUMN_X,
    y: 56,
    width: BUTTON_WIDTH,
    height: 64
});
addLabel(screens.settings, {
    key: 'settings_hint',
    role: 'placeholder',
    x: COLUMN_X,
    y: 120,
    width: BUTTON_WIDTH,
    height: 32
});

// The right half is written with normalized helpers: the slider starts just past the
// middle of the canvas and the readout keeps a fixed slice of the right edge.
const MARGIN = UI.w(0.06);                       // 48 canvas units
const SLIDER_X = UI.x(0.51);                     // 408
const READOUT_WIDTH = UI.w(0.09);                // 72
const READOUT_X = UI.width - MARGIN - READOUT_WIDTH;
const SLIDER_WIDTH = READOUT_X - SLIDER_X - MARGIN;

// The settings rows are tighter than the menu's button pitch: a slider row is 40 high.
const SETTINGS_ROW_PITCH = 56;

const settingsRows = [
    { label: 'settings_volume_master', field: 'master' },
    { label: 'settings_volume_music', field: 'music' },
    { label: 'settings_volume_effects', field: 'effects' }
];

let rowY = 176;
for (const row of settingsRows) {
    addLabel(screens.settings, {
        key: row.label,
        role: 'label',
        align: 'right',
        x: MARGIN,
        y: rowY,
        width: Math.max(SLIDER_X - MARGIN - MARGIN, 120),
        height: 40
    });
    const slider = new GUI.Slider({
        x: SLIDER_X,
        y: rowY + 4,
        width: SLIDER_WIDTH,
        height: 32,
        min: 0,
        max: 1,
        step: 0.01,
        value: volumes[row.field]
    });
    const readout = addLabel(screens.settings, {
        text: `${Math.round(volumes[row.field] * 100)}%`,
        role: 'label',
        align: 'left',
        x: READOUT_X,
        y: rowY,
        width: READOUT_WIDTH,
        height: 40
    });
    slider.onChange = function (value) {
        volumes[row.field] = value;
        readout.text = `${Math.round(value * 100)}%`;
        applyVolumes();
        persistVolumes();
    };
    screens.settings.widgets.push(slider);
    rowY += SETTINGS_ROW_PITCH;
}

addButton(screens.settings, 'menu_back',
    { x: COLUMN_X, y: rowY + 24, width: BUTTON_WIDTH, height: BUTTON_HEIGHT },
    { enabled: true, onClick: () => showScreen('menu') });

// 7. Start on the menu; the settings screen exists but is hidden.
showScreen('menu');
print(`Menu ready: ${screens.menu.widgets.length + screens.settings.widgets.length} widget(s)`);
