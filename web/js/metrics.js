// Metrics Module
const MetricsModule = {
    ws: null,

    init() {
        this.setupWebSocket();
    },

    setupWebSocket() {
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const wsUrl = `${protocol}//${window.location.host}/ws/metrics`;
        
        this.ws = new WebSocket(wsUrl);

        this.ws.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                this.updateMetricsUI(data);
            } catch (e) {
                console.error('Failed to parse metrics data', e);
            }
        };

        this.ws.onclose = () => {
            setTimeout(() => this.setupWebSocket(), 3000); // Reconnect
        };
    },

    updateMetricsUI(data) {
        // Just a simple textual update for now. 
        // A full implementation would use Chart.js or Canvas API here.
        const container = document.getElementById('metrics-section');
        if (!container.classList.contains('hidden')) {
            const card = container.querySelector('.glass-card');
            if (card) {
                card.innerHTML = `
                    <div class="grid-2col">
                        <div>
                            <h3>Performance</h3>
                            <p class="mt-4" style="font-size: 2rem; color: var(--accent-primary)">${data.fps.toFixed(1)} FPS</p>
                            <p class="mt-4"><strong>Inference Latency:</strong> ${data.inference_ms.toFixed(1)} ms</p>
                        </div>
                        <div>
                            <h3>Detections</h3>
                            <p class="mt-4" style="font-size: 2rem; color: var(--success)">${data.total_detections}</p>
                            <p class="mt-4">objects detected in current frame</p>
                        </div>
                    </div>
                `;
            }
        }
    }
};

export default MetricsModule;
