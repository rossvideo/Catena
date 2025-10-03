var ctrl = (function ($, _) {
    var api = {};
    
    var events = {
        'volume_change': function(ev, param) {
            $.debug('volume slider change event: ' + ev + ' value: ' + param.getValue());
            var volumeValue = param.getValue();
            _.setValue("volume", 0, volumeValue);
        }
    }

    api.event = function(ev) {
        $.debug('handling volume slider event: ' + ev);
        if (ev in events) {
            events[ev].apply(this, arguments);
        } else {
            $.debug('Unrecognized volume slider event: ' + ev);
        }
    }

    api.hello = function() {$.debug('volume slider hello');}

    return api;
}(ogscript, params));