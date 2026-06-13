// API Utility
class ApiClient {
    static get authHeaders() {
        const token = localStorage.getItem('yolo_token');
        return token ? { 'Authorization': `Bearer ${token}` } : {};
    }

    static async request(endpoint, options = {}) {
        const url = `/api${endpoint}`;
        const headers = { ...this.authHeaders, ...options.headers };
        if (options.body && !(options.body instanceof FormData)) {
            headers['Content-Type'] = 'application/json';
            options.body = JSON.stringify(options.body);
        }

        try {
            const response = await fetch(url, { ...options, headers });
            if (response.status === 401) {
                app.logout();
                throw new Error("Unauthorized");
            }
            
            // Check content type to see if it's JSON or Raw (like YAML)
            const contentType = response.headers.get("content-type");
            let data;
            if (contentType && contentType.indexOf("application/json") !== -1) {
                data = await response.json();
            } else {
                data = await response.text();
            }

            if (!response.ok) {
                throw new Error(data.error || `HTTP error! status: ${response.status}`);
            }
            return data;
        } catch (error) {
            console.error(`API Error (${endpoint}):`, error);
            throw error;
        }
    }

    static async get(endpoint) { return this.request(endpoint, { method: 'GET' }); }
    static async post(endpoint, data) { return this.request(endpoint, { method: 'POST', body: data }); }
}

// Toast System
class Toast {
    static show(message, type = 'info') {
        const container = document.getElementById('toast-container');
        const toast = document.createElement('div');
        toast.className = `toast toast-${type}`;
        toast.innerText = message;
        container.appendChild(toast);
        setTimeout(() => {
            toast.style.opacity = '0';
            setTimeout(() => toast.remove(), 300);
        }, 3000);
    }
}

// Main App Controller
const app = {
    init() {
        this.bindEvents();
        this.checkAuth();
        
        // Handle initial routing
        window.addEventListener('hashchange', () => this.handleRoute());
    },

    bindEvents() {
        document.getElementById('login-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.login();
        });

        document.getElementById('logout-btn').addEventListener('click', () => {
            ApiClient.post('/auth/logout').finally(() => this.logout());
        });

        // Navigation clicks
        document.querySelectorAll('.nav-links a').forEach(link => {
            link.addEventListener('click', (e) => {
                document.querySelectorAll('.nav-links a').forEach(l => l.classList.remove('active'));
                e.currentTarget.classList.add('active');
            });
        });
    },

    checkAuth() {
        const token = localStorage.getItem('yolo_token');
        if (token) {
            this.showApp();
        } else {
            this.showLogin();
        }
    },

    async login() {
        const user = document.getElementById('username').value;
        const pass = document.getElementById('password').value;
        
        try {
            const res = await ApiClient.post('/auth/login', { username: user, password: pass });
            localStorage.setItem('yolo_token', res.token);
            Toast.show('Login successful', 'success');
            this.showApp();
        } catch (e) {
            Toast.show(e.message, 'error');
        }
    },

    logout() {
        localStorage.removeItem('yolo_token');
        this.showLogin();
    },

    showLogin() {
        document.getElementById('login-overlay').classList.add('active');
        document.getElementById('app-container').classList.add('hidden');
    },

    showApp() {
        document.getElementById('login-overlay').classList.remove('active');
        document.getElementById('app-container').classList.remove('hidden');
        this.handleRoute();
        this.loadInitialData();
    },

    handleRoute() {
        const hash = window.location.hash || '#camera';
        const sectionId = hash.substring(1) + '-section';
        
        document.querySelectorAll('.content-section').forEach(sec => sec.classList.add('hidden'));
        
        const targetSection = document.getElementById(sectionId);
        if (targetSection) {
            targetSection.classList.remove('hidden');
            document.querySelector(`[data-section="${hash.substring(1)}"]`)?.classList.add('active');
            
            // Refresh data dynamically when navigating
            if (hash === '#hardware' && window.HardwareModule) {
                window.HardwareModule.fetchHardware();
            } else if (hash === '#model' && window.InferenceModule) {
                window.InferenceModule.fetchModels();
            } else if (hash === '#config' && window.ConfigModule) {
                window.ConfigModule.loadConfig();
            }
        } else {
            // Default to camera
            document.getElementById('camera-section').classList.remove('hidden');
            document.querySelector('[data-section="camera"]')?.classList.add('active');
        }
    },

    async loadInitialData() {
        if (this.initialized) return;
        this.initialized = true;
        // Init modules
        import('./camera.js').then(m => { window.CameraModule = m.default; m.default.init(); });
        import('./hardware.js').then(m => { window.HardwareModule = m.default; m.default.init(); });
        import('./config.js').then(m => { window.ConfigModule = m.default; m.default.init(); });
        import('./inference.js').then(m => { window.InferenceModule = m.default; m.default.init(); });
        import('./recorder.js').then(m => { window.RecorderModule = m.default; m.default.init(); });
    }
};

window.ApiClient = ApiClient;
window.Toast = Toast;
window.app = app;

document.addEventListener('DOMContentLoaded', () => app.init());
