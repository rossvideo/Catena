var v = 0
var gain = 0;

function updateMeter(baseOID) {
    var gain = params.getValue(baseOID + ".gain", 0);
    //var inverted = params.getValue(baseOID + ".inverted", 0);
    
    var min = -10, max = 10;
    v = ((min + 3)+ Math.random() * (7)) + gain;

    if (getInverted(baseOID) == 1) {
        v = -v;
    }

    params.setValue(baseOID + ".input", 0, v);
}

function getInverted(baseOID) {
    return params.getValue(baseOID + ".invert", 0);
}

function setStereo(value) {
    stereo = value;
}

function updateTotalInput(baseOID) {
    var total = 0;
    for (var i = 0; i < 8; i++) {
        var v = params.getValue("audio_deck/" + i + ".input", 0);
        // Ensure numeric addition
        total += (typeof v === 'number') ? v : parseFloat(v) || 0;
    }
    params.setValue(baseOID + ".input", 0, total);
}