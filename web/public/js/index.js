var config;
var sensors;
var sensorgroups;
var weather;
var controllers;

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
    getControllers();
    getSensorGroups();

    // weather is dependent on location
    var interval = setInterval(function() {
        if ( config == undefined || config.location == undefined) {
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
            $(".navbar-brand").notify(data.text, "error");
        }
    }).error(function(data) {
        $(".navbar-brand").notify("failed to fetch config from api","error");
    })
}

function getSensors() {
    $.getJSON("/api/sensors", function (data) {
        if (data.result) {
            sensors = data.payload;
            console.log(sensors);
        } else {
            $(".navbar-brand").notify(data.text, "error");
        }
    }).error(function(data) {
        $(".navbar-brand").notify("failed to fetch sensors from api","error");
    });
}

function getControllers() {
    $.getJSON("/api/controllers", function (data) {
        if (data.result) {
            controllers = data.payload;
            console.log(controllers);
        } else {
            $(".navbar-brand").notify(data.text, "error");
        }
    }).error(function(data) {
        $(".navbar-brand").notify("failed to fetch controllers from api","error");
    });
}

function getSensorGroups() {
    $.getJSON("/api/sensorgroups", function (data) {
        if (data.result) {
            sensorgroups = data.payload;
            if ($("#sensorgroup-nav-list") != undefined) {
                if($(".sensorgroup.active > a")[0] != undefined) {
                    populateSensorGroups($(".sensorgroup.active > a")[0].id);
                } else {
                    populateSensorGroups();
                }
            }
        } else {
            $(".navbar-brand").notify(data.text, "error");
        }
    }).error(function(data) {
        $(".navbar-brand").notify("failed to fetch sensor groups from api","error");
    });
}

function getWeather() {
    $.getJSON("/api/weather", function(data) {
        if(data.result) {
            weather = JSON.parse(data.payload);
            populateWeather();
        } else {
            $(".navbar-brand").notify(data.text, "error");
        }
    }).error(function(data) {
        $(".navbar-brand").notify("failed to fetch weather from api","error");
    });
}



