var top = 10;
var left = 10;
var width = 180;
var height = 15;
var vspace = 0;

var channelCount = 0;
var maxChannels = 8;

var channels = [];
var channelIndexByInput = {}; // InputName -> baseoid

function debug(message, data) {
    var log = message + " | " + JSON.stringify(data);
    ogscript.debug(log);
}

function add() {
    var InputName = params.getValue('InputName', 0);
    if (InputName == "") {
        ogscript.debug("No input name given");
        return
    }

    channelCount++;
    if (channelCount > maxChannels) {
        debug('add() early return: max channels reached', { channelCount: channelCount, maxChannels: maxChannels });
        return;
    }

    var ChannelName = 'Channel ' + (channelCount);
    
    var baseoid = 'ch' + (channelCount);
    var channelNameOID = baseoid + '.channelLabel';
    var inputNameOID = baseoid + '.inputLabel';

    makeInputParams(baseoid);
    makeLabel(channelNameOID);

    params.setValue(channelNameOID, 0, ChannelName);
    params.setValue(inputNameOID, 0, InputName);

    var xml = makeWidget(baseoid);
    ogscript.appendXML('channelHost', xml);

    var rawXml = makeRawWidget(baseoid);
    appendRaw(rawXml);
    // Track
    channels.push({ baseoid: baseoid, channelName: ChannelName, inputName: InputName });
    channelIndexByInput[InputName] = baseoid;

    // Render/refresh selection list
    renderChannelList();

    clearTextBoxes();
}

function remove(OID) {
    ogscript.debug("Removing...")
}

// Params Creation Functions

function makeWidget(baseoid) {
    var xml = '<widget widgetid="test_channel" baseoid="' + baseoid + '" left="' + left + '" top="' + top + '"/>';
    left += width + vspace;
    return xml;
}

function makeInputParams(baseoid) {
    params.createCopy('gain', baseoid + '.gain');
    params.createCopy('input', baseoid + '.input');
    params.createCopy('inputLabel', baseoid + '.inputLabel');
    params.createCopy('invert', baseoid + '.invert')
}

function makeLabel(OID) {
    var param = {
        oid: OID,
        type: "STRING",
        value: "value",
    }
    params.createParam(param);
}

function clearTextBoxes() {
    params.setValue('InputName', 0, "");
}

// Raw-data helpers
var rawTop = 10;
var rawLeft = 10;
var rawVSpace = 10;

function makeRawWidget(baseoid) {
    var xml = '<widget widgetid="input_raw" baseoid="' + baseoid + '" left="' + rawLeft + '" top="' + rawTop + '"/>';
    rawTop += 110; // stack vertically
    return xml;
}

function appendRaw(xml) {
    ogscript.appendXML('rawHost', xml);
}

// Selection list rendering (chanList as a list of input names)
function renderChannelList() {
    // rebuild simple list widget each time, using INT_CHOICE keys (0..n-1)
    var selected = '0';
    var listXml = '<param oid="chanListUi" widget="list" constrainttype="INT_CHOICE" width="200" height="170" left="0" top="0" value="' + (channels.length > 0 ? selected : '') + '">';
    for (var i = 0; i < channels.length; i++) {
        var name = channels[i].inputName || channels[i].channelName;
        var key = '' + i;
        listXml += '<constraint key="' + key + '">' + escapeXml(name) + '</constraint>';
    }
    listXml += '</param>';
    ogscript.setObjectHtml('chanListHost', listXml);
}

function escapeXml(unsafe) {
    if (!unsafe) return '';
    return unsafe.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;').replace(/'/g, '&apos;');
}

// Remove currently selected channel from list
function removeSelectedChannel() {
    var selectedIdx = params.getValue('chanListUi', 0);
    if (selectedIdx === undefined || selectedIdx === null || selectedIdx === '') {
        ogscript.debug('No selection to remove');
        return;
    }
    // selectedIdx corresponds to constraint key (we built keys as 0..n-1)
    var idx = parseInt(selectedIdx, 10);
    if (isNaN(idx) || idx < 0 || idx >= channels.length) {
        ogscript.debug('Selection index out of range: ' + selectedIdx);
        return;
    }
    var ch = channels[idx];
    removeChannelByBaseoid(ch.baseoid);
}

function removeChannelByBaseoid(baseoid) {
    // Remove the displayed test_channel widget and its raw widget by querying objects and removing children
    // Remove from channelHost
    removeWidgetsByBase('channelHost', baseoid);
    // Remove from rawHost
    removeWidgetsByBase('rawHost', baseoid);

    // Remove tracking and fix layout
    var i;
    for (i = 0; i < channels.length; i++) {
        if (channels[i].baseoid === baseoid) {
            break;
        }
    }
    if (i < channels.length) {
        channels.splice(i, 1);
    }
    // Re-layout channel widgets horizontally
    relayoutChannels();
    // Re-layout raw widgets vertically
    relayoutRaw();
    // Rebuild list
    renderChannelList();
}

function removeWidgetsByBase(containerId, baseoid) {
    // Remove any child widget whose baseoid matches
    var xmlObj = ogscript.getObject(containerId);
    if (!xmlObj) return;
    var nodes = xmlObj.getElementsByTagName('widget');
    // iterate backwards to safely remove
    for (var i = nodes.getLength() - 1; i >= 0; i--) {
        var node = nodes.item(i);
        var b = node.getAttribute('baseoid');
        if (b === baseoid) {
            node.getParentNode().removeChild(node);
        }
    }
}

function relayoutChannels() {
    // clear and rebuild channelHost from channels[]
    ogscript.setObjectHtml('channelHost', '');
    left = 10;
    for (var i = 0; i < channels.length; i++) {
        var xml = makeWidget(channels[i].baseoid);
        ogscript.appendXML('channelHost', xml);
    }
}

function relayoutRaw() {
    // clear and rebuild rawHost from channels[]
    ogscript.setObjectHtml('rawHost', '');
    rawTop = 10;
    for (var i = 0; i < channels.length; i++) {
        var xml = makeRawWidget(channels[i].baseoid);
        ogscript.appendXML('rawHost', xml);
    }
}