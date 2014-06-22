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
        // populate weather
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
});

function renderVoltage(sensor) {
    $.getJSON("/api/sensors/" + sensor[0].id.replace("sensor ","") + "/data?value=voltage&hours=1", function (data) {
        if(data.result) {
            var voltage = data.payload[data.payload.length-1].voltage;
            $("#sensor-voltage-value").html(voltage + " V");
            if (voltage > 2.5) {
                $("#sensor-voltage-span").addClass('label-success');
            } else if (voltage > 2.2) {
                $("#sensor-voltage-span").addClass('label-warning');
            } else {
                $("#sensor-voltage-span").addClass('label-danger');
            }
        } else {

        }
    });
}

function getSensorData(sensor, hours, datatype) {
    var graphdata = [];
    var data = JSON.parse($.ajax({
        type: "GET",
        url: "/api/sensors/" + sensor[0].id.replace("sensor","") + "/data?value=" + datatype + "&hours=" + config.graphtime.value,
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

function renderSensorGraph(sensor, value, hours, targetdiv) {
    var temperaturedata = getSensorData(sensor, hours, "temperature");
    var humiditydata    = getSensorData(sensor, hours, "humidity");
    var templabel = 'Degrees ' + config.tempunits.value;
    var humlabel = 'Humidity % ';
    if (temperaturedata.length > 0) {
        var dataset = [{data:humiditydata, label:humlabel, color:"green", lines:{show:true}, yaxis: 2},
                       {data:temperaturedata, label:templabel, color: "#0062E3", lines:{show:true}}];
        var options = {
            axisLabels: {show:true},
            series: {shadowSize:5, lines:{show:true},},
            xaxes: [{mode:"time", timezone: "browser"}],
            yaxes: [{alignTicksWithAxis:1, position:"right", axisLabel:templabel, autoscaleMargin:4},
                    {alignTicksWithAxis:1, position:"left", axisLabel:humlabel}],
            legend: {position:'sw'}
        };

        $.plot($("#sensor-graph"), dataset, options );
        $("#sensor-graph").css("background", "linear-gradient(to top, #ebebeb , white)");
    }
}



function populateSensors() {
    for (i = 0; i < sensors.length; i++) {
        var $row = "<li class='sensor'><a id='sensor" + sensors[i].sensor_address + "' href='#'>sensor " + sensors[i].sensor_address + "</a></li>";
        if (sensors[i].display_name != null) { 
            var $row = "<li class='sensor'><a id='sensor " + sensors[i].sensor_address + "' href='#'>" + sensors[i].display_name + "</a></li>";
        }
        $("#sensor-nav-list").append($row);
    }

    $(".nav-stacked > .sensor > a").on("click", function(event) {
        $(".nav-stacked > .sensor").removeClass('active');
        $(this).parent().addClass('active');
        renderSensorGraph($(this), "temperature", 6, $("#sensor-graph"));
        $("#sensor-name-input").val($(this).html());
        $("#sensor-panel-title").html(event.target.id);
        renderVoltage($(this));
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
        console.log(weather);
        console.log(weather.weather);
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
