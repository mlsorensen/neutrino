///////////////////
// form controls //
///////////////////

function getTempUnits() {
    $.getJSON("/api/config/tempunits", function(data) {
        if (data.result) {
            model.tempunits(data.payload.value);
        } else {
            // server-side failure or improper format
            $("#comfort-dropdown").notify("error on fetching temperature preference:" + $.parseJSON(data.responseText).text, "error");
        }
    }).error(function(data) {
        $("#comfort-dropdown").notify("error on fetching temperature preference:" + $.parseJSON(data.responseText).text, "error");
    });
}

function getLocation() {
    $.getJSON("/api/config/location", function (data) {
        if (data.result) {
            model.locale(data.payload.value);
        } else {
            // server-side failure or improper format
            $("#comfort-dropdown").notify("error on fetching location data:" + $.parseJSON(data.responseText).text, "error");
        }
    }).error(function(data) { 
        $("#comfort-dropdown").notify("error on fetching location data:" + $.parseJSON(data.responseText).text, "error");
    });
}

// populate page
$(document).on('DOMNodeInserted', function(event) {
    if (event.target.id == 'comfort-home-div') {
        $("#home-location-value").html(model.locale());
        $.getJSON("http://api.openweathermap.org/data/2.5/weather?q=" + model.locale(), function (weather) {
            if (typeof weather.main != "undefined") {
                $("#home-weather-value").html(weather.weather[0].description +
                                              "<img src='http://openweathermap.org/img/w/" + weather.weather[0].icon + ".png' />");
                if (model.tempunits() == 'Celsius') {
                    $("#home-temperature-value").html(((weather.main.temp-273.15)).toFixed(2) + " C");
                } else {
                    $("#home-temperature-value").html(((weather.main.temp-273.15) * 1.8 + 32).toFixed(2) + " F");
                }
                    $("#home-humidity-value").html(weather.main.humidity + "%");
                }
        });
    }

    // sensors

    if (event.target.id == 'comfort-sensors-div') {
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
    }
});
