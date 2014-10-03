function populateSensorHomeNav(activegroupid) {
    $("comfort-sensorgroup-nav-list").html("");
    for (var key in sensorgroups) {
        $("#comfort-sensorgroup-nav-list").append("<li class='sensorgroup-home'><a id='"+ sensorgroups[key].id + "' href='#'>" + sensorgroups[key].display_name + "</a></li>");
    }

    $(".nav-stacked > .sensorgroup-home > a").on("click", function(event) {
        var sensorgroupid = event.target.id;
        var controllerid = sensorgroups[sensorgroupid].controller_id;
        var controller = controllers[controllerid];
        $(".nav-stacked > .sensorgroup-home").removeClass('active');
        $(this).parent().addClass('active');
        populateGroupHomeData(sensorgroupid);
    });

    if(activegroupid != undefined) {
        $(".sensorgroup-home > a#" + activesensorgroupid).trigger("click");
    } else {
        $(".sensorgroup-home").children().first().trigger("click");
    }

}

function populateGroupHomeData(sensorgroupid) {
    $("#comfort-home-data-title").html(sensorgroups[sensorgroupid].display_name);
    $("#comfort-home-data-temperature").html("Temperature:" + parseFloat(sensorgroups[sensorgroupid].stats.temperature).toFixed(2));
    $("#comfort-home-data-humidity").html("Humidity:" + parseFloat(sensorgroups[sensorgroupid].stats.humidity).toFixed(2));
    $("#comfort-home-data-pressure").html("Pressure:" + parseFloat(sensorgroups[sensorgroupid].stats.pressure).toFixed(2));

    if(sensorgroups[sensorgroupid].controller_id !== null) {
        var controllerinfo = "Controller attached: " + controllers[sensorgroups[sensorgroupid].controller_id].display_name;
        $("#comfort-home-data-controller").html(controllerinfo);
    } else {
        var controllerinfo = "Controller attached: none";
        $("#comfort-home-data-controller").html(controllerinfo);
    }
}
