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
            await ApiClient.post('/config', yaml);
            Toast.show('Configuration saved', 'success');
        } catch (e) {
            Toast.show('Failed to save configuration', 'error');
        }
    }
};

export default ConfigModule;
