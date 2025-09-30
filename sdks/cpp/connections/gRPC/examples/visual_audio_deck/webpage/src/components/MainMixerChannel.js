const { useState, useEffect, useRef } = React;

function MainMixerChannel({ channelInputs = [], mainData = {}, onChannelUpdate = () => {} }) {
  // Main mixer state - use backend data when available
  const [volume_m, setVolume_m] = useState(mainData.slider || 0);
  const [select_m, setSelect_m] = useState(mainData.select || false);
  const [solo_m, setSolo_m] = useState(mainData.solo || false);
  const [mute_m, setMute_m] = useState(mainData.mute || false);
  const [display_m, setDisplay_m] = useState({
    img: mainData.display_img || '/src/assets/signal-wave.png',
    mode: mainData.display_mode || 'LR',
    text: mainData.display_text || 'main'
  });

  // Update state when mainData changes
  useEffect(() => {
    if (mainData.slider !== undefined) setVolume_m(mainData.slider);
    if (mainData.select !== undefined) {
      setSelect_m(mainData.select);
      
      // Update display based on selection state
      if (mainData.select) {
        // Main is selected - show main mode (use logo as placeholder for ross-video-icon)
        setDisplay_m({
          img: '/src/assets/logo.png',
          mode: 'LR',
          text: 'main'
        });
      } else {
        // Main is not selected - show default mode
        setDisplay_m({
          img: '/src/assets/signal-wave.png',
          mode: 'LR',
          text: 'main'
        });
      }
    }
    if (mainData.solo !== undefined) setSolo_m(mainData.solo);
    if (mainData.mute !== undefined) setMute_m(mainData.mute);
    
    // Update display info from backend (but only if not overridden by selection logic above)
    if (!mainData.select && (mainData.display_img || mainData.display_mode || mainData.display_text)) {
      setDisplay_m({
        img: mainData.display_img || '/src/assets/signal-wave.png',
        mode: mainData.display_mode || 'LR',
        text: mainData.display_text || 'main'
      });
    }
  }, [mainData]);

  // Cleanup timer on unmount
  useEffect(() => {
    return () => {
      if (sliderUpdateTimer.current) {
        clearTimeout(sliderUpdateTimer.current);
      }
    };
  }, []);

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

  // Use backend main output data directly, or calculate as fallback
  const getMainOutput = () => {
    // Use backend data if available
    if (mainData.output !== undefined) {
      return mainData.output;
    }
    
    // Fallback calculation if backend data not available
    const mainVolumeMultiplier = mute_m ? 0 : Math.pow(10, volume_m / 20);
    return channelInputs.reduce((sum, input) => {
      const signal = input.signal || 0;
      return sum + signal;
    }, 0) * mainVolumeMultiplier;
  };

  // Main mixer controls - with optimistic updates
  const handleSelectMain = () => {
    console.log('🎛️ Main Select clicked');
    // Don't do optimistic update here - let the parent handle it
    // This ensures proper coordination between main and all channels
    onChannelUpdate({
      type: 'MAIN_SELECT'
    });
  };

  const handleSoloMain = () => {
    console.log('🎛️ Main Solo clicked');
    // Optimistic update - toggle immediately
    setSolo_m(prev => !prev);
    
    // Then update server
    onChannelUpdate({
      type: 'MAIN_SOLO'
    });
  };

  const handleMuteMain = () => {
    console.log('🎛️ Main Mute clicked');
    // Optimistic update - toggle immediately
    setMute_m(prev => !prev);
    
    // Then update server
    onChannelUpdate({
      type: 'MAIN_MUTE'
    });
  };

  const handleClearMain = () => {
    console.log('🎛️ Clear All clicked');
    // Optimistic update - clear solo immediately
    setSolo_m(false);
    
    // Then update server
    onChannelUpdate({
      type: 'CLEAR_ALL_SOLOS'
    });
  };

  // Debounce timer for slider updates
  const sliderUpdateTimer = useRef(null);

  const handleSliderChange = (value) => {
    const dbValue = linearToLog(value);
    setVolume_m(dbValue); // Update UI immediately
    
    // Debounce server updates - only send after user stops moving slider for 150ms
    if (sliderUpdateTimer.current) {
      clearTimeout(sliderUpdateTimer.current);
    }
    
    sliderUpdateTimer.current = setTimeout(() => {
      onChannelUpdate({
        type: 'MAIN_SLIDER_UPDATE',
        value: dbValue
      });
    }, 150);
  };

  const handleSliderRelease = (value) => {
    const dbValue = linearToLog(value);
    // Clear any pending debounced update and send immediately on release
    if (sliderUpdateTimer.current) {
      clearTimeout(sliderUpdateTimer.current);
    }
    onChannelUpdate({
      type: 'MAIN_SLIDER_UPDATE',
      value: dbValue
    });
  };


  const mainOutput = getMainOutput();

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
          height: `${isNaN(mainOutput) ? 0 : Math.min(100, Math.abs(mainOutput) * 50)}%`,
          backgroundColor: Math.abs(mainOutput) > 1 ? '#ff4444' : '#44ff44'
        }}></div>
        <span className="output-value">{isNaN(mainOutput) ? '0.00' : mainOutput.toFixed(6)}</span>
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
          onChange={(e) => handleSliderChange(e.target.value)}
          onMouseUp={(e) => handleSliderRelease(e.target.value)}
          onTouchEnd={(e) => handleSliderRelease(e.target.value)}
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
