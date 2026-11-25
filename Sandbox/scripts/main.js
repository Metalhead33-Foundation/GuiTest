const CONFIG = require('CONFIG');

print("Hello World from C++/JS!");
for (const [key, value] of Object.entries(CONFIG)) {
    console.log(`[${key}]`);
    for(const [jey, jalue] of Object.entries(value)) {
        console.log(`${jey}: ${jalue}`);
    }
}

/*
The old legacy script:

const CONFIG = require('CONFIG');
const localizations = require('localization');
const IO = require('IO');
const Audio = require('Audio');
const GlobalMixer = require('GLOBAL_AUDIO_MIXER');
const GUI = require('GUI');
const system = require('System');

print("Hello World from C++/JS!");
for (const [key, value] of Object.entries(CONFIG)) {
    console.log(`${key}`);
    for(const [jey, jalue] of Object.entries(value)) {
        console.log(`${jey}: ${jalue}`);
    }
}
for (const [key, value] of Object.entries(localizations)) {
  console.log(`${key}: ${value}`);
}
const stdout = new IO.StdOut();
stdout.writeString("Hello from raw IO out!");
stdout.writeString("Well, whatever");
//print(CONFIG.Video.iHeight);
const audiofile = new IO.PhysFsFile('/sounds/rubies.ogg',1);
const rubiesMusic = new Audio.Streamer(audiofile);
rubiesMusic.state = 0;
rubiesMusic.cursor = 0;
rubiesMusic.repeating = true;
GlobalMixer.insert(rubiesMusic,0.5);
const tex = system.gibTexture('/textures/menuButton2.png');
const texCoords = [0, 0, 1, 0.2, 0, 0.2, 1, 0.4, 0, 0, 1, 0.2, 0, 0.4, 1, 0.6, 0, 0, 1, 0.2, 0, 0.6, 1, 0.8, 0, 0, 0, 0, 0, 0.8, 1, 1 ];
const guiElement = new GUI.TexturedButton(tex,texCoords);
guiElement.onStateChange = function(val1, val2) {
    print(`Old value: ${val1}\nNew value: ${val2}`);
    if(guiElement.clicked) rubiesMusic.state = 1;
    else rubiesMusic.state = 0;
};
guiElement.enabled = true;
system.addWidget(guiElement);
print(`\n${guiElement.enabled}\n`);
print(`${rubiesMusic.channelCount}\n`);

*/