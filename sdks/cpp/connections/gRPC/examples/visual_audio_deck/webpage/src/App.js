const { useState, useCallback, useEffect } = React;

function App() {
  const [channelStates, setChannelStates] = useState(
    Array.from({ length: 8 }, (_, i) => ({
      id: i + 1,
      signal: 0,
      volume: 0,
      frequency: 0,
      select: false,
      solo: false,
      mute: false,
      warning_comp: false,
      warning_clip: 0,
      warning_freq: false,
      display_img: '',
      display_text: `Channel ${i + 1}`,
      display_mode: `${i + 1}`
    }))
  );

  const [mainState, setMainState] = useState({
    output: 0,
    slider: -20.0,
    select: false,
    solo: false,
    mute: false,
    display_img: '',
    display_text: 'main',
    display_mode: 'LR'
  });

  const [connectionStatus, setConnectionStatus] = useState('Connecting...');

  // API utility functions
  const sendCommand = useCallback(async (command) => {
    try {
      console.log(`🔵 Sending command: ${command}`);
      const response = await fetch('/api/command', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: `command=${encodeURIComponent(command)}`
      });
      
      console.log(`🔵 Response status: ${response.status}`);
      
      if (!response.ok) {
        const errorText = await response.text();
        console.error(`🔴 Command failed: ${response.status} - ${errorText}`);
        throw new Error(`Command failed: ${response.status}`);
      }
      
      const result = await response.json();
      console.log(`🟢 Command ${command} result:`, result);
      return result;
    } catch (error) {
      console.error(`🔴 Error executing command ${command}:`, error);
      throw error;
    }
  }, []);

  const updateParameter = useCallback(async (param, value) => {
    try {
      const response = await fetch('/api/update', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: `param=${encodeURIComponent(param)}&value=${encodeURIComponent(value)}`
      });
      
      if (!response.ok) {
        throw new Error(`Parameter update failed: ${response.status}`);
      }
      
      const result = await response.json();
      console.log(`Parameter ${param} updated to ${value}:`, result);
      return result;
    } catch (error) {
      console.error(`Error updating parameter ${param}:`, error);
      throw error;
    }
  }, []);

  // Fetch parameters from the C++ backend API
  const fetchParams = useCallback(async () => {
    try {
      const response = await fetch('/api/params');
      if (!response.ok) throw new Error('Failed to fetch parameters');
      const paramsData = await response.json();
      
      // Update main state
      if (paramsData.main) {
        setMainState(prev => ({
          ...prev,
          output: paramsData.main.output || 0,
          slider: paramsData.main.slider || -20.0,
          select: paramsData.main.select || false,
          solo: paramsData.main.solo || false,
          mute: paramsData.main.mute || false,
          display_img: paramsData.main.display_img || '',
          display_text: paramsData.main.display_text || 'main',
          display_mode: paramsData.main.display_mode || 'LR'
        }));
      }

      // Update channel states
      if (paramsData.channels) {
        setChannelStates(prev => 
          prev.map(channel => {
            const backendChannel = paramsData.channels.find(ch => ch.id === channel.id);
            if (backendChannel) {
              return {
                ...channel,
                signal: backendChannel.signal || 0,
                volume: backendChannel.slider || 0,
                frequency: backendChannel.frequency || 0,
                select: backendChannel.select || false,
                solo: backendChannel.solo || false,
                mute: backendChannel.mute || false,
                warning_comp: backendChannel.warning_comp || false,
                warning_clip: backendChannel.warning_clip || 0,
                warning_freq: backendChannel.warning_freq || false,
                display_img: backendChannel.display_img || '',
                display_text: backendChannel.display_text || `Channel ${channel.id}`,
                display_mode: backendChannel.display_mode || `${channel.id}`
              };
            }
            return channel;
          })
        );
      }

      setConnectionStatus('Connected');
    } catch (error) {
      console.error('Error fetching parameters:', error);
      setConnectionStatus('Not Connected');
    }
  }, []);

  // Auto-refresh parameters every 500ms for real-time updates
  useEffect(() => {
    fetchParams(); // Initial fetch
    const interval = setInterval(fetchParams, 500);
    return () => clearInterval(interval);
  }, [fetchParams]);

  // Handle updates from individual channels
  const handleChannelUpdate = useCallback(async (channelId, updateInfo) => {
    console.log(`Channel ${channelId} update received:`, updateInfo);
    try {
      switch (updateInfo.type) {
        case 'CHANNEL_SELECT':
          console.log(`Sending ch${channelId}_select_cmd`);
          await sendCommand(`ch${channelId}_select_cmd`);
          break;
        case 'CHANNEL_SOLO':
          console.log(`Sending ch${channelId}_solo_cmd`);
          await sendCommand(`ch${channelId}_solo_cmd`);
          break;
        case 'CHANNEL_MUTE':
          console.log(`Sending ch${channelId}_mute_cmd`);
          await sendCommand(`ch${channelId}_mute_cmd`);
          break;
        case 'CHANNEL_SLIDER_UPDATE':
          console.log(`Updating ch${channelId}_slider to:`, updateInfo.value);
          await updateParameter(`ch${channelId}_slider`, updateInfo.value);
          break;
        case 'CHANNEL_FREQUENCY_UPDATE':
          console.log(`Updating ch${channelId}_frequency to:`, updateInfo.value);
          await updateParameter(`ch${channelId}_frequency`, updateInfo.value);
          break;
      }
    } catch (error) {
      console.error(`Error handling channel ${channelId} update:`, error);
    }
  }, [sendCommand, updateParameter]);

  // Handle updates from main mixer
  const handleMainMixerUpdate = useCallback(async (updateInfo) => {
    try {
      switch (updateInfo.type) {
        case 'MAIN_SELECT':
          await sendCommand('main_select_cmd');
          break;
        case 'MAIN_SOLO':
          await sendCommand('main_solo_cmd');
          break;
        case 'MAIN_MUTE':
          await sendCommand('main_mute_cmd');
          break;
        case 'CLEAR_ALL_SOLOS':
          await sendCommand('clear_all');
          break;
        case 'MAIN_SLIDER_UPDATE':
          await updateParameter('main_slider', updateInfo.value);
          break;
      }
    } catch (error) {
      console.error('Error handling main mixer update:', error);
    }
  }, [sendCommand, updateParameter]);

  // Prepare channel inputs for main mixer
  const channelInputs = channelStates.map(channel => ({
    id: channel.id,
    signal: channel.signal,
    volume: channel.volume,
    mute: channel.mute
  }));

  return (
    <div className="app">
      <header className="top-header">
        <img src="/src/assets/logo.png" alt="AudioDeck" />
        <h1>AudioDeck</h1>
      </header>
      
      <main className="app-main">
        <div className="mixer-board">
          {channelStates.map((channel, index) => (
            <MixerChannel 
              key={channel.id}
              channelNumber={channel.id}
              initialVolume={channel.volume}
              channelData={channel}
              onUpdate={(updateData) => handleChannelUpdate(channel.id, updateData)}
              mainSelectActive={mainState.select}
              clearSoloTrigger={channelStates.every(ch => !ch.solo)}
            />
          ))}
          <MainMixerChannel 
            channelInputs={channelInputs}
            mainData={mainState}
            onChannelUpdate={handleMainMixerUpdate}
          />
        </div>
      </main>
    </div>
  );
}
