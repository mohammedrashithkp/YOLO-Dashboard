// Camera Module
const CameraModule = {
    init() {
        this.bindEvents();
        this.fetchCameras();
        
        // Setup WS stream
        this.setupStreamWebSocket();
    },

    bindEvents() {
        document.getElementById('refresh-cameras').addEventListener('click', () => this.fetchCameras());
        document.getElementById('connect-camera').addEventListener('click', () => this.connectCamera());
    },

    async fetchCameras() {
        try {
            const select = document.getElementById('camera-select');
            select.innerHTML = '<option>Loading...</option>';
            
            const cameras = await ApiClient.get('/cameras');
            select.innerHTML = '';
            
            if (cameras.length === 0) {
                select.innerHTML = '<option value="">No cameras found</option>';
                return;
            }

            cameras.forEach(cam => {
                const opt = document.createElement('option');
                opt.value = cam.id;
                opt.text = `[${cam.id}] ${cam.name} (${cam.device_path})`;
                select.appendChild(opt);
            });
        } catch (e) {
            Toast.show('Failed to fetch cameras', 'error');
        }
    },

    async connectCamera() {
        const id = document.getElementById('camera-select').value;
        if (!id) return;

        try {
            await ApiClient.post('/cameras/select', { id: parseInt(id), width: 640, height: 480 });
            Toast.show('Camera connected', 'success');
            
            document.getElementById('camera-status-dot').className = 'dot dot-green';
            document.getElementById('camera-status-text').innerText = 'Connected';
            
            // Show MJPEG stream fallback if WS fails
            // document.getElementById('camera-preview').src = `/api/stream?t=${Date.now()}`;
            // document.getElementById('camera-preview').classList.remove('hidden');
            // document.getElementById('camera-placeholder').classList.add('hidden');
            
        } catch (e) {
            Toast.show('Failed to connect camera', 'error');
            document.getElementById('camera-status-dot').className = 'dot dot-red';
            document.getElementById('camera-status-text').innerText = 'Disconnected';
        }
    },

    setupStreamWebSocket() {
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const wsUrl = `${protocol}//${window.location.host}/ws/stream`;
        
        const ws = new WebSocket(wsUrl);
        ws.binaryType = 'arraybuffer';
        
        const img = document.getElementById('camera-preview');
        const placeholder = document.getElementById('camera-placeholder');

        ws.onmessage = (event) => {
            const blob = new Blob([event.data], {type: 'image/jpeg'});
            const url = URL.createObjectURL(blob);
            
            // Handle memory leak of object URLs
            img.onload = () => URL.revokeObjectURL(url);
            img.src = url;
            
            img.classList.remove('hidden');
            placeholder.classList.add('hidden');
        };

        ws.onclose = () => {
            setTimeout(() => this.setupStreamWebSocket(), 3000); // Reconnect
        };
    }
};

export default CameraModule;
