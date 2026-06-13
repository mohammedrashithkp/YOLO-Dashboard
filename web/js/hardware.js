// Hardware Module
const HardwareModule = {
    init() {
        this.fetchHardware();
    },

    async fetchHardware() {
        try {
            const hw = await ApiClient.get('/hardware');
            const container = document.getElementById('hardware-cards');
            container.innerHTML = '';

            // CPU Card
            const cpuCard = document.createElement('div');
            cpuCard.className = 'glass-card';
            cpuCard.innerHTML = `
                <h3>CPU</h3>
                <div class="mt-4">
                    <p><strong>Model:</strong> ${hw.cpu.model}</p>
                    <p><strong>Architecture:</strong> ${hw.cpu.arch}</p>
                    <p><strong>Cores:</strong> ${hw.cpu.cores}</p>
                    <p class="mt-4"><strong>RAM Available:</strong> ${hw.memory.available_mb} MB / ${hw.memory.total_mb} MB</p>
                </div>
            `;
            container.appendChild(cpuCard);

            // NPU Card
            const npuCard = document.createElement('div');
            npuCard.className = 'glass-card';
            
            if (hw.npus && hw.npus.length > 0) {
                let npuHtml = '<h3>NPU Accelerators</h3><div class="mt-4">';
                hw.npus.forEach(npu => {
                    const status = npu.available ? '<span class="dot dot-green"></span> Online' : '<span class="dot dot-red"></span> Offline';
                    npuHtml += `
                        <div class="mb-4">
                            <p><strong>Name:</strong> ${npu.name}</p>
                            <p><strong>Performance:</strong> ${npu.tops} TOPS</p>
                            <p><strong>Status:</strong> ${status}</p>
                        </div>
                    `;
                });
                npuHtml += '</div>';
                npuCard.innerHTML = npuHtml;
            } else {
                npuCard.innerHTML = `
                    <h3>NPU Accelerators</h3>
                    <div class="mt-4 placeholder">No NPU detected</div>
                `;
            }
            container.appendChild(npuCard);

        } catch (e) {
            Toast.show('Failed to fetch hardware info', 'error');
        }
    }
};

export default HardwareModule;
