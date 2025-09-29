const { useState, useEffect, useRef } = React;

function MainMixerChannel({ channelInputs = [], onChannelUpdate = () => {} }) {
  // Main mixer state
  const [volume_m, setVolume_m] = useState(0); // -80.0 to 10.0
  const [select_m, setSelect_m] = useState(false);
  const [solo_m, setSolo_m] = useState(false);
  const [mute_m, setMute_m] = useState(false);
  const [display_m, setDisplay_m] = useState({
    img: 'src/assets/signal-wave.png',
    mode: 'LR',
    text: 'main'
  });

  // Convert linear slider value to logarithmic dB scale
  const linearToLog = (linear) => {
    // Map 0-100 to -80 to 10 dB with log curve
    if (linear === 0) return -80;
    const normalized = linear / 100;
    return -80 + (90 * Math.pow(normalized, 0.25)); // Gentle log curve
  };

  const logToLinear = (dB) => {
    if (dB <= -80) return 0;
    const normalized = (dB + 80) / 90;
    return Math.pow(normalized, 4) * 100; // Inverse of the log curve
  };

  // Calculate main output (sum of channel inputs * master volume)
  const calculateMainOutput = () => {
    const mainVolumeMultiplier = mute_m ? 0 : Math.pow(10, volume_m / 20);
    return channelInputs.reduce((sum, input) => sum + input.signal, 0) * mainVolumeMultiplier;
  };

  // Main mixer controls
  const handleSelectMain = () => {
    const newSelectState = !select_m;
    setSelect_m(newSelectState);
    
    if (newSelectState) {
      // Turning on main select
      setDisplay_m({
        img: 'src/assets/signal-wave.png',
        mode: 'LR',
        text: 'main'
      });
      
      // Notify parent to turn off all channel selects and set to volume mode
      onChannelUpdate({
        type: 'MAIN_SELECT_ON',
        payload: {}
      });
    } else {
      // Turning off main select
      setDisplay_m({
        img: 'src/assets/signal-wave.png',
        mode: 'MAIN',
        text: 'master'
      });
    }
  };

  const handleSoloMain = () => {
    setSolo_m(!solo_m);
  };

  const handleMuteMain = () => {
    setMute_m(!mute_m);
  };

  const handleClearMain = () => {
    // Notify parent to clear all channel solos
    onChannelUpdate({
      type: 'CLEAR_ALL_SOLOS',
      payload: {}
    });
  };


  const mainOutput = calculateMainOutput();

  return (
    <div className="mixer-channel main-channel">
      {/* 1. SELECT Button */}
      <div className="top-button">
        <button 
          className={`control-btn select-btn ${select_m ? 'active' : ''}`}
          onClick={handleSelectMain}
        >
          SELECT
        </button>
      </div>

      {/* 3. SOLO Button */}
      <div className="solo-button">
        <button 
          className={`control-btn solo-btn ${solo_m ? 'active' : ''}`}
          onClick={handleSoloMain}
        >
          SOLO
        </button>
      </div>

      {/* 3. CLEAR Button */}
      <div className="clear-button">
        <button 
          className="control-btn clear-btn"
          onClick={handleClearMain}
        >
          CLEAR
        </button>
      </div>

      {/* 4. Screen */}
      <div className="display-section">
        <div className="display-screen">
          <img src={display_m.img} alt="display" className="display-img" />
          <div className="display-mode">{display_m.mode}</div>
          <div className="display-text">{display_m.text}</div>
        </div>
      </div>

      {/* 5. Freq Screen (Output Meter) */}
      <div className="main-output-meter">
        <div className="output-level" style={{
          height: `${Math.min(100, Math.abs(mainOutput) * 50)}%`,
          backgroundColor: Math.abs(mainOutput) > 1 ? '#ff4444' : '#44ff44'
        }}></div>
        <span className="output-value">{mainOutput.toFixed(2)}</span>
      </div>

      {/* 6. MUTE Button */}
      <div className="mute-button">
        <button 
          className={`control-btn mute-btn ${mute_m ? 'active' : ''}`}
          onClick={handleMuteMain}
        >
          MUTE
        </button>
      </div>

      {/* 7. Volume Fader */}
      <div className="volume-fader">
        <input
          type="range"
          min="0"
          max="100"
          value={logToLinear(volume_m)}
          onChange={(e) => setVolume_m(linearToLog(e.target.value))}
          className="vertical-slider"
          orient="vertical"
        />
        <div className="volume-scale">
          <span>10</span>
          <span>0</span>
          <span>-20</span>
          <span>-40</span>
          <span>-80</span>
        </div>
      </div>

      <div className="channel-label">
        <span>MASTER</span>
      </div>
    </div>
  );
}
