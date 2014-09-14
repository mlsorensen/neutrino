///////////////////
// form controls //
///////////////////

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

function componentToHex(c) {
    var hex = c.toString(16);
    return hex.length == 1 ? "0" + hex : hex;
}

function rgbToHex(r, g, b) {
    return "#" + componentToHex(r) + componentToHex(g) + componentToHex(b);
}

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

        $("#sensor-delete").on("click", function(event) {
            $("#sensor-delete").notify({title:"Are you sure?", button:''}, {style:'confirm-popup', autoHide: false, clickToHide: false});
            var sensoraddr = $(".active.sensor > a")[0].id;
            $(document).on('click', '.confirm-no', function() {
                $(this).trigger('notify-hide');
            });
            $(document).on('click', '.confirm-yes', function() {
                deleteSensor(sensoraddr);
                $(this).trigger('notify-hide');
            });
        }).hover(function(){
            $("#sensor-delete").css("background-color","red");
        }, function(){
            $("#sensor-delete").css("background-color","#8F2400");
        });
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

        // save new sensor group or save name
        $("#sensorgroup-name-save").on("click", function(event) {
            var newname = $("#sensorgroup-name-input").val();
            // if current context is existing sensor group, rename it. Otherwise, create new sensor group
            if ($(".sensorgroup.active > a").length != 0) {
                var sensorgroupid = $(".sensorgroup.active > a")[0].id;
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
                            getSensorGroups();
                        },
                        error: function(data) {
                            $("#sensorgroup-name-save").notify("error on saving newgroup" + $.parseJSON(data.responseText).text, "error");
                        }
                });
            }
        });

        //delete sensorgroup button
        $('#sensorgroup-delete').on('click', function(e) {
            var sensorid = $(".sensorgroup.active > a")[0].id;
            $.ajax({url: "/api/sensorgroups/" + sensorid,
                    type: 'DELETE',
                    success: function(data) {
                        $(".sensorgroup.active > a")[0].remove();
                        getSensorGroups();
                    },
                    error: function(data) {
                        $("#sensorgroup-delete").notify($.parseJSON(data.responseText).text, "error");
                    }
            });
        });
    } 
});

function deleteSensor(sensorid) {
    $.ajax({url:     "/api/sensors/" + sensorid,
            type:    "DELETE",
            async:   false,
            success: function() {
                delete sensors[sensorid];
                $("#sensor-nav-list > li > a#" + sensorid).remove();    
                $(".nav-stacked > .sensor > a").first().trigger("click");
            },
            error:   function() {$("#sensor-delete").notify("failed to delete", "error", {autoHideDelay:1000})}
    });
}

function renderVoltage(sensor) {
    $.getJSON("/api/sensors/" + sensor[0].id + "/data?value=voltage&hours=1", function (data) {
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
        url: "/api/sensors/" + sensor + "/data?value=" + datatype + "&hours=" + config.graphtime.value,
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

function sensorIdsFromGroup(sensorgroupid) {
    if (Object.keys(sensorgroups[sensorgroupid].members).length == 0) {
        console.log("no sensors attached to group");
        return;
    }

    var ids = [];
    console.log(sensorgroups[sensorgroupid]);

    Object.keys(sensorgroups[sensorgroupid].members).forEach(function(member) {
        ids.push(sensorgroups[sensorgroupid].members[member].sensor_address);
    });
    return ids;
}

function populateSensors() {
    for (var key in sensors) {
        var $row = "<li class='sensor'><a id='" + key + "' href='#'>sensor " + key + "</a></li>";
        if (sensors[key].display_name != null) { 
            var $row = "<li class='sensor'><a id='" + key + "' href='#'>" + sensors[key].display_name + "</a></li>";
        }
        $("#sensor-nav-list").append($row);
    }

    // populate info on click
    $(".nav-stacked > .sensor > a").on("click", function(event) {
        clickedsensor = this;
        $(".nav-stacked > .sensor").removeClass('active');
        $(this).parent().addClass('active');
        sensorGraph([$(this)[0].id], ["Fahrenheit"], ["00AA33"],config.graphtime.value, null, null, $("#sensor-graph-upper"));
        sensorGraph([$(this)[0].id], ["Humidity"], ["AA0033"],config.graphtime.value, null, null, $("#sensor-graph-lower"));
        $("#sensor-name-input").val($(this).html());
        $("#sensor-address-input").val(sensors[$(this)[0].id].sensor_address);
        renderVoltage($(this));
    });

    $(".nav-stacked > .sensor > a").first().trigger("click");

    // save sensor name edit
    $("#sensor-name-save").on("click", function(event) {
        var sensorid = $(".active.sensor > a")[0].id;
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

function populateSensorGroups(activesensorgroupid) {
    $("#sensorgroup-nav-list").html("");
    var sensornavrow;
    for (var key in sensorgroups) {
        sensornavrow = "<li class='sensorgroup'><a id='"+ sensorgroups[key].id + "' href='#'>" + sensorgroups[key].display_name + "</a></li>";
        $("#sensorgroup-nav-list").append(sensornavrow);
    }

    // add  a + sign to sensorgroup nav
    sensornavrow = "<li class=''><a id='addsensorgroup' class='glyphicon glyphicon-plus label-info'" + 
                   " style='color:white;text-align:center' href='#'></a></li>";
    $("#sensorgroup-nav-list").append(sensornavrow);

    // empty out forms for new sensorgroup registration
    $("#addsensorgroup").on("click", function() {
        if ($(".active.sensorgroup") != undefined) {
            $(".active.sensorgroup").removeClass("active");
        }
        $("#sensorgroup-name-input").val('');
        $("#sensorgroup-controllerselect-list").html('');
        $("#sensorgroup-sensorselect-list").html('');
        $("#sensorgroup-controllerselect-dropdown").addClass('hidden');
        $("#sensorgroup-sensorselect-dropdown").addClass('hidden');
        $("#sensorgroup-controllerbar").addClass('hidden');
        $("#sensorgroup-graph").addClass('hidden');
    });

    // populate info on selected sensor group 
    $(".nav-stacked > .sensorgroup > a").on("click", function(event) {
        var sensorgroupid = event.target.id;
        clickedsensorgroup = this;
        var controllerid = sensorgroups[sensorgroupid].controller_id;
        var controller = controllers[controllerid];
        $(".nav-stacked > .sensorgroup").removeClass('active');
        $(this).parent().addClass('active');
        $("#sensorgroup-name-input").val($(this).html());

        if (controller != undefined) {
            console.log("Controller:");
            console.log(controller);
            $("#sensorgroup-controllerbar").removeClass('hidden');
            $("#controllerbar-name").html(controller.display_name + "&nbsp:");
            $("#controllerbar-mintemp").html(controller.capabilities.heat.setpoint + " ");
            $("#controllerbar-maxtemp").html(controller.capabilities.cool.setpoint + " ");
            $("#controllerbar-humidity").html(controller.capabilities.humidify.setpoint + " ");
            $("#controllerbar-fanmode").html(controller.fan_mode + " ");
            if (controller.status == "idle") {
                $("#controllerbar-status").html("<b style='color:green'>&nbsp idle</b>");
            } else if (controller.status == "heating") {
                $("#controllerbar-status").html("<b style='color:red'>&nbsp heating</b>");
            } else if (controller.status == "cooling") {
                $("#controllerbar-status").html("<b style='color:blue'>&nbsp cooling</b>");
            } else if (controller.status == "humidifying") {
                $("#controllerbar-status").html("<b style='color:dodgerblue'>&nbsp humidifying</b>");
            }
            if (controller.enabled == "on") {
                $("#controllerbar-enable").html("on ");
            } else if (controller.enabled == "off") {
                $("#controllerbar-enable").html("off ");
            } else {
                $("#controllerbar-enable").html("? ");
            }
            $("#controllerbar-mintemptable").removeClass("hidden");
            $("#controllerbar-maxtemptable").removeClass("hidden");
            $("#controllerbar-humiditytable").removeClass("hidden");
            $("#controllerbar-fanmodetable").removeClass("hidden");
            $("#controllerbar-enabledropdown").removeClass("hidden");

            // populate mintemp options
            $("#controllerbar-mintemp-list").html('');
            for (var i = 50; i < 90; i++) {
                $("#controllerbar-mintemp-list").append("<li><a class='controllerbar-mintemp-item' href='#'>" + i + "</a></li>");
            }
            $("#controllerbar-mintemp-list > li > a").on("click", function(e) {
                controllerSaveSetting(controller.id, $("#controllerbar-mintemp"), $(this).html(), "heat");
            });

            // populate maxtemp options
            $("#controllerbar-maxtemp-list").html('');
            for (var i = 50; i <= 90; i++) {
                $("#controllerbar-maxtemp-list").append("<li><a class='controllerbar-maxtemp-item' href='#'>" + i + "</a></li>");
            }
            $("#controllerbar-maxtemp-list > li > a").on("click", function(e) {
                controllerSaveSetting(controller.id, $("#controllerbar-maxtemp"), $(this).html(), "cool");
            });

             // populate humidify options
            $("#controllerbar-humidity-list").html('');
            for (var i = 0; i <= 50; i++) {
                $("#controllerbar-humidity-list").append("<li><a class='controllerbar-humidity-item' href='#'>" + i + "</a></li>");
            }
            $("#controllerbar-humidity-list > li > a").on("click", function(e) {
                controllerSaveSetting(controller.id, $("#controllerbar-humidity"), $(this).html(), "humidify");
            });

            // populate fanmode options
            $("#controllerbar-fanmode-list").html('');
            $("#controllerbar-fanmode-list").append("<li><a class='controllerbar-fanmode-item' href='#'>auto</a></li>");
            $("#controllerbar-fanmode-list").append("<li><a class='controllerbar-fanmode-item' href='#'>on</a></li>");
            $("#controllerbar-fanmode-list > li > a").on("click", function(e) {
                controllerSaveSetting(controller.id, $("#controllerbar-fanmode"), $(this).html(), "fan_mode");
            });

            // populate controller on/off options
            $("#controllerbar-enable-list").html('');
            $("#controllerbar-enable-list").append("<li><a class='controllerbar-enable-item' href='#'>on</a></li>");
            $("#controllerbar-enable-list").append("<li><a class='controllerbar-enable-item' href='#'>off</a></li>");
            $("#controllerbar-enable-list > li > a").on("click", function(e) {
                controllerSaveSetting(controller.id, $("#controllerbar-enable"), $(this).html(), "enabled");
            });
        } else {
            $("#sensorgroup-controllerbar").addClass("hidden");
        }

        // populate controller select dropdown
        $("#sensorgroup-controllerselect-dropdown").removeClass('hidden');
        //$("#sensorgroup-controllerbar").removeClass('hidden');
        $("#sensorgroup-controllerselect-list").html('');
        $("#sensorgroup-controllerselect-list").append('<li><a id="0" class="sensorgroup-controllerlist" href="#">none</a></li>');
        for(var key in controllers) {
            li = '<li><a id="' + key + '" class="sensorgroup-controllerlist" href="#">' + 
                  controllers[key].display_name + '</a></li>'
            $("#sensorgroup-controllerselect-list").append(li);
        }

        // populate sensor select dropdown
        $("#sensorgroup-sensorselect-dropdown").removeClass('hidden');
        $("#sensorgroup-delete").removeClass('hidden');
        $("#sensorgroup-sensorselect-list").html('');
        var sensorgroupid = event.target.id;
        for(var key in sensors) {
            var li;
            if (sensorgroups[sensorgroupid] != undefined && sensorgroups[sensorgroupid].members[key] != undefined) {
                li = '<li><label class="checkbox"><input id="' + key +
                     '" type="checkbox" checked>' + sensors[key].display_name + '</label></li>';
            } else {
                li = '<li><label class="checkbox"><input id="' + key +
                     '" type="checkbox">' + sensors[key].display_name + '</label></li>';
            }
            $("#sensorgroup-sensorselect-list").append(li);
        }

        // save controller on change
        $('#sensorgroup-controllerselect-list > li > a').on('click', function(e) {
             var sensorgroupid = $(".sensorgroup.active > a")[0].id;
             assignControllerToSensorGroup(sensorgroupid, $(this)[0].id);
        });

        // save sensors to sensorgroups
        $('#sensorgroup-sensorselect-list > li > label > input').on('change', function(e) {
            var sensorid = $(this)[0].id;
            var sensorgroupid = $(".sensorgroup.active > a")[0].id;
            if ($(this).is(':checked')) {
                addSensorToGroup(sensorid, sensorgroupid);
            } else {
                removeSensorFromGroup(sensorid, sensorgroupid);
            }
            getSensorGroups();
        });

        // keep menu open when clicking checkboxes
        $('#sensorgroup-sensorselect-list').on('click', function(e) {
            e.stopPropagation();
        });

        // render group graph
        if (controller !== undefined) {
            sensorGraph(sensorIdsFromGroup(sensorgroupid), ["Fahrenheit"], ["00AA33"],config.graphtime.value, null, null, $("#sensorgroup-graph"), controller);
        } else {
            sensorGraph(sensorIdsFromGroup(sensorgroupid), ["Fahrenheit"], ["00AA33"],config.graphtime.value, null, null, $("#sensorgroup-graph"));
        }
    });

    // select a group to show by default
    if (activesensorgroupid != undefined) {
       $(".sensorgroup > a#" + activesensorgroupid).trigger("click");
    } else {
       $(".sensorgroup").children().last().trigger("click"); 
    }
}

function controllerSaveSetting(controllerid, targetdiv, settingvalue, settingname) {
    $.ajax({url: "/api/controllers/" + controllerid + "/" + settingname,
            type: "POST",
            data: {"value":settingvalue},
            async: false,
            success: function() {
                targetdiv.html(settingvalue + " ");
                targetdiv.notify("success","info", {autoHideDelay:1000});
                if ( settingname == "heat" || settingname == "cool" || settingname == "humidify") {
                    controllers[controllerid].capabilities[settingname].setpoint = settingvalue;
                }
                getControllers();
                sensorGraph(sensorIdsFromGroup($(".sensorgroup.active > a")[0].id),["Fahrenheit"], ["00AA33"],config.graphtime.value, null, null, $("#sensorgroup-graph"), controllers[controllerid]);
                return 1;
            },
            error: function() {
                targetdiv.notify("Failed to save","error", {autoHideDelay:1000});
                return 0;
            } 
    });
}

function assignControllerToSensorGroup(sensorgroupid, controllerid) {
    $.ajax({url: "/api/controllers/" + controllerid + "/sensorgroup",
            type: "POST",
            data: {"value":sensorgroupid},
            async: false,
            success: function() {
                if (controllerid == 0) {
                    $("#sensorgroup-controllerselect-dropdown").notify("removed", "info", {autoHideDelay:1000});
                    $("#sensorgroup-controllerbar").addClass('hidden')
                    sensorgroups[sensorgroupid].controller_id = undefined;
                } else {
                    $("#sensorgroup-controllerselect-dropdown").notify("added", "info", {autoHideDelay:1000});
                    sensorgroups[sensorgroupid].controller_id = controllerid;
                }
                $(".sensorgroup > a#" + sensorgroupid).trigger("click");
                getSensorGroups();
            },
            error: function(data) { 
                    $("#sensorgroup-controllerselect-dropdown").notify($.parseJSON(data.responseText).text, "error", {autoHideDelay:1000})
            }

    });
}

function addSensorToGroup(sensorid, sensorgroupid) {
    $.ajax({url: "/api/sensorgroups/" + sensorgroupid + "/sensors/" + sensorid,
            type: "PUT",
            async: false,
            success: function() { 
                $("#sensorgroup-sensorselect-dropdown").notify("added", "info", {autoHideDelay:1000}); 
                sensorgroups[sensorgroupid].members[sensorid] = 1; 
                sensorGraph(sensorIdsFromGroup(sensorgroupid),["Fahrenheit"], ["00AA33"],config.graphtime.value, null, null, $("#sensorgroup-graph"));
            },
            error: function() { $("#sensorgroup-sensorselect-dropdown").notify("failure to add", "error", {autoHideDelay:1000})}
    });
}

function removeSensorFromGroup(sensorid, sensorgroupid) {
    $.ajax({url:     "/api/sensorgroups/" + sensorgroupid + "/sensors/" + sensorid,
            type:    "DELETE",
            async:   false,
            success: function() {
                $("#sensorgroup-sensorselect-dropdown").notify("removed", "info", {autoHideDelay:1000});
                delete sensorgroups[sensorgroupid].members[sensorid];
                sensorGraph(sensorIdsFromGroup($(".sensorgroup.active > a")[0].id),["Fahrenheit"], ["00AA33"],config.graphtime.value, null, null, $("#sensorgroup-graph"));
            },
            error:   function() {$("#sensorgroup-sensorselect-dropdown").notify("failure to remove", "error", {autoHideDelay:1000})}
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
