const { useState, useEffect, useRef } = React;

function MixerChannel({ 
  channelNumber = 1, 
  initialVolume = 75, 
  channelData = {},
  onUpdate = () => {},
  mainSelectActive = false,
  clearSoloTrigger = false 
}) {
  const [volume, setVolume] = useState(0); // -80.0 to 10.0 dB
  const [isMuted, setIsMuted] = useState(false);
  const [isSolo, setIsSolo] = useState(false);
  const [isSelected, setIsSelected] = useState(false);
  const [signal, setSignal] = useState(0);
  
  // Frequency data
  const [frequencies, setFrequencies] = useState({
    10: 0, 21: 0, 42: 0, 83: 0, 166: 0, 333: 0, 577: 0, 1000: 0
  });
  
  const sliderToFreq = {
    1: 10, 2: 21, 3: 42, 4: 83, 5: 166, 6: 333, 7: 577, 8: 1000
  };
  
  const [selectedFreq, setSelectedFreq] = useState(10);
  const [freqEditMode, setFreqEditMode] = useState(false);
  const [channelName] = useState(`Channel ${channelNumber}`);
  
  // Warning states - now driven by backend data
  const [warnings, setWarnings] = useState({
    COMP: channelData.warning_comp || false,
    CLIP: channelData.warning_clip || 0,
    Freq: channelData.warning_freq || false
  });
  
  // Display state
  const [display, setDisplay] = useState({
    img: '/src/assets/signal-wave.png',
    mode: channelNumber.toString(),
    text: `Channel ${channelNumber}`
  });

  const animationRef = useRef();
  const timeRef = useRef(0);

  // Update component state when channelData changes
  useEffect(() => {
    if (channelData.signal !== undefined) setSignal(channelData.signal);
    if (channelData.volume !== undefined) setVolume(channelData.volume);
    if (channelData.frequency !== undefined) setFrequencies(prev => ({ ...prev, [selectedFreq]: channelData.frequency }));
    if (channelData.select !== undefined) {
      setIsSelected(channelData.select);
      
      // Update display based on selection state
      if (channelData.select) {
        // Channel is selected - show frequency mode
        setDisplay({
          img: '/src/assets/freq-edit-icon.png',
          mode: 'Freq',
          text: `Channel ${channelNumber}`
        });
      } else {
        // Channel is not selected - show channel mode
        setDisplay({
          img: '/src/assets/signal-wave.png',
          mode: channelNumber.toString(),
          text: `Channel ${channelNumber}`
        });
      }
    }
    if (channelData.solo !== undefined) setIsSolo(channelData.solo);
    if (channelData.mute !== undefined) setIsMuted(channelData.mute);
    
    // Update warnings from backend
    setWarnings({
      COMP: channelData.warning_comp || false,
      CLIP: channelData.warning_clip || 0,
      Freq: channelData.warning_freq || false
    });
    
    // Update display info from backend (but only if not overridden by selection logic above)
    if (!channelData.select && (channelData.display_img || channelData.display_text || channelData.display_mode)) {
      setDisplay({
        img: channelData.display_img || '/src/assets/signal-wave.png',
        mode: channelData.display_mode || channelNumber.toString(),
        text: channelData.display_text || `Channel ${channelNumber}`
      });
    }
  }, [channelData, selectedFreq, channelNumber]);

  // Cleanup timers on unmount
  useEffect(() => {
    return () => {
      if (sliderUpdateTimer.current) {
        clearTimeout(sliderUpdateTimer.current);
      }
      if (frequencyUpdateTimer.current) {
        clearTimeout(frequencyUpdateTimer.current);
      }
    };
  }, []);

  // Convert linear slider value to logarithmic dB scale
  const linearToLog = (linear) => {
    if (linear === 0) return -80;
    const normalized = linear / 100;
    return -80 + (90 * Math.pow(normalized, 0.25));
  };

  const logToLinear = (dB) => {
    if (dB <= -80) return 0;
    const normalized = (dB + 80) / 90;
    return Math.pow(normalized, 4) * 100;
  };

  // Initialize volume from initialVolume prop
  useEffect(() => {
    setVolume(linearToLog(initialVolume));
  }, [initialVolume]);

  // Update parent component when state changes (signal now comes from backend)
  useEffect(() => {
    const volumeMultiplier = isMuted ? 0 : Math.pow(10, volume / 20);
    onUpdate({
      signal: signal * volumeMultiplier,
      volume: volume,
      select: isSelected,
      solo: isSolo,
      mute: isMuted
    });
  }, [signal, volume, isMuted, isSelected, isSolo, onUpdate]);

  // Clear solo when triggered from main mixer
  useEffect(() => {
    if (clearSoloTrigger && isSolo) {
      setIsSolo(false);
    }
  }, [clearSoloTrigger, isSolo]);

  // Debounce timer for slider updates
  const sliderUpdateTimer = useRef(null);

  const handleVolumeChange = (e) => {
    const dBValue = linearToLog(e.target.value);
    setVolume(dBValue); // Update UI immediately
    
    // Debounce server updates - only send after user stops moving slider for 150ms
    if (sliderUpdateTimer.current) {
      clearTimeout(sliderUpdateTimer.current);
    }
    
    sliderUpdateTimer.current = setTimeout(() => {
      onUpdate({
        type: 'CHANNEL_SLIDER_UPDATE',
        value: dBValue
      });
    }, 150);
  };

  const handleVolumeRelease = (e) => {
    const dBValue = linearToLog(e.target.value);
    // Clear any pending debounced update and send immediately on release
    if (sliderUpdateTimer.current) {
      clearTimeout(sliderUpdateTimer.current);
    }
    onUpdate({
      type: 'CHANNEL_SLIDER_UPDATE',
      value: dBValue
    });
  };

  const toggleMute = () => {
    console.log(`Channel ${channelNumber} Mute clicked`);
    // Optimistic update - toggle immediately
    setIsMuted(prev => !prev);
    
    // Then update server
    onUpdate({
      type: 'CHANNEL_MUTE'
    });
  };

  const toggleSolo = () => {
    console.log(`Channel ${channelNumber} Solo clicked`);
    if (isSelected && !freqEditMode) {
      // Enter frequency edit mode - optimistic update first
      setIsSolo(true);
      setFreqEditMode(true);
      setDisplay(prev => ({
        ...prev,
        mode: 'SET FREQ',
        img: '/src/assets/freq-edit-icon.png',
        text: selectedFreq.toString()
      }));
      
      // Then update server
      onUpdate({
        type: 'CHANNEL_SOLO'
      });
    } else if (isSelected && freqEditMode) {
      // Exit frequency edit mode - optimistic update first
      setIsSolo(false);
      setFreqEditMode(false);
      setDisplay(prev => ({
        ...prev,
        mode: channelNumber.toString(),
        img: '/src/assets/signal-wave.png',
        text: `Channel ${channelNumber}`
      }));
      
      // Then update server
      onUpdate({
        type: 'CHANNEL_SOLO'
      });
      onUpdate({
        type: 'CHANNEL_FREQUENCY_UPDATE',
        value: frequencies[selectedFreq]
      });
    } else {
      // Normal solo toggle - optimistic update first
      setIsSolo(prev => !prev);
      
      // Then update server
      onUpdate({
        type: 'CHANNEL_SOLO'
      });
    }
  };

  const toggleSelect = () => {
    console.log(`Channel ${channelNumber} Select clicked`);
    // Don't do optimistic update here - let the parent handle it
    // This ensures proper coordination between all channels and main
    onUpdate({
      type: 'CHANNEL_SELECT'
    });
  };

  // Debounce timer for frequency updates
  const frequencyUpdateTimer = useRef(null);

  const handleFrequencyChange = (sliderIndex, value) => {
    const freq = sliderToFreq[sliderIndex];
    setFrequencies(prev => ({
      ...prev,
      [freq]: value
    }));
    setWarnings(prev => ({ ...prev, Freq: true }));
    
    // Debounce frequency updates - only send after user stops moving slider for 300ms
    if (frequencyUpdateTimer.current) {
      clearTimeout(frequencyUpdateTimer.current);
    }
    
    frequencyUpdateTimer.current = setTimeout(() => {
      onUpdate({
        type: 'CHANNEL_FREQUENCY_UPDATE',
        value: value
      });
    }, 300);
  };

  const handleFrequencyEdit = (newFreq) => {
    const clampedFreq = Math.round(Math.max(10, Math.min(20000, newFreq)));
    setSelectedFreq(clampedFreq);
    setDisplay(prev => ({
      ...prev,
      text: clampedFreq.toString()
    }));
  };

  return (
    <div className="mixer-channel">

      {/* 1. SELECT Button at top */}
      <div className="top-button">
        <button 
          className={`control-btn select-btn ${isSelected ? 'active' : ''}`}
          onClick={toggleSelect}
        >
          SELECT
        </button>
      </div>

      {/* 2. SOLO Button */}
      <div className="solo-button">
        <button 
          className={`control-btn solo-btn ${isSolo ? 'active' : ''}`}
          onClick={toggleSolo}
        >
          SOLO
        </button>
      </div>

      {/* 3. Display Screen */}
      <div className="display-section">
        <div className="display-screen">
          <img src={display.img} alt="display" className="display-img" />
          <div className="display-mode">{display.mode}</div>
          <div className="display-text">{display.text}</div>
        </div>
      </div>

      {/* Warning indicators */}
      <div className="warning-indicators">
        {warnings.COMP && <div className="warning-light comp">COMP</div>}
        {warnings.CLIP > 0 && <div className="warning-light clip">CLIP {warnings.CLIP}</div>}
        {warnings.Freq && <div className="warning-light freq">FREQ</div>}
      </div>

      {/* Frequency sliders (shown when channel is selected) */}
      {isSelected && !freqEditMode && (
        <div className="frequency-section">
          {Object.entries(sliderToFreq).map(([sliderIndex, freq]) => (
            <div key={sliderIndex} className="freq-slider">
              <label>{freq}Hz</label>
              <input
                type="range"
                min="0"
                max="100"
                value={frequencies[freq]}
                onChange={(e) => handleFrequencyChange(sliderIndex, e.target.value)}
                className="freq-range"
              />
            </div>
          ))}
        </div>
      )}

      {/* Frequency edit mode (shown when solo is active on selected channel) */}
      {freqEditMode && (
        <div className="freq-edit-section">
          <input
            type="range"
            min="10"
            max="20000"
            value={selectedFreq}
            onChange={(e) => handleFrequencyEdit(parseInt(e.target.value))}
            className="freq-edit-slider"
          />
          <span className="freq-display">{selectedFreq}Hz</span>
        </div>
      )}

      {/* 4. MUTE Button */}
      <div className="mute-button">
        <button 
          className={`control-btn mute-btn ${isMuted ? 'active' : ''}`}
          onClick={toggleMute}
        >
          MUTE
        </button>
      </div>

      {/* 5. Volume Fader */}
      <div className="volume-fader">
        <input
          type="range"
          min="0"
          max="100"
          value={logToLinear(volume)}
          onChange={handleVolumeChange}
          onMouseUp={handleVolumeRelease}
          onTouchEnd={handleVolumeRelease}
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

      {/* Channel Label */}
      <div className="channel-label">
        <span>{channelName}</span>
      </div>
    </div>
  );
}
