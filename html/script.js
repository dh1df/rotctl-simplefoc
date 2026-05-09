class Maidenhead {
    /**
     * Maidenhead Locator zu Lat/Lon
     * Unterstützt 2, 4, 6, 8 characters (z.B. JN, JN48, JN48BD, JN48BD55)
     */
    static locatorToLatLon(locator) {
        locator = locator.trim().toUpperCase();

        // Validation: Nur gerade Anzahl characters, max 8, min 2
        if (locator.length < 2 || locator.length > 8 || locator.length % 2 !== 0) {
            throw new Error('Ungültiger Locator: ' + locator.length + ' characters. Allowed: 2,4,6,8');
        }

        if (!/^[A-R]{2}([0-9]{2}([A-X]{2}([0-9]{2})?)?)?$/.test(locator)) {
            throw new Error('Ungültiges Locator-Format');
        }

        // Feld (erste 2 Buchstaben): 20° x 10°
        let lon = (locator.charCodeAt(0) - 65) * 20 - 180;
        let lat = (locator.charCodeAt(1) - 65) * 10 - 90;

        if (locator.length >= 4) {
            // Quadrat (nächste 2 Ziffern): 2° x 1°
            lon += parseInt(locator[2]) * 2;
            lat += parseInt(locator[3]) * 1;
        }

        if (locator.length >= 6) {
            // Sub-Quadrat (nächste 2 Buchstaben): 5' x 2.5' = 0.08333° x 0.041667°
            lon += (locator.charCodeAt(4) - 65) * (5 / 60);
            lat += (locator.charCodeAt(5) - 65) * (2.5 / 60);
        }

        if (locator.length >= 8) {
            // Sub-Sub-Quadrat (letzte 2 Ziffern): 0.5' x 0.25' = 0.008333° x 0.004167°
            lon += parseInt(locator[6]) * (0.5 / 60);
            lat += parseInt(locator[7]) * (0.25 / 60);
        }

        // Mittelpunkt des Feldes (nicht Ecke)
        if (locator.length === 2) { lon += 10; lat += 5; }
        else if (locator.length === 4) { lon += 1; lat += 0.5; }
        else if (locator.length === 6) { lon += (2.5 / 60); lat += (1.25 / 60); }
        else if (locator.length === 8) { lon += (0.25 / 60); lat += (0.125 / 60); }

        return { lat, lon };
    }

    /**
     * Berechne Azimuth zwischen zwei Punkten
     */
    static bearing(fromLat, fromLon, toLat, toLon) {
        const φ1 = fromLat * Math.PI / 180;
        const φ2 = toLat * Math.PI / 180;
        const Δλ = (toLon - fromLon) * Math.PI / 180;

        const y = Math.sin(Δλ) * Math.cos(φ2);
        const x = Math.cos(φ1) * Math.sin(φ2) -
                  Math.sin(φ1) * Math.cos(φ2) * Math.cos(Δλ);

        let θ = Math.atan2(y, x) * 180 / Math.PI;
        return (θ + 360) % 360; // 0-360°
    }
}

class AzimuthControl {
    constructor() {
        this.svgDoc = null;
        this.currentAzimuth = 0;
        this.beamwidth = 45;
        this.connected = false;
        this.rotatorInfo = {
            model: 'Default',
            minAz: 0,
            maxAz: 360,
        };
        this.centerX = 297;
        this.centerY = 421;
        this.radius = 255;
	    this.loc = Maidenhead.locatorToLatLon("JO60om");

        // Zoom and pan properties
        this.zoomLevel = 1;
        this.panX = 0;
        this.panY = 0;
        this.isPanning = false;
        this.panStartX = 0;
        this.panStartY = 0;

        this.init();
    }

    async init() {
        this.bindElements();
        this.attachEventListeners();
        await this.loadSVG();
        this.attachZoomPanHandlers();
        this.startPolling();
    }

    bindElements() {
        this.mapObject = document.getElementById('azimuth-map');
        this.statusIndicator = document.getElementById('status-indicator');
        this.statusText = document.getElementById('status-text');
        this.currentAzimuthSpan = document.getElementById('current-azimuth');
        this.manualInput = document.getElementById('manual-azimuth');
        this.setBtn = document.getElementById('set-azimuth-btn');
        this.rotatorModel = document.getElementById('rotator-model');
        this.minAzSpan = document.getElementById('min-az');
        this.maxAzSpan = document.getElementById('max-az');
    }

    attachEventListeners() {
        this.setBtn.addEventListener('click', () => this.setManualAzimuth());
        this.manualInput.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') this.setManualAzimuth();
        });
    }

    async loadSVG() {
        return new Promise((resolve) => {
            this.mapObject.addEventListener('load', () => {
                this.svgDoc = this.mapObject.contentDocument;
                this.createSector();
                this.createLine();
                this.updateDisplay();
                this.attachSVGClickHandler();
                resolve();
            });
        });
    }

    createSector() {
        if (!this.svgDoc) return;

        // Entferne alten Sektor falls vorhanden
        const oldSector = this.svgDoc.getElementById('antenna-sector');
        if (oldSector) oldSector.remove();

        // Create neuen Sektor
        const sector = this.svgDoc.createElementNS('http://www.w3.org/2000/svg', 'path');
        sector.setAttribute('id', 'antenna-sector');
        sector.setAttribute('class', 'antenna-sector');
        sector.setAttribute('fill', '#3fff00');
        sector.setAttribute('opacity', '0.2');

        this.svgDoc.getElementsByTagName('svg')[0].appendChild(sector);
    }

    createLine() {
        if (!this.svgDoc) return;

        // Entferne alte Linie falls vorhanden
        const oldLine = this.svgDoc.getElementById('direction-line');
        if (oldLine) oldLine.remove();
        const line = this.svgDoc.createElementNS('http://www.w3.org/2000/svg', 'line');
        line.setAttribute('id', 'direction-line');
        line.setAttribute('x1', this.centerX);
        line.setAttribute('y1', this.centerY);
        line.setAttribute('x2', this.centerX);
        line.setAttribute('y2', this.centerY);
        line.setAttribute('stroke', '#ff3300');
        line.setAttribute('stroke-width', '3');
        line.setAttribute('stroke-dasharray', '8,4');
        // line.setAttribute('marker-end', 'url(#arrowhead)');
        // line.setAttribute('pointer-events', 'none');
        this.svgDoc.getElementsByTagName('svg')[0].appendChild(line);
    }

    attachSVGClickHandler() {
        if (!this.svgDoc) return;

        const svg = this.svgDoc.documentElement;
        svg.addEventListener('click', (e) => {
            const rect = svg.getBoundingClientRect();
            const x = e.clientX; //  - rect.left;
            const y = e.clientY; //  - rect.top;

            // Berechne Angle relativ zum Mittelpunkt
            const dx = x - this.centerX;
            const dy = this.centerY - y; // Umgekehrt für korrekte Himmelsrichtung

            let angle = Math.atan2(dx, dy) * 180 / Math.PI;
            if (angle < 0) angle += 360;

            // Runde auf ganze degrees
            angle = Math.round(angle);
	        // console.log("click", dx, dy, angle, rect, e.clientX, e.clientY, this.centerX, this.centerY);

            this.setAzimuth(angle);
        });
    }

    attachZoomPanHandlers() {
        if (!this.svgDoc) return;

        const svg = this.svgDoc.documentElement;
        const svgElement = this.mapObject;

        // Mouse wheel zoom
        svgElement.addEventListener('wheel', (e) => {
            e.preventDefault();
            const zoomSpeed = 0.1;
            const delta = e.deltaY > 0 ? -zoomSpeed : zoomSpeed;
            this.zoomLevel = Math.max(0.5, Math.min(5, this.zoomLevel + delta));
            this.updateTransform();
        });

        // Pan on mouse drag - right-click or middle-click
        svgElement.addEventListener('mousedown', (e) => {
            if (e.button !== 2 && e.button !== 1) return; // Right-click (2) or middle-click (1)
            e.preventDefault();
            this.isPanning = true;
            this.panStartX = e.clientX - this.panX;
            this.panStartY = e.clientY - this.panY;
        });

        document.addEventListener('mousemove', (e) => {
            if (!this.isPanning) return;
            this.panX = e.clientX - this.panStartX;
            this.panY = e.clientY - this.panStartY;
            this.updateTransform();
        });

        document.addEventListener('mouseup', () => {
            this.isPanning = false;
        });

        // Touch support: pinch to zoom and drag to pan
        let touchStartDistance = 0;
        let touchStartPan = { x: 0, y: 0 };

        svgElement.addEventListener('touchstart', (e) => {
            if (e.touches.length === 2) {
                // Pinch zoom
                const touch1 = e.touches[0];
                const touch2 = e.touches[1];
                const dx = touch2.clientX - touch1.clientX;
                const dy = touch2.clientY - touch1.clientY;
                touchStartDistance = Math.sqrt(dx * dx + dy * dy);
            } else if (e.touches.length === 1) {
                // Single touch pan
                this.isPanning = true;
                touchStartPan = { x: e.touches[0].clientX - this.panX, y: e.touches[0].clientY - this.panY };
            }
        });

        svgElement.addEventListener('touchmove', (e) => {
            e.preventDefault();

            if (e.touches.length === 2) {
                // Pinch zoom
                const touch1 = e.touches[0];
                const touch2 = e.touches[1];
                const dx = touch2.clientX - touch1.clientX;
                const dy = touch2.clientY - touch1.clientY;
                const currentDistance = Math.sqrt(dx * dx + dy * dy);
                const zoomDelta = (currentDistance - touchStartDistance) * 0.01;
                this.zoomLevel = Math.max(0.5, Math.min(5, this.zoomLevel + zoomDelta));
                touchStartDistance = currentDistance;
                this.updateTransform();
            } else if (e.touches.length === 1 && this.isPanning) {
                // Single touch pan
                this.panX = e.touches[0].clientX - touchStartPan.x;
                this.panY = e.touches[0].clientY - touchStartPan.y;
                this.updateTransform();
            }
        });

        svgElement.addEventListener('touchend', () => {
            this.isPanning = false;
            touchStartDistance = 0;
        });
    }

    updateTransform() {
        if (!this.svgDoc) return;
        const svg = this.svgDoc.documentElement;
        svg.style.transform = `translate(${this.panX}px, ${this.panY}px) scale(${this.zoomLevel})`;
        svg.style.transformOrigin = 'center';
        svg.style.transition = 'none';
    }

    updateSector() {
        if (!this.svgDoc) return;

        const sector = this.svgDoc.getElementById('antenna-sector');
        if (!sector) return;

        // Update Linie
        const angleRad = this.currentAzimuth * Math.PI / 180;
        const length = 190; // Länge der Linie
        const endX = this.centerX + length * Math.sin(angleRad);
        const endY = this.centerY - length * Math.cos(angleRad);

        // line.setAttribute('x2', endX);
        // line.setAttribute('y2', endY);

        // Update Sektor
        const startAngle = this.currentAzimuth - this.beamwidth/2;
        const endAngle = this.currentAzimuth + this.beamwidth/2;

        const startRad = startAngle * Math.PI / 180;
        const endRad = endAngle * Math.PI / 180;


        const startX = this.centerX + this.radius * Math.sin(startRad);
        const startY = this.centerY - this.radius * Math.cos(startRad);
        const endX2 = this.centerX + this.radius * Math.sin(endRad);
        const endY2 = this.centerY - this.radius * Math.cos(endRad);

        const largeArc = (endAngle - startAngle) > 180 ? 1 : 0;

        const pathData = [
            `M ${this.centerX} ${this.centerY}`,
            `L ${startX} ${startY}`,
            `A ${this.radius} ${this.radius} 0 ${largeArc} 1 ${endX2} ${endY2}`,
            'Z'
        ].join(' ');

        sector.setAttribute('d', pathData);

        // Update Text
        const azimuthText = this.svgDoc.getElementById('azimuth-text');
        if (azimuthText) {
            azimuthText.textContent = this.currentAzimuth.toFixed(1) + '°';
        }
    }

    updateLine(azimuth) {
        const line = this.svgDoc.getElementById('direction-line');

        if (!line) return;
        const azimuthRad = azimuth * Math.PI / 180;
        line.setAttribute('x2', this.centerX + this.radius * Math.sin(azimuthRad));
        line.setAttribute('y2', this.centerY - this.radius * Math.cos(azimuthRad));
    }

    updateDisplay() {
        this.currentAzimuthSpan.textContent = this.currentAzimuth.toFixed(1) + '°';
        // this.manualInput.value = this.currentAzimuth;
        this.updateSector();
    }

    async setAzimuth(azimuth) {
        // Stelle sicher, dass der Wert im gültigen Range liegt
        azimuth = Math.max(0, Math.min(359, azimuth));

        if (this.currentAzimuth === azimuth) return;

        // this.currentAzimuth = azimuth;
        // this.updateDisplay();
        this.manualInput.value = azimuth;
        this.updateLine(azimuth);

        // Send to rotator
        await this.sendToRotator(azimuth);
    }

    async setManualAzimuth() {
        const val=this.manualInput.value.toUpperCase();
        
        if (/^[A-R]{2}([0-9]{2}([A-X]{2}([0-9]{2})?)?)?$/.test(val)) {
	        const dst = Maidenhead.locatorToLatLon(val);
            // console.log("Maidenhead2", val, this.loc, dst);
            // console.log("Maidenhead3", this.loc.lat, this.loc.lon);
    	    // console.log("Maidenhead4", Maidenhead.bearing(this.loc.lat, this.loc.lon, dst.lat, dst.lon));
    	    await this.setAzimuth(Math.round(Maidenhead.bearing(this.loc.lat, this.loc.lon, dst.lat, dst.lon)));
        } else {
       	    const azimuth = parseInt(val);
            if (!isNaN(azimuth)) {
                await this.setAzimuth(azimuth);
            }
        }
    }

    async sendToRotator(azimuth) {
        try {
            const response = await fetch('http://rotor:8080/api/position', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    azimuth: azimuth,
                    elevation: 0,
                })
            });

            const data = await response.json();
            if (!data.success) {
                console.error('Error sending to rotator:', data.error);
                this.setStatus('error', 'Error sending');
            }
        } catch (error) {
            console.error('Network error:', error);
            this.setStatus('disconnected', 'Connection error (send)');
        }
    }

    async pollRotator() {
        try {
            const response = await fetch('http://rotor:8080/api/position');
            const data = await response.json();
            if (data.success) {
                this.connected = true;
                this.setStatus('connected', 'Connected');
                if (data.azimuth !== undefined) {
                    this.currentAzimuth = data.azimuth;
                    this.updateDisplay();
                }
                /*
                // Update Rotator Info
                if (data.info) {
                    this.rotatorInfo = { ...this.rotatorInfo, ...data.info };
                    this.updateRotatorInfo();
                }
				*/
            } else {
                this.connected = false;
                this.setStatus('disconnected', 'Disconnected');
            }
        } catch (error) {
            console.error('Polling Error:', error);
            this.connected = false;
            this.setStatus('disconnected', 'Connection error (poll)');
        }
    }

    setStatus(state, text) {
        this.statusIndicator.className = 'status-indicator ' + state;
        this.statusText.textContent = text;
    }

    updateRotatorInfo() {
        this.rotatorModel.textContent = this.rotatorInfo.model;
        this.minAzSpan.textContent = this.rotatorInfo.minAz + '°';
        this.maxAzSpan.textContent = this.rotatorInfo.maxAz + '°';
    }

    startPolling() {
        this.pollRotator();
        setInterval(() => this.pollRotator(), 1000); // Poll alle 1 Sekunde
    }
}

// Initialization
document.addEventListener('DOMContentLoaded', () => {
    new AzimuthControl();
});
