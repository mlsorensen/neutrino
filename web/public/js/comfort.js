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

    // sensorgroups
    if (event.target.id == 'comfort-sensorgroups-div') {
        //sensorgroup nav bar
        var interval = setInterval(function() {
            if (sensorgroups == undefined) {
                return;
            }
            clearInterval(interval);
            populateSensorGroups();
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
        url: "/api/sensors/" + sensor[0].id.replace("sensor ","") + "/data?value=" + datatype + "&hours=" + config.graphtime.value,
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
            xaxes: [{mode:"time", timezone: "browser", twelveHourClock: true}],
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
        var $row = "<li class='sensor'><a id='sensor " + sensors[i].sensor_address + "' href='#'>sensor " + sensors[i].sensor_address + "</a></li>";
        if (sensors[i].display_name != null) { 
            var $row = "<li class='sensor'><a id='sensor " + sensors[i].sensor_address + "' href='#'>" + sensors[i].display_name + "</a></li>";
        }
        $("#sensor-nav-list").append($row);
    }

    // populate info on click
    $(".nav-stacked > .sensor > a").on("click", function(event) {
        $(".nav-stacked > .sensor").removeClass('active');
        $(this).parent().addClass('active');
        renderSensorGraph($(this), "temperature", 6, $("#sensor-graph"));
        $("#sensor-name-input").val($(this).html());
        $("#sensor-panel-title").html(event.target.id);
        renderVoltage($(this));
    });

    $(".nav-stacked > .sensor > a").first().trigger("click");

    // save sensor name edit
    $("#sensor-name-save").on("click", function(event) {
        var sensorid = $(".active.sensor > a")[0].id.replace("sensor ","")
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

function populateSensorGroups(activesensorid) {
    $("#sensorgroup-nav-list").html("");
    var sensornavrow;
    for (var key in sensorgroups) {
        if (activesensorid != undefined && activesensorid == "sensorgroup " + sensorgroups[key].id) {
            sensornavrow = "<li class='sensorgroup active'><a id='sensorgroup "+ sensorgroups[key].id + "' href='#'>" + sensorgroups[key].display_name + "</a></li>";
        } else {
            sensornavrow = "<li class='sensorgroup'><a id='sensorgroup "+ sensorgroups[key].id + "' href='#'>" + sensorgroups[key].display_name + "</a></li>";
        }
        $("#sensorgroup-nav-list").append(sensornavrow);
    }

    // add  a + sign to sensorgroup nav
    sensornavrow = "<li class=''><a id='addsensorgroup' class='glyphicon glyphicon-plus label-info'" + 
                   " style='color:white;text-align:center' href='#'></a></li>";
    $("#sensorgroup-nav-list").append(sensornavrow);

    $("#addsensorgroup").on("click", function() {
        $("#comfort-sensor_groups").trigger("click");
    });

    // populate info on selected sensor group 
    $(".nav-stacked > .sensorgroup > a").on("click", function(event) {
        $(".nav-stacked > .sensorgroup").removeClass('active');
        $(this).parent().addClass('active');
        $("#sensorgroup-name-input").val($(this).html());

        // populate sensor select dropdown
        $("#sensorgroup-sensorselect-dropdown").removeClass('hidden');
        $("#sensorgroup-delete").removeClass('hidden');
        $("#sensorgroup-sensorselect-list").html('');
        var sensorgroupid = event.target.id.replace("sensorgroup ","");
        console.log($(this));
        for(i = 0; i < sensors.length; i++) {
            var li;
            var sensorid = sensors[i].id;
            if (sensorgroups[sensorgroupid] != undefined && sensorgroups[sensorgroupid].members[sensorid] != undefined) {
                li = '<li><label class="checkbox"><input id="sensorgroup-sensor ' + sensors[i].id +
                     '" type="checkbox" checked>' + sensors[i].display_name + '</label></li>';
            } else {
                li = '<li><label class="checkbox"><input id="sensorgroup-sensor ' + sensors[i].id +
                     '" type="checkbox">' + sensors[i].display_name + '</label></li>';
            }
            $("#sensorgroup-sensorselect-list").append(li);
        }

        // keep menu open when clicking checkboxes
        $('#sensorgroup-sensorselect-list').on('click', function(e) {
            e.stopPropagation();
        });

        // save sensors to sensorgroups
        $('#sensorgroup-sensorselect-list > li > label > input').on('change', function(e) {
            console.log($(this));
            var sensorid = $(this)[0].id.replace("sensorgroup-sensor ", "");
            var sensorgroupid = $(".sensorgroup.active > a")[0].id.replace("sensorgroup ", "");
            if ($(this).is(':checked')) {
                addSensorToGroup(sensorid, sensorgroupid);
                console.log("adding sensor id " + sensorid + " to sensorgroup id " + sensorgroupid);
            } else {
                removeSensorFromGroup(sensorid, sensorgroupid);
                console.log("removing sensor id " + sensorid + " from sensorgroup id " + sensorgroupid);
            }
            getSensorGroups();
        });
    });

    //delete group
    $('#sensorgroup-delete').on('click', function(e) {
        var sensorid = $(".sensorgroup.active > a")[0].id.replace("sensorgroup ","");
        $.ajax({url: "/api/sensorgroups/" + sensorid,
                type: 'DELETE',
                success: function(data) {
                    getSensorGroups();
                    $("#addsensorgroup").trigger("click");
                    //$(".nav-stacked > .sensorgroup.active > a")[0].remove();
                },
                error: function(data) {
                    $("#sensorgroup-delete").notify("error on deleting group", "error");
                }
        });
    });

    // save new sensor group or save name
    $("#sensorgroup-name-save").on("click", function(event) {
        var newname = $("#sensorgroup-name-input").val();
        // if current context is existing sensor group, rename it. Otherwise, create new sensor group
        if ($(".sensorgroup.active > a").length != 0) {
            var sensorgroupid = $(".sensorgroup.active > a")[0].id.replace("sensorgroup ","");
            var newname = $("#sensorgroup-name-input").val();
            $.post("/api/sensorgroups/" + sensorgroupid + "/name", {"value": newname}, function(data) {
                $("#sensorgroup-name-save").notify(data.text, "info");
                $(".active.sensorgroup > a").html(newname);
                getSensorGroups();
                
            }).error(function(data) {
                $("#sensorgroup-name-save").notify("error on saving name", "error");
            });
        } else {
            $.ajax({url: "/api/sensorgroups",
                    type: 'PUT',
                    data: {"value": newname},
                    success: function(data) {
                        $("#sensorgroup-name-save").notify(data.text, "info");
                        $(".active.sensorgroup > a").html(newname);
                        getSensorGroups();
                    },
                    error: function(data) {
                        $("#sensorgroup-name-save").notify("error on saving newgroup" + $.parseJSON(data.responseText).text, "error");
                    }
            });
        }
    });
}

function addSensorToGroup(sensorid, sensorgroupid) {
    $.ajax({url: "/api/sensorgroups/" + sensorgroupid + "/sensors/" + sensorid,
            type: "PUT",
            async: false,
            success: function() { $("#sensorgroup-sensorselect-list").notify("added", "info")},
            error: function() { $("#sensorgroup-sensorselect-list").notify("failure to add", "error")}
            });
}

function removeSensorFromGroup(sensorid, sensorgroupid) {
    $.ajax({url:     "/api/sensorgroups/" + sensorgroupid + "/sensors/" + sensorid,
            type:    "DELETE",
            async:   false,
            success: function() {$("#sensorgroup-sensorselect-list").notify("removed", "info")},
            error:   function() {$("#sensorgroup-sensorselect-list").notify("failure to remove", "error")}
           });
}

function populateWeather() {
    if ($("#home-location-value") == undefined) {
        return;
    }
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
