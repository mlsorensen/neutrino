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
    $.getJSON("http://api.openweathermap.org/data/2.5/weather?q=" + config.location.value, function (data) {
        if (typeof data.main != "undefined") {
            weather = data;
        } else {
            //alert
        }
    });
}



