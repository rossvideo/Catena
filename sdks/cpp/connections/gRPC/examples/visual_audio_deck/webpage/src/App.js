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
  const handleChannelUpdate = useCallback((channelId, updateData) => {
    setChannelStates(prev => 
      prev.map(channel => 
        channel.id === channelId 
          ? { ...channel, ...updateData }
          : channel
      )
    );
  }, []);

  // Handle updates from main mixer
  const handleMainMixerUpdate = useCallback((updateInfo) => {
    switch (updateInfo.type) {
      case 'MAIN_SELECT_ON':
        setChannelStates(prev =>
          prev.map(channel => ({
            ...channel,
            select: false
          }))
        );
        break;
      case 'CLEAR_ALL_SOLOS':
        setChannelStates(prev =>
          prev.map(channel => ({
            ...channel,
            solo: false
          }))
        );
        break;
    }
  }, []);

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
        <div className="connection-status">
          Status: <span style={{
            color: connectionStatus === 'Connected' ? 'green' : 'red',
            fontWeight: 'bold'
          }}>{connectionStatus}</span>
        </div>
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
