// Load the http module to create an http server.
var http = require('http');
var path = require('path');
var lazy = require('lazy');
var fs = require('fs');
var mapnik = require('mapnik');


// Configure our HTTP server to respond with Hello World to all requests.
var server = http.createServer(function (request, response) {
    /*
    new lazy(fs.createReadStream('test.txt'))
        .lines.forEach(function(line){
            console.log(line.toString());
            //$$A1,15254,15:36:34,52.145255,000.542061,00118,0000,03,3F4D3F2F,45*62
            var regexp = new RegExp(/^\$\$(.*?)\,(.*?)\,(\d{2}\:\d{2}\:\d{2})\,(.*?)\,(.*?)\,(.*?)\,(.*?)\,(.*?)\,(.*?)\,(.*?)\*(\d.*)$/);
            //var matches = line.match(regexp);
            var matches = line.toString().match(regexp);
            for(var x=1;x<matches.length;x++){
                console.log(x + ". " + matches[x]);
            }
        }
    );
    response.writeHead(200, {"Content-Type": "text/plain"});
    response.end("Hello World\n");

    */
    mapnik.register_default_fonts();
    if (mapnik.register_default_input_plugins) mapnik.register_default_input_plugins();

    //response.writeHead(200, {"Content-Type": 'image/png'});

    var map = new mapnik.Map(1000, 1000);
    map.load('stylesheet.xml', function(err,map) {
        if (err) throw err;
        map.zoomAll();
        var im = new mapnik.Image(1000, 1000);
        im.background = new mapnik.Color('white');
        map.render(im, function(err,im) {
            if (err) throw err;
            im.encode('png', function(err,buffer) {
                if (err) throw err;
                fs.writeFile('map.png',buffer, function(err) {
                    if (err) throw err;
                    console.log('saved map image to map.png');
                });
            });
        });
    });

});

// Listen on port 3000, IP defaults to 127.0.0.1
server.listen(3000);

// Put a friendly message on the terminal
console.log("Server running at http://127.0.0.1:3000/");