// Hardware Module
const HardwareModule = {
    init() {
        this.fetchHardware();
    },

    async fetchHardware() {
        const container = document.getElementById('hardware-cards');
        if (!container) return;
        
        // Show loading state
        container.innerHTML = '<div class="glass-card"><p class="text-muted">Loading hardware info...</p></div>';
        
        try {
            const hw = await ApiClient.get('/hardware');
            container.innerHTML = '';

            // CPU Card
            const cpuCard = document.createElement('div');
            cpuCard.className = 'glass-card';
            const cpuModel = (hw.cpu && hw.cpu.model) || 'Unknown';
            const cpuArch = (hw.cpu && hw.cpu.arch) || 'Unknown';
            const cpuCores = (hw.cpu && hw.cpu.cores) || 'N/A';
            const memTotal = (hw.memory && hw.memory.total_mb) || 'N/A';
            const memAvail = (hw.memory && hw.memory.available_mb) || 'N/A';
            
            cpuCard.innerHTML = `
                <h3>🖥️ CPU</h3>
                <div class="mt-4">
                    <p><strong>Model:</strong> ${cpuModel}</p>
                    <p><strong>Architecture:</strong> ${cpuArch}</p>
                    <p><strong>Cores:</strong> ${cpuCores}</p>
                    <p class="mt-4"><strong>RAM:</strong> ${memAvail} MB available / ${memTotal} MB total</p>
                </div>
            `;
            container.appendChild(cpuCard);

            // NPU Card
            const npuCard = document.createElement('div');
            npuCard.className = 'glass-card';
            
            if (hw.npus && hw.npus.length > 0) {
                let npuHtml = '<h3>⚡ NPU Accelerators</h3><div class="mt-4">';
                hw.npus.forEach(npu => {
                    const status = npu.available ? '<span class="dot dot-green"></span> Online' : '<span class="dot dot-red"></span> Offline';
                    npuHtml += `
                        <div class="mb-4">
                            <p><strong>Name:</strong> ${npu.name}</p>
                            <p><strong>Performance:</strong> ${npu.tops || 'N/A'} TOPS</p>
                            <p><strong>Status:</strong> ${status}</p>
                        </div>
                    `;
                });
                npuHtml += '</div>';
                npuCard.innerHTML = npuHtml;
            } else {
                npuCard.innerHTML = `
                    <h3>⚡ NPU Accelerators</h3>
                    <div class="mt-4 text-muted">No NPU detected on this device.</div>
                `;
            }
            container.appendChild(npuCard);

        } catch (e) {
            console.error('Hardware fetch failed:', e);
            container.innerHTML = `
                <div class="glass-card">
                    <h3>⚠️ Hardware Info Unavailable</h3>
                    <p class="text-muted mt-4">Could not retrieve hardware information. This may happen if the backend is still starting up.</p>
                    <button class="btn btn-secondary mt-4" onclick="window.HardwareModule && window.HardwareModule.fetchHardware()">Retry</button>
                </div>
            `;
        }
    }
};

export default HardwareModule;
