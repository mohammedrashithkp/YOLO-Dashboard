// Config Module
const ConfigModule = {
    init() {
        this.bindEvents();
        this.loadConfig();
    },

    bindEvents() {
        document.getElementById('save-config').addEventListener('click', () => this.saveConfig());
        document.getElementById('reload-config').addEventListener('click', () => this.loadConfig());
    },

    async loadConfig() {
        try {
            // Get raw YAML
            const res = await fetch('/api/config', { headers: ApiClient.authHeaders });
            if (!res.ok) throw new Error("Failed to load config");
            const yaml = await res.text();
            
            document.getElementById('config-editor').value = yaml;
        } catch (e) {
            Toast.show('Failed to load configuration', 'error');
        }
    },

    async saveConfig() {
        const yaml = document.getElementById('config-editor').value;
        try {
            // Send raw YAML text with correct content type (not JSON)
            const res = await fetch('/api/config', {
                method: 'POST',
                headers: {
                    ...ApiClient.authHeaders,
                    'Content-Type': 'application/x-yaml'
                },
                body: yaml
            });
            if (!res.ok) {
                const data = await res.json().catch(() => ({}));
                throw new Error(data.error || 'Failed to save');
            }
            Toast.show('Configuration saved successfully', 'success');
        } catch (e) {
            Toast.show('Failed to save configuration: ' + e.message, 'error');
        }
    }
};

export default ConfigModule;
