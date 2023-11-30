function generateNearbyCoordinates(center, radiusInMiles, numberofDevices) {
    // Convert latitude and longitude to radians
    let lat = center[0]*(Math.PI/180);
    let lon = center[1]*(Math.PI/180);
    let devices_coordinates = [];

    for (let i = 0; i < numberofDevices; i++) {
        let radiusInKm = radiusInMiles * 1.60934;       // Convert radius from miles to kilometers
        let radiusInRadians = radiusInKm / 6371;        // Convert radius to radians
        let bearing = Math.random() * 2 * Math.PI;      // Generate a random bearing (direction)

        // Calculate new devices_coordinates
        let newLat = Math.asin(Math.sin(lat) * Math.cos(radiusInRadians) + Math.cos(lat) * Math.sin(radiusInRadians) * Math.cos(bearing));
        let newLon = lon + Math.atan2(Math.sin(bearing) * Math.sin(radiusInRadians) * Math.cos(lat), Math.cos(radiusInRadians) - Math.sin(lat) * Math.sin(newLat));

        // Convert devices_coordinates from radians to degrees
        newLat = newLat * (180 / Math.PI);
        newLon = newLon * (180 / Math.PI);
        devices_coordinates.push([newLat, newLon]);
    }
    return devices_coordinates;
}


// map API https://leafletjs.com/reference.html#map-example
class LeafletMapObject {
    // let main_coordinate = [37.78425807817066, -122.40636030396368]; // San Francisco

    constructor(id = 'map', center = [41.11536086134376, -85.13907697741708], zoom = 17) {
        let mapOptions = {
            zoomControl: true, 
            dragging: true, 
            keyboard: false, 
        }

        this.center = center
        this.map = L.map(id, mapOptions).setView(center, zoom)
    }

    showMap(){
        this.urlTemplate = 'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png';
        this.baseLayer = L.tileLayer.offline(this.urlTemplate,{
            attribution: 'Map data {attribution.OpenStreetMap}',
            subdomains: 'abc',
            // minZoom: this.zoom,
            // maxZoom: this.zoom,
        }).addTo(this.map)

        this.addMapControl()
        var point = turf.point([-85.13907697741708, 41.11536086134376]);              
        var buffered = turf.buffer(point, 50, {units: 'meters'});
        var buffer = L.geoJSON(buffered);
        buffer.addTo(this.map); 
        
        //Zoom Function for this.map
        this.map.on('zoomend', () =>{
            this.map.setView(this.main_coordinate, this.map.getZoom())
        })
    }


    addMarkers() {
        this.devices_coordinates = generateNearbyCoordinates(this.center, 0.05, 5);

        //Label device Locations with each coordinate
        this.devices_coordinates.forEach((coordinate, index) => {
            L.marker(coordinate).addTo(this.map)
                .bindPopup(`Device ID${index + 1}`)
                .openPopup()
        });
    }

    addPoly(coordinates, fillColor = '#8E44AD', color = 'green') {
        var poly1 = L.polygon(coordinates, {
            color: color,
            fillColor: fillColor,
            fillOpacity: 0.7,
        }).addTo(this.map)

        // L.point(41.11709282130384, -85.14070916717945).addTo(this.map)
    }

    addCircle() {
        var circle = L.circle(this.center, {
            color: 'red',
            fillColor: '#16A085',
            fillOpacity: 0.5,
            radius: 500
        }).addTo(this.map)
    }

    addDots() {
        // Define a custom icon for the markers
        var dotIcon = L.divIcon({
            className: 'dot-icon',
            iconSize: [8, 8],  // Adjust the size of the dot
            iconAnchor: [4, 4], // Center the dot
        })
        
        let points = [
            {lat: 41.11571828118311, lng: -85.13588633749958, popup: 'marker1'},
            {lat: 41.1164728399848, lng: -85.135605643708, popup: 'marker2'}, 
            {lat: 41.11721297203876, lng: -85.13648600150891, popup: 'marker3'}, 
            {lat: 41.11738598871805, lng: -85.1380489555758, popup: 'marker4'}, 
            {lat: 41.11735234662168, lng: -85.13999467390394, popup: 'marker5'}, 
            {lat: 41.11722739011278, lng: -85.14102175800502, popup: 'marker6'}, 
        ]

        points.forEach((point) => {
            var marker = L.marker([point.lat, point.lng], { icon: dotIcon }).addTo(this.map)
        })
    }

    addMapControl(){
        // Add map control buttons to save tiles in area viewed
        const control = L.control.savetiles(this.baseLayer, {
            zoomlevels: [this.zoom], // optional zoomlevels to save, default current zoomlevel
            confirm(layer, successCallback) {
                // eslint-disable-next-line no-alert
                if (window.confirm(`Save ${layer._tilesforSave.length}`)) {
                    successCallback()
                }
            },
            confirmRemoval(layer, successCallback) {
                // eslint-disable-next-line no-alert
                if (window.confirm('Remove all the tiles?')) {
                    successCallback()
                }
            },
            saveText:
                '<i class="download-icon" aria-hidden="true" title="Save tiles"></i>',
            rmText: '<i class="trash-icon" aria-hidden="true"  title="Remove tiles"></i>',
        })
        control.addTo(this.map)
    }

    addDrawingTools(){
        // Add drawing tools to the map
        const drawnItems = L.featureGroup().addTo(this.map)

        L.control.layers({}, {'drawlayer': drawnItems}, {position: 'topleft', collapsed: false}).addTo(this.map)

        this.map.addControl( new L.Control.Draw({
            edit: {
                featureGroup: drawnItems,
                poly: {
                    allowIntersection: false,

                },
            },
            draw: {
                polygon: {
                    allowIntersection: false,
                    showArea: true,
                    shapeOptions: {
                        color: 'red'
                    }
                },
            }
        }));

        this.map.on(L.Draw.Event.CREATED, (event) => {
            const layer = event.layer
            let color = prompt("Enter the color for the shape:")
            if (color) {
                layer.setStyle({
                    color: color
                });
            }
            drawnItems.addLayer(layer)
        })

        
    }
}

