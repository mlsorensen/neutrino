var config;
var sensors;
var weather;

$(document).ready(function(){
    ////////////////////
    // main nav links //
    ////////////////////

    $(".nav > .dropdown").on("click", function(event) {
        $(".nav > .dropdown").removeClass('active');
        $( this ).addClass('active');
    });

    $(".nav > .dropdown > ul > li > a").on("click", function(event) {
        $.ajax({
            url: event.target.id.replace("-","/") + ".html",
            success: function(data) {
                $("#content-box").html(data);
            }
        });
    });
    
    getConfig();
    getSensors();

    // weather is dependent on location
    var interval = setInterval(function() {
        if (config.location.value == undefined) {
            return;
        }
        getWeather();
        clearInterval(interval);
    },100);
});

function getConfig() {
    $.getJSON("/api/config", function (data) {
        if(data.result) {
            config = data.payload;
        } else {
            //alert server error
        }
    });
}

function getSensors() {
    $.getJSON("/api/sensors", function (data) {
        if (data.result) {
            sensors = data.payload;
        } else {
            //need a place to publish notifications
        }
    });
}

function getWeather() {
    $.getJSON("/api/weather", function(data) {
        if(data.result) {
            weather = data.payload;
            $(".navbar-brand").notify("updated weather info","success");
        } else {
            $(".navbar-brand").notify(data.text, "error");
        }
    }).error(function(data) {
        $(".navbar-brand").notify("failed to fetch weather from api","error");
    });
/*
    $.getJSON("http://api.openweathermap.org/data/2.5/weather?q=" + config.location.value, function (data) {
        if (typeof data.main != "undefined") {
            weather = data;
            $(".navbar-brand").notify("updated weather info","success");
        } else {
            //alert
            
        }
    }).error(function() {
        $(".navbar-brand").notify("failed to update weather info","error");
    });
*/
}



