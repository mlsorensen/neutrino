///////////////////
// form controls //
///////////////////

// populate page
$(document).on('DOMNodeInserted', function(event) {

    $(".nav > .dropdown").on("click", function(event) {
        $(".nav > .dropdown").removeClass('active');
        $( this ).addClass('active');
    });

    // control intervals for periodic updates
    var weatherinterval = -1;

    if (event.target.id == 'comfort-home-div') {
        var interval = setInterval(function() {
            if (config.location.value == undefined) {
                return;
            }
            clearInterval(interval);
            populateWeather();
            getWeather();
            weatherinterval = setInterval(function(){ populateWeather(); getWeather(); }, 300000);
        }, 200);
        // don't refresh if we click away
        $("#comfort-home-div").on('remove', function () {
            clearInterval(weatherinterval);
            weatherinterval = -1;
        });
    }

    // sensors

    if (event.target.id == 'comfort-sensors-div') {
        //sensors nav bar
        var interval = setInterval(function() {
            if (sensors == undefined) {
                return;
            }
            clearInterval(interval);
            populateSensors();
        }, 200);
    }

/* graph proof of concept
        $.getJSON("/api/sensors/0/data?value=temperature&hours=72", function (data) {
            if(data.result) {
                var graphdata = [];
                for (i = 0; i < data.payload.length; i++) {
                    var element = [];
                    element[0] = data.payload[i].epoch * 1000;
                    element[1] = data.payload[i].temperature;
                    graphdata[i] = element;
                } 
                var dataset = [{data :graphdata, label:'sensor0',color: "#0062E3",lines:{show:true},curvedLines: {apply:true}}];
                var options = { series: {shadowSize:5, curvedLines:{active:true}}, xaxes:[{mode:"time"}], yaxis:{axisLabel:"Temperature"}};
                $.plot($("#sensor1-graph"), dataset, options );
            }
        });
*/
});

function renderSensorGraph(sensor, value, hours, targetdiv) {
    $.getJSON("/api/sensors/" + sensor[0].id.replace("sensor","") + "/data?value=" + value + "&hours=" + hours, function (data) {
        if(data.result) {
            var graphdata = [];
            for (i = 0; i < data.payload.length; i++) {
                var element = [];
                element[0] = data.payload[i].epoch * 1000;
                element[1] = data.payload[i].temperature;
                graphdata[i] = element;
            }
            var dataset = [{data :graphdata, label:sensor.html(), color: "#0062E3", lines:{show:true}, curvedLines: {apply:false}}];
            var options = { series: {shadowSize:5, curvedLines:{active:true}}, xaxes:[{mode:"time"}], yaxis:{axisLabel:"Temperature"}};
            $.plot($("#sensor-graph"), dataset, options );
            $("#sensor-graph").css("background", "linear-gradient(to top, #ebebeb , white)");
        }
    });
}



function populateSensors() {
    for (i = 0; i < sensors.length; i++) {
        var $row = "<li class='sensor'><a id='sensor" + sensors[i].sensor_address + "' href='#'>sensor " + sensors[i].sensor_address + "</a></li>";
        if(sensors[i].display_name != null) { 
            var $row = "<li class='sensor'><a id='sensor" + sensors[i].sensor_address + "' href='#'>" + sensors[i].display_name + "</a></li>";
        }
        $("#sensor-nav-list").append($row);
    }

    $(".nav-stacked > .sensor > a").on("click", function(event) {
        $(".nav-stacked > .sensor").removeClass('active');
        $(this).parent().addClass('active');
        renderSensorGraph($(this), "temperature", 12, $("#sensor-graph"));
        $("#sensor-name-input").val($(this).html());
    });

    // save sensor name edit
    $("#sensor-name-save").on("click", function(event) {
        var sensorid = $(".active.sensor > a")[0].id.replace("sensor","")
        var newname = $("#sensor-name-input").val();
        $.post("/api/sensors/" + sensorid + "/name", {"value": newname}, function(data) {
            $("#sensor-name-save").notify(data.text, "info");
            $(".active.sensor > a").html(newname);
            getSensors();
        }).error(function(data) {
            $("#sensor-name-save").notify("error on saving name:" + $.parseJSON(data.responseText).text, "error");
        });
    });
}

function populateWeather() {
    $("#home-location-value").html(config.location.value);
    if (weather != undefined) {
        $("#home-weather-value").html(weather.weather[0].description +
                                      "<img src='http://openweathermap.org/img/w/" + weather.weather[0].icon + ".png' />");
        if (config.tempunits.value == 'Celsius') {
            $("#home-temperature-value").html(((weather.main.temp-273.15)).toFixed(2) + " C");
        } else {
            $("#home-temperature-value").html(((weather.main.temp-273.15) * 1.8 + 32).toFixed(2) + " F");
        }
        $("#home-humidity-value").html(weather.main.humidity + "%");
    }
}
