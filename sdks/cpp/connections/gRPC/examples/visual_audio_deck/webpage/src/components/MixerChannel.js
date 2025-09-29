const { useState, useEffect, useRef } = React;

function MixerChannel({ 
  channelNumber = 1, 
  initialVolume = 75, 
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
  
  // Warning states
  const [warnings, setWarnings] = useState({
    COMP: false,
    CLIP: 0,
    Freq: false
  });
  
  // Warning cooldown refs
  const warningUpdateRef = useRef(0);
  const lastWarningUpdateRef = useRef(0);
  
  // Display state
  const [display, setDisplay] = useState({
    img: 'src/assets/signal-wave.png',
    mode: channelNumber.toString(),
    text: `Channel ${channelNumber}`
  });

  const animationRef = useRef();
  const timeRef = useRef(0);

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

  // Signal generation at 60Hz
  useEffect(() => {
    const generateSignal = () => {
      timeRef.current += 1/60; // 60Hz update
      const t = timeRef.current;
      
      let newSignal = 0;
      const f0 = 440 + (channelNumber * 50); // Different base frequency per channel
      const alpha = 0.1;
      
      // Generate complex signal: sum of harmonics with decay
      for (let n = 1; n <= 6; n++) {
        newSignal += (1/n) * Math.sin(2 * Math.PI * n * (f0 + n * 10) * t);
      }
      newSignal *= Math.sin(-alpha * t);
      
      // Apply frequency filtering
      let filteredSignal = newSignal;
      Object.entries(frequencies).forEach(([freq, level]) => {
        if (level !== 0) {
          const freqNum = parseInt(freq);
          const filterEffect = level / 100;
          filteredSignal += newSignal * filterEffect * Math.sin(2 * Math.PI * freqNum * t * 0.001);
        }
      });
      
      // Calculate warnings with cooldown (update every 200ms)
      const currentTime = Date.now();
      if (currentTime - lastWarningUpdateRef.current >= 200) {
        const newWarnings = { ...warnings };
        
        if (Math.abs(filteredSignal) > 1) {
          newWarnings.CLIP = Math.round((Math.abs(filteredSignal) - 1) * 100);
        } else {
          newWarnings.CLIP = 0;
        }
        
        newWarnings.COMP = Math.abs(filteredSignal) > 0.8;
        
        setWarnings(newWarnings);
        lastWarningUpdateRef.current = currentTime;
      }
      
      setSignal(filteredSignal);
      
      // Update parent with signal and volume
      const volumeMultiplier = isMuted ? 0 : Math.pow(10, volume / 20);
      onUpdate({
        signal: filteredSignal * volumeMultiplier,
        volume: volume,
        select: isSelected,
        solo: isSolo,
        mute: isMuted
      });
      
      animationRef.current = requestAnimationFrame(generateSignal);
    };
    
    animationRef.current = requestAnimationFrame(generateSignal);
    
    return () => {
      if (animationRef.current) {
        cancelAnimationFrame(animationRef.current);
      }
    };
  }, [volume, isMuted, isSelected, isSolo, frequencies, channelNumber, onUpdate]);

  // Clear solo when triggered from main mixer
  useEffect(() => {
    if (clearSoloTrigger && isSolo) {
      setIsSolo(false);
    }
  }, [clearSoloTrigger, isSolo]);

  const handleVolumeChange = (e) => {
    const dBValue = linearToLog(e.target.value);
    setVolume(dBValue);
  };

  const toggleMute = () => {
    setIsMuted(!isMuted);
  };

  const toggleSolo = () => {
    if (isSelected && !freqEditMode) {
      // Enter frequency edit mode
      setIsSolo(true);
      setFreqEditMode(true);
      setDisplay({
        img: 'src/assets/freq-edit-icon.png',
        mode: 'SET FREQ',
        text: selectedFreq.toString()
      });
    } else if (isSelected && freqEditMode) {
      // Exit frequency edit mode
      setIsSolo(false);
      setFreqEditMode(false);
      setWarnings(prev => ({ ...prev, Freq: true }));
      setDisplay({
        img: 'src/assets/signal-wave.png',
        mode: 'Freq',
        text: Object.keys(frequencies)[0]
      });
    } else {
      // Normal solo toggle
      setIsSolo(!isSolo);
    }
  };

  const toggleSelect = () => {
    const newSelectState = !isSelected;
    setIsSelected(newSelectState);
    
    if (newSelectState) {
      // Set to frequency mode
      setDisplay({
        img: 'src/assets/signal-wave.png',
        mode: 'Freq',
        text: Object.keys(frequencies)[0]
      });
    } else {
      // Return to channel mode
      setDisplay({
        img: 'src/assets/signal-wave.png',
        mode: channelNumber.toString(),
        text: channelName
      });
    }
  };

  const handleFrequencyChange = (sliderIndex, value) => {
    const freq = sliderToFreq[sliderIndex];
    setFrequencies(prev => ({
      ...prev,
      [freq]: value
    }));
    setWarnings(prev => ({ ...prev, Freq: true }));
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
