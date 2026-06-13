const RecorderModule = {
    init() {
        this.bindEvents();
        this.fetchRecordings();
    },

    formatDuration(totalSeconds) {
        if (typeof totalSeconds !== 'number' || isNaN(totalSeconds)) return '00:00:00';
        const h = Math.floor(totalSeconds / 3600);
        const m = Math.floor((totalSeconds % 3600) / 60);
        const s = Math.floor(totalSeconds % 60);
        return [h, m, s].map(v => v.toString().padStart(2, '0')).join(':');
    },

    bindEvents() {
        document.getElementById('start-recording').addEventListener('click', () => this.startRecording());
        document.getElementById('stop-recording').addEventListener('click', () => this.stopRecording());
        const refreshBtn = document.getElementById('refresh-recordings');
        if (refreshBtn) refreshBtn.addEventListener('click', () => this.fetchRecordings());

        // Bulk action events
        const selectAll = document.getElementById('select-all-recordings');
        if (selectAll) selectAll.addEventListener('change', (e) => this.toggleAllRecordings(e.target.checked));
        
        const btnDownloadSelected = document.getElementById('download-selected');
        if (btnDownloadSelected) btnDownloadSelected.addEventListener('click', () => this.downloadSelected());
        
        const btnDeleteSelected = document.getElementById('delete-selected');
        if (btnDeleteSelected) btnDeleteSelected.addEventListener('click', () => this.deleteSelected());
    },

    async fetchRecordings() {
        try {
            const recordings = await ApiClient.get('/recording/list');
            const list = document.getElementById('recordings-list');
            list.innerHTML = '';
            
            if (recordings.length === 0) {
                list.innerHTML = '<li class="text-muted">No recordings found.</li>';
                return;
            }

            recordings.forEach(rec => {
                const li = document.createElement('li');
                li.className = 'flex-row justify-between mb-2 p-2 glass-card';
                
                const duration = this.formatDuration(rec.duration_sec);
                const size = typeof rec.size_mb === 'number' ? rec.size_mb.toFixed(2) : rec.size_mb;
                
                li.innerHTML = `
                    <div class="flex-row align-center" style="gap: 1rem;">
                        <input type="checkbox" class="rec-checkbox" value="${rec.filename}">
                        <div>
                            <strong>${rec.filename}</strong><br>
                            <span class="text-muted">${duration} | ${size} MB</span>
                        </div>
                    </div>
                    <div class="flex-row">
                        <button class="btn btn-secondary btn-sm download-btn" data-file="${rec.filename}">Download</button>
                        <button class="btn btn-secondary btn-sm delete-btn" data-file="${rec.filename}" style="color: var(--danger);">Delete</button>
                    </div>
                `;
                list.appendChild(li);
            });

            // Update bulk action buttons when checkboxes change
            document.querySelectorAll('.rec-checkbox').forEach(cb => {
                cb.addEventListener('change', () => this.updateBulkActionState());
            });
            this.updateBulkActionState();

            // Bind dynamic buttons
            document.querySelectorAll('.download-btn').forEach(btn => {
                btn.addEventListener('click', (e) => {
                    const file = e.target.getAttribute('data-file');
                    const token = localStorage.getItem('yolo_token');
                    window.open('/api/recording/download/' + encodeURIComponent(file) + '?token=' + token, '_blank');
                });
            });

            document.querySelectorAll('.delete-btn').forEach(btn => {
                btn.addEventListener('click', async (e) => {
                    if (confirm('Are you sure you want to delete this recording?')) {
                        const file = e.target.getAttribute('data-file');
                        try {
                            await ApiClient.post('/recording/delete', { filename: file });
                            Toast.show('Deleted recording', 'success');
                            this.fetchRecordings();
                        } catch (err) {
                            Toast.show('Failed to delete recording', 'error');
                        }
                    }
                });
            });
        } catch (e) {
            Toast.show('Failed to fetch recordings', 'error');
        }
    },

    async startRecording() {
        try {
            await ApiClient.post('/recording/start');
            
            document.getElementById('start-recording').disabled = true;
            document.getElementById('stop-recording').disabled = false;
            
            const recTag = document.getElementById('recording-tag');
            if (recTag) recTag.classList.remove('hidden');
        } catch (e) {
            if (e.message.includes('Already recording')) {
                document.getElementById('start-recording').disabled = true;
                document.getElementById('stop-recording').disabled = false;
                const recTag = document.getElementById('recording-tag');
                if (recTag) recTag.classList.remove('hidden');
            } else {
                Toast.show('Failed to start recording', 'error');
            }
        }
    },

    async stopRecording() {
        try {
            await ApiClient.post('/recording/stop');
            
            document.getElementById('start-recording').disabled = false;
            document.getElementById('stop-recording').disabled = true;
            
            const recTag = document.getElementById('recording-tag');
            if (recTag) recTag.classList.add('hidden');
            
            this.fetchRecordings();
        } catch (e) {
            if (e.message.includes('Not recording')) {
                document.getElementById('start-recording').disabled = false;
                document.getElementById('stop-recording').disabled = true;
                const recTag = document.getElementById('recording-tag');
                if (recTag) recTag.classList.add('hidden');
            } else {
                Toast.show('Failed to stop recording', 'error');
            }
        }
    },

    toggleAllRecordings(checked) {
        document.querySelectorAll('.rec-checkbox').forEach(cb => {
            cb.checked = checked;
        });
        this.updateBulkActionState();
    },

    updateBulkActionState() {
        const anyChecked = Array.from(document.querySelectorAll('.rec-checkbox')).some(cb => cb.checked);
        const btnDownload = document.getElementById('download-selected');
        const btnDelete = document.getElementById('delete-selected');
        if (btnDownload) btnDownload.disabled = !anyChecked;
        if (btnDelete) btnDelete.disabled = !anyChecked;
    },

    downloadSelected() {
        const selected = Array.from(document.querySelectorAll('.rec-checkbox'))
            .filter(cb => cb.checked)
            .map(cb => cb.value);
            
        if (selected.length === 0) return;
        
        const token = localStorage.getItem('yolo_token');
        selected.forEach((file, index) => {
            setTimeout(() => {
                window.open('/api/recording/download/' + encodeURIComponent(file) + '?token=' + token, '_blank');
            }, index * 500); // Stagger downloads slightly
        });
    },

    async deleteSelected() {
        const selected = Array.from(document.querySelectorAll('.rec-checkbox'))
            .filter(cb => cb.checked)
            .map(cb => cb.value);
            
        if (selected.length === 0) return;
        
        if (confirm(`Are you sure you want to delete ${selected.length} recordings?`)) {
            let successCount = 0;
            for (const file of selected) {
                try {
                    await ApiClient.post('/recording/delete', { filename: file });
                    successCount++;
                } catch (err) {
                    console.error('Failed to delete', file, err);
                }
            }
            if (successCount === selected.length) {
                Toast.show(`Deleted ${successCount} recordings`, 'success');
            } else {
                Toast.show(`Deleted ${successCount} of ${selected.length} recordings`, 'warning');
            }
            
            const selectAll = document.getElementById('select-all-recordings');
            if (selectAll) selectAll.checked = false;
            
            this.fetchRecordings();
        }
    }
};

export default RecorderModule;
