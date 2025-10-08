var inverted = false;
var gain = 0;
var v = 0

function updateMeter(baseOID) {
    var min = -10, max = 10;
    v = ((min + 3)+ Math.random() * (7)) + gain;

    if (inverted) {
        v = -v;
    }
    params.setValue(baseOID + ".input", 0, v);
    setRawInput();
}

function setInverted() {
    inverted = !inverted;
}

function setGain(value) {
    gain = value;
}
  
function setRawInput() {
    params.setValue("raw_input", 0, Math.round(v * 10) / 10);
    params.setValue("raw_gain", 0, Math.round(gain * 10) / 10);
    params.setValue("raw_inverted", 0, inverted);
}