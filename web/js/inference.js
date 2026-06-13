// Inference Module
const InferenceModule = {
    selectedModelPath: '',
    isRunning: false,

    init() {
        this.bindEvents();
        this.fetchModels();
        this.checkStatus();
    },

    bindEvents() {
        document.getElementById('refresh-models').addEventListener('click', () => this.fetchModels());
        document.getElementById('toggle-inference').addEventListener('click', () => this.toggleInference());
        
        const confSlider = document.getElementById('conf-thresh');
        const confVal = document.getElementById('conf-val');
        if (confSlider && confVal) {
            confSlider.addEventListener('input', (e) => {
                confVal.innerText = parseFloat(e.target.value).toFixed(2);
                // Can send config update to backend if live update is supported
            });
        }
    },

    async fetchModels(path = './data/models') {
        try {
            const models = await ApiClient.get(`/models/browse?path=${encodeURIComponent(path)}`);
            const list = document.getElementById('model-list');
            list.innerHTML = '';

            if (models.length === 0) {
                list.innerHTML = '<li><span class="placeholder">No models found in ./data/models</span></li>';
                return;
            }

            models.forEach(model => {
                const li = document.createElement('li');
                const icon = model.is_dir ? '📁' : '📄';
                li.innerText = `${icon} ${model.name}`;
                if (!model.is_dir) {
                    li.addEventListener('click', () => {
                        // Deselect others
                        document.querySelectorAll('.file-tree li').forEach(el => el.style.background = '');
                        li.style.background = 'rgba(255,255,255,0.1)';
                        this.selectedModelPath = model.path;
                        Toast.show(`Selected model: ${model.name}`);
                    });
                }
                list.appendChild(li);
            });
        } catch (e) {
            Toast.show('Failed to fetch models', 'error');
        }
    },

    async toggleInference() {
        const btn = document.getElementById('toggle-inference');
        
        if (this.isRunning) {
            try {
                await ApiClient.post('/inference/stop');
                this.isRunning = false;
                btn.innerText = 'Start Inference';
                btn.className = 'btn btn-primary btn-block mt-4';
                Toast.show('Inference stopped');
            } catch (e) {
                Toast.show('Failed to stop inference', 'error');
            }
        } else {
            if (!this.selectedModelPath) {
                Toast.show('Please select a model first', 'warning');
                return;
            }
            try {
                btn.innerText = 'Loading...';
                await ApiClient.post('/inference/start', { model_path: this.selectedModelPath });
                this.isRunning = true;
                btn.innerText = 'Stop Inference';
                btn.className = 'btn btn-secondary btn-block mt-4';
                Toast.show('Inference started', 'success');
            } catch (e) {
                btn.innerText = 'Start Inference';
                Toast.show('Failed to start inference', 'error');
            }
        }
    },

    async checkStatus() {
        try {
            const status = await ApiClient.get('/inference/status');
            this.isRunning = status.running;
            
            const btn = document.getElementById('toggle-inference');
            if (this.isRunning) {
                btn.innerText = 'Stop Inference';
                btn.className = 'btn btn-secondary btn-block mt-4';
            } else {
                btn.innerText = 'Start Inference';
                btn.className = 'btn btn-primary btn-block mt-4';
            }
        } catch (e) {
            console.error('Failed to get inference status', e);
        }
    }
};

export default InferenceModule;
