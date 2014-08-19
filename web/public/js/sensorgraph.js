////`///////////////////////////
// graph api for sensor data //
///////////////////////////////

/*
var colors = {
    blue  : [],
    red   : [],
    green : []
};

for (i=1; i <= 5; i++) {
    var o1  = (i * 40);
    var o3  = 200 + (i * 10)

    colors.blue.push(rgbToHex(o1, 200-o1, o3));
    colors.blue.push(rgbToHex(200-o1, o1, o3));

    colors.red.push(rgbToHex(o3, 200-o1, o1));
    colors.red.push(rgbToHex(o3, o1, 200-o1));
    
    colors.green.push(rgbToHex(200-o1, o3, o1));
    colors.green.push(rgbToHex(o1, o3, 200-o1));
}
*/

function componentToHex(c) {
    var hex = c.toString(16);
    return hex.length == 1 ? "0" + hex : hex;
}

function rgbToHex(r, g, b) {
    return "#" + componentToHex(r) + componentToHex(g) + componentToHex(b);
}

function hexToRgb(hex) {
    var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16)
    } : null;
}

// this code is for testing
$(document).ready(function(){
    $('#test-button1').click(function() {
        // graph( sensorids, axes, time, graphx, graphy, divid);
        var sensorids = [1];
        var axes = ['fahrenheit','humidity'];
        sensorGraph(sensorids, axes, 1, 400, 150, $('#graphdiv'));
    });
});

/*
sensorids = array of sensor ids
axes      = array of labels for axes, needs to be one of valid data types Fahrenheit, Celsius, Humidity, Voltage
colors    = array of colors for axes, if each axis has multiple lines we should morph the selected color for that axis
time      = integer number of hours to graph
graphx    = graph should be x pixels wide, null inherits width from targetdiv
graphy    = graph should be y pixels tall, null inherits height from targetdiv
targetdiv = location to render graph
*/

function sensorGraph( sensorids, axes, colors, time, graphx, graphy, targetdiv) {
    var sensordata = [];
    var dataset = [];

    if(axes.length > 2 || axes.length <= 0) {
        // we only support two axes
        targetdiv.html("only one or two axes, please");
        return;
    }

    if (graphx == null) {
        graphx = targetdiv.width();
    }

    if (graphy == null) {
        graphy = targetdiv.height();
    }
    
    for( i = 0; i < sensorids.length; i++) {
        var sensorid = sensorids[i];
        for ( j = 0; j < axes.length; j++) {
            console.log("fetching data " + axes[j]+ " for sensor" + sensorid);
            dataset.push({data: getSensorData(sensorid, time, axes[j]), 
                          label: axes[j],
                          color: chooseColor(colors[j], sensorid),
                          lines: {show:true},
                          yaxis: j + 1
                         });
        }
    }

    var yaxesdata = [];
    yaxesdata.push({alignTicksWithAxis:1, position:"right", axisLabel:axes[0], autoscaleMargin:.3 });
    if (axes[1]) {
        yaxesdata.push({alignTicksWithAxis:1, position:"left", axisLabel:axes[1], autoscaleMargin:.3});
    }

    var options = {
        axisLabels: {show:true},
        series: {shadowSize:5, lines:{show:true},},
        xaxes: [{mode:"time", timezone: "browser", twelveHourClock: true}],
        yaxes: yaxesdata,
        legend: {position:'sw'}
    };

    console.log(dataset);
    console.log(options);

    targetdiv.css("width", graphx + "px");
    targetdiv.css("height", graphy + "px");
    targetdiv.css("background", "linear-gradient(to top, #ebebeb , white)");
    $.plot(targetdiv, dataset, options);
}

function getSensorData(sensor, hours, datatype) {
    var graphdata = [];
    var data = JSON.parse($.ajax({
        type: "GET",
        url: "/api/sensors/" + sensor + "/data?value=" + datatype + "&hours=" + hours,
        async: false
    }).responseText);

    if (data.result) {
        for (i = 0; i < data.payload.length; i++) {
            var element = [];
            element[0] = data.payload[i].epoch * 1000;
            element[1] = parseFloat(data.payload[i][datatype]);
            graphdata[i] = element;
        }
    }
    return graphdata;
}

function chooseColor(startcolor, id) {
    var rgb = hexToRgb(startcolor);
    var multiplier = id * 20;
    return rgbToHex(rgb.r + multiplier, rgb.g + multiplier, rgb.b + multiplier);
}


