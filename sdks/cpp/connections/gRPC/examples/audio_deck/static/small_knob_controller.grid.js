var ctrl = (function ($, _) {
    var api = {};
    var range = 16383.0;
    var ctrl_octaves = 14.0;
    var freq_min = 20.0;
    var freq_max = 20000.0;
    var freq_octaves = Math.log(freq_max / freq_min)/Math.log(2.0);
    var idx_per_octave = range / ctrl_octaves;
    var octave_scaling = freq_octaves / ctrl_octaves;
    var f_min = 20.0;

    var events = {
        'wheel': function(ev, param) {
            $.debug('small knob wheel event: ' + ev + ' value: ' + param.getValue());
            var f = f_min * Math.pow(2, (param.getValue() / idx_per_octave) * octave_scaling);
            $.debug('frequency: ' + f);
            _.setValue("freq", 0, f);
        }
    }

    api.event = function(ev) {
        $.debug('handling small knob event: ' + ev);
        if (ev in events) {
            events[ev].apply(this, arguments);
        } else {
            $.debug('Unrecognized small knob event: ' + ev);
        }
    }

    api.hello = function() {$.debug('small knob hello');}

    return api;
}(ogscript, params));
