var config;
var sensors;
var sensorgroups;
var weather;
var controllers;
var clickedsensor;
var clickedsensorgroup;

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

    // refresh sensor graph if active
    var sensorgraphinterval = setInterval(function() {
        if (clickedsensor !== undefined && $(clickedsensor).is(":visible")) {
             console.log("triggering graph refresh for sensor");
             console.log(clickedsensor);
             $(clickedsensor).trigger("click");
        }
    }, 60000);

    // refresh sensor graph if active
    var sensorgroupgraphinterval = setInterval(function() {
        if (clickedsensorgroup !== undefined && $(clickedsensorgroup).is(":visible")) {
             console.log("triggering graph refresh for sensorgroup");
             console.log(clickedsensorgroup);
             $(clickedsensorgroup).trigger("click");
        }
    }, 60000);

    // add notification styles
    $.notify.addStyle('confirm-popup', {
    html: 
    "<div>" +
        "<div class='clearfix' style='background-color:#ebebeb;border-radius:10px;padding:5px'>" +
            "<div style='width:150px;height:40px' class='confirm-popup-title' data-notify-html='title'/>" +
            "<div class='confirm-popup-buttons'>" +
                "<button class='confirm-no' style='padding:5px' >No</button>" +
                "<button class='confirm-yes' style='padding:5px' >Yes</button>" +
            "</div>" +
        "</div>" +
    "</div>"
    });

    $("a#comfort-home").trigger("click");
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
                    console.log("rendering sensor groups with active group id " + $(".sensorgroup.active > a")[0].id);
                    populateSensorGroups($(".sensorgroup.active > a")[0].id);
                } else {
                    var lastkey = 0;
                    for (var key in sensorgroups) {
                        if (key > lastkey) {
                            lastkey = key;
                        }
                    }
                    console.log("rendering sensor groups, no active id found, using id " + key);
                    populateSensorGroups(key);
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



