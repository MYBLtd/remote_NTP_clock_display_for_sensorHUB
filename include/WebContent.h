#pragma once

const char SETUP_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Control Panel</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        /* Theme variables */
        :root {
            /* Light theme (default) */
            --bg-color: #f5f5f5;
            --card-bg: white;
            --text-color: #333;
            --heading-color: #1a73e8;
            --subheading-color: #5f6368;
            --border-color: #eee;
            --input-bg: #f8f9fa;
            --input-hover: #e8f0fe;
            --slider-bg: #e0e0e0;
            --value-bg: #e8f0fe;
            --button-color: #1a73e8;
            --button-hover: #1557b0;
            --status-bg: #f8f9fa;
            --status-text: #5f6368;
            --success-bg: #e6f4ea;
            --success-text: #1e8e3e;
            --error-bg: #fce8e6;
            --error-text: #d93025;
            --on-color: #1e8e3e;
            --off-color: #d93025;
        }

        [data-theme="dark"] {
            --bg-color: #121212;
            --card-bg: #1e1e1e;
            --text-color: #e0e0e0;
            --heading-color: #90caf9;
            --subheading-color: #b0bec5;
            --border-color: #333;
            --input-bg: #2d2d2d;
            --input-hover: #3d3d3d;
            --slider-bg: #424242;
            --value-bg: #3d3d3d;
            --button-color: #64b5f6;
            --button-hover: #42a5f5;
            --status-bg: #2d2d2d;
            --status-text: #b0bec5;
            --success-bg: #1b5e20;
            --success-text: #a5d6a7;
            --error-bg: #b71c1c;
            --error-text: #ef9a9a;
            --on-color: #66bb6a;
            --off-color: #ef5350;
        }

        /* Base styles */
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
            margin: 0 auto;
            max-width: 600px;
            padding: 20px;
            background-color: var(--bg-color);
            color: var(--text-color);
            line-height: 1.6;
            transition: background-color 0.3s ease, color 0.3s ease;
        }

        .card {
            background: var(--card-bg);
            padding: 24px;
            border-radius: 12px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            margin-bottom: 20px;
            position: relative;
            transition: background-color 0.3s ease;
        }

        .section {
            margin-bottom: 24px;
            padding-bottom: 16px;
            border-bottom: 1px solid var(--border-color);
        }

        .section:last-child {
            border-bottom: none;
        }

        h1 {
            font-size: 24px;
            font-weight: 600;
            margin: 0 0 24px 0;
            color: var(--heading-color);
        }

        h2 {
            font-size: 18px;
            font-weight: 500;
            margin: 0 0 16px 0;
            color: var(--subheading-color);
        }

        .form-group {
            margin-bottom: 16px;
        }

        .radio-group {
            display: flex;
            gap: 20px;
            margin: 10px 0;
        }

        .radio-label {
            display: flex;
            align-items: center;
            cursor: pointer;
            padding: 8px 16px;
            background: var(--input-bg);
            border-radius: 8px;
            transition: all 0.2s ease;
        }

        .radio-label:hover {
            background: var(--input-hover);
        }

        .radio-label input[type="radio"] {
            margin-right: 8px;
        }

        .status-info {
            margin-top: 16px;
            padding: 12px;
            background: var(--status-bg);
            border-radius: 8px;
            display: flex;
            justify-content: space-between;
        }

        .status-info span {
            color: var(--status-text);
        }

        #relay-current-state.on {
            color: var(--on-color);
            font-weight: 600;
        }

        #relay-current-state.off {
            color: var(--off-color);
            font-weight: 600;
        }

        .slider-container {
            margin: 20px 0;
        }

        .slider-container label {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 8px;
        }

        .slider-value {
            background: var(--value-bg);
            padding: 2px 8px;
            border-radius: 4px;
            min-width: 30px;
            text-align: center;
        }

        input[type="range"] {
            width: 100%;
            height: 6px;
            background: var(--slider-bg);
            border-radius: 3px;
            outline: none;
            -webkit-appearance: none;
        }

        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 20px;
            height: 20px;
            background: var(--button-color);
            border-radius: 50%;
            cursor: pointer;
        }

        .time-picker-container {
            display: flex;
            gap: 20px;
            margin: 20px 0;
        }

        .time-picker {
            flex: 1;
        }

        .time-picker select {
            width: 100%;
            padding: 8px;
            border: 1px solid var(--border-color);
            border-radius: 8px;
            background: var(--input-bg);
            font-size: 16px;
            color: var(--text-color);
        }

        .save-button {
            background: var(--button-color);
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 500;
            cursor: pointer;
            width: 100%;
            transition: background-color 0.2s;
        }

        .save-button:hover {
            background: var(--button-hover);
        }

        .status {
            display: none;
            padding: 12px;
            border-radius: 8px;
            margin-top: 16px;
            font-size: 14px;
        }

        .status.success {
            background: var(--success-bg);
            color: var(--success-text);
        }

        .status.error {
            background: var(--error-bg);
            color: var(--error-text);
        }

        /* Theme toggle button */
        .theme-toggle {
            position: absolute;
            top: 24px;
            right: 24px;
        }

        #theme-toggle-btn {
            background: none;
            border: none;
            cursor: pointer;
            color: var(--heading-color);
            padding: 5px;
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: background-color 0.3s ease;
        }

        #theme-toggle-btn:hover {
            background-color: var(--input-hover);
        }

        @media (max-width: 600px) {
            .theme-toggle {
                top: 20px;
                right: 20px;
            }
        }
    </style>
</head>
<body>
    <div class="card">
        <h1>ESP32 Control Panel</h1>
        
        <!-- Theme Toggle Button -->
        <div class="theme-toggle">
            <button id="theme-toggle-btn" aria-label="Toggle dark/light mode">
                <svg id="sun-icon" xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                    <circle cx="12" cy="12" r="5"></circle>
                    <line x1="12" y1="1" x2="12" y2="3"></line>
                    <line x1="12" y1="21" x2="12" y2="23"></line>
                    <line x1="4.22" y1="4.22" x2="5.64" y2="5.64"></line>
                    <line x1="18.36" y1="18.36" x2="19.78" y2="19.78"></line>
                    <line x1="1" y1="12" x2="3" y2="12"></line>
                    <line x1="21" y1="12" x2="23" y2="12"></line>
                    <line x1="4.22" y1="19.78" x2="5.64" y2="18.36"></line>
                    <line x1="18.36" y1="5.64" x2="19.78" y2="4.22"></line>
                </svg>
                <svg id="moon-icon" xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="display: none;">
                    <path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"></path>
                </svg>
            </button>
        </div>

        <!-- Relay Control Section -->
        <div class="section">
            <h2>Relay Control</h2>
            <div class="form-group">
                <div class="radio-group">
                    <label class="radio-label">
                        <input type="radio" name="relayState" value="ON">
                        ON
                    </label>
                    <label class="radio-label">
                        <input type="radio" name="relayState" value="OFF">
                        OFF
                    </label>
                </div>
                <div class="status-info">
                    <span>Current State: <span id="relay-current-state">Loading...</span></span>
                    <span>Override: <span id="relay-override-state">Loading...</span></span>
                </div>
            </div>
        </div>

        <!-- Display Preferences Section -->
        <form id="preferences-form">
            <div class="section">
                <h2>Night Mode</h2>
                <div class="form-group">
                    <div class="radio-group">
                        <label class="radio-label">
                            <input type="radio" name="nightDimming" value="enabled" required>
                            Enabled
                        </label>
                        <label class="radio-label">
                            <input type="radio" name="nightDimming" value="disabled" required>
                            Disabled
                        </label>
                    </div>
                </div>
            </div>

            <div class="section">
                <h2>Brightness Settings</h2>
                <div class="slider-container">
                    <label>
                        Day Brightness
                        <span class="slider-value" id="day-brightness-value">10</span>
                    </label>
                    <input type="range" id="day-brightness" name="dayBrightness" 
                           min="1" max="25" value="10" 
                           oninput="document.getElementById('day-brightness-value').textContent = this.value">
                </div>

                <div class="slider-container">
                    <label>
                        Night Brightness
                        <span class="slider-value" id="night-brightness-value">5</span>
                    </label>
                    <input type="range" id="night-brightness" name="nightBrightness" 
                           min="1" max="25" value="5" 
                           oninput="document.getElementById('night-brightness-value').textContent = this.value">
                </div>
            </div>

            <div class="section">
                <h2>Night Mode Hours</h2>
                <div class="time-picker-container">
                    <div class="time-picker">
                        <label>Start Time</label>
                        <select name="nightStartHour" id="night-start"></select>
                    </div>
                    <div class="time-picker">
                        <label>End Time</label>
                        <select name="nightEndHour" id="night-end"></select>
                    </div>
                </div>
            </div>

            <button type="submit" class="save-button">Save Preferences</button>
        </form>

        <div id="status" class="status"></div>
    </div>

    <script>
        // Populate time options
        function populateTimeOptions() {
            const startSelect = document.getElementById('night-start');
            const endSelect = document.getElementById('night-end');
            
            if (!startSelect || !endSelect) return;
            
            for (let i = 0; i < 24; i++) {
                const hour = i.toString().padStart(2, '0');
                const timeString = `${hour}:00`;
                
                const startOption = new Option(timeString, i);
                const endOption = new Option(timeString, i);
                
                startSelect.add(startOption);
                endSelect.add(endOption);
            }
        }

        // Theme toggle functionality
        function setupThemeToggle() {
            const themeToggleBtn = document.getElementById('theme-toggle-btn');
            const sunIcon = document.getElementById('sun-icon');
            const moonIcon = document.getElementById('moon-icon');
            
            if (!themeToggleBtn || !sunIcon || !moonIcon) return;
            
            // Check for saved theme preference or use system preference
            const savedTheme = localStorage.getItem('theme');
            const prefersDark = window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches;
            
            // Set initial theme
            if (savedTheme === 'dark' || (!savedTheme && prefersDark)) {
                document.documentElement.setAttribute('data-theme', 'dark');
                sunIcon.style.display = 'none';
                moonIcon.style.display = 'block';
            }
            
            // Toggle theme
            themeToggleBtn.addEventListener('click', () => {
                const currentTheme = document.documentElement.getAttribute('data-theme');
                if (currentTheme === 'dark') {
                    document.documentElement.setAttribute('data-theme', 'light');
                    localStorage.setItem('theme', 'light');
                    sunIcon.style.display = 'block';
                    moonIcon.style.display = 'none';
                } else {
                    document.documentElement.setAttribute('data-theme', 'dark');
                    localStorage.setItem('theme', 'dark');
                    sunIcon.style.display = 'none';
                    moonIcon.style.display = 'block';
                }
            });
        }

        document.addEventListener('DOMContentLoaded', function() {
            // Initialize theme toggle
            setupThemeToggle();
            populateTimeOptions();
            let relayUpdateInterval;

            // Status message handler
            function showStatus(message, isError = false) {
                const statusDiv = document.getElementById('status');
                if (!statusDiv) return;
                
                statusDiv.textContent = message;
                statusDiv.style.display = 'block';
                statusDiv.className = 'status ' + (isError ? 'error' : 'success');
                
                setTimeout(() => {
                    statusDiv.style.display = 'none';
                }, 3000);
            }

            // Relay status update function
            async function updateRelayStatus() {
                try {
                    const response = await fetch('/api/relay');
                    if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
                    
                    const result = await response.json();
                    
                    const stateElement = document.getElementById('relay-current-state');
                    const overrideElement = document.getElementById('relay-override-state');
                    const radioButtons = document.getElementsByName('relayState');
                    
                    if (stateElement) {
                        stateElement.textContent = result.state;
                        stateElement.className = result.state.toLowerCase();
                    }
                    
                    if (overrideElement) {
                        overrideElement.textContent = result.override ? 'Yes' : 'No';
                    }
                    
                    for (let radio of radioButtons) {
                        radio.checked = radio.value === result.state;
                    }
                } catch (error) {
                    console.error('Error updating relay status:', error);
                }
            }

            // Set up relay control handlers
            const relayRadios = document.getElementsByName('relayState');
            for (let radio of relayRadios) {
                radio.addEventListener('change', async function() {
                    try {
                        const response = await fetch('/api/relay', {
                            method: 'POST',
                            headers: {
                                'Content-Type': 'application/json',
                            },
                            body: JSON.stringify({ state: this.value })
                        });

                        const result = await response.json();
                        if (response.ok && result.success) {
                            showStatus(`Relay ${this.value} command sent successfully`);
                            updateRelayStatus();
                        } else {
                            throw new Error(result.error || 'Failed to control relay');
                        }
                    } catch (error) {
                        console.error('Error controlling relay:', error);
                        showStatus('Failed to control relay: ' + error.message, true);
                    }
                });
            }

            // Load current preferences
            async function loadPreferences() {
                try {
                    const response = await fetch('/api/preferences', {
                        method: 'GET',
                        headers: {
                            'Accept': 'application/json'
                        }
                    });

                    if (!response.ok) {
                        throw new Error(`HTTP error! status: ${response.status}`);
                    }

                    const result = await response.json();
                    console.log('Loaded preferences:', result);

                    if (!result.success) {
                        throw new Error(result.error || 'Failed to load preferences');
                    }

                    const data = result.data;  // Get preferences from data field

                    // Set night dimming radio
                    const radioButtons = document.getElementsByName('nightDimming');
                    for (let radio of radioButtons) {
                        if (radio.value === (data.nightDimming ? 'enabled' : 'disabled')) {
                            radio.checked = true;
                        }
                    }

                    // Set brightness sliders
                    const dayBrightness = document.getElementById('day-brightness');
                    const nightBrightness = document.getElementById('night-brightness');
                    const dayValue = document.getElementById('day-brightness-value');
                    const nightValue = document.getElementById('night-brightness-value');

                    if (dayBrightness && dayValue) {
                        dayBrightness.value = data.dayBrightness;
                        dayValue.textContent = data.dayBrightness;
                    }

                    if (nightBrightness && nightValue) {
                        nightBrightness.value = data.nightBrightness;
                        nightValue.textContent = data.nightBrightness;
                    }

                    // Set time selects
                    const startSelect = document.getElementById('night-start');
                    const endSelect = document.getElementById('night-end');

                    if (startSelect) startSelect.value = data.nightStartHour;
                    if (endSelect) endSelect.value = data.nightEndHour;

                } catch (error) {
                    console.error('Error loading preferences:', error);
                    showStatus('Failed to load preferences: ' + error.message, true);
                }
            }

            // Start periodic relay updates
            updateRelayStatus();
            relayUpdateInterval = setInterval(updateRelayStatus, 5000);
            
            // Load initial preferences
            loadPreferences();

            // Handle preferences form submission
            const form = document.getElementById('preferences-form');
            if (form) {
                form.onsubmit = async function(e) {
                    e.preventDefault();
                    
                    try {
                        const formData = new FormData(this);
                        const preferences = {
                            nightDimming: formData.get('nightDimming') === 'enabled',
                            dayBrightness: parseInt(formData.get('dayBrightness')),
                            nightBrightness: parseInt(formData.get('nightBrightness')),
                            nightStartHour: parseInt(formData.get('nightStartHour')),
                            nightEndHour: parseInt(formData.get('nightEndHour'))
                        };

                        const response = await fetch('/api/preferences', {
                            method: 'POST',
                            headers: {
                                'Content-Type': 'application/json',
                                'Accept': 'application/json'
                            },
                            body: JSON.stringify(preferences)
                        });

                        const result = await response.json();
                        if (response.ok && result.success) {
                            showStatus('Preferences saved successfully');
                        } else {
                            throw new Error(result.error || 'Failed to save preferences');
                        }
                    } catch (error) {
                        console.error('Error saving preferences:', error);
                        showStatus('Failed to save preferences: ' + error.message, true);
                    }
                };
            }
        });

        // Clean up intervals when page is unloaded
        window.addEventListener('unload', function() {
            if (typeof relayUpdateInterval !== 'undefined') {
                clearInterval(relayUpdateInterval);
            }
        });
    </script>
</body>
</html>
)rawliteral";

const char* const WIFI_CONFIG_TITLE PROGMEM = "WiFi Configuration";
const char* const SCAN_NETWORKS_MSG PROGMEM = "Scanning for networks...";

const char PREFERENCES_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Display Preferences</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        /* Theme variables */
        :root {
            /* Light theme (default) */
            --bg-color: #f5f5f5;
            --card-bg: white;
            --text-color: #333;
            --heading-color: #1a73e8;
            --subheading-color: #5f6368;
            --border-color: #eee;
            --input-bg: #f8f9fa;
            --input-hover: #e8f0fe;
            --slider-bg: #e0e0e0;
            --value-bg: #e8f0fe;
            --button-color: #1a73e8;
            --button-hover: #1557b0;
            --status-bg: #f8f9fa;
            --status-text: #5f6368;
            --success-bg: #e6f4ea;
            --success-text: #1e8e3e;
            --error-bg: #fce8e6;
            --error-text: #d93025;
            --on-color: #1e8e3e;
            --off-color: #d93025;
        }

        [data-theme="dark"] {
            --bg-color: #121212;
            --card-bg: #1e1e1e;
            --text-color: #e0e0e0;
            --heading-color: #90caf9;
            --subheading-color: #b0bec5;
            --border-color: #333;
            --input-bg: #2d2d2d;
            --input-hover: #3d3d3d;
            --slider-bg: #424242;
            --value-bg: #3d3d3d;
            --button-color: #64b5f6;
            --button-hover: #42a5f5;
            --status-bg: #2d2d2d;
            --status-text: #b0bec5;
            --success-bg: #1b5e20;
            --success-text: #a5d6a7;
            --error-bg: #b71c1c;
            --error-text: #ef9a9a;
            --on-color: #66bb6a;
            --off-color: #ef5350;
        }

        /* Base styles */
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            margin: 0 auto;
            max-width: 600px;
            padding: 20px;
            background-color: var(--bg-color);
            color: var(--text-color);
            line-height: 1.6;
            transition: background-color 0.3s ease, color 0.3s ease;
        }

        /* Card container */
        .card {
            background: var(--card-bg);
            padding: 24px;
            border-radius: 12px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            margin-bottom: 20px;
            position: relative;
            transition: background-color 0.3s ease;
        }

        /* Section styling */
        .section {
            margin-bottom: 24px;
            padding-bottom: 16px;
            border-bottom: 1px solid var(--border-color);
        }

        .section:last-child {
            border-bottom: none;
        }

        h1 {
            font-size: 24px;
            font-weight: 600;
            margin: 0 0 24px 0;
            color: var(--heading-color);
        }

        h2 {
            font-size: 18px;
            font-weight: 500;
            margin: 0 0 16px 0;
            color: var(--subheading-color);
        }

        h3 {
            font-size: 16px;
            font-weight: 500;
            margin: 0 0 12px 0;
            color: var(--subheading-color);
        }

        /* Form controls */
        .form-group {
            margin-bottom: 16px;
        }

        .form-group label {
            display: block;
            margin-bottom: 8px;
            color: var(--subheading-color);
        }

        /* Custom radio button styling */
        .radio-group {
            display: flex;
            gap: 20px;
            margin: 10px 0;
        }

        .radio-label {
            display: flex;
            align-items: center;
            cursor: pointer;
            padding: 8px 16px;
            background: var(--input-bg);
            border-radius: 8px;
            transition: all 0.2s ease;
        }

        .radio-label:hover {
            background: var(--input-hover);
        }

        .radio-label input[type="radio"] {
            margin-right: 8px;
        }

        /* Slider styling */
        .slider-container {
            margin: 20px 0;
        }

        .slider-container label {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 8px;
        }

        .slider-value {
            background: var(--value-bg);
            padding: 2px 8px;
            border-radius: 4px;
            min-width: 30px;
            text-align: center;
        }

        input[type="range"] {
            width: 100%;
            height: 6px;
            background: var(--slider-bg);
            border-radius: 3px;
            outline: none;
            -webkit-appearance: none;
        }

        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 20px;
            height: 20px;
            background: var(--button-color);
            border-radius: 50%;
            cursor: pointer;
            transition: background .15s ease-in-out;
        }

        input[type="range"]::-webkit-slider-thumb:hover {
            background: var(--button-hover);
        }

        /* Time picker styling */
        .time-picker-container {
            display: flex;
            gap: 20px;
            margin: 20px 0;
        }

        .time-picker {
            flex: 1;
        }

        .time-picker select {
            width: 100%;
            padding: 8px;
            border: 1px solid var(--border-color);
            border-radius: 8px;
            background: var(--input-bg);
            font-size: 16px;
            color: var(--text-color);
        }

        /* Save button */
        .save-button {
            background: var(--button-color);
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 500;
            cursor: pointer;
            width: 100%;
            transition: background-color 0.2s;
        }

        .save-button:hover {
            background: var(--button-hover);
        }

        .save-button:active {
            background: var(--button-hover);
        }

        /* Status message */
        .status {
            display: none;
            padding: 12px;
            border-radius: 8px;
            margin-top: 16px;
            font-size: 14px;
        }

        .status.success {
            background: var(--success-bg);
            color: var(--success-text);
        }

        .status.error {
            background: var(--error-bg);
            color: var(--error-text);
        }

        .status-info {
            margin-top: 16px;
            padding: 12px;
            background: var(--status-bg);
            border-radius: 8px;
            display: flex;
            justify-content: space-between;
        }

        .status-info span {
            color: var(--status-text);
        }

        #relay0-current-state.on, #relay1-current-state.on {
            color: var(--on-color);
            font-weight: 600;
        }

        #relay0-current-state.off, #relay1-current-state.off {
            color: var(--off-color);
            font-weight: 600;
        }

        .relay-control {
            margin-bottom: 24px;
        }

        .relay-control:last-child {
            margin-bottom: 0;
        }

        /* Theme toggle button */
        .theme-toggle {
            position: absolute;
            top: 24px;
            right: 24px;
        }

        #theme-toggle-btn {
            background: none;
            border: none;
            cursor: pointer;
            color: var(--heading-color);
            padding: 5px;
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: background-color 0.3s ease;
        }

        #theme-toggle-btn:hover {
            background-color: var(--input-hover);
        }

        @media (max-width: 600px) {
            .theme-toggle {
                top: 20px;
                right: 20px;
            }
        }
    </style>
</head>
<body>
    <div class="card">
        <h1>ESP32 Control Panel</h1>
        
        <!-- Theme Toggle Button -->
        <div class="theme-toggle">
            <button id="theme-toggle-btn" aria-label="Toggle dark/light mode">
                <svg id="sun-icon" xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                    <circle cx="12" cy="12" r="5"></circle>
                    <line x1="12" y1="1" x2="12" y2="3"></line>
                    <line x1="12" y1="21" x2="12" y2="23"></line>
                    <line x1="4.22" y1="4.22" x2="5.64" y2="5.64"></line>
                    <line x1="18.36" y1="18.36" x2="19.78" y2="19.78"></line>
                    <line x1="1" y1="12" x2="3" y2="12"></line>
                    <line x1="21" y1="12" x2="23" y2="12"></line>
                    <line x1="4.22" y1="19.78" x2="5.64" y2="18.36"></line>
                    <line x1="18.36" y1="5.64" x2="19.78" y2="4.22"></line>
                </svg>
                <svg id="moon-icon" xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="display: none;">
                    <path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"></path>
                </svg>
            </button>
        </div>
        
        <!-- Relay Control Section -->
        <div class="section">
            <h2>Relay Control</h2>
            
            <!-- Relay 1 -->
            <div class="relay-control">
                <h3>Relay 1</h3>
                <div class="form-group">
                    <div class="radio-group">
                        <label class="radio-label">
                            <input type="radio" name="relay0State" value="ON">
                            ON
                        </label>
                        <label class="radio-label">
                            <input type="radio" name="relay0State" value="OFF">
                            OFF
                        </label>
                    </div>
                    <div class="status-info">
                        <span>Current State: <span id="relay0-current-state">Loading...</span></span>
                        <span>Override: <span id="relay0-override-state">Loading...</span></span>
                    </div>
                </div>
            </div>

            <!-- Relay 2 -->
            <div class="relay-control">
                <h3>Relay 2</h3>
                <div class="form-group">
                    <div class="radio-group">
                        <label class="radio-label">
                            <input type="radio" name="relay1State" value="ON">
                            ON
                        </label>
                        <label class="radio-label">
                            <input type="radio" name="relay1State" value="OFF">
                            OFF
                        </label>
                    </div>
                    <div class="status-info">
                        <span>Current State: <span id="relay1-current-state">Loading...</span></span>
                        <span>Override: <span id="relay1-override-state">Loading...</span></span>
                    </div>
                </div>
            </div>
        </div>
        
        <form id="preferences-form">
            <div class="section">
                <h2>Night Mode Dimming</h2>
                <div class="form-group">
                    <div class="radio-group">
                        <label class="radio-label">
                            <input type="radio" name="nightDimming" value="enabled" required>
                            Enabled
                        </label>
                        <label class="radio-label">
                            <input type="radio" name="nightDimming" value="disabled" required>
                            Disabled
                        </label>
                    </div>
                </div>
            </div>

            <div class="section">
                <h2>Brightness Settings</h2>
                <div class="slider-container">
                    <label>
                        Day Brightness
                        <span class="slider-value" id="day-brightness-value">10</span>
                    </label>
                    <input type="range" id="day-brightness" name="dayBrightness" 
                           min="1" max="25" value="10" 
                           oninput="document.getElementById('day-brightness-value').textContent = this.value">
                </div>

                <div class="slider-container">
                    <label>
                        Night Brightness
                        <span class="slider-value" id="night-brightness-value">5</span>
                    </label>
                    <input type="range" id="night-brightness" name="nightBrightness" 
                           min="1" max="25" value="5" 
                           oninput="document.getElementById('night-brightness-value').textContent = this.value">
                </div>
            </div>

            <div class="section">
                <h2>Night Mode Hours</h2>
                <div class="time-picker-container">
                    <div class="time-picker">
                        <label>Start Time</label>
                        <select name="nightStartHour" id="night-start">
                            <!-- Options filled by JavaScript -->
                        </select>
                    </div>
                    <div class="time-picker">
                        <label>End Time</label>
                        <select name="nightEndHour" id="night-end">
                            <!-- Options filled by JavaScript -->
                        </select>
                    </div>
                </div>
            </div>

            <button type="submit" class="save-button">Save Preferences</button>
        </form>

        <div id="status" class="status"></div>
    </div>

<script>
    // Populate time select options
    function populateTimeOptions() {
        const startSelect = document.getElementById('night-start');
        const endSelect = document.getElementById('night-end');
        
        if (!startSelect || !endSelect) return;
        
        for (let i = 0; i < 24; i++) {
            const hour = i.toString().padStart(2, '0');
            const timeString = `${hour}:00`;
            
            const startOption = new Option(timeString, i);
            const endOption = new Option(timeString, i);
            
            startSelect.add(startOption);
            endSelect.add(endOption);
        }
    }

    // Theme toggle functionality
    function setupThemeToggle() {
        const themeToggleBtn = document.getElementById('theme-toggle-btn');
        const sunIcon = document.getElementById('sun-icon');
        const moonIcon = document.getElementById('moon-icon');
        
        if (!themeToggleBtn || !sunIcon || !moonIcon) return;
        
        // Check for saved theme preference or use system preference
        const savedTheme = localStorage.getItem('theme');
        const prefersDark = window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches;
        
        // Set initial theme
        if (savedTheme === 'dark' || (!savedTheme && prefersDark)) {
            document.documentElement.setAttribute('data-theme', 'dark');
            sunIcon.style.display = 'none';
            moonIcon.style.display = 'block';
        }
        
        // Toggle theme
        themeToggleBtn.addEventListener('click', () => {
            const currentTheme = document.documentElement.getAttribute('data-theme');
            if (currentTheme === 'dark') {
                document.documentElement.setAttribute('data-theme', 'light');
                localStorage.setItem('theme', 'light');
                sunIcon.style.display = 'block';
                moonIcon.style.display = 'none';
            } else {
                document.documentElement.setAttribute('data-theme', 'dark');
                localStorage.setItem('theme', 'dark');
                sunIcon.style.display = 'none';
                moonIcon.style.display = 'block';
            }
        });
    }

    // Function to show status message
    function showStatus(message, isError = false) {
        const statusDiv = document.getElementById('status');
        if (!statusDiv) return;
        
        statusDiv.textContent = message;
        statusDiv.style.display = 'block';
        statusDiv.className = 'status ' + (isError ? 'error' : 'success');
        
        setTimeout(() => {
            statusDiv.style.display = 'none';
        }, 3000);
    }

    // Load current preferences
    async function loadPreferences() {
        try {
            const response = await fetch('/api/preferences', {
                method: 'GET',
                headers: {
                    'Accept': 'application/json'
                }
            });

            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }

            const result = await response.json();
            console.log('Loaded preferences:', result);

            if (!result.success) {
                throw new Error(result.error || 'Failed to load preferences');
            }

            const data = result.data;  // Get preferences from data field

            // Set night dimming radio
            const radioButtons = document.getElementsByName('nightDimming');
            for (let radio of radioButtons) {
                if (radio.value === (data.nightDimming ? 'enabled' : 'disabled')) {
                    radio.checked = true;
                }
            }

            // Set brightness sliders
            const dayBrightness = document.getElementById('day-brightness');
            const nightBrightness = document.getElementById('night-brightness');
            const dayValue = document.getElementById('day-brightness-value');
            const nightValue = document.getElementById('night-brightness-value');

            if (dayBrightness && dayValue) {
                dayBrightness.value = data.dayBrightness;
                dayValue.textContent = data.dayBrightness;
            }

            if (nightBrightness && nightValue) {
                nightBrightness.value = data.nightBrightness;
                nightValue.textContent = data.nightBrightness;
            }

            // Set time selects
            const startSelect = document.getElementById('night-start');
            const endSelect = document.getElementById('night-end');

            if (startSelect) startSelect.value = data.nightStartHour;
            if (endSelect) endSelect.value = data.nightEndHour;

        } catch (error) {
            console.error('Error loading preferences:', error);
            showStatus('Failed to load preferences: ' + error.message, true);
        }
    }

    // Relay status update function
    async function updateRelayStatus() {
        try {
            const response = await fetch('/api/relay');
            if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
            
            const relays = await response.json();
            
            // Update both relays
            for (let i = 0; i < 2; i++) {
                const relay = relays[i];
                const stateElement = document.getElementById(`relay${i}-current-state`);
                const overrideElement = document.getElementById(`relay${i}-override-state`);
                const radioButtons = document.getElementsByName(`relay${i}State`);
                
                if (stateElement) {
                    stateElement.textContent = relay.state;
                    stateElement.className = relay.state.toLowerCase();
                }
                
                if (overrideElement) {
                    overrideElement.textContent = relay.override ? 'Yes' : 'No';
                }
                
                for (let radio of radioButtons) {
                    radio.checked = radio.value === relay.state;
                }
            }
        } catch (error) {
            console.error('Error updating relay status:', error);
        }
    }

    function setupRelayControls() {
        for (let i = 0; i < 2; i++) {
            const relayRadios = document.getElementsByName(`relay${i}State`);
            for (let radio of relayRadios) {
                radio.addEventListener('change', async function() {
                    try {
                        const response = await fetch('/api/relay', {
                            method: 'POST',
                            headers: {
                                'Content-Type': 'application/json',
                            },
                            body: JSON.stringify({
                                relay_id: i,
                                state: this.value
                            })
                        });

                        const result = await response.json();
                        if (response.ok && result.success) {
                            showStatus(`Relay ${i + 1} ${this.value} command sent successfully`);
                            updateRelayStatus();
                        } else {
                            throw new Error(result.error || 'Failed to control relay');
                        }
                    } catch (error) {
                        console.error('Error controlling relay:', error);
                        showStatus('Failed to control relay: ' + error.message, true);
                    }
                });
            }
        }
    }

    document.addEventListener('DOMContentLoaded', function() {
        // Initialize theme toggle and UI
        setupThemeToggle();
        populateTimeOptions();
        setupRelayControls();
        
        // Set up relay status updates
        updateRelayStatus();
        const relayUpdateInterval = setInterval(updateRelayStatus, 5000);
        
        // Load initial preferences
        loadPreferences();

        // Handle form submission
        const form = document.getElementById('preferences-form');
        if (form) {
            form.onsubmit = async function(e) {
                e.preventDefault();
                
                try {
                    const formData = new FormData(this);
                    const preferences = {
                        nightDimming: formData.get('nightDimming') === 'enabled',
                        dayBrightness: parseInt(formData.get('dayBrightness')),
                        nightBrightness: parseInt(formData.get('nightBrightness')),
                        nightStartHour: parseInt(formData.get('nightStartHour')),
                        nightEndHour: parseInt(formData.get('nightEndHour'))
                    };

                    console.log('Sending preferences:', preferences);

                    const response = await fetch('/api/preferences', {
                        method: 'POST',
                        headers: {
                            'Content-Type': 'application/json',
                            'Accept': 'application/json'
                        },
                        body: JSON.stringify(preferences)
                    });

                    const result = await response.json();
                    console.log('Server response:', result);
                    
                    if (response.ok && result.success) {
                        showStatus('Preferences saved successfully');
                    } else {
                        throw new Error(result.error || 'Failed to save preferences');
                    }
                } catch (error) {
                    console.error('Error saving preferences:', error);
                    showStatus('Failed to save preferences: ' + error.message, true);
                }
            };
        }
        
        // Clean up intervals when page is unloaded
        window.addEventListener('unload', function() {
            if (relayUpdateInterval) {
                clearInterval(relayUpdateInterval);
            }
        });
    });
</script>
</body>
</html>
)rawliteral";